#include "util.h"
#include "../context.h"
namespace core { namespace extension {

    void util::declare(php::module_entry& entry) {
        entry
            .declare<snowflake>();
        entry
            - php::function<unique_id>("flame\\util\\unique_id", {
                { php::TYPE_INTEGER },                              // 返回: 无
                { php::TYPE_INTEGER, "node" },                    // 节点
                { php::TYPE_INTEGER | php::ALLOW_NULL, "epoch" }, // 可选，时间原点
            })
            - php::function<util::get>("flame\\util\\get", {
                { php::FAKE_MIXED }, // 返回值，取决于实际获取到的数据
                { php::TYPE_ARRAY,  "array"}, // 源数组
                { php::TYPE_STRING, "keys" }, // 操作 KEY
            })
            - php::function<util::set>("flame\\util\\set", {
                { php::FAKE_MIXED }, // 返回值，取决于实际获取到的数据
                { php::TYPE_ARRAY | php::BYREF, "array"}, // 源数组(引用)
                { php::TYPE_STRING, "keys"}, // 操作 KEY
                { php::FAKE_MIXED, "value"}, // 设置值
            });
    }
    // 
    void util::snowflake::declare(php::module_entry& entry) {
        entry.declare<snowflake>("flame\\util\\snowflake")
            - php::method<&snowflake::__construct>("__construct", {
                { php::FAKE_NONE }, // OR TYPE_UNDEFINED
                { php::TYPE_INTEGER | php::ALLOW_NULL, "node" },
                { php::TYPE_INTEGER | php::ALLOW_NULL, "epoch", PHP_DEFAULT_VALUE(1288834974657)}, // 注意这里的常量须与 C++ 定义统一
            })
            - php::method<&snowflake::next_id>("next_id", {
                { php::TYPE_INTEGER },
            });
    }
    // 唯一 id 生成
    php::value util::unique_id(php::parameters& params) {
        // desprecated use flame\snowflake class instead;
        basic_snowflake s { static_cast<std::int16_t>($context->status.pid) };
        return s.next_id();
    }
    // 简化多级 KEY 数组读取
    php::value util::get(php::parameters& params) {
        return php::array::get(params[0], params[1]);
    }
    // 简化多级 KEY 数组读取
    php::value util::set(php::parameters& params) {
        php::array::set(params[0], params[1], params[2]);
        return nullptr;
    }
    // 
    util::snowflake::snowflake() 
    : basic_snowflake(0) {}
    // 对可选参数处理，并填充基类数据
    php::value util::snowflake::__construct(php::parameters& params) {
        parts_.node  = static_cast<int64_t>(params[0]) % 1024;
        if(params.size() > 1)
            epoch_ = static_cast<int64_t>(params[1]);
        return nullptr;
    }
    // 导出代理基类函数
    php::value util::snowflake::next_id(php::parameters& params) {
        return basic_snowflake::next_id();
    }
}}