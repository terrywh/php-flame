#include "util.h"

namespace core { namespace extension {

    void util::declare(php::module_entry& entry) {
        entry
            .declare<snowflake>();
        
    }

    void util::snowflake::declare(php::module_entry& entry) {}


    // 唯一 id 生成
    php::value util::unique_id(php::parameters& params) {
        // desprecated use flame\snowflake class instead;
        basic_snowflake s { static_cast<std::int16_t>($context->env.pid) };
        return s.next_id();
    }

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