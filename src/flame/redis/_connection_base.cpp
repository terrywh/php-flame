#include "_connection_base.h"

namespace flame::redis {
    
    static php::value simple2value(redisReply* rp) {
        if (!rp) return nullptr;
        switch (rp->type) {
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
        case REDIS_REPLY_STRING:
            return php::string(rp->str, rp->len);
        case REDIS_REPLY_INTEGER:
            return static_cast<std::int64_t>(rp->integer);
        case REDIS_REPLY_ARRAY: {
            php::array data(rp->elements);
            for(int i=0;i<rp->elements;++i) {
                data.set(data.size(), simple2value(rp->element[i]));
            }
            return data;
        }
        case REDIS_REPLY_NIL:
        default:
            return nullptr;
        }
    }

    php::value _connection_base::reply2value(redisReply* rp, php::array &argv, reply_type rt) {
        switch(rt) {
            case reply_type::ASSOC_ARRAY_1: {
                php::array data(rp->elements/2 + 1);
                for (int i = 0; i < rp->elements; i += 2) {
                    php::string key = simple2value(rp->element[i]).to_string();
                    data.set(key, simple2value(rp->element[i+1]));
                }
                return data;
            }
            case reply_type::ASSOC_ARRAY_2: {
                assert(rp->elements == 2 && rp->element[0]->type == REDIS_REPLY_STRING && rp->element[1]->type == REDIS_REPLY_ARRAY);
                php::array wrap(2);
                php::array data(rp->element[1]->elements/2 + 1);
                wrap.set(0, simple2value(rp->element[0]));
                for (int i = 0; i < rp->element[1]->elements; i += 2) {
                    php::string key = simple2value(rp->element[1]->element[i]).to_string();
                    data.set(key, simple2value(rp->element[1]->element[i+1]));
                }
                wrap.set(1, data); 
                return wrap;
            }
            case reply_type::ASSOC_ARRAY_3: {
                php::array data(rp->elements/2 + 1);
                for (int i = 0; i < rp->elements; i += 2) {
                    php::string key = simple2value(rp->element[i+1]).to_string();
                    data.set(key, simple2value(rp->element[i]));
                }
                return data;
            }
            case reply_type::COMBINE_1: {
                php::array data(rp->elements);
                for (int i = 0; i < rp->elements; ++i) {
                    php::string key = argv[i].to_string();
                    data.set(key, simple2value(rp->element[i]));
                }
                return data;
            }
            case reply_type::COMBINE_2: {
                php::array data(rp->elements);
                for (int i = 0; i < rp->elements; ++i) {
                    php::string key = argv[i+1].to_string();
                    data.set(key, simple2value(rp->element[i]));
                }
                return data;
            }
            case reply_type::SIMPLE:
            default:
                return simple2value(rp);
        }
    }

    std::string _connection_base::format(php::string& name, php::array& argv) {
        std::ostringstream ss;
        ss << "*" << 1 + argv.size() << "\r\n$" << name.size() << "\r\n" << name << "\r\n";
        for (auto i = argv.begin(); i != argv.end(); ++i) {
            php::string v = i->second.to_string();
            if (v.size() > 0) ss << "$" << v.size() << "\r\n" << v << "\r\n";
            else ss << "$0\r\n\r\n";
        }
        return ss.str();
    }
}