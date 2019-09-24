#include "../vendor.h"
#include "../util.h"
#include "controller.h"
#include "version.h"
#include "worker.h"
#include "master.h"

#define EXTENSION_NAME    "flame"
#define EXTENSION_VERSION "0.17.3"

extern "C" {
    ZEND_DLEXPORT zend_module_entry *get_module()  {
        static php::extension_entry ext(EXTENSION_NAME, EXTENSION_VERSION);
        std::string sapi = php::constant("PHP_SAPI");
        if (sapi != "cli") {
            std::cerr << "[" << util::system_time() << "] (WARNING) FLAME disabled: SAPI='cli' mode only\n";
            return ext;
        }
        // 内置一个 C++ 函数包裹类
        php::class_entry<php::closure> class_closure("flame\\closure");
        class_closure.method<&php::closure::__invoke>("__invoke");
        ext.add(std::move(class_closure));
        // 全局控制器
        flame::gcontroller.reset(new flame::controller());
        // 扩展版本
        flame::version::declare(ext);
        // 主进程与工作进程注册不同的函数实现
        if (flame::gcontroller->type == flame::controller::process_type::WORKER) flame::worker::declare(ext);
        else flame::master::declare(ext);

        return ext;
    }
};
