#include "_executor.h"
#include "_decode.h"

namespace flame::toml {

void _executor::operator ()(const toml_parser::parser& p, std::string_view chunk) {
    buffer_.append(chunk.data(), chunk.size());
}

void _executor::operator ()(const toml_parser::parser& p) {
    php::string raw = std::move(buffer_);
    std::cout << "(" << (int)p.value_type() << ") " << p.field() << " => [" << raw << "]\n";
    php::value  val = _executor::restore(raw, p.value_type());
    
    // switch(p.container_type()) {
    // case toml_parser::CONTAINER_TYPE_ARRAY:
    // break;
    // case toml_parser::CONTAINER_TYPE_ARRAY_TABLE:
    // break;
    // case toml_parser::CONTAINER_TYPE_TABLE:
    // default:
    //     ;
    // }
    set(root_, p.field(), p.value_array_index(), val);
}

php::value _executor::restore(php::string& raw, std::uint8_t value_type) {
    php::value val;
    // 根据类型还原数据
    switch(value_type) {
    case toml_parser::VALUE_TYPE_BOOLEAN:
        val = raw.to_boolean();
        break;
    case toml_parser::VALUE_TYPE_INTEGER:
        val = raw.to_integer();
        break;
    case toml_parser::VALUE_TYPE_HEX_INTEGER:
        val = std::strtol(raw.data(), nullptr, 16);
        break;
    case toml_parser::VALUE_TYPE_OCT_INTEGER:
        val = std::strtol(raw.data(), nullptr, 8);
        break;
    case toml_parser::VALUE_TYPE_BIN_INTEGER:
        val = std::strtol(raw.data(), nullptr, 2);
        break;
    case toml_parser::VALUE_TYPE_INFINITY:
        val = raw.data()[0] == '-' ? - std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
        break;
    case toml_parser::VALUE_TYPE_NAN:
        val = NAN;
        break;
    case toml_parser::VALUE_TYPE_FLOAT:
        val = raw.to_float();
        break;
    case toml_parser::VALUE_TYPE_DATE:
        val = php::datetime(raw.c_str());
        break;
    case toml_parser::VALUE_TYPE_BASIC_STRING:
        decode_inplace(raw);
        // fallthrough
    case toml_parser::VALUE_TYPE_UNKNOWN: // 未知类型的数据按原始字符串处理
    case toml_parser::VALUE_TYPE_RAW_STRING:
    default:
        val = raw;
    }

    return val;
}

}
