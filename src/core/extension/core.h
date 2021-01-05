#ifndef CORE_EXTENSION_CORE_H
#define CORE_EXTENSION_CORE_H

#include <phpext.h>

namespace core { namespace extension {

    class core {
    public:
        static void declare(php::module_entry& entry);
    private:
        // 框架入口（启动“主”协程）
        static php::value run(php::parameters& params);
        // 启动协程
        static php::value go(php::parameters& params);
        // 框架消息·
        static php::value on(php::parameters& params);
    };

}}

#endif // CORE_EXTENSION_CORE_H

