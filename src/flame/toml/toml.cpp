#include "toml.h"
#include "_executor.h"
#include <fstream>

namespace flame::toml {

    void set(php::array& root, std::string_view prefix, std::size_t index, const php::value& v) {
        std::size_t pe = -1, ps = -1;
        zval x;
        ZVAL_ARR(&x, static_cast<zend_array*>(root));
        php::array ctr (&x, true); // 纯引用：容器数组
        std::string_view field = prefix.substr( prefix.find_last_of('.') + 1 );
        prefix = prefix.substr(0, prefix.size() - field.size() - 1);
        do {
            ps = pe = pe + 1;
            pe = prefix.find_first_of('.', ps); // prefix 存在，但不含有 . 也需要进行下面动作
            auto sv = prefix.substr(ps, pe - ps);
            if (sv.size() == 0 || sv == "$") {
                // root 元素不做处理
            }
            else if (sv == "#") {
                php::value y = ctr.get(index);
                if(y.type_of(php::TYPE::UNDEFINED)) {
                    php::value z = php::array(4);
                    ctr.set(index, z, /* seperate = */false); // 更新原数组
                    ZVAL_ARR(&x, static_cast<zend_array*>(z));
                }
                else if (y.type_of(php::TYPE::ARRAY)) ZVAL_ARR(&x, static_cast<zend_array*>(y));
                else throw php::exception(zend_ce_type_error, "Failed to set: container (index) is not an array", -1);
            }
            else {
                php::string key {sv.data(), sv.size()};
                php::value y = ctr.get(key);
                if(y.type_of(php::TYPE::UNDEFINED)) {
                    php::value z = php::array(4);
                    ctr.set(key, z, /* seperate = */false); // 更新原数组
                    ZVAL_ARR(&x, static_cast<zend_array*>(z));
                }
                else if(y.type_of(php::TYPE::ARRAY)) ZVAL_ARR(&x, static_cast<zend_array*>(y));
                else throw php::exception(zend_ce_type_error, "Failed to set: container (field) is not an array", -1);
            }
        } while(pe != prefix.npos);

        if(field.empty() || field == "#") ctr.set(ctr.size(), v);
        else ctr.set({field.data(), field.size()}, v);
    }

    php::value get(php::array root, std::string_view prefix, std::size_t index) {
        std::size_t pe = -1, ps = -1;
        zval x;
        ZVAL_ARR(&x, static_cast<zend_array*>(root));
        php::array ctr (&x, true); // 指向容器数组
        std::string_view field = prefix.substr( prefix.find_last_of('.') + 1 );
        prefix = prefix.substr(0, prefix.size() - field.size() - 1);
        do {
            ps = pe = pe + 1;
            pe = prefix.find_first_of('.', ps); // prefix 存在，但不含有 . 也需要进行下面动作
            auto sv = prefix.substr(ps, pe - ps);
            if (sv.size() == 0 || sv == "$") {
                // root 元素不做处理
                // 无 prefix 不做处理
            }
            else if (sv == "#") {
                php::value y = ctr.get(index);
                if (y.type_of(php::TYPE::UNDEFINED)) return php::value();
                else if (y.type_of(php::TYPE::ARRAY)) ZVAL_ARR(&x, static_cast<zend_array*>(y));
                else throw php::exception(zend_ce_type_error, "Failed to get: container (index) is not an array", -1);
            }
            else {
                php::value y = ctr.get({sv.data(), sv.size()});
                if (y.type_of(php::TYPE::UNDEFINED)) return php::value();
                else if (y.type_of(php::TYPE::ARRAY)) ZVAL_ARR(&x, static_cast<zend_array*>(y));
                else throw php::exception(zend_ce_type_error, "Failed to get: container (field) is not an array", -1);
            }
        } while(pe != prefix.npos);

        if(field.empty()) return ctr;
        else return ctr.get({field.data(), field.size()});
    }

    static php::value parse_string(php::parameters& params) {
        php::string s = params[0];
        php::array r(8);
        _executor  e(r);
        llparse::toml::parser p({std::ref(e), std::ref(e)});
        p.parse({s.data(), s.size()});
        return r;
    }

    static php::value parse_file(php::parameters& params) {
        std::string   f = params[0];
        std::ifstream s {f.c_str()};
        if (!s.is_open()) 
            throw php::exception(zend_ce_error_exception, (boost::format("Failed to parse toml: unable to open file '%s'") % f).str(), -1);
        
        php::array r(8);
        _executor  e(r);
        llparse::toml::parser p({std::ref(e), std::ref(e)});
        char        buffer[2048];
        std::size_t length = 0;
        while(!s.eof()) {
            s.read(buffer, sizeof(buffer));
            length = s.gcount();
            if (length > 0) p.parse({buffer, length});
        }
        return r;
    }

    

    void declare(php::extension_entry &ext) {
        ext
            .function<parse_string>("flame\\toml\\parse_string", {
                {"toml", php::TYPE::STRING},
            })
            .function<parse_file>("flame\\toml\\parse_file", {
                {"file", php::TYPE::STRING},
            });
    }
}