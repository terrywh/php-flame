#include "log.h"
#include "../context.h"
#include "../exception.h"

namespace core { namespace extension {

    static core::logger::severity_t value2severity(const php::value& v) {
        int s = 0;
        if(v.is(php::TYPE_STRING)) {
            std::string_view sv = v;
            for(const auto& str : core::logger::severity_str) {
                if(strncasecmp(str.c_str(), sv.data(), str.size()) == 0) break;
                ++s;
            }
            return static_cast<core::logger::severity_t>(s);
        }
        else if(v.is(php::TYPE_INTEGER)) {
            switch(static_cast<int>(v)) {
            case E_DONT_BAIL:
                return core::logger::severity_t::ALERT;
            case E_PARSE:
            case E_COMPILE_ERROR:
            case E_CORE_ERROR:
                return core::logger::severity_t::CRITICAL;
            case E_RECOVERABLE_ERROR:
            case E_ERROR:
            case E_USER_ERROR:
                return core::logger::severity_t::ERROR;
            case E_USER_WARNING:
            case E_WARNING:
            case E_CORE_WARNING:
            case E_COMPILE_WARNING:
                return core::logger::severity_t::WARNING;
           
            case E_NOTICE:
            case E_USER_NOTICE:            
            case E_DEPRECATED:
            case E_USER_DEPRECATED:
                return core::logger::severity_t::NOTICE;
            case E_STRICT:
                return core::logger::severity_t::INFO;
            default:
                return core::logger::severity_t::DEBUG;
            }
        }
        else {
            return core::logger::severity_t::DEBUG;
        }
    }
    // 包裹一个 structured data 定义（对应 PHP 数组）
    class sdata: public php::class_basic<sdata> {
    public:
        static void declare(php::module_entry& entry) {
            entry.declare<sdata>("flame\\log\\sdata");
        }
        void write(std::ostream& os) {
            // TODO 是否应该生成唯一标识？
            os << "[SDATA@" << self()->handle_id();
            if(!data_.is(php::TYPE_ARRAY)) {
                os << data_;
                return;
            }
            for(auto& entry: php::cast<php::array>(data_)) {
                os << " " << entry.first << "=\"" << entry.second << "\"";
            }
            os.put(']');
        }
        php::value data_;
    };
    // 用于生成简单的 KEY=VALUE, KEY=VALUE 形式的字符串（方便 LOGGER 的解析器采集）
    static php::value tagkv(php::parameters& params) {
        php::value s = php::create_object<sdata>();
        php::cast<sdata>(s).data_ = params[0];
        return s;
    }

    static std::nullptr_t write_log(core::logger::severity_t s, php::parameters& params) {
        if(!$context->logger) core::raise<php::compile_error>("default logger cannot be used outside coroutines");

        $context->logger->write(s, [&params] (std::ostream& os) {
            for(auto& p: params) {
                if(p.of<sdata>()) php::cast<sdata>(p).write(os);
                else os << p;
                os.put(' ');
            }
        });
        return nullptr;
    }

    static php::value debug(php::parameters& params) {
        return write_log(core::logger::severity_t::DEBUG, params);
    }
    
    void log::declare(php::module_entry& entry) {
        entry.declare<sdata>();
        entry
            - php::function<tagkv>("flame\\log\\tagkv", {
                { php::TYPE_OBJECT },
                { php::TYPE_ARRAY, "tags"},
            })
            - php::function<debug>("flame\\log\\debug", {
                { php::FAKE_VOID },
                { php::FAKE_MIXED | php::VARIADIC, "x" },
            });
    }
}}