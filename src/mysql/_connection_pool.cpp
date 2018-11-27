#include "../controller.h"
#include "_connection_pool.h"

namespace flame::mysql
{
        
    _connection_pool::_connection_pool(url u)
        : url_(std::move(u)), min_(2), max_(8), size_(0), guard_(gcontroller->context_y), tm_(gcontroller->context_y)
    {
        if(!url_.query.count("charset")) {
            url_.query["charset"] = "utf8";
        }
    }
    
    _connection_pool::~_connection_pool()
    {
        while (!conn_.empty())
        {
            mysql_close(conn_.front().conn);
            conn_.pop_front();
        }
    }
    
    std::shared_ptr<MYSQL> _connection_pool::acquire(coroutine_handler& ch)
    {
        std::shared_ptr<MYSQL> conn;
        // 提交异步任务
        boost::asio::post(guard_, [this, &conn, &ch] () {
            // 设置对应的回调, 在获取连接后恢复协程
            await_.push_back([&conn, &ch] (std::shared_ptr<MYSQL> c) {
                conn = c;
                // RESUME 需要在主线程进行
                ch.resume();
            });
            while (!conn_.empty())
            {
                if (mysql_ping(conn_.front().conn) == 0)
                { // 可用连接
                    MYSQL *c = conn_.front().conn;
                    conn_.pop_front();
                    release(c);
                    return;
                }
                else
                { // 连接已丢失，回收资源
                    mysql_close(conn_.front().conn);
                    conn_.pop_front();
                    --size_;
                }
            }
            if (size_ >= max_) return; // 已建立了足够多的连接, 需要等待已分配连接释放

            MYSQL* c = create();
            if(c == nullptr) {
                await_.pop_back();
                ch.resume();
            }else{
                ++size_; // 当前还存在的连接数量
                release(c);
            }
        });
        // 暂停, 等待连接获取(异步任务)
        ch.suspend();
        if(!conn) {
            throw php::exception(zend_ce_exception, "failed to connect to MySQL server", -1);
        }
        // 恢复, 已经填充连接
        return conn;
    }

    MYSQL* _connection_pool::create() {
        MYSQL* c = mysql_init(nullptr);
        mysql_options(c, MYSQL_SET_CHARSET_NAME, url_.query["charset"].c_str());
        unsigned int timeout = 5; // 连接超时
        mysql_options(c, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        if (!mysql_real_connect(c, url_.host.c_str(), url_.user.c_str(), url_.pass.c_str(), url_.path.c_str() + 1, url_.port, nullptr, 0))
        {
            return nullptr;
        }
        return c;
    }

    void _connection_pool::release(MYSQL *c)
    {
        if (await_.empty())
        { // 无等待分配的请求
            conn_.push_back({c, std::chrono::steady_clock::now()});
        }
        else
        { // 立刻分配使用
            std::function<void(std::shared_ptr<MYSQL> c)> cb = await_.front();
            await_.pop_front();
            auto self = this->shared_from_this();
            cb(
                // 释放回调函数须持有当前对象引用 self
                // (否则连接池可能先于连接归还被销毁)
                std::shared_ptr<MYSQL>(c, [this, self](MYSQL *c) {
                    boost::asio::post(guard_, std::bind(&_connection_pool::release, self, c));
                }));
        }
    }

    void _connection_pool::sweep() {
        tm_.expires_from_now(std::chrono::seconds(60));
        // 注意, 实际的清理流程需要保证 guard_ 串行流程
        tm_.async_wait(boost::asio::bind_executor(guard_, [this] (const boost::system::error_code &error) {
            if(error) return; // 当前对象销毁时会发生对应的 abort 错误
            auto now = std::chrono::steady_clock::now();
            for (auto i = conn_.begin(); i != conn_.end() && size_ > min_;)
            {
                // 超低水位，关闭不活跃或已丢失的连接
                auto duration = now - (*i).ttl;
                if (duration > std::chrono::seconds(60) || mysql_ping((*i).conn) != 0)
                {
                    mysql_close((*i).conn);
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

} // namespace flame::mysql
