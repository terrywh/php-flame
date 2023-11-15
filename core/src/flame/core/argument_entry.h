#ifndef FLAME_CORE_ARGUMENT_ENTRY_H
#define FLAME_CORE_ARGUMENT_ENTRY_H
#include "value.h"
#include "argument_desc.h"
#include <memory>
#include <string>
#include <vector>

namespace flame::core {

struct argument_entry_store;
class argument_entry {
    std::shared_ptr<argument_entry_store> store_;
    std::size_t                            size_;
    // argument::type        type_; // return type
    // std::vector<argument_entry> list_;

public:
    argument_entry(argument_desc::type type, std::initializer_list<argument_desc> list);
    argument_entry(argument_entry&& m);
   ~argument_entry();
    
    std::size_t size() const { return size_; }
    void * finalize();
};

} // namespace flame::core

#endif // FLAME_CORE_ARGUMENT_ENTRY_H
