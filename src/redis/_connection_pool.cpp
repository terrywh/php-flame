#include "../controller.h"
#include "_connection_pool.h"

namespace flame::redis
{

    _connection_pool::_connection_pool(url u)
        : url_(std::move(u)), min_(1), max_(4), size_(0), guard_(gcontroller->context_y), tm_(gcontroller->context_y)
    {
        
    }
    _connection_pool::~_connection_pool()
    {
        while (!conn_.empty())
        {
            redisFree(conn_.front().conn);
            conn_.pop_front();
        }
    }

    std::shared_ptr<redisContext> _connection_pool::acquire(coroutine_handler &ch)
    {
        std::shared_ptr<redisContext> conn;
        // 提交异步任务
        boost::asio::post(guard_, [this, &conn, &ch]() {
            // 设置对应的回调, 在获取连接后恢复协程
            await_.push_back([&conn, &ch](std::shared_ptr<redisContext> c) {
                conn = c;
                // RESUME 需要在主线程进行
                boost::asio::post(gcontroller->context_x, std::bind(&coroutine_handler::resume, ch));
            });
            while (!conn_.empty())
            {
                if(ping(conn_.front().conn))
                {
                    // 可用连接
                    redisContext *c = conn_.front().conn;
                    conn_.pop_front();
                    release(c);
                    return;
                }
                else
                { // 连接已丢失，回收资源
                    redisFree(conn_.front().conn);
                    conn_.pop_front();
                    --size_;
                }
            }
            if (size_ >= max_)
                return; // 已建立了足够多的连接, 需要等待已分配连接释放

            redisContext *c = create();
            if (c == nullptr)
            {
                await_.pop_back();
                boost::asio::post(gcontroller->context_x, std::bind(&coroutine_handler::resume, ch));
            }
            else
            {
                ++size_; // 当前还存在的连接数量
                release(c);
            }
        });
        // 暂停, 等待连接获取(异步任务)
        ch.suspend();
        if (!conn)
        {
            throw php::exception(zend_ce_exception, "failed to connect to MySQL server", -1);
        }
        // 恢复, 已经填充连接
        return conn;
    }
    
    php::value _connection_pool::exec(std::shared_ptr<redisContext> rc, php::string &name, php::array &argv, reply_type rt, coroutine_handler& ch)
    {
        std::string ss = format(name, argv);
        redisReply *rp = nullptr;
        boost::asio::post(gcontroller->context_y, [&rc, &ss, &ch, &rp]() {
            redisAppendFormattedCommand(rc.get(), ss.c_str(), ss.size());
            redisGetReply(rc.get(), (void **)&rp);
            boost::asio::post(gcontroller->context_x, std::bind(&coroutine_handler::resume, ch));
        });
        ch.suspend();
        php::value rv = reply2value(rp, argv, rt);
        freeReplyObject(rp);
        return std::move(rv);
    }
    php::value _connection_pool::exec(std::shared_ptr<redisContext> rc, php::string &name, php::parameters &argv, reply_type rt, coroutine_handler& ch)
    {
        php::array data {argv};
        return exec(rc, name, data, rt, ch);
    }

    redisContext *_connection_pool::create()
    {
        struct timeval tv {5, 0};
        redisContext* c = redisConnectWithTimeout(url_.host.c_str(), url_.port, tv);
        if(!c || c->err) {
            return nullptr;
        }
        if(!url_.pass.empty()) {
            // 认证
            redisReply* rp = (redisReply*)redisCommand(c, "AUTH %s", url_.pass.c_str());
            if(!rp || rp->type == REDIS_REPLY_ERROR) {
                if(rp) {
                    redisFree(c);
                    freeReplyObject(rp);
                }
                return nullptr;
            }
        }
        if(url_.path.length() > 1) {
            // 指定数据库
            redisReply *rp = (redisReply *)redisCommand(c, "SELECT %d", std::atoi(url_.path.c_str() + 1));
            if(!rp || rp->type == REDIS_REPLY_ERROR) {
                if(rp) {
                    redisFree(c);
                    freeReplyObject(rp);
                }
                return nullptr;
            }
        }
        return c;
    }
    void _connection_pool::release(redisContext *c)
    {
        if (await_.empty())
        { // 无等待分配的请求
            conn_.push_back({c, std::chrono::steady_clock::now()});
        }
        else
        { // 立刻分配使用
            std::function<void(std::shared_ptr<redisContext> c)> cb = await_.front();
            await_.pop_front();
            auto self = this->shared_from_this();
            cb(
                // 释放回调函数须持有当前对象引用 self
                // (否则连接池可能先于连接归还被销毁)
                std::shared_ptr<redisContext>(c, [this, self](redisContext *c)
                {
                    boost::asio::post(guard_, std::bind(&_connection_pool::release, self, c));
                }));
        }
    }
    bool _connection_pool::ping(redisContext* c) {
        redisReply *rp = (redisReply *)redisCommand(c, "PING");
        if (!rp || rp->type == REDIS_REPLY_ERROR)
        {
            if (rp) freeReplyObject(rp);
            return false;
        }
        return true;
    }

    void _connection_pool::sweep()
    {
        tm_.expires_from_now(std::chrono::seconds(60));
        // 注意, 实际的清理流程需要保证 guard_ 串行流程
        tm_.async_wait(boost::asio::bind_executor(guard_, [this](const boost::system::error_code &error) {
            if (error)
                return; // 当前对象销毁时会发生对应的 abort 错误
            auto now = std::chrono::steady_clock::now();
            for (auto i = conn_.begin(); i != conn_.end() && size_ > min_;)
            {
                // 超低水位，关闭不活跃连接
                auto duration = now - (*i).ttl;
                if (duration > std::chrono::seconds(60) || !ping((*i).conn))
                {
                    redisFree((*i).conn);
                    --size_;
                    i = conn_.erase(i);
                }
                else
                {
                    ++i;
                } // 所有连接还活跃（即使超过低水位）
            }
            // 再次启动
            sweep();
        }));
    }

} // namespace flame::redis
