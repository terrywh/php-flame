#pragma once
#include "../../vendor.h"
#include "kafka.h"

namespace flame::kafka {
    
    class _producer;
    class producer : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters &params); // 私有
        php::value __destruct(php::parameters &params);
        php::value publish(php::parameters &params);
        php::value publish_to(php::parameters& params);
        php::value flush(php::parameters &params);
        php::value close(php::parameters &params);
    private:
        std::shared_ptr<_producer> pd_;
        friend php::value produce(php::parameters &params);
    };
} // namespace flame::kafka
