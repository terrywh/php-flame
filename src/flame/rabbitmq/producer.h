#pragma once
#include "../../vendor.h"
#include "rabbitmq.h"

namespace flame::rabbitmq {
    class _client;
    class producer: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params); // 私有
        php::value publish(php::parameters& params);
    private:
        // 实际的客户端对象可能超过当前对象的生存期
        std::shared_ptr<_client> cc_;
        
        friend php::value produce(php::parameters& params);
        friend class _client;
    };
} // namespace flame::rabbitmq
