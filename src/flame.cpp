#include "vendor.h"
#include "controller.h"
#include "controller_master.h"
#include "controller_worker.h"
#include "coroutine.h"
#include "core.h"
#include "log/log.h"
#include "os/os.h"
#include "time/time.h"
#include "mysql/mysql.h"
#include "redis/redis.h"
#include "mongodb/mongodb.h"
#include "kafka/kafka.h"
#include "rabbitmq/rabbitmq.h"
#include "tcp/tcp.h"
#include "udp/udp.h"
#include "http/http.h"
#include "hash/hash.h"
#include "encoding/encoding.h"
#include "compress/compress.h"
#include "toml/toml.h"
#include <boost/version.hpp>
#include <nghttp2/nghttp2ver.h>

namespace flame {
    static php::value init(php::parameters& params) {
        php::array options(0);
        if (params.size() > 1 && params[1].type_of(php::TYPE::ARRAY)) options = params[1];
        gcontroller->initialize(params[0], options);
        return nullptr;
    }

    static php::value go(php::parameters& params) {
        php::callable fn = params[0];
        coroutine::start(php::callable([fn] (php::parameters& params) -> php::value {
            php::value rv;
            try {
                rv = fn.call();
            } catch (const php::exception &ex) {
                auto ft = gcontroller->cbmap->equal_range("exception");
                for(auto i=ft.first; i!=ft.second; ++i) i->second.call({ex});
                php::object obj = ex;
                std::cerr << "[" << time::iso() << "] (FATAL) " << obj.call("__toString") << "\n";

                boost::asio::post(gcontroller->context_x, [] () {
                    gcontroller->status |= controller::controller_status::STATUS_EXCEPTION;
                    gcontroller->context_x.stop();
                    gcontroller->context_y.stop();
                });
            }
            return rv;
        }));
        return nullptr;
    }

    static php::value fake_fn(php::parameters& params) {
        return nullptr;
    }

    static php::value on(php::parameters &params) {
        std::string event = params[0].to_string();
        if (!params[1].type_of(php::TYPE::CALLABLE)) throw php::exception(zend_ce_type_error
            , "Failed to set callback: callable required"
            , -1);
        gcontroller->cbmap->insert({event, params[1]});
        return nullptr;
    }

    static php::value run(php::parameters& params) {
        if (gcontroller->status & controller::STATUS_INITIALIZED) {
            gcontroller->default_execute_data = EG(current_execute_data);
            gcontroller->run();
        }
        else throw php::exception(zend_ce_parse_error
            , "Failed to run flame: exception or missing 'flame\\init()' ?"
            , -1);
        return nullptr;
    }

    static php::value quit(php::parameters& params) {
        gcontroller->context_x.stop();
        gcontroller->context_y.stop();
        return nullptr;
    }

    static php::value get(php::parameters& params) {
        php::array  target = params[0];
        php::string fields = params[1];
        return flame::toml::get(target, { fields.data(), fields.size() }, 0);
    }

    static php::value set(php::parameters& params) {
        php::array  target = params.get(0, true);
        php::string fields = params[1];
        php::value  values = params[2];
        flame::toml::set(target, {fields.data(), fields.size()}, 0, values);
        return nullptr;
    }
}

static std::string openssl_version_str() {
    std::string version = (boost::format("%d.%d.%d")
        % ((OPENSSL_VERSION_NUMBER & 0xff0000000L) >> 28)
        % ((OPENSSL_VERSION_NUMBER & 0x00ff00000L) >> 20)
        % ((OPENSSL_VERSION_NUMBER & 0x0000ff000L) >> 12) ).str();
    char status = (OPENSSL_VERSION_NUMBER & 0x000000ff0L) >>  4;
    if (status > 0) {
        version.push_back('a' + status - 1);
    }
    return version;
}
#define VERSION_MACRO(major, minor, patch) VERSION_JOIN(major, minor, patch)
#define VERSION_JOIN(major, minor, patch) #major"."#minor"."#patch

extern "C" {
    ZEND_DLEXPORT zend_module_entry *get_module()  {
        static php::extension_entry ext(EXTENSION_NAME, EXTENSION_VERSION);
        std::string sapi = php::constant("PHP_SAPI");
        if (sapi != "cli") {
            std::cerr << "[WARNING] FLAME disabled: SAPI='cli' mode only\n";
            return ext;
        }

        php::class_entry<php::closure> class_closure("flame\\closure");
        class_closure.method<&php::closure::__invoke>("__invoke");
        ext.add(std::move(class_closure));

        ext
            .desc({"vendor/openssl", openssl_version_str()})
            .desc({"vendor/boost", BOOST_LIB_VERSION})
            .desc({"vendor/phpext", PHPEXT_LIB_VERSION})
            .desc({"vendor/hiredis", VERSION_MACRO(HIREDIS_MAJOR, HIREDIS_MINOR, HIREDIS_PATCH)})
            // .desc({"vendor/mysqlc", mysql_get_client_info()})
            .desc({"vendor/mariac", MARIADB_PACKAGE_VERSION})
            .desc({"vendor/amqpcpp", "4.1.5"})
            .desc({"vendor/rdkafka", rd_kafka_version_str()})
            .desc({"vendor/mongoc", MONGOC_VERSION_S})
            .desc({"vendor/nghttp2", NGHTTP2_VERSION})
            .desc({"vendor/curl", LIBCURL_VERSION});

        flame::gcontroller.reset(new flame::controller());
        ext
            .function<flame::init>("flame\\init", {
                {"process_name", php::TYPE::STRING},
                {"options", php::TYPE::ARRAY, false, true},
            })
            .function<flame::run>("flame\\run");
        if (flame::gcontroller->type == flame::controller::process_type::WORKER) {
            ext
            .function<flame::go>("flame\\go", {
                {"coroutine", php::TYPE::CALLABLE},
            })
            .function<flame::on>("flame\\on", {
                {"event", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .function<flame::quit>("flame\\quit")
            .function<flame::set>("flame\\set", {
                {"target", php::TYPE::ARRAY, true},
                {"fields", php::TYPE::STRING},
                {"values", php::TYPE::UNDEFINED},
            })
            .function<flame::get>("flame\\get", {
                {"target", php::TYPE::ARRAY},
                {"fields", php::TYPE::STRING},
            });

            flame::core::declare(ext);
            flame::log::declare(ext);
            flame::os::declare(ext);
            flame::time::declare(ext);
            flame::mysql::declare(ext);
            flame::redis::declare(ext);
            flame::mongodb::declare(ext);
            flame::kafka::declare(ext);
            flame::rabbitmq::declare(ext);
            flame::tcp::declare(ext);
            flame::udp::declare(ext);
            flame::http::declare(ext);
            flame::hash::declare(ext);
            flame::encoding::declare(ext);
            flame::compress::declare(ext);
            flame::toml::declare(ext);
        }
        else {
            ext
                .function<flame::fake_fn>("flame\\go")
                .function<flame::fake_fn>("flame\\on");
        }
        return ext;
    }
};
