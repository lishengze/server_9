// json_utils.h - 轻量级 JSON 解析/序列化工具（mock 组件共用）
//
// 设计目标：零外部依赖，兼容 gcc 4.8.5，供 mock_client 与 98_counter_mock
//   解析 JSON 配置文件与测试用例。
// 主要类：
//   - JsonValue：可存储 7 种 JSON 类型的变体节点（Null/Bool/Int/Double/String/Array/Object）
//   - JsonParser：递归下降解析器（parse / parse_file）
//   - JsonWriter：递归序列化器（write）
//
// 类型安全：as_xxx() 在类型不匹配时抛 std::runtime_error，调用方需保证类型正确。

#ifndef MOCK_JSON_UTILS_H
#define MOCK_JSON_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

namespace mock {

/// JSON 值类型
enum class JsonType {
    Null,
    Bool,
    Int,
    Double,
    String,
    Array,
    Object
};

/// JSON 值节点
class JsonValue {
public:
    JsonValue() : type_(JsonType::Null) {}
    JsonValue(JsonType t) : type_(t) {}
    JsonValue(bool v) : type_(JsonType::Bool), bool_val_(v) {}
    JsonValue(int64_t v) : type_(JsonType::Int), int_val_(v) {}
    JsonValue(double v) : type_(JsonType::Double), double_val_(v) {}
    JsonValue(const std::string& v) : type_(JsonType::String), str_val_(v) {}
    JsonValue(const char* v) : type_(JsonType::String), str_val_(v) {}

    JsonType type() const { return type_; }

    bool is_null() const { return type_ == JsonType::Null; }
    bool is_bool() const { return type_ == JsonType::Bool; }
    bool is_int() const { return type_ == JsonType::Int; }
    bool is_double() const { return type_ == JsonType::Double; }
    bool is_string() const { return type_ == JsonType::String; }
    bool is_array() const { return type_ == JsonType::Array; }
    bool is_object() const { return type_ == JsonType::Object; }

    bool as_bool() const;
    int64_t as_int() const;
    double as_double() const;
    std::string as_string() const;
    std::string to_string() const;  // 序列化为 JSON 字符串

    // 数组访问
    size_t size() const;
    const JsonValue& operator[](size_t index) const;
    JsonValue& operator[](size_t index);

    // 对象访问
    bool has(const std::string& key) const;
    const JsonValue& operator[](const std::string& key) const;
    JsonValue& operator[](const std::string& key);
    std::vector<std::string> keys() const;

    // 数组/对象添加
    void push_back(const JsonValue& val);
    void set(const std::string& key, const JsonValue& val);

private:
    JsonType type_;
    bool bool_val_ = false;
    int64_t int_val_ = 0;
    double double_val_ = 0.0;
    std::string str_val_;
    std::vector<JsonValue> arr_val_;
    std::map<std::string, JsonValue> obj_val_;
};

/// JSON 解析器
class JsonParser {
public:
    /// 从字符串解析 JSON
    static JsonValue parse(const std::string& json_str);
    /// 从文件解析 JSON
    static JsonValue parse_file(const std::string& file_path);
};

/// JSON 序列化器
class JsonWriter {
public:
    static std::string write(const JsonValue& val, bool pretty = true);
};

} // namespace mock

#endif // MOCK_JSON_UTILS_H
