#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_lock.h"

namespace flame::redis
{

    _connection_lock::_connection_lock(std::shared_ptr<redisContext> c)
        : conn_(c)
    {
    }

    _connection_lock::~_connection_lock()
    {
    }

    std::shared_ptr<redisContext> _connection_lock::acquire(coroutine_handler &ch)
    {
        return conn_;
    }

    void _connection_lock::push(php::string &name, php::array &argv, reply_type type)
    {
        // 暂未提交, 仅记录
        // TODO 优化: 是否可以在提交前在进行 format 操作 (减少内存占用的持续时间) ?
        cmds_.push_back({name, argv, type, format(name, argv)});
    }
    void _connection_lock::push(php::string &name, php::parameters& params, reply_type type) {
        php::array argv {params};
        push(name, argv, type);
    }
    php::array _connection_lock::exec(coroutine_handler &ch) 
    {
        boost::asio::post(gcontroller->context_x, [this, &ch] ()
        {
            // 在工作线程中, 提交所有待执行命令
            for(auto i=cmds_.begin(); i!=cmds_.end(); ++i) {
                redisAppendFormattedCommand(conn_.get(), i->strs.c_str(), i->strs.size());
            }
            // 读取对应的返回值
            for(auto i=cmds_.begin(); i!=cmds_.end(); ++i) {
                redisGetReply(conn_.get(), (void**)&i->reply);
            }
            // 回到主线程
            boost::asio::post(gcontroller->context_x, std::bind(&coroutine_handler::resume, ch));
        });
        ch.suspend();
        // 整合个命令返回值
        php::array data(cmds_.size());
        for(auto i=cmds_.begin(); i!=cmds_.end(); ++i) {
            data.set(data.size(), reply2value(i->reply, i->argv, i->type));
        }
        // 命令执行完毕 (所有返回值处理完成)
        cmds_.clear();
        return std::move(data);
    }

} // namespace flame::redis
