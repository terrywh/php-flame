#ifndef FLAME_CORE_CLASS_ENTRY_DESC_H
#define FLAME_CORE_CLASS_ENTRY_DESC_H
#include <string>

struct _zend_object;
struct _zend_class_entry;

namespace flame::core {

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
    virtual void do_register(struct _zend_class_entry* ce) {};
    int offset() const { return size_; }
    const std::string& name() const { return name_; }
    struct _zend_object* create_object(struct _zend_class_entry* ce) const;
    void destroy_object(struct _zend_object* obj) const;
};

template <class T>
struct class_entry_cache {
    static std::string                name;
    static struct _zend_class_entry* entry;
};

template <class T>
struct _zend_class_entry* class_entry_cache<T>::entry;
template <class T>
std::string class_entry_cache<T>::name;

template <class T>
class class_entry_desc_basic: public class_entry_desc {
   
public:
    class_entry_desc_basic(const std::string& name)
    : class_entry_desc(name, size()) { }

    static constexpr int size() {
        return (sizeof(T) + 15) / 16 * 16;
    }

    void create(void *at) const override {
        new (at) T();
    }

    void destroy(void *at) const override {
        reinterpret_cast<T*>(at)->~T();
    }

    virtual void do_register(struct _zend_class_entry* pce) override {
        class_entry_cache<T>::name = name();
        class_entry_cache<T>::entry = pce;
    }
};


} // flame::core

#endif // FLAME_CORE_CLASS_ENTRY_DESC_H