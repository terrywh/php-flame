#include "argument_entry.h"
#include <php/Zend/zend_API.h>
#include <memory>
#include <vector>
#include <cstring>

namespace flame::core {

struct argument_entry_store {
    std::vector<_zend_internal_arg_info> info;
};

static std::vector<std::shared_ptr<argument_entry_store>> store;

argument_entry::argument_entry(argument_desc::type type, std::initializer_list<argument_desc> list)
: store_(store.emplace_back(std::make_shared<argument_entry_store>()))
, size_(list.size()) {
    store_->info.reserve(list.size()+1);
    auto *info = reinterpret_cast<zend_internal_function_info*>(&store_->info.emplace_back());
    info->default_value = nullptr;
    info->required_num_args = 0;
    type.finalize(&info->type);
    for (auto& desc : list) {
        auto& e = store_->info.emplace_back();
        desc.finalize(&e);
        if (!ZEND_TYPE_ALLOW_NULL(e.type)) {
            ++info->required_num_args;
        }
    }
}

argument_entry::argument_entry(argument_entry&& m) = default;
argument_entry::~argument_entry() = default;

void* argument_entry::finalize() {
    return store_->info.data();
}

} // flame::core
