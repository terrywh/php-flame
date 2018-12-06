#include "object_id.h"

namespace flame::mongodb
{
    void object_id::declare(php::extension_entry &ext)
    {
        php::class_entry<object_id> class_object_id("flame\\mongodb\\object_id");
        class_object_id
            .implements(&php_json_serializable_ce)
            .method<&object_id::__construct>("__construct")
            .method<&object_id::to_string>("__toString")
            .method<&object_id::to_json>("__toJSON")
            .method<&object_id::to_datetime>("__toDateTime")
            .method<&object_id::unix>("unix")
            .method<&object_id::to_json>("jsonSerialize")
            .method<&object_id::to_json>("__debugInfo")
            .method<&object_id::equal>("equal",
                                       {{"object_id", "?flame\\mongodb\\object_id"}});
        ext.add(std::move(class_object_id));
    }
    php::value object_id::__construct(php::parameters &params)
    {
        if (params.size() > 0)
        {
            php::string oid = params[0].to_string();
            bson_oid_init_from_string(&oid_, oid.c_str());
        }
        else
        {
            bson_oid_init(&oid_, nullptr);
        }
        return nullptr;
    }
    php::value object_id::to_string(php::parameters &params)
    {
        php::string str(24);
        bson_oid_to_string(&oid_, str.data());
        return std::move(str);
    }
    php::value object_id::unix(php::parameters &params)
    {
        return bson_oid_get_time_t(&oid_);
    }
    php::value object_id::to_datetime(php::parameters &params)
    {
        return php::datetime(bson_oid_get_time_t(&oid_) * 1000);
    }
    php::value object_id::to_json(php::parameters &params)
    {
        // 定制 JSON 输出形式 (为何官方 MongoDB 驱动保持一致)
        php::array oid(1);
        php::string str(24);
        bson_oid_to_string(&oid_, str.data());
        oid.set("$oid", str);
        return std::move(oid);
    }
    php::value object_id::equal(php::parameters &params)
    {
        if (params[0].typeof(php::TYPE::STRING))
        {
            php::string data = params[0];
            bson_oid_t oid;
            bson_oid_init_from_string(&oid, data.c_str());
            return bson_oid_equal(&oid_, &oid);
        }
        else if (params[0].instanceof (php::class_entry<object_id>::entry()))
        {
            php::object obj = params[0];
            object_id *ptr = static_cast<object_id *>(php::native(obj));
            return bson_oid_equal(&oid_, &ptr->oid_);
        }
        else
        {
            return false;
        }
    }
} // namespace flame::mongodb
