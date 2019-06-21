#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "kafka.h"

namespace flame::kafka {

    class _consumer;
    class consumer : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters &params); // 私有
        php::value run(php::parameters &params);
        php::value commit(php::parameters &params);
        php::value close(php::parameters &params);

    private:
        std::shared_ptr<_consumer> cs_;
        int                        cc_ = 8;
        php::callable              cb_;

        friend php::value consume(php::parameters &params);
    };
} // namespace flame::kafka
