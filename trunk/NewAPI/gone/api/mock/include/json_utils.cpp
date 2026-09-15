// json_utils.cpp - 轻量级 JSON 解析/序列化工具实现
//
// 设计目标：零外部依赖，兼容 gcc 4.8.5，仅支持 mock_client 测试场景所需的
//   JSON 子集（嵌套对象/数组、字符串、数字、布尔、null）。
//
// 解析器（JsonParserImpl）：递归下降解析器，逐字符扫描输入字符串。
// 序列化器（write_json_value）：递归遍历 JsonValue 树，输出 JSON 文本。
//
// 已知限制：
//   - 不支持浮点数科学计数法（如 1.5e10）
//   - 不支持大整数（超出 int64_t 范围）
//   - Unicode 转义（\\uXXXX）仅跳过保留原始文本，不实际解码

#include "json_utils.h"

namespace mock {

// ==================== JsonValue 实现 ====================
// JsonValue 是一个可存储 7 种 JSON 类型的变体类：
//   Null / Bool / Int / Double / String / Array / Object
// 类型转换方法（as_xxx）在类型不匹配时抛 std::runtime_error。

bool JsonValue::as_bool() const {
    if (type_ == JsonType::Bool) return bool_val_;
    if (type_ == JsonType::Int) return int_val_ != 0;
    throw std::runtime_error("JsonValue: not a bool");
}

int64_t JsonValue::as_int() const {
    if (type_ == JsonType::Int) return int_val_;
    if (type_ == JsonType::Double) return static_cast<int64_t>(double_val_);
    throw std::runtime_error("JsonValue: not an int");
}

double JsonValue::as_double() const {
    if (type_ == JsonType::Double) return double_val_;
    if (type_ == JsonType::Int) return static_cast<double>(int_val_);
    throw std::runtime_error("JsonValue: not a double");
}

std::string JsonValue::as_string() const {
    if (type_ == JsonType::String) return str_val_;
    throw std::runtime_error("JsonValue: not a string");
}

std::string JsonValue::to_string() const {
    return JsonWriter::write(*this, false);
}

size_t JsonValue::size() const {
    if (type_ == JsonType::Array) return arr_val_.size();
    if (type_ == JsonType::Object) return obj_val_.size();
    return 0;
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (type_ != JsonType::Array) throw std::runtime_error("JsonValue: not an array");
    return arr_val_.at(index);
}

JsonValue& JsonValue::operator[](size_t index) {
    if (type_ != JsonType::Array) throw std::runtime_error("JsonValue: not an array");
    return arr_val_.at(index);
}

bool JsonValue::has(const std::string& key) const {
    if (type_ != JsonType::Object) return false;
    return obj_val_.find(key) != obj_val_.end();
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (type_ != JsonType::Object) throw std::runtime_error("JsonValue: not an object");
    auto it = obj_val_.find(key);
    if (it == obj_val_.end()) {
        throw std::runtime_error("JsonValue: key not found: " + key);
    }
    return it->second;
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (type_ != JsonType::Object) throw std::runtime_error("JsonValue: not an object");
    return obj_val_[key];
}

std::vector<std::string> JsonValue::keys() const {
    std::vector<std::string> result;
    if (type_ == JsonType::Object) {
        for (const auto& pair : obj_val_) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void JsonValue::push_back(const JsonValue& val) {
    if (type_ != JsonType::Array) {
        type_ = JsonType::Array;
        arr_val_.clear();
    }
    arr_val_.push_back(val);
}

void JsonValue::set(const std::string& key, const JsonValue& val) {
    if (type_ != JsonType::Object) {
        type_ = JsonType::Object;
        obj_val_.clear();
    }
    obj_val_[key] = val;
}

// ==================== JsonParser 实现 ====================
// JsonParserImpl 是递归下降解析器的内部实现类：
//   - pos_ 指向当前扫描位置
//   - skip_whitespace() 跳过空白字符
//   - parse() 依据首字符分发到 parse_object / parse_array / parse_string /
//     parse_bool / parse_null / parse_number
//   - expect(c) 断言下一个字符为 c，否则抛异常

class JsonParserImpl {
public:
    JsonParserImpl(const std::string& input)
        : input_(input), pos_(0) {}

    JsonValue parse() {
        skip_whitespace();
        if (pos_ >= input_.size()) return JsonValue();
        char c = input_[pos_];
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        return parse_number();
    }

private:
    std::string input_;
    size_t pos_;

    void skip_whitespace() {
        while (pos_ < input_.size() && (input_[pos_] == ' ' || input_[pos_] == '\t' ||
               input_[pos_] == '\n' || input_[pos_] == '\r')) {
            pos_++;
        }
    }

    char peek() {
        skip_whitespace();
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    char next() {
        skip_whitespace();
        return pos_ < input_.size() ? input_[pos_++] : '\0';
    }

    void expect(char c) {
        char got = next();
        if (got != c) {
            std::string msg = "JsonParser: expected '";
            msg += c;
            msg += "' but got '";
            msg += got;
            msg += "'";
            throw std::runtime_error(msg);
        }
    }

    JsonValue parse_object() {
        // 解析对象：{ "key": value, ... }
        // 递归调用 parse() 解析每个 value
        JsonValue obj(JsonType::Object);
        expect('{');
        if (peek() == '}') { next(); return obj; }
        while (true) {
            JsonValue key = parse_string();
            expect(':');
            JsonValue val = parse();
            obj.set(key.as_string(), val);
            if (peek() == ',') { next(); continue; }
            break;
        }
        expect('}');
        return obj;
    }

    JsonValue parse_array() {
        // 解析数组：[ value, ... ]
        // 递归调用 parse() 解析每个元素
        JsonValue arr(JsonType::Array);
        expect('[');
        if (peek() == ']') { next(); return arr; }
        while (true) {
            arr.push_back(parse());
            if (peek() == ',') { next(); continue; }
            break;
        }
        expect(']');
        return arr;
    }

    // parse_string: 解析字符串 "..."，支持转义序列与 \\uXXXX（仅跳过保留原文）
    // 注意：case 'u' 用 continue 而非 break，避免 off-by-one 多跳字符
    JsonValue parse_string() {
        expect('"');
        std::string result;
        while (pos_ < input_.size() && input_[pos_] != '"') {
            if (input_[pos_] == '\\') {
                pos_++;
                if (pos_ >= input_.size()) break;
                switch (input_[pos_]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        // 简单处理：跳过 unicode 转义（保留原始文本）
                        result += "\\u";
                        for (int i = 0; i < 4 && pos_ + 1 < input_.size(); i++) {
                            result += input_[++pos_];
                        }
                        continue; // 跳过末尾的 pos_++，因为 for 循环已推进到正确位置
                    }
                    default: result += input_[pos_]; break;
                }
                pos_++;
            } else {
                result += input_[pos_++];
            }
        }
        if (pos_ < input_.size()) pos_++; // skip closing quote
        return JsonValue(result);
    }

    // parse_number: 解析数字（整数或浮点数）
    // 依据是否出现小数点/指数标记判定为 double 或 int64
    JsonValue parse_number() {
        size_t start = pos_;
        if (pos_ < input_.size() && input_[pos_] == '-') pos_++;
        while (pos_ < input_.size() && input_[pos_] >= '0' && input_[pos_] <= '9') pos_++;
        bool is_double = false;
        if (pos_ < input_.size() && input_[pos_] == '.') {
            is_double = true;
            pos_++;
            while (pos_ < input_.size() && input_[pos_] >= '0' && input_[pos_] <= '9') pos_++;
        }
        if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            is_double = true;
            pos_++;
            if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) pos_++;
            while (pos_ < input_.size() && input_[pos_] >= '0' && input_[pos_] <= '9') pos_++;
        }
        std::string num_str = input_.substr(start, pos_ - start);
        if (is_double) {
            return JsonValue(std::strtod(num_str.c_str(), nullptr));
        } else {
            return JsonValue(static_cast<int64_t>(std::strtoll(num_str.c_str(), nullptr, 10)));
        }
    }

    JsonValue parse_bool() {
        if (input_.substr(pos_, 4) == "true") {
            pos_ += 4;
            return JsonValue(true);
        }
        if (input_.substr(pos_, 5) == "false") {
            pos_ += 5;
            return JsonValue(false);
        }
        throw std::runtime_error("JsonParser: invalid bool");
    }

    JsonValue parse_null() {
        if (input_.substr(pos_, 4) == "null") {
            pos_ += 4;
            return JsonValue(JsonType::Null);
        }
        throw std::runtime_error("JsonParser: invalid null");
    }
};

// 入口：从字符串解析 JSON，返回 JsonValue 根节点
JsonValue JsonParser::parse(const std::string& json_str) {
    JsonParserImpl impl(json_str);
    return impl.parse();
}

// 入口：从文件读取内容并解析为 JSON
// 文件无法打开时抛 std::runtime_error
JsonValue JsonParser::parse_file(const std::string& file_path) {
    std::ifstream file(file_path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

// ==================== JsonWriter 实现 ====================
// write_json_value 是递归序列化核心函数，按 JsonValue 类型分发到对应格式：
//   - Null → "null"
//   - Bool → "true"/"false"
//   - Int/Double → to_string()
//   - String → 转义特殊字符后加双引号
//   - Array → [elem, ...]（pretty 模式下每行缩进）
//   - Object → {"key": value, ...}（pretty 模式下每行缩进）

static std::string write_json_value(const JsonValue& val, bool pretty, int indent) {
    std::string result;
    std::string indent_str(indent, ' ');
    std::string indent_next(indent + 2, ' ');

    switch (val.type()) {
        case JsonType::Null:
            result += "null";
            break;
        case JsonType::Bool:
            result += val.as_bool() ? "true" : "false";
            break;
        case JsonType::Int:
            result += std::to_string(val.as_int());
            break;
        case JsonType::Double:
            result += std::to_string(val.as_double());
            break;
        case JsonType::String: {
            result += '"';
            std::string s = val.as_string();
            for (size_t i = 0; i < s.size(); i++) {
                char c = s[i];
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\b': result += "\\b"; break;
                    case '\f': result += "\\f"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result += c;
                }
            }
            result += '"';
            break;
        }
        case JsonType::Array: {
            result += '[';
            if (pretty && val.size() > 0) result += '\n';
            for (size_t i = 0; i < val.size(); i++) {
                if (i > 0) {
                    result += ',';
                    if (pretty) result += '\n';
                }
                if (pretty) result += indent_next;
                result += write_json_value(val[i], pretty, indent + 2);
            }
            if (pretty && val.size() > 0) { result += '\n'; result += indent_str; }
            result += ']';
            break;
        }
        case JsonType::Object: {
            result += '{';
            if (pretty && val.size() > 0) result += '\n';
            std::vector<std::string> keys = val.keys();
            for (size_t i = 0; i < keys.size(); i++) {
                if (i > 0) {
                    result += ',';
                    if (pretty) result += '\n';
                }
                if (pretty) result += indent_next;
                result += '"' + keys[i] + '"';
                result += pretty ? ": " : ":";
                result += write_json_value(val[keys[i]], pretty, indent + 2);
            }
            if (pretty && val.size() > 0) { result += '\n'; result += indent_str; }
            result += '}';
            break;
        }
    }
    return result;
}

// 入口：将 JsonValue 序列化为 JSON 字符串
// pretty=true 时输出带缩进格式，否则输出紧凑格式
std::string JsonWriter::write(const JsonValue& val, bool pretty) {
    return write_json_value(val, pretty, 0);
}

} // namespace mock
