#include "../coroutine.h"
#include "mongodb.h"
#include "_connection_pool.h"
#include "client.h"
#include "cursor.h"
#include "collection.h"
#include "object_id.h"
#include "date_time.h"

namespace flame::mongodb {

    void declare(php::extension_entry &ext) {
        gcontroller->on_init([] (const php::array& options) {
                mongoc_init();
            })->on_stop([]() {
                // mongoc_cleanup();
            });
        ext
            .function<connect>("flame\\mongodb\\connect");
        client::declare(ext);
        object_id::declare(ext);
        date_time::declare(ext);
        cursor::declare(ext);
        collection::declare(ext);
    }

    php::value connect(php::parameters &params) {
        php::object obj(php::class_entry<client>::entry());
        client *ptr = static_cast<client *>(php::native(obj));
        std::string url = params[0];
        ptr->cp_.reset(new _connection_pool(url));

        // TODO 优化: 实际确认建立第一个连接?
        return std::move(obj);
    }

    php::value iter2value(bson_iter_t *i) {
        switch (bson_iter_type(i)) {
        case BSON_TYPE_DOUBLE:
            return bson_iter_double(i);
        case BSON_TYPE_UTF8: {
            std::uint32_t size = 0;
            const char *data = bson_iter_utf8(i, &size);
            return php::string(data, size);
        }
        case BSON_TYPE_DOCUMENT:
        case BSON_TYPE_ARRAY: {
            bson_iter_t j;
            bson_iter_recurse(i, &j);
            php::array a(4);
            while (bson_iter_next(&j)) a.set(bson_iter_key(&j), iter2value(&j));
            return std::move(a);
        }
        case BSON_TYPE_BINARY: {
            std::uint32_t size = 0;
            const unsigned char *data;
            bson_iter_binary(i, nullptr, &size, &data);
            return php::string((const char *)data, size);
        }
        case BSON_TYPE_OID: {
            php::object o(php::class_entry<object_id>::entry());
            object_id *o_ = static_cast<object_id *>(php::native(o));
            bson_oid_copy(bson_iter_oid(i), &o_->oid_);
            return std::move(o);
        }
        case BSON_TYPE_BOOL:
            return bson_iter_bool(i);
        case BSON_TYPE_DATE_TIME: {
            php::object o(php::class_entry<date_time>::entry());
            date_time *o_ = static_cast<date_time *>(php::native(o));
            o_->tm_ = bson_iter_date_time(i);
            return o;
        }
        case BSON_TYPE_INT32:
            return bson_iter_int32(i);
        case BSON_TYPE_INT64:
            return bson_iter_int64(i);
        default:
            return nullptr;
        }
    }

    php::array bson2array(std::shared_ptr<bson_t> v) {
        return bson2array(v.get());
    }

    php::array bson2array(bson_t* v) {
        if (v == nullptr) return nullptr;

        php::array doc(4);

        bson_iter_t i;
        bson_oid_t oid;
        bson_iter_init(&i, v);
        while (bson_iter_next(&i)) doc.set(bson_iter_key(&i), iter2value(&i));
        return std::move(doc);
    }

    std::shared_ptr<bson_t> array2bson(const php::array &v) {
        assert(v.type_of(php::TYPE::ARRAY));
        bson_t *doc = bson_new();
        for (auto i = v.begin(); i != v.end(); ++i) {
            php::string key = i->first;
            key.to_string();
            php::value val = i->second;
            switch (Z_TYPE_P(static_cast<zval *>(val))) {
            case IS_UNDEF:
                break;
            case IS_NULL:
                bson_append_null(doc, key.c_str(), key.size());
                break;
            case IS_TRUE:
                bson_append_bool(doc, key.c_str(), key.size(), true);
                break;
            case IS_FALSE:
                bson_append_bool(doc, key.c_str(), key.size(), false);
                break;
            case IS_LONG:
                bson_append_int64(doc, key.c_str(), key.size(), static_cast<std::int64_t>(val));
                break;
            case IS_DOUBLE:
                bson_append_double(doc, key.c_str(), key.size(), static_cast<double>(val));
                break;
            case IS_STRING: {
                php::string str = val;
                bson_append_utf8(doc, key.c_str(), key.size(), str.c_str(), str.size());
                break;
            }
            case IS_ARRAY: {
                auto a = array2bson(val);
                if (bson_has_field(a.get(), "0") || bson_count_keys(a.get()) == 0)
                    bson_append_array(doc, key.c_str(), key.size(), a.get());
                else
                    bson_append_document(doc, key.c_str(), key.size(), a.get());
                break;
            }
            case IS_OBJECT: {
                php::object o = val;
                if (o.instanceof (php_date_get_date_ce())) // PHP 内置的 DateTime 类型
                    bson_append_date_time(doc, key.c_str(), key.size(), static_cast<std::int64_t>(o.call("getTimestamp")) * 1000);
                else if (o.instanceof (php::class_entry<date_time>::entry())) {
                    date_time *o_ = static_cast<date_time *>(php::native(o));
                    bson_append_date_time(doc, key.c_str(), key.size(), o_->tm_);
                }
                else if (o.instanceof (php::class_entry<object_id>::entry())) {
                    object_id *o_ = static_cast<object_id *>(php::native(o));
                    bson_append_oid(doc, key.c_str(), key.size(), &o_->oid_);
                }
                else {
                    bson_t uninitialized;
                    bson_append_document_begin(doc, key.c_str(), key.size(), &uninitialized);
                    bson_append_document_end(doc, &uninitialized);
                }
            }
            }
        }
        return std::shared_ptr<bson_t>(doc, bson_destroy);
    }
} // namespace flame::mongodb
