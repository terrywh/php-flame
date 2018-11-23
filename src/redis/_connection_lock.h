#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "_connection_base.h"

namespace flame::redis
{
class _connection_lock : public _connection_base, public std::enable_shared_from_this<_connection_lock>
{
  public:
    _connection_lock(std::shared_ptr<redisContext> c);
    ~_connection_lock();
    std::shared_ptr<redisContext> acquire(coroutine_handler &ch) override;
    void push(php::string &name, php::array &argv, reply_type rt);
    void push(php::string &name, php::parameters &argv, reply_type rt);
    php::array exec(coroutine_handler& ch);
    struct command_t {
        php::string name;
        php::array  argv;
        reply_type  type;
        std::string strs;
        redisReply* reply;
    };
  private:
    std::shared_ptr<redisContext> conn_;
    std::list<command_t> cmds_;
};
} // namespace flame::redis
