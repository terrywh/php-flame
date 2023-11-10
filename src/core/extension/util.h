#ifndef CORE_EXTENSION_UTIL_H
#define CORE_EXTENSION_UTIL_H

#include "../vendor.h"
#include "../basic_snowflake.h"

namespace core { namespace extension {

    class util {
    public:
        static void declare(php::module_entry& entry);
    private:
        // 获取一个 snowflake 兼容的唯一标识
        static php::value unique_id(php::parameters& params);
        // 简化多级 KEY 数组读取
        static php::value get(php::parameters& params);
        // 简化多级 KEY 数组读取
        static php::value set(php::parameters& params);
        // 封装 snowflake 兼容的简化版本唯一标识生成算法
        class snowflake: public basic_snowflake {
        public:
            static void declare(php::module_entry& entry);
            snowflake();
            // function __construct(int $node = null, int $epoch = null);
            php::value __construct(php::parameters& params);
            // function next_id()
            php::value   next_id(php::parameters& params);
        };
    };

}}

#endif // CORE_EXTENSION_UTIL_H
