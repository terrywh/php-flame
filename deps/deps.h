#pragma once

#include <algorithm>
#include <memory>
#include <stack>
#include <deque>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <random>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "multipart-parser-c/multipart_parser.h"
#include "kv-parser/kv_parser.h"
#include "fastcgi-parser/fastcgi_parser.h"
#include "libphpext/phpext.h"
#include "uv.h"
#include "http_parser.h"
#include "fmt/format.h"
#include "curl/curl.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"
#include "mysql/mysql.h"
#include "libbson-1.0/bson.h"
#include "libmongoc-1.0/mongoc.h"
#include "librdkafka/rdkafka.h"
#include "amqp.h"
#include "amqp_tcp_socket.h"

#include <cstring>
#include <cstdlib>
#include <cassert>
#include <ctime>
