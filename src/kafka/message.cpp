#include "kafka.h"
#include "message.h"

namespace flame {
namespace kafka {
    void message::declare(php::extension_entry& ext) {
        php::class_entry<message> class_message("flame\\kafka\\message");
		class_message
			.property({"topic", ""})
			.property({"partition", -1})
			.property({"key", ""})
			.property({"value", nullptr})
			.property({"header", nullptr})
			.property({"offset", -1})
			.property({"timestamp", 0})
			.method<&message::__construct>("__construct", {
				{"body", php::TYPE::STRING, false, true},
			})
			.method<&message::to_string>("__toString")
            .method<&message::to_json>("jsonSerialize");

		ext.add(std::move(class_message));
    }
    php::value message::__construct(php::parameters& params) {
        return nullptr;
    }
    php::value message::to_json(php::parameters& params) {
        return nullptr;
    }
    php::value message::to_string(php::parameters& params) {
        return nullptr;
    }
}
}