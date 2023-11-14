#ifndef FLAME_CORE_VALUE_H
#define FLAME_CORE_VALUE_H
#include <string>
#include <cstdint>

struct _zval_struct;

namespace flame::core {

class parameter_list;
class value {
    alignas(16) std::byte storage_[16];

public:
    class type {
        std::uint8_t code_;
    public:
        type()
        : code_ {0} {}
        explicit type(std::uint8_t typ)
        : code_(typ) {}

        std::uint8_t code() const {
            return code_;
        }
        const char* name() const;
        bool operator == (const type& r) const;
        inline bool operator !=(const type& r) const {
            return !this->operator==(r);
        }

        // bool is_undefined();
        // bool is_null();
        // bool is_boolean();

        static type undefined; // unknown
        static type null;
        static type boolean; // 仅用于类型比较（实际对应 true / false 两种类型）
        static type integer; // int64
        static type doubles; // float point number (double)
        static type number; // 仅用于类型比较（实际对应 integer / double 两种类型）
        static type string;
        static type array;
        static type object;
        static type reference;
    };


    value();
    value(_zval_struct* v, bool ref = false);
    value(const value& v);
    value(value&& v);
    value(std::nullptr_t);
    value(bool b);
    value(int i);
    value(std::int64_t i);
    value(float f);
    value(double f);
    value(const char* s);
    value(std::string_view s);
    ~value(); // 所有集成对象均使用同样的销毁策略，故无须 virtual 机制
    operator _zval_struct*() const&;
    _zval_struct* ptr() const&;

    operator bool() const&;
    operator int() const&;
    operator std::int64_t() const&;
    operator float() const&;
    operator double() const&;
    operator std::string_view() const&;
    operator std::string() const&;

    value operator ()() const&;
    value operator ()(parameter_list& list) const&;
    
    value to_reference();
    value::type of_type() const;

    friend class string;
};

} // flame::core

#endif // FLAME_CORE_VALUE_H
