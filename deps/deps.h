#pragma once

#include <cstring>
#include <cstdlib>
#include <memory>
#include <stack>
#include <map>
#include <future>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "curl/include/curl/curl.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"
#include "libuv/include/uv.h"
#include "multipart-parser-c/multipart_parser.h"
#include "libphpext/phpext.h"
