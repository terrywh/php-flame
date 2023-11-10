#include <flame/core/parameter.h>
#include <php/Zend/zend_API.h>
#include <boost/assert.hpp>
#include <iostream>

namespace flame::core {

parameter_list parameter_list::prepare(_zend_execute_data* execute_data) {
    parameter_list list;
    int size = ZEND_NUM_ARGS();
    zval data[size];
    zend_get_parameters_array_ex(size, data);
    for (auto& item : data) {
        list.list_.emplace_back(&item);
    }
    return list;
}

parameter_list::parameter_list(std::initializer_list<value> list)
: list_(list) {

}
} // flame::core
