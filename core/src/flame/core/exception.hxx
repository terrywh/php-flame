#ifndef FLAME_CORE_EXCEPTION_I
#define FLAME_CORE_EXCEPTION_I

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

namespace flame::core {

typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace> with_stacktrace;

} // flame::core

#endif // FLAME_CORE_EXCEPTION_I
