#include "exception.h"
#include "exception.hxx"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_builtin_functions.h>
#include <php/Zend/zend_exceptions.h>
#include <boost/core/demangle.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <format>
#include <iostream>

namespace flame::core {

type_error::type_error(flame::core::type expect, const value& value)
: exception(std::format("(type error) expect '{}', got '{}'", expect.name(), zend_zval_type_name(value))) {}

type_error::type_error(flame::core::type expect)
: exception(std::format("(type error) expect '{}'", expect.name())) {}

void throw_exception(const exception& e) {
    throw boost::enable_error_info(e) << with_stacktrace(boost::stacktrace::stacktrace());
}
// 
static void cpp_print_trace(std::ostream& os, const boost::stacktrace::stacktrace* trace) {
    int n = 0;
    for (auto i=trace->begin(); i!=trace->end(); ++i) {
        std::string frame = boost::stacktrace::to_string(*i);
        if (!boost::algorithm::ends_with(frame, ".so")) break;
        os << '#' << (n++) << ' ' << frame << '\n';
    }
}
// 参考 zend_builtin_functions.c:debug_print_backtrace 相关实现
static void php_print_trace(std::ostream& os) {
    zval trace;
    zend_fetch_debug_backtrace(&trace, 0, 0, 0);
    zend_string* str = zend_trace_to_string(Z_ARRVAL(trace), false);
    os.write(ZSTR_VAL(str), ZSTR_LEN(str));
    zend_string_release(str);
    zval_ptr_dtor(&trace);
}
// 
void exception_handler() {
    std::set_terminate( 0 );
    try {
        throw;
    }
    catch(const flame::core::exception& x) {
        std::cerr << "std::terminate called after throwing an exception:\n"
            << "      type: flame::core::exception\n"
            << "    what(): " << x.what() << '\n';
        
        if (const boost::stacktrace::stacktrace* trace = boost::get_error_info<flame::core::with_stacktrace>(x);
            trace != nullptr) {
            std::cerr << "\ncpp:\n";
            cpp_print_trace(std::cerr, trace);
        }
    }
    catch (const std::exception& x) {
        std::cerr << "std::terminate called after throwing an exception:\n"
            << "      type: " << boost::core::demangle(typeid(x).name())
            << "    what(): " << x.what() << '\n';
    }
    catch(...) {
        std::fputs( "unknown exception\n", stderr);
    }
    std::cerr << "\nphp:\n";
    php_print_trace(std::cerr);

    std::cerr.flush();
    std::abort();
}


} // flame::core
