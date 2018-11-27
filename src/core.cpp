#include "controller.h"
#include "coroutine.h"
#include "core.h"
#include "coroutine_queue.h"
#include "queue.h"
#include "mutex.h"

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
        gcontroller->core_execute_data = EG(current_execute_data);
        gcontroller->run();
        return nullptr;
    }
    // static coroutine_queue<int> q;
    // static php::value produce(php::parameters& params)
    // {
    //     coroutine_handler ch{coroutine::current};
    //     q.push(static_cast<int>(params[0]), ch);
    //     return nullptr;
    // }
    // static php::value consume(php::parameters& params)
    // {
    //     coroutine_handler ch{coroutine::current};
    //     auto x = q.pop(ch);
    //     if(x) {
    //         return x.value();
    //     }else{
    //         return nullptr;
    //     }
    // }
    php::value select(php::parameters& params)
    {
        std::vector< std::shared_ptr<coroutine_queue<php::value>> > qs;
        std::map< std::shared_ptr<coroutine_queue<php::value>>, php::object > mm;
        for (auto i = 0; i < params.size(); ++i)
        {
            if(!params[i].instanceof(php::class_entry<queue>::entry()))
            {
                throw php::exception(zend_ce_type_error, "only flame\\queue can be selected", -1);
            }
            php::object obj = params[i];
            queue* ptr = static_cast<queue*>(php::native(obj));
            qs.push_back( ptr->q_ );
            mm.insert({ptr->q_, obj});
        }
        coroutine_handler ch{coroutine::current};
        std::shared_ptr<coroutine_queue<php::value>> q = select_queue(qs, ch);
        return mm[q];
    }
    void declare(php::extension_entry &ext) {
        gcontroller.reset(new controller());
        ext
            .function<init>("flame\\init", {
                {"process_name", php::TYPE::STRING},
            })
            .function<go>("flame\\go", {
                {"coroutine", php::TYPE::CALLABLE},
            })
            .function<run>("flame\\run")
            .function<select>("flame\\select");

        queue::declare(ext);
        mutex::declare(ext);
        guard::declare(ext);
    }
    
}