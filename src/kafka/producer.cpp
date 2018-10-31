#include "../coroutine.h"
#include "kafka.h"
#include "message.h"
#include "producer.h"

namespace flame {
namespace kafka {
    void producer::declare(php::extension_entry& ext) {

    }
    php::value producer::__construct(php::parameters& params) {
        return nullptr;
    }
    php::value producer::publish(php::parameters& params) {
        return nullptr;
    }
}
}