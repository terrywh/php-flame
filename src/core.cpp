#include "controller.h"
#include "coroutine.h"
#include "core.h"

namespace flame {
    static php::value init(php::parameters& params) {
        gcontroller->initialize();
        return nullptr;
    }
    static php::value go(php::parameters& params) {
        php::callable fn = params[0];
        coroutine::start(fn);
        return nullptr;
    }
    static php::value run(php::parameters& params) {
        gcontroller->run();
        return nullptr;
    }
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
    
}