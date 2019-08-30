#include "encoding.h"
#include "../mongodb/mongodb.h"

namespace flame::encoding {
    static php::value bson_encode(php::parameters &params) {
        if (params[0].type_of(php::TYPE::ARRAY)) {
            auto doc = flame::mongodb::array2bson(params[0]);
            return php::string((const char*)bson_get_data(doc.get()), doc->len);
        }
        else throw php::exception(zend_ce_type_error
            , "Failed to encode: typeof 'array' required"
            , -1);
    }
    
    static php::value bson_decode(php::parameters &params) {
        php::string data = params[0].to_string();
        bson_t* doc = bson_new_from_data((const uint8_t*)data.data(), data.size());
        if (doc == nullptr) return nullptr;
        php::array rv = flame::mongodb::bson2array(doc);
        bson_destroy(doc);
        return std::move(rv);
    }
    
    void declare(php::extension_entry &ext) {
        ext
            .function<bson_encode>("flame\\encoding\\bson_encode", {
                {"data", php::TYPE::ARRAY},
            })
            .function<bson_decode>("flame\\encoding\\bson_decode", {
                {"data", php::TYPE::STRING},
            });
    }
}
