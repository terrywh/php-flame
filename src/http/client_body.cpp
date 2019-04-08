#include "client_body.h"

namespace flame::http {
	void client_body::declare(php::extension_entry& ext) {
		php::class_entry<client_body> class_client_body("flame\\http\\client_body");
        ext.add(std::move(class_client_body));
	}
}