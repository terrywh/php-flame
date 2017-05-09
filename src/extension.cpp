#include "vendor.h"
#include "core.h"
#include "net/init.h"
#include "net/http/init.h"

extern "C" {
	ZEND_DLEXPORT zend_module_entry* get_module() {
		static php::extension_entry extension(EXTENSION_NAME, EXTENSION_VERSION);
		core::init(extension);
		net::init(extension);
        net::http::init(extension);
		return extension;
	}
}
