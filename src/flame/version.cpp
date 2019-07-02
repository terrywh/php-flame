#include "version.h"
#include <boost/version.hpp>
#include <hiredis/hiredis.h>
#include <nghttp2/nghttp2ver.h>
#include <openssl/opensslv.h>
#include <mariadb/mariadb_version.h>
#include <librdkafka/rdkafka.h>
// #include <mongoc/mongoc-version.h>
#include <mongoc/mongoc.h>
#include <curl/curlver.h>

namespace flame {

#define VERSION_MACRO(major, minor, patch) VERSION_JOIN(major, minor, patch)
#define VERSION_JOIN(major, minor, patch) #major"."#minor"."#patch

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

    void version::declare(php::extension_entry& ext) {
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
    }
}