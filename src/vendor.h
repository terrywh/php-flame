#pragma once

#define EXTENSION_NAME "flame"
#define EXTENSION_VERSION "0.13.99"


#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <set>
#include <stack>
#include <string_view>
#include <optional>
#include <unordered_set>
#include <utility>

#include <phpext/phpext.h>
#include <parser/separator_parser.hpp>
#include <parser/multipart_parser.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/context/fiber.hpp>
#include <boost/context/segmented_stack.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
