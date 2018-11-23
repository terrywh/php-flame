#include "vendor.h"
#include "core.h"
#include "os/os.h"
#include "time/time.h"
#include "mysql/mysql.h"
#include "redis/redis.h"
#include "mongodb/mongodb.h"

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

        flame::declare(ext);
        flame::os::declare(ext);
        flame::time::declare(ext);
        flame::mysql::declare(ext);
        flame::redis::declare(ext);
        flame::mongodb::declare(ext);
        // flame::log::declare(ext);
        // flame::udp::declare(ext);
        // flame::tcp::declare(ext);
        // flame::http::declare(ext);
        
        // flame::rabbitmq::declare(ext);
        
        
        // flame::kafka::declare(ext);
        return ext;
    }
};