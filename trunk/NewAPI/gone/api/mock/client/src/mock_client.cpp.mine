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
#include "logger.h"
#include <dlfcn.h>
#include <thread>
#include <chrono>

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
// 用 dladdr 定位实际生效的库路径，与 --lib 参数比对，避免用户误以为指定路径生效。
bool MockClient::load_api(const std::string& lib_path) {
    LOG_INFO("[MockClient] 使用直接链接方式加载 API");
    Dl_info info;
    if (dladdr((void*)&lb_api::api_config::create_config, &info) && info.dli_fname) {
        std::string actual(info.dli_fname);
        LOG_INFO("[MockClient] 实际加载的 API 库: " << actual);
        if (!lib_path.empty() && actual != lib_path) {
            LOG_WARN("[MockClient] --lib 指定路径与实际加载库不一致: 指定=" << lib_path
                      << ", 实际=" << actual << " (直接链接模式下 --lib 不生效)");
        }
    } else {
        LOG_WARN("[MockClient] 无法定位 API 库路径 (dladdr 失败)");
    }
    return true;
}

// wait_link_ready: 等待柜台链接就绪
// api_->start() 返回成功仅表示 API 实例启动，与柜台(FTE/98)的 TCP 链接是异步建立的，
// 由 on_link_status 回调通知。此处轮询 last_link_status() 直到链接就绪或超时，
// 避免在链接未就绪时执行测试导致大面积"发送请求失败"。
bool MockClient::wait_link_ready(int timeout_ms) {
    if (!callback_) {
        LOG_WARN("[MockClient] 回调未初始化，跳过链接就绪等待");
        return false;
    }
    auto start = std::chrono::steady_clock::now();
    while (!callback_->last_link_status()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count() > timeout_ms) {
            LOG_WARN("[MockClient] 等待链接就绪超时 (" << timeout_ms << "ms)");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    LOG_INFO("[MockClient] 链接已就绪");
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
        JsonValue config = JsonParser::parse_file(config_path);
        return init_from_json(config);
    } catch (const std::exception& e) {
        LOG_ERROR("[MockClient] 初始化异常: " << e.what());
        return false;
    }
}

// init_from_json: 从已解析的 JSON 配置节点初始化（供 test_plan 主配置模式复用）
bool MockClient::init_from_json(const JsonValue& config) {
    try {
        // 创建 API 配置
        lb_api::api_config* cfg = lb_api::api_config::create_config();
        if (!cfg) {
            LOG_ERROR("[MockClient] create_config 失败");
            return false;
        }

        // 设置配置属性
        int32_t ret_attr = 0;
        ret_attr = cfg->set_attr("api_instance_name", config["api_instance_name"].as_string().c_str());
        if (ret_attr) LOG_DEBUG("set_attr(api_instance_name)=" << ret_attr);
        ret_attr = cfg->set_attr("market_type", static_cast<int8_t>(config["market_type"].as_int()));
        if (ret_attr) LOG_DEBUG("set_attr(market_type)=" << ret_attr);
        ret_attr = cfg->set_attr("fast_counter_type", static_cast<int32_t>(config["fast_counter_type"].as_int()));
        if (ret_attr) LOG_DEBUG("set_attr(fast_counter_type)=" << ret_attr);
        ret_attr = cfg->set_attr("speed_link_type", static_cast<int32_t>(config["speed_link_type"].as_int()));
        if (ret_attr) LOG_DEBUG("set_attr(speed_link_type)=" << ret_attr);

        // 柜台地址
        std::string speed_ip = config["speed_counter_addr"]["ip"].as_string();
        int speed_port = config["speed_counter_addr"]["port"].as_int();
        std::string speed_addr = speed_ip + ":" + std::to_string(speed_port);
        ret_attr = cfg->set_attr("speed_counter_addr", speed_addr.c_str());
        if (ret_attr) LOG_DEBUG("set_attr(speed_counter_addr)=" << ret_attr);

        std::string c98_ip = config["counter98_addr"]["ip"].as_string();
        int c98_port = config["counter98_addr"]["port"].as_int();
        std::string c98_addr = c98_ip + ":" + std::to_string(c98_port);
        ret_attr = cfg->set_attr("counter98_addr", c98_addr.c_str());
        if (ret_attr) LOG_DEBUG("set_attr(counter98_addr)=" << ret_attr);

        ret_attr = cfg->set_attr("98agw_user", config["98agw_user"].as_string().c_str());
        if (ret_attr) LOG_DEBUG("set_attr(98agw_user)=" << ret_attr);
        ret_attr = cfg->set_attr("98agw_user_password", config["98agw_user_password"].as_string().c_str());
        if (ret_attr) LOG_DEBUG("set_attr(98agw_user_password)=" << ret_attr);
        ret_attr = cfg->set_attr("heartbeat_interval", static_cast<int32_t>(config["heartbeat_interval"].as_int()));
        if (ret_attr) LOG_DEBUG("set_attr(heartbeat_interval)=" << ret_attr);
        ret_attr = cfg->set_attr("agw_user_login_timeout", static_cast<int32_t>(config["agw_user_login_timeout"].as_int()));
        if (ret_attr) LOG_DEBUG("set_attr(agw_user_login_timeout)=" << ret_attr);
        ret_attr = cfg->set_attr("log_level", static_cast<int32_t>(config["log_level"].as_int()));
        if (ret_attr) LOG_DEBUG("set_attr(log_level)=" << ret_attr);
        ret_attr = cfg->set_attr("log_output_dir", config["log_output_dir"].as_string().c_str());
        if (ret_attr) LOG_DEBUG("set_attr(log_output_dir)=" << ret_attr);

        // 可选：发送队列大小(MB)，默认 2MB；压测高 TPS 时建议调大避免 SEND_QUEUE_FULL
        if (config.has("send_queue_size_mb")) {
            ret_attr = cfg->set_attr("send_queue_size_mb", static_cast<int32_t>(config["send_queue_size_mb"].as_int()));
            if (ret_attr) LOG_DEBUG("set_attr(send_queue_size_mb)=" << ret_attr);
        }

        // 可选：单链接单客户模式（默认 true）。true=单客户（登录缓存为成员，委托直接用），false=多客户（以 fund_account_id 为 key 存 map）
        if (config.has("single_cust_per_link")) {
            ret_attr = cfg->set_attr("single_cust_per_link", config["single_cust_per_link"].as_bool());
            if (ret_attr) LOG_DEBUG("set_attr(single_cust_per_link)=" << ret_attr);
        }

        // 创建回调
        callback_ = new CallbackHandler();

        // 创建 API 实例（直接调用工厂函数）
        int32_t ret = lb_api::api_interface::create_instance(api_, *cfg, callback_);
        lb_api::api_config::destroy_config(cfg);

        if (ret != 0 || !api_) {
            LOG_ERROR("[MockClient] create_instance 失败: " << ret);
            delete callback_;
            callback_ = nullptr;
            return false;
        }

        LOG_INFO("[MockClient] API 实例创建成功");

        // 启动
        ret = api_->start();
        if (ret != 0) {
            LOG_ERROR("[MockClient] api->start() 失败: " << ret);
            delete callback_;
            callback_ = nullptr;
            lb_api::api_interface::release_instance(api_);
            api_ = nullptr;
            return false;
        }
        LOG_INFO("[MockClient] API 启动成功");

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
        LOG_ERROR("[MockClient] 初始化异常: " << e.what());
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
        LOG_ERROR("[MockClient] MockClient 未初始化");
        return std::vector<TestResult>();
    }

    int count = runner_->load_test_dir(test_dir);
    if (count == 0) {
        LOG_ERROR("[MockClient] 未找到测试用例: " << test_dir);
        return std::vector<TestResult>();
    }

    LOG_INFO("[MockClient] 已加载 " << count << " 个测试用例");
    std::vector<TestResult> results = runner_->execute_all();

    for (size_t i = 0; i < results.size(); i++) {
        report_.add_result(results[i]);
    }
    return results;
}

// run_test_plan: 运行测试计划（test_plan.json 主配置文件模式）
// 流程：
//   1. 从 plan_root 提取 connection 子节点，调用 init_from_json 完成 API 初始化
//   2. 从 plan_root 提取 test_plan 子节点，调用 runner_->load_plan() 加载场景
//      （场景引用的 request_file / expected_file 以 base_dir 为相对基准）
//   3. 逐一执行场景并计入报告
//   4. 若 test_plan.perf_test 配置了独立 request_file，则注入 order 模板后执行性能测试
bool MockClient::run_test_plan(const JsonValue& plan_root, const std::string& base_dir) {
    // 1. 初始化：connection 节点优先；否则兼容 connection_config_file 引用
    //    注意：JsonValue::operator[] 对缺失 key 抛异常，必须先 has() 判断
    JsonValue conn;
    if (plan_root.has("connection")) {
        conn = plan_root["connection"];
    }
    if (conn.is_null() && plan_root.has("connection_config_file")) {
        std::string conn_file = plan_root["connection_config_file"].as_string();
        std::string conn_path = conn_file.empty() ? "" : (base_dir + "/" + conn_file);
        try {
            conn = JsonParser::parse_file(conn_path);
            LOG_INFO("[MockClient] 从文件加载连接配置: " << conn_path);
        } catch (const std::exception& e) {
            LOG_ERROR("[MockClient] 加载连接配置失败: " << conn_path << " - " << e.what());
            return false;
        }
    }
    if (conn.is_null() || !conn.is_object()) {
        LOG_ERROR("[MockClient] 测试计划缺少 connection 配置");
        return false;
    }
    if (!init_from_json(conn)) {
        LOG_ERROR("[MockClient] 测试计划初始化失败");
        return false;
    }

    // 2. 加载计划场景
    JsonValue test_plan;
    if (plan_root.has("test_plan")) {
        test_plan = plan_root["test_plan"];
    } else {
        LOG_ERROR("[MockClient] 测试计划缺少 test_plan 节点");
        return false;
    }
    int count = 0;
    try {
        count = runner_->load_plan(test_plan, base_dir);
    } catch (const std::exception& e) {
        LOG_ERROR("[MockClient] 加载测试计划异常: " << e.what());
        return false;
    }
    if (count == 0) {
        LOG_ERROR("[MockClient] 测试计划未加载到任何已启用场景");
        return false;
    }
    LOG_INFO("[MockClient] 已加载 " << count << " 个计划场景");

    // 2.5 等待柜台链接就绪（避免链接未建立时执行测试导致大面积失败）
    wait_link_ready(10000);

    // 3. 执行功能测试场景
    std::vector<TestResult> results = runner_->execute_all();
    for (size_t i = 0; i < results.size(); i++) {
        report_.add_result(results[i]);
    }

    // 4. 性能测试（test_plan.perf_test）
    bool perf_ok = true;
    if (test_plan.has("perf_test")) {
        JsonValue perf_node = test_plan["perf_test"];
        // 若 perf_test 配置了独立委托模板文件（order_file），加载并注入 order 节点
        if (perf_node.is_object() && perf_node.has("order_file")) {
            std::string order_file = perf_node["order_file"].as_string();
            if (!order_file.empty()) {
                try {
                    JsonValue order_root = JsonParser::parse_file(base_dir + "/" + order_file);
                    JsonValue order = order_root.has("order") ? order_root["order"] : order_root;
                    JsonValue new_perf = perf_node;
                    new_perf.set("order", order);
                    perf_node = new_perf;
                    LOG_INFO("[MockClient] 从文件加载性能委托模板: " << order_file);
                } catch (const std::exception& e) {
                    LOG_ERROR("[MockClient] 加载性能委托模板失败: " << e.what());
                    return false;
                }
            }
        }
        // 仅当 perf_test 显式启用时才执行；未启用/未配置 enable 视为跳过（不算失败）
        bool perf_enabled = perf_node.is_object() && perf_node.has("enable")
                            && perf_node["enable"].as_bool();
        if (perf_enabled) {
            perf_ok = run_perf_test(perf_node);
        } else {
            LOG_INFO("[MockClient] 性能测试未开启（perf_test.enable=false）");
        }
    }

    return perf_ok;
}

// run_perf_test: 运行性能测试
// 从 connection_config.json 的 "perf_test" 节点加载配置，若 enable=true 则执行。
bool MockClient::run_perf_test(const JsonValue& perf_node) {
    if (!perf_runner_) {
        LOG_ERROR("[MockClient] PerfRunner 未初始化");
        return false;
    }

    if (!perf_runner_->load_config(perf_node)) {
        LOG_ERROR("[MockClient] 性能测试配置加载失败");
        return false;
    }

    if (!perf_runner_->enabled()) {
        LOG_INFO("[MockClient] 性能测试未开启（perf_test.enable=false）");
        return false;
    }

    return perf_runner_->run();
}

// shutdown: 关闭并释放全部资源
// 逆序释放：先停止并释放 API 实例，再释放 runner_、perf_runner_ 和 callback_。
void MockClient::shutdown() {
    if (api_) {
        api_->stop();
        LOG_INFO("[MockClient] API 已停止");

        lb_api::api_interface::release_instance(api_);
        LOG_INFO("[MockClient] API 实例已释放");
        api_ = nullptr;
    }

    delete runner_;
    runner_ = nullptr;

    delete perf_runner_;
    perf_runner_ = nullptr;

    delete callback_;
    callback_ = nullptr;
}

// write_analysis: 写入测试结果分析文件
bool MockClient::write_analysis(const std::string& path, const std::string& plan_desc) const {
    std::string perf_report;
    long long perf_failed = -1; // -1 表示未执行
    if (perf_runner_ && perf_runner_->ran()) {
        perf_report = perf_runner_->report_text();
        perf_failed = static_cast<long long>(perf_runner_->failed());
    }
    return ResultAnalysis::write(path, report_, perf_report, perf_failed, plan_desc);
}

} // namespace mock
