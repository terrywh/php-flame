#pragma once

#define EXTENSION_NAME "flame"
#define EXTENSION_VERSION "0.13.1"

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstdio>
#include <limits>
#include <algorithm>
#include <functional> // for std::functional
#include <cctype>
#include <utility>
#include <memory>
#include <stack>
#include <string_view>
#include <optional>
#include <unordered_set>
#include <phpext/phpext.h>
#include <http_parser.h>
#include <hiredis/hiredis.h>
#include <parser/separator_parser.hpp>
#include <parser/multipart_parser.hpp>
#include <boost/version.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/context/fiber.hpp>
#include <boost/context/segmented_stack.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/coroutine.hpp>
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
namespace ssl = boost::asio::ssl;
#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/crc.hpp>
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <mariadb/mysql.h>
// #include <my_sys.h>
#include <mongoc.h>
#include <librdkafka/rdkafka.h>
extern "C" {
    #include <librdkafka/snappy.h>
    #include <librdkafka/rdmurmur2.h>
    #include <librdkafka/xxhash.h>
}
#include <nghttp2/nghttp2ver.h>
#include <curl/curl.h>
