#include "coroutine.h"
#include "queue.h"

namespace flame
{
    void queue::declare(php::extension_entry &ext)
    {
        php::class_entry<queue> class_queue("flame\\queue");
        class_queue
            .method<&queue::__construct>("__construct",
            {
                {"n", php::TYPE::INTEGER, false, true},
            })
            .method<&queue::push>("push",
            {
                {"value", php::TYPE::UNDEFINED}
            })
            .method<&queue::pop>("pop")
            .method<&queue::close>("close")
            .method<&queue::is_closed>("is_closed");
        ext.add(std::move(class_queue));
    }
    php::value queue::__construct(php::parameters& params)
    {
        if(params.size() > 0)
        {
            q_.reset(new coroutine_queue<php::value>(static_cast<int>(params[0])));
        }else{
            q_.reset(new coroutine_queue<php::value>(1));
        }
        return nullptr;
    }
    php::value queue::push(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        q_->push(params[0], ch);
        return nullptr;
    }
    php::value queue::pop(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto x = q_->pop(ch);
        if(x)
        {
            return x.value();
        }
        else
        {
            return nullptr;
        }
    }
    php::value queue::close(php::parameters &params)
    {
        q_->close();
        return nullptr;
    }
    php::value queue::is_closed(php::parameters& params) {
        return q_->is_closed();
    }
}
