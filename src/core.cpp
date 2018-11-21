#include "controller.h"
#include "coroutine.h"
#include "core.h"

namespace flame {
    void declare(php::extension_entry &ext) {
        ext
            .on_module_startup([](php::extension_entry &ext) -> bool {
                gcontroller.reset(new controller());
                return true;
            })
            .function<init>("flame\\init", {
                {"process_name", php::TYPE::STRING},
            })
            .function<go>("flame\\go", {
                {"coroutine", php::TYPE::CALLABLE},
            })
            .function<run>("flame\\run");
    }
    php::value init(php::parameters& params) {
        gcontroller->initialize();
        return nullptr;
    }
    php::value go(php::parameters& params) {
        php::callable fn = params[0];
        coroutine::start(fn);
        return nullptr;
    }
    
    php::value run(php::parameters& params) {
        gcontroller->context_x.run();
        return nullptr;
    }
}