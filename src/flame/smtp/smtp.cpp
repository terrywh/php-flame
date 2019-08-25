#include "smtp.h"
#include "message.h"
#include "client.h"
#include "../../url.h"

namespace flame::smtp {
    void declare(php::extension_entry& ext) {
        ext
            .on_module_startup([] (php::extension_entry& ext) -> bool {
                curl_global_init(CURL_GLOBAL_DEFAULT);
                return true;
            })
            .function<connect>("flame\\smtp\\connect", {
                {"url", php::TYPE::STRING},
            });
        message::declare(ext);
        client::declare(ext);
    }
    
    php::value connect(php::parameters& params) {
        php::object obj(php::class_entry<client>::entry());
        client* cli = static_cast<client*>(php::native(obj));

        cli->c_rurl_ = static_cast<std::string>(params[0]);
        cli->c_from_ = url(cli->c_rurl_).user;
        return obj;
    }
}