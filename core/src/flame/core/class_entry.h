#ifndef FLAME_CORE_CLASS_ENTRY_H
#define FLAME_CORE_CLASS_ENTRY_H
#include "access_entry.h"
#include "class_entry_desc.h"
#include <memory>

struct _zend_object;
struct _zend_class_entry;

namespace flame::core {

class module_entry;
class function_entry;
class method_entry;
class property_entry;
class constant_entry;

class class_entry_provider;
struct class_entry_store;
class class_entry_base {
    std::unique_ptr<class_entry_desc>   desc_;
    std::shared_ptr<class_entry_store> store_;
    access_entry acc_;
    
    class_entry_base(std::unique_ptr<class_entry_desc> traits);
    static struct _zend_object *create_object(struct _zend_class_entry *ce);
    static void destroy_object(struct _zend_object *obj);

    void extends(std::unique_ptr<class_entry_provider> entry);
    void implements(std::unique_ptr<class_entry_provider> entry);
public:
    static class_entry_base& create(std::unique_ptr<class_entry_desc> traits);
    
    class_entry_base& operator +(function_entry&& entry);
    class_entry_base& operator +(method_entry&&   entry);
    class_entry_base& operator +(property_entry&& entry);
    class_entry_base& operator +(constant_entry&& entry);

    template <class T>
    class_entry_base& extends() {
        extends(class_entry_desc_basic<T>::entry);
        return *this;
    }

    template <class T>
    class_entry_base& implements() {
        implements(class_entry_desc_basic<T>::entry);
        return *this;
    }

    class_entry_base& extends(const std::string& name);
    class_entry_base& implements(const std::string& name);

    class_entry_base& operator %(access_entry::modifier m);

    void finalize();
};

class class_entry_provider {
public:
    virtual struct _zend_class_entry* provide() const = 0;
    virtual const std::string& name() const = 0;
};

template <class T>
class class_entry_provider_cpp: public class_entry_provider {
public:
    struct _zend_class_entry* provide() const override {
        return class_entry_desc_basic<T>::entry;
    }
    const std::string& name() const override {
        return class_entry_desc_basic<T>::name;
    }
};

class class_entry_provider_php: public class_entry_provider {
    std::string name_;
public:
    class_entry_provider_php(const std::string& name)
    : name_(name) {}
    struct _zend_class_entry* provide() const override;
    const std::string& name() const override {
        return name_;
    }
};


} // flame::core

#endif // FLAME_CORE_CLASS_ENTRY_H