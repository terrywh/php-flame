#pragma once

namespace flame::http {
    class _connection_pool;
    class client: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        client();
        php::value __construct(php::parameters& params);
        php::value exec(php::parameters& params);
        php::value get(php::parameters& params);
        php::value post(php::parameters& params);
        php::value put(php::parameters& params);
        php::value delete_(php::parameters& params);
    private:
        php::value exec_ex(const php::object& req);
        std::shared_ptr<_connection_pool> cp_;
    };
}
