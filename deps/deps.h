#pragma once

#include <string.h>

#include <cstring>
#include <cstdlib>
#include <cassert>
#include <ctime>

#include <algorithm>
#include <memory>
#include <stack>
#include <deque>
#include <map>
#include <set>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libphpext/phpext.h"
#include "libuv/include/uv.h"
#include "curl/include/curl/curl.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"
#include "fastcgi-parser/fastcgi_parser.h"
#include "multipart-parser-c/multipart_parser.h"
#include "kv-parser/kv_parser.h"
#include "mongo-c-driver/bin/include/libbson-1.0/bson.h"
#include "mongo-c-driver/bin/include/libmongoc-1.0/mongoc.h"
#include "librdkafka/src/rdkafka.h"
#include "fmt/fmt/format.h"
#include "http-parser/http_parser.h"
#include "rabbitmq-c/librabbitmq/amqp.h"
#include "rabbitmq-c/librabbitmq/amqp_tcp_socket.h"
#include "mysql-connector-c/include/mysql.h"
