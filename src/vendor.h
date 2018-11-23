#pragma once

#define EXTENSION_NAME "flame"
#define EXTENSION_VERSION "0.12.0"

#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>

#include <limits>
#include <algorithm>
#include <functional> // for std::functional
#include <cctype>
#include <utility>
#include <memory>
#include <stack>
#include <unordered_set>
#include <phpext/phpext.h>
#include <http_parser.h>
#include <hiredis/hiredis.h>
#include <parser/separator_parser.hpp>
#include <parser/multipart_parser.hpp>
#include <boost/version.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/context/continuation.hpp>
#include <boost/context/fiber.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/process.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/coroutine.hpp>
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
namespace ssl = boost::asio::ssl;
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <mysql.h>
#include <my_sys.h>
#include <mongoc.h>
#include <librdkafka/rdkafka.h>
