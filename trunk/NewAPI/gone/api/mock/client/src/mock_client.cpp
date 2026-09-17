// mock_client.cpp - MockClient 主类实现
//
// 职责：封装被测对象（liblbapi.so）的完整生命周期管理：
//   1. load_api()   加载动态库（当前为直接链接，为未来 dlopen 扩展保留接口）
//   2. init()       加载连接配置 → 创建 api_config → 设置属性 → 创建回调 → 创建 API 实例 → start()
//   3. run_test()   运行单个测试用例（返回首个 TestResult）
//   4. run_all_tests() 运行目录下全部测试用例
//   5. shutdown()   停止 API 并释放 runner_ / callback_ 资源
//
// 被测链路：MockClient → api_interface → gw_counter_direct → FTE 柜台

#include "mock_client.h"
#include "api_interface.h"
#include "api_config.h"
#include <iostream>

namespace mock {

MockClient::MockClient()
    : api_(nullptr)
    , callback_(nullptr)
    , runner_(nullptr)
    , perf_runner_(nullptr)
{
}

MockClient::~MockClient() {
    shutdown();
}

// load_api: 加载被测 API 动态库
// 当前实现采用「直接链接」方式（编译时已链接 liblbapi.so），
// 因此本方法仅做占位并打印信息，lib_path 参数保留给未来 dlopen 动态加载扩展。
bool MockClient::load_api(const std::string& lib_path) {
    std::cout << "[MockClient] 使用直接链接方式加载 API" << std::endl;
    return true;
}

// init: 初始化测试环境
// 完整流程：
//   1. 解析连接配置 JSON（api_instance_name / market_type / 柜台地址 / 98agw 账号等）
//   2. 通过 api_config::create_config() 创建配置对象
//   3. 调用 set_attr() 逐项写入配置属性（柜台地址拼接为 "ip:port" 字符串）
//   4. 创建 CallbackHandler 回调处理器
//   5. 调用 api_interface::create_instance() 创建 API 实例
//   6. 调用 api_->start() 启动 API（建立柜台链接）
//   7. 创建 TestCaseRunner 测试执行器
// 任一步失败都会释放已分配资源并返回 false。
bool MockClient::init(const std::string& config_path) {
    try {
        // 加载 JSON 配置
        JsonValue config = JsonParser::parse_file(config_path);

        // 创建 API 配置
        lb_api::api_config* cfg = lb_api::api_config::create_config();
        if (!cfg) {
            std::cerr << "[MockClient] create_config 失败" << std::endl;
            return false;
        }

        // 设置配置属性
        int32_t ret_attr = 0;
        ret_attr = cfg->set_attr("api_instance_name", config["api_instance_name"].as_string().c_str());
        if (ret_attr) std::cerr << "[DEBUG] set_attr(api_instance_name)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("market_type", static_cast<int8_t>(config["market_type"].as_int()));
        if (ret_attr) std::cerr << "[DEBUG] set_attr(market_type)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("fast_counter_type", static_cast<int32_t>(config["fast_counter_type"].as_int()));
        if (ret_attr) std::cerr << "[DEBUG] set_attr(fast_counter_type)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("speed_link_type", static_cast<int32_t>(config["speed_link_type"].as_int()));
        if (ret_attr) std::cerr << "[DEBUG] set_attr(speed_link_type)=" << ret_attr << std::endl;

        // 柜台地址
        std::string speed_ip = config["speed_counter_addr"]["ip"].as_string();
        int speed_port = config["speed_counter_addr"]["port"].as_int();
        std::string speed_addr = speed_ip + ":" + std::to_string(speed_port);
        ret_attr = cfg->set_attr("speed_counter_addr", speed_addr.c_str());
        if (ret_attr) std::cerr << "[DEBUG] set_attr(speed_counter_addr)=" << ret_attr << std::endl;

        std::string c98_ip = config["counter98_addr"]["ip"].as_string();
        int c98_port = config["counter98_addr"]["port"].as_int();
        std::string c98_addr = c98_ip + ":" + std::to_string(c98_port);
        ret_attr = cfg->set_attr("counter98_addr", c98_addr.c_str());
        if (ret_attr) std::cerr << "[DEBUG] set_attr(counter98_addr)=" << ret_attr << std::endl;

        ret_attr = cfg->set_attr("98agw_user", config["98agw_user"].as_string().c_str());
        if (ret_attr) std::cerr << "[DEBUG] set_attr(98agw_user)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("98agw_user_password", config["98agw_user_password"].as_string().c_str());
        if (ret_attr) std::cerr << "[DEBUG] set_attr(98agw_user_password)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("heartbeat_interval", static_cast<int32_t>(config["heartbeat_interval"].as_int()));
        if (ret_attr) std::cerr << "[DEBUG] set_attr(heartbeat_interval)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("agw_user_login_timeout", static_cast<int32_t>(config["agw_user_login_timeout"].as_int()));
        if (ret_attr) std::cerr << "[DEBUG] set_attr(agw_user_login_timeout)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("log_level", static_cast<int32_t>(config["log_level"].as_int()));
        if (ret_attr) std::cerr << "[DEBUG] set_attr(log_level)=" << ret_attr << std::endl;
        ret_attr = cfg->set_attr("log_output_dir", config["log_output_dir"].as_string().c_str());
        if (ret_attr) std::cerr << "[DEBUG] set_attr(log_output_dir)=" << ret_attr << std::endl;

        // 可选：发送队列大小(MB)，默认 2MB；压测高 TPS 时建议调大避免 SEND_QUEUE_FULL
        if (config.has("send_queue_size_mb")) {
            ret_attr = cfg->set_attr("send_queue_size_mb", static_cast<int32_t>(config["send_queue_size_mb"].as_int()));
            if (ret_attr) std::cerr << "[DEBUG] set_attr(send_queue_size_mb)=" << ret_attr << std::endl;
        }

        // 可选：单链接单客户模式（默认 true）。true=单客户（登录缓存为成员，委托直接用），false=多客户（以 fund_account_id 为 key 存 map）
        if (config.has("single_cust_per_link")) {
            ret_attr = cfg->set_attr("single_cust_per_link", config["single_cust_per_link"].as_bool());
            if (ret_attr) std::cerr << "[DEBUG] set_attr(single_cust_per_link)=" << ret_attr << std::endl;
        }

        // 创建回调
        callback_ = new CallbackHandler();

        // 创建 API 实例（直接调用工厂函数）
        int32_t ret = lb_api::api_interface::create_instance(api_, *cfg, callback_);
        lb_api::api_config::destroy_config(cfg);

        if (ret != 0 || !api_) {
            std::cerr << "[MockClient] create_instance 失败: " << ret << std::endl;
            delete callback_;
            callback_ = nullptr;
            return false;
        }

        std::cout << "[MockClient] API 实例创建成功" << std::endl;

        // 启动
        ret = api_->start();
        if (ret != 0) {
            std::cerr << "[MockClient] api->start() 失败: " << ret << std::endl;
            delete callback_;
            callback_ = nullptr;
            lb_api::api_interface::release_instance(api_);
            api_ = nullptr;
            return false;
        }
        std::cout << "[MockClient] API 启动成功" << std::endl;

        // 创建测试执行器
        runner_ = new TestCaseRunner(api_, callback_);

        // 创建性能测试执行器
        perf_runner_ = new PerfRunner(api_);

        // 根据柜台类型设置性能测试报告标题名称
        int32_t fct = static_cast<int32_t>(config["fast_counter_type"].as_int());
        std::string counter_name = "FTE"; // 默认 gw counter
        if (fct == 2) {
            counter_name = "GOne";        // fpga_direct
        } else if (fct == 3) {
            counter_name = "GOne-GW";     // fpga_gateway
        }
        perf_runner_->set_counter_name(counter_name);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[MockClient] 初始化异常: " << e.what() << std::endl;
        return false;
    }
}

// run_test: 运行单个测试用例文件
// 加载指定 JSON 测试用例 → execute_all() 执行 → 将全部结果计入报告。
// 返回第一个 TestResult；加载/执行失败时返回带错误信息的 TestResult。
TestResult MockClient::run_test(const std::string& testcase_path) {
    if (!runner_) {
        TestResult r;
        r.case_name = "ERROR";
        r.passed = false;
        r.fail_reason = "MockClient 未初始化";
        return r;
    }

    if (!runner_->load_test_case(testcase_path)) {
        TestResult r;
        r.case_name = "LOAD_ERROR";
        r.passed = false;
        r.fail_reason = "加载测试用例失败: " + testcase_path;
        return r;
    }

    std::vector<TestResult> results = runner_->execute_all();
    if (results.empty()) {
        TestResult r;
        r.case_name = "NO_RESULT";
        r.passed = false;
        r.fail_reason = "未产生测试结果";
        return r;
    }

    for (size_t i = 0; i < results.size(); i++) {
        report_.add_result(results[i]);
    }
    return results[0];
}

// run_all_tests: 运行指定目录下全部测试用例
// 通过 load_test_dir() 扫描目录下所有 .json 文件并加载，逐个 execute_all() 执行。
std::vector<TestResult> MockClient::run_all_tests(const std::string& test_dir) {
    if (!runner_) {
        std::cerr << "[MockClient] MockClient 未初始化" << std::endl;
        return std::vector<TestResult>();
    }

    int count = runner_->load_test_dir(test_dir);
    if (count == 0) {
        std::cerr << "[MockClient] 未找到测试用例: " << test_dir << std::endl;
        return std::vector<TestResult>();
    }

    std::cout << "[MockClient] 已加载 " << count << " 个测试用例" << std::endl;
    std::vector<TestResult> results = runner_->execute_all();

    for (size_t i = 0; i < results.size(); i++) {
        report_.add_result(results[i]);
    }
    return results;
}

// run_perf_test: 运行性能测试
// 从 connection_config.json 的 "perf_test" 节点加载配置，若 enable=true 则执行。
bool MockClient::run_perf_test(const JsonValue& perf_node) {
    if (!perf_runner_) {
        std::cerr << "[MockClient] PerfRunner 未初始化" << std::endl;
        return false;
    }

    if (!perf_runner_->load_config(perf_node)) {
        std::cerr << "[MockClient] 性能测试配置加载失败" << std::endl;
        return false;
    }

    if (!perf_runner_->enabled()) {
        std::cout << "[MockClient] 性能测试未开启（perf_test.enable=false）" << std::endl;
        return false;
    }

    return perf_runner_->run();
}

// shutdown: 关闭并释放全部资源
// 逆序释放：先停止并释放 API 实例，再释放 runner_、perf_runner_ 和 callback_。
void MockClient::shutdown() {
    if (api_) {
        api_->stop();
        std::cout << "[MockClient] API 已停止" << std::endl;

        lb_api::api_interface::release_instance(api_);
        std::cout << "[MockClient] API 实例已释放" << std::endl;
        api_ = nullptr;
    }

    delete runner_;
    runner_ = nullptr;

    delete perf_runner_;
    perf_runner_ = nullptr;

    delete callback_;
    callback_ = nullptr;
}

} // namespace mock
