#include "../controller.h"
#include "../coroutine.h"
#include "../url.h"
#include "_client.h"
#include "rabbitmq.h"
#include "consumer.h"
#include "producer.h"
#include "message.h"

namespace flame::rabbitmq {
    void declare(php::extension_entry &ext) {
        ext
            .function<consume>("flame\\rabbitmq\\consume", {
                {"address", php::TYPE::STRING},
                {"queue", php::TYPE::STRING},
            })
            .function<produce>("flame\\rabbitmq\\produce", {
                {"address", php::TYPE::STRING},
            });

        consumer::declare(ext);
        producer::declare(ext);
        message::declare(ext);
    }

    php::value consume(php::parameters &params) {
        coroutine_handler ch {coroutine::current};
        url u(params[0]);
        //  u.query.clear();
        std::shared_ptr<_client> cc = std::make_shared<_client>(u, ch);

        // cc->connect(ch);

        php::object obj(php::class_entry<consumer>::entry());
        consumer* ptr = static_cast<consumer*>(php::native(obj));
        ptr->cc_ = cc;
        ptr->qn_ = static_cast<std::string>(params[1]);
        return std::move(obj);
    }

    php::value produce(php::parameters &params) {
        coroutine_handler ch{coroutine::current};
        url u(params[0]);
        // u.query.clear();
        std::shared_ptr<_client> cc = std::make_shared<_client>(u, ch);
        
        // cc->connect(ch);

        php::object obj(php::class_entry<producer>::entry());
        producer *ptr = static_cast<producer*>(php::native(obj));
        ptr->cc_ = cc;
        return std::move(obj);
    }
    // 仅支持一维数组
    php::array table2array(const AMQP::Table& table) {
        php::array data(table.size());
        for (auto key : table.keys())  {
            const AMQP::Field &field = table.get(key);
            if (field.isBoolean()) data.set(key, static_cast<uint8_t>(field) ? true : false);
            else if (field.isInteger()) data.set(key, static_cast<int64_t>(field));
            else if (field.isDecimal()) data.set(key, static_cast<double>(field));
            else if (field.isString()) data.set(key, (const std::string &)field);
            else if (field.typeID() == 'd') data.set(key, static_cast<double>(field));
            else if (field.typeID() == 'f') data.set(key, static_cast<float>(field));
            // TODO 记录警告信息?
        }
        return std::move(data);
    }
    // 仅支持一维数组
    AMQP::Table array2table(const php::array& table) {
        AMQP::Table data;
        if (!table.typeof(php::TYPE::ARRAY) || table.empty()) return std::move(data);
        for (auto i = table.begin(); i != table.end(); ++i) {
            if (i->second.typeof(php::TYPE::BOOLEAN)) data.set(i->first.to_string(), i->second.to_boolean());
            else if (i->second.typeof(php::TYPE::INTEGER)) data.set(i->first.to_string(), i->second.to_integer());
            else if (i->second.typeof(php::TYPE::STRING)) data.set(i->first.to_string(), i->second.to_string());
            else if (i->second.typeof(php::TYPE::FLOAT)) data.set(i->first.to_string(), AMQP::Double{i->second.to_float()});
            else throw php::exception(zend_ce_type_error
                , (boost::format("Failed to convert array: unsupported type '%s'") % i->second.typeof().name()).str()
                , -1);
        }
        return std::move(data);
    }
} // namespace flame::rabbitmq
