#ifndef CORE_VENDOR_H
#define CORE_VENDOR_H

#include <phpext.h>
#include <boost/asio.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/context/fiber.hpp>
#include <boost/process.hpp>
#include <boost/serialization/singleton.hpp>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h> // for constants

#include <strings.h>
#include <cstdint>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <limits>
#include <memory>
#include <ostream>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#endif // CORE_VENDOR_H
