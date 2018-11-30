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
#include "http/http.h"

namespace flame 
{
    static php::value init(php::parameters& params)
    {
        php::array options(0);
        if(params.size() > 1 && params[1].typeof(php::TYPE::ARRAY))
        {
            options = params[1];
        }
        gcontroller->core_execute_data = EG(current_execute_data);
        gcontroller->initialize(params[0], options);
        return nullptr;
    }
    static php::value go(php::parameters& params)
    {
        php::callable fn = params[0];
        coroutine::start(php::callable([fn] (php::parameters& params) -> php::value
        {
            php::value rv;
            try{
                rv = fn.call();
            }
            catch (const php::exception &ex)
            {
                int x = 0;
                auto ft = gcontroller->cbmap->equal_range("exception");
                for(auto i=ft.first; i!=ft.second; ++i)
                {
                    ++x;
                    i->second.call({ex});
                }
                if(x==0)
                {
                    std::cerr << "[" << time::iso() << "] (FATAL) " << ex.what() << "\n";
                    boost::asio::post(gcontroller->context_x, [] () {
                        gcontroller->context_x.stop();
                    });
                }else
                {
                    std::cerr << "[" << time::iso() << "] (ERROR) " << ex.what() << "\n";
                }
            }
            return rv;
        }));
        return nullptr;
    }
    static php::value fake_fn(php::parameters& params)
    {
        return nullptr;
    }
    static php::value on(php::parameters &params)
    {
        std::string event = params[0].to_string();
        if (!params[1].typeof(php::TYPE::CALLABLE))
        {
            throw php::exception(zend_ce_type_error, "callable is required", -1);
        }
        gcontroller->cbmap->insert({event, params[1]});
        return nullptr;
    }
    static php::value run(php::parameters& params)
    {
        if(gcontroller->status & controller::STATUS_INITIALIZED)
        {
            gcontroller->core_execute_data = EG(current_execute_data);
            gcontroller->run();
        }else{
            throw php::exception(zend_ce_type_error, "failed to run flame: not yet initialized (forget to call 'flame\\init()' ?)");
        }
        return nullptr;
    }
    static php::value quit(php::parameters& params)
    {
        gcontroller->context_x.stop();
        return nullptr;
    }
}

extern "C"
{
    ZEND_DLEXPORT zend_module_entry *get_module()
    {
        static php::extension_entry ext(EXTENSION_NAME, EXTENSION_VERSION);
        std::string sapi = php::constant("PHP_SAPI");
        if (sapi != "cli")
        {
            std::cerr << "Flame can only be using in SAPI='cli' mode\n";
            return ext;
        }

        php::class_entry<php::closure> class_closure("flame\\closure");
        class_closure.method<&php::closure::__invoke>("__invoke");
        ext.add(std::move(class_closure));

        ext
            .desc({"vendor/boost", BOOST_LIB_VERSION})
            .desc({"vendor/libphpext", PHPEXT_LIB_VERSION})
            .desc({"vendor/amqpcpp", "4.0.0"})
            .desc({"vendor/mysqlc", PACKAGE_VERSION})
            .desc({"vendor/librdkafka", rd_kafka_version_str()})
            .desc({"vendor/mongoc", MONGOC_VERSION_S});

        flame::gcontroller.reset(new flame::controller());
        ext
            .function<flame::init>("flame\\init",
            {
                {"process_name", php::TYPE::STRING},
                {"options", php::TYPE::ARRAY, false, true},
            })
            .function<flame::run>("flame\\run");
        if(flame::gcontroller->type == flame::controller::process_type::WORKER)
        {
            ext
            .function<flame::go>("flame\\go",
            {
                {"coroutine", php::TYPE::CALLABLE},
            })
            .function<flame::on>("flame\\on",
            {
                {"event", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .function<flame::quit>("flame\\quit");
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
            // flame::udp::declare(ext);
            flame::http::declare(ext);
            // flame::log::declare(ext);
        }
        else
        {
            ext
            .function<flame::fake_fn>("flame\\go")
            .function<flame::fake_fn>("flame\\on");
        }
        return ext;
    }
};
