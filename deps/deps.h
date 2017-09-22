#pragma once
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <memory>
#include <stack>
#include <map>
#include <thread>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mongo-c-driver/bin/include/libbson-1.0/bson.h"
#include "mongo-c-driver/bin/include/libmongoc-1.0/mongoc.h"
#include "curl/include/curl/curl.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"
#include "libuv/include/uv.h"
#include "fastcgi-parser/fastcgi_parser.h"
#include "multipart-parser-c/multipart_parser.h"
#include "kv-parser/kv_parser.h"
#include "libphpext/phpext.h"
#include "ext/mysqlnd/mysqlnd.h"
