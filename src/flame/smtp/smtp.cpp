#include "smtp.h"
#include "message.h"
#include "client.h"
#include "../../url.h"
#include "../../util.h"

namespace flame::smtp {

    static php::value create_boundary(php::parameters& params) {
        int size = params[0];
        return php::string(util::random_string(size), size);
    }

    void declare(php::extension_entry& ext) {
        ext
            .on_module_startup([] (php::extension_entry& ext) -> bool {
                curl_global_init(CURL_GLOBAL_DEFAULT);
                return true;
            })
            .function<connect>("flame\\smtp\\connect", {
                {"url", php::TYPE::STRING},
            })
            .function<create_boundary>("flame\\smtp\\create_boundary", {
                {"size", php::TYPE::INTEGER}
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