#pragma once

namespace flame::rabbitmq
{

    class message : public php::class_base
    {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters &params); // 私有
        php::value to_json(php::parameters &params);
        php::value to_string(php::parameters &params);

        void build_ex(const AMQP::Message &msg, std::uint64_t tag);
        void build_ex(AMQP::Envelope &env);
        std::uint64_t tag_;
    };
    
} // namespace flame::rabbitmq
