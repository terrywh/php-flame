#include "../coroutine.h"
#include "kafka.h"
#include "consumer.h"
#include "producer.h"
#include "message.h"

namespace flame {
namespace kafka {
    void declare(php::extension_entry& ext) {
        ext
			.function<consume>("flame\\kafka\\consume", {
				{"options", php::TYPE::ARRAY},
                {"topics", php::TYPE::ARRAY},
			});
		consumer::declare(ext);
		producer::declare(ext);
		message::declare(ext);
    }
    php::value consume(php::parameters& params) {
        return nullptr;
    }
    php::value produce(php::parameters& params) {
        return nullptr;
    }

    php::array convert(rd_kafka_headers_t* headers) {
        return nullptr;
    }
    rd_kafka_headers_t* convert(php::array headers) {
        return nullptr;
    }
}
}
