#include "mock_client.h"
#include "api_interface.h"
#include "api_config.h"
#include <iostream>

namespace mock {

MockClient::MockClient()
    : api_(nullptr)
    , callback_(nullptr)
    , runner_(nullptr)
{
}

MockClient::~MockClient() {
    shutdown();
}

bool MockClient::load_api(const std::string& lib_path) {
    // 直接链接 liblbapi.so，无需 dlopen
    // lib_path 保留供未来动态加载扩展使用
    std::cout << "[MockClient] 使用直接链接方式加载 API" << std::endl;
    return true;
}

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

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[MockClient] 初始化异常: " << e.what() << std::endl;
        return false;
    }
}

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

    delete callback_;
    callback_ = nullptr;
}

} // namespace mock
