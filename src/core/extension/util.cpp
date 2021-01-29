#include "util.h"

namespace core { namespace extension {

    void util::declare(php::module_entry& entry) {
        entry
            .declare<snowflake>();
        entry
            - php::function<unique_id>("flame\\util\\unique_id", {
                php::TYPE_VOID,                 // 返回: 无
                php::TYPE_INTEGER,                   // 节点
                php::TYPE_INTEGER | php::ALLOW_NULL, // 可选，时间原点
            })
            - php::function<get>("flame\\util\\get", {
                php::FAKE_MIXED, // 返回值，取决于实际获取到的数据
                php::TYPE_ARRAY, // 源数组
                php::TYPE_STRING | php::TYPE_ARRAY, // 操作 KEY
            })
            - php::function<set>("flame\\util\\set", {
                php::FAKE_MIXED, // 返回值，取决于实际获取到的数据
                php::TYPE_ARRAY, // 源数组
                php::TYPE_STRING | php::TYPE_ARRAY, // 操作 KEY
                php::FAKE_MIXED, // 设置值
            });
    }
    // 
    void util::snowflake::declare(php::module_entry& entry) {
        entry.declare<snowflake>("flame\\util\\snowflake")
            - php::method<&snowflake::__construct>("__construct")
            - php::method<&snowflake::next_id>("next_id");
    }
    // 唯一 id 生成
    php::value util::unique_id(php::parameters& params) {
        // desprecated use flame\snowflake class instead;
        basic_snowflake s { static_cast<std::int16_t>($context->env.pid) };
        return s.next_id();
    }
    // 简化多级 KEY 数组读取
    php::value get(php::parameters& params) {
        return php::array::get(params[0], params[1]);
    }
    // 简化多级 KEY 数组读取
    php::value set(php::parameters& params) {
        php::array::set(params[0], params[1], params[2]);
        return nullptr;
    }
    // 
    util::snowflake::snowflake() 
    : basic_snowflake() {}
    // 对可选参数处理，并填充基类数据
    php::value util::snowflake::__construct(php::parameters& params) {
        if(params.size() > 0)
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