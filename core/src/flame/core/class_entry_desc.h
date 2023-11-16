#ifndef FLAME_CORE_CLASS_ENTRY_DESC_H
#define FLAME_CORE_CLASS_ENTRY_DESC_H
#include <string>

struct _zend_object;
struct _zend_class_entry;

namespace flame::core {


class class_entry_base;
class class_entry_desc {
    std::string name_;
    int         size_;

protected:
    class_entry_desc(const std::string& name, int size)
    : name_(name), size_(size) {}
    virtual void create(void *at) const = 0;
    virtual void destroy(void *at) const = 0;
public:
    virtual ~class_entry_desc() = default;
    virtual void do_register(struct _zend_class_entry* ce, class_entry_base* entry) {};
    int offset() const { return size_; }
    const std::string& name() const { return name_; }
    struct _zend_object* create_object(struct _zend_class_entry* ce) const;
    void destroy_object(struct _zend_object* obj) const;
};

template <class T>
class class_entry_desc_basic: public class_entry_desc {
   
public:
    static std::string                name;
    static struct _zend_class_entry*    ce;
    static class_entry_base*         entry;

    class_entry_desc_basic(const std::string& name)
    : class_entry_desc(name, size()) {
        class_entry_desc_basic<T>::name = name;
    }

    static constexpr int size() {
        return (sizeof(T) + 15) / 16 * 16;
    }

    static constexpr T* z2c(struct _zend_object* obj) {
        return reinterpret_cast<T*>(
            const_cast<char*>(reinterpret_cast<const char*>( obj )) - class_entry_desc_basic<T>::size());
    }

    static constexpr struct _zend_object* c2z(void *obj) {
        return reinterpret_cast<struct _zend_object*>(
            reinterpret_cast<char*>(obj) + class_entry_desc_basic<T>::size());
    }

    void create(void *at) const override {
        new (at) T();
    }

    void destroy(void *at) const override {
        reinterpret_cast<T*>(at)->~T();
    }

    virtual void do_register(struct _zend_class_entry* ce, class_entry_base* entry) override {
        class_entry_desc_basic<T>::ce = ce;
        class_entry_desc_basic<T>::entry = entry;
    }
};

template <class T>
struct _zend_class_entry* class_entry_desc_basic<T>::ce;
template <class T>
std::string class_entry_desc_basic<T>::name;
template <class T>
class_entry_base* class_entry_desc_basic<T>::entry;


} // flame::core

#endif // FLAME_CORE_CLASS_ENTRY_DESC_H