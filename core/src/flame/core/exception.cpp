#include "exception.hxx"
#include <php/Zend/zend_API.h>
#include <flame/core/exception.h>
#include <format>

namespace flame::core {

type_error::type_error(value::type expect, const value& value)
: exception(std::format("(type error) expect '{}', got '{}'", expect.name(), zend_zval_type_name(value))) {}

type_error::type_error(value::type expect)
: exception(std::format("(type error) expect '{}'", expect.name())) {}

void throw_exception(const exception& e) {
    throw boost::enable_error_info(e) << with_stacktrace(boost::stacktrace::stacktrace());
}

} // flame::core
