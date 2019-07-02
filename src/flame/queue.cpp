#include "coroutine.h"
#include "queue.h"

namespace flame {
    void queue::declare(php::extension_entry &ext) {
        php::class_entry<queue> class_queue("flame\\queue");
        class_queue
            .method<&queue::__construct>("__construct", {
                {"n", php::TYPE::INTEGER, false, true},
            })
            .method<&queue::push>("push", {
                {"value", php::TYPE::UNDEFINED}
            })
            .method<&queue::pop>("pop")
            .method<&queue::close>("close")
            .method<&queue::is_closed>("is_closed");
        ext
            .add(std::move(class_queue))
            .function<queue::select>("flame\\select");
    }

    php::value queue::__construct(php::parameters& params) {
        if (params.size() > 0)
            q_.reset(new coroutine_queue<php::value>(static_cast<int>(params[0])));
        else
            q_.reset(new coroutine_queue<php::value>(1));
        return nullptr;
    }

    php::value queue::push(php::parameters &params) {
        coroutine_handler ch{coroutine::current};
        q_->push(params[0], ch);
        return nullptr;
    }

    php::value queue::pop(php::parameters &params) {
        coroutine_handler ch{coroutine::current};
        auto x = q_->pop(ch);
        if (x) return x.value();
        else return nullptr;
    }

    php::value queue::close(php::parameters &params) {
        q_->close();
        return nullptr;
    }

    php::value queue::is_closed(php::parameters& params) {
        return q_->is_closed();
    }
    
    php::value queue::select(php::parameters& params) {
        std::vector< std::shared_ptr<coroutine_queue<php::value>> > qs;
        std::map< std::shared_ptr<coroutine_queue<php::value>>, php::object > mm;
        for (auto i = 0; i < params.size(); ++i) {
            if (!params[i].instanceof(php::class_entry<queue>::entry())) 
                throw php::exception(zend_ce_type_error, "Failed to select: instanceof flame\\queue required", -1);
            php::object obj = params[i];
            queue* ptr = static_cast<queue*>(php::native(obj));
            qs.push_back( ptr->q_ );
            mm.insert({ptr->q_, obj});
        }
        coroutine_handler ch{coroutine::current};
        std::shared_ptr<coroutine_queue<php::value>> q = select_queue(qs, ch);
        return mm[q];
    }    
}
