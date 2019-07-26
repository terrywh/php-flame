#include "../controller.h"
#include "_connection_pool.h"

namespace flame::mysql {
    _connection_pool::_connection_pool(url u)
    : url_(std::move(u)), min_(2), max_(6), size_(0)
    , guard_(gcontroller->context_y)
    , sweep_(gcontroller->context_y)
    , flag_(FLAG_UNKNOWN) {
        if(url_.query.count("proxy") > 0 && std::stoi(url_.query["proxy"]) != 0)
            flag_ =  FLAG_CHARSET_EQUAL | FLAG_REUSE_BY_PROXY;
    }

    _connection_pool::~_connection_pool() {
        close();
    }

    std::shared_ptr<MYSQL> _connection_pool::acquire(coroutine_handler& ch) {
        std::shared_ptr<MYSQL> conn;
        int         errnum = 0;
        std::string errmsg;
        // 提交异步任务
        boost::asio::post(guard_, [this, &errnum, &errmsg, &conn, &ch] () {
            // 设置对应的回调, 在获取连接后恢复协程
            await_.push_back([&conn, &ch] (std::shared_ptr<MYSQL> cc) {
                conn = cc;
                // RESUME 需要在主线程进行
                ch.resume();
            });
            auto now = std::chrono::steady_clock::now();
            while (!conn_.empty()) {
                if (now - conn_.front().ttl < std::chrono::seconds(15)
                    || mysql_ping(conn_.front().conn) == 0) {  // 可用连接

                    MYSQL* c = conn_.front().conn;
                    conn_.pop_front();
                    release(c);
                    return;
                }
                else { // 连接已丢失，回收资源
                    mysql_close(conn_.front().conn);
                    conn_.pop_front();
                    --size_;
                }
            }
            if (size_ >= max_) return; // 已建立了足够多的连接, 需要等待已分配连接释放
            MYSQL* c = mysql_init(nullptr);
            init_options(c);
            if (mysql_real_connect(c, url_.host.c_str(), url_.user.c_str(), url_.pass.c_str(), url_.path.c_str() + 1, url_.port, nullptr, 0)) {
                if (flag_ & FLAG_REUSE_BY_PROXY) set_names(c);
                ++size_; // 当前还存在的连接数量
                release(c);
            }
            else {
                errnum = mysql_errno(c);
                errmsg = mysql_error(c);
                mysql_close(c);
                await_.pop_back();
                ch.resume();
            }
        });
        // 暂停, 等待连接获取(异步任务)
        ch.suspend();
        if (!conn) {
            if (errnum) throw php::exception(zend_ce_exception
                    , (boost::format("Failed to connect to MySQL server: %s") % errmsg).str()
                    , errnum);
            else throw php::exception(zend_ce_exception
                , "Failed to connect to MySQL server", 0);
        }
        // 恢复, 已经填充连接
        return conn;
    }

    void _connection_pool::init_options(MYSQL* c) {
        // 这里的 CHARSET 设定会被 reset_connection 重置为系统值
        mysql_options(c, MYSQL_SET_CHARSET_NAME, url_.query["charset"].c_str());
        // 版本 8.0.3 后默认使用 caching_sha2_password 进行认证，低版本不支持
        mysql_options(c, MYSQL_DEFAULT_AUTH, url_.query["auth"].c_str());
        unsigned int timeout = 5; // 连接超时
        mysql_options(c, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    }

    void _connection_pool::set_names(MYSQL* c) {
        // 兼容 proxysql 流程：纯粹的字符集设置，首次 SQL 会卡住:
        // 代理：2019, Can't initialize character set (null) (path: compiled_in)
        // 后端：不断尝试连接、断开
        std::string sql = (boost::format("SET NAMES '%s'") % url_.query["charset"]).str();
        mysql_real_query(c, sql.c_str(), sql.size());
    }

    void _connection_pool::release(MYSQL *c) {
        // 无等待分配的请求, 保存连接(待后续分配)
        if (await_.empty()) conn_.push_back({c, std::chrono::steady_clock::now()});
        else { // 立刻分配使用
            // 每次连接复用前，需要清理状态;
            // 这里兼容不支持 mysql_reset_connection() 新 API 的情况
            // （不支持自动切换到 mysql_change_user() 兼容老版本或变异版本）
            if (!(flag_ & FLAG_REUSE_MASK)) query_version(c);
            if (flag_ & FLAG_REUSE_BY_RESET) mysql_reset_connection(c);
            else if (flag_ & FLAG_REUSE_BY_CUSER) mysql_change_user(c, url_.user.c_str(), url_.pass.c_str(), url_.path.c_str() + 1);
            // 由于 reset 动作会导致字符集被重置为服务端字符集，确认字符集是否匹配
            if (!(flag_ & FLAG_CHARSET_MASK)) query_charset(c);
            // else if (flag_ & FLAG_REUSE_BY_PROXY) ; // PROXY 已处理复用流程，此处无需处理
            if (flag_ & FLAG_CHARSET_DIFFER)
                mysql_set_character_set(c, url_.query["charset"].c_str());
            std::function<void(std::shared_ptr<MYSQL> c)> cb = await_.front();
            await_.pop_front();
            auto self = this->shared_from_this();
            // 释放回调函数须持有当前对象引用 self (否则连接池可能先于连接归还被销毁)
            cb(std::shared_ptr<MYSQL>(c, [this, self] (MYSQL *c) {
                boost::asio::post(guard_, std::bind(&_connection_pool::release, self, c));
            }));
        }
    }

    void _connection_pool::query_charset(MYSQL* c) {
        // 使用 mysql_get_charactor_set_name() 获取到的字符集与下述查询不同
        if (0 == mysql_real_query(c, "SHOW VARIABLES WHERE `Variable_name`='character_set_client'", 59)) {
            std::unique_ptr<MYSQL_RES, void (*)(MYSQL_RES*)> rst(mysql_store_result(c), mysql_free_result);
            if (!rst) return;
            MYSQL_ROW row = mysql_fetch_row(rst.get());
            if (!row) return;
            unsigned long* len = mysql_fetch_lengths(rst.get());
            std::string charset(row[1], len[1]);
            flag_ |= charset == url_.query["charset"] ? FLAG_CHARSET_EQUAL : FLAG_CHARSET_DIFFER;
        }
        else flag_ |= FLAG_CHARSET_DIFFER; // 这里忽略了 charset 查询的错误
    }

    void _connection_pool::query_version(MYSQL* c) {
        // PROXY 存在时不会进行下述动作
        flag_ |= mysql_get_server_version(c) >= 50700
            ? FLAG_REUSE_BY_RESET
            : FLAG_REUSE_BY_CUSER;
    }

    void _connection_pool::sweep() {
        sweep_.expires_from_now(std::chrono::seconds(60));
        // 注意, 实际的清理流程需要保证 guard_ 串行流程
        sweep_.async_wait(boost::asio::bind_executor(guard_, [this] (const boost::system::error_code &error) {
            if (error) return; // 当前对象销毁时会发生对应的 abort 错误
            auto now = std::chrono::steady_clock::now();
            // 超低水位，关闭不活跃或已丢失的连接
            for (auto i = conn_.begin(); i != conn_.end() && size_ > min_;) {
                auto duration = now - (*i).ttl;
                if (duration > std::chrono::seconds(60) || mysql_ping((*i).conn) != 0) {
                    mysql_close((*i).conn);
                    --size_;
                    i = conn_.erase(i);
                }
                else  ++i;
            }
            // 再次启动
            sweep();
        }));
    }

    void _connection_pool::close() {
        sweep_.cancel();
        while (!conn_.empty()) {
            mysql_close(conn_.front().conn);
            conn_.pop_front();
        }
    }

} // namespace flame::mysql
