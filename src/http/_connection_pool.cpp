#include "../controller.h"
#include "../coroutine.h"
#include "value_body.h"
#include "client_request.h"
#include "client_response.h"
#include "_connection_pool.h"
#include "../url.h"

namespace flame::http
{
    _connection_pool::_connection_pool(uint32_t cph)
    : max_(cph), tm_(gcontroller->context_x), resolver_(gcontroller->context_x) {
    }

    _connection_pool::~_connection_pool() {
    }

    void _connection_pool::acquire(std::shared_ptr<url> url, std::shared_ptr<_connection_pool::context_t> ctx) {
        auto type = url->schema + url->host + std::to_string(url->port);
        while(p_.count(type) > 0) {
            auto it = p_.find(type);
            if(it != p_.end()) {
                auto s = it->second.first;
                p_.erase(it);
                s->async_wait(tcp::socket::wait_write, [ctx](const boost::system::error_code& ec) {
                    if(ec == boost::asio::error::operation_aborted || (ctx->timeout())) return ;
                    ctx->ec_ = ec;
                    ctx->ch_.resume();
                });
                ctx->ch_.suspend();
                if(ctx->timeout())
                    goto DONE;
                if(!ctx->ec_) {
                    ctx->c_ = s;
                    goto DONE;
                }
                // 连接异常，直接释放
                release(type, nullptr);
            }
            ctx->ec_ = boost::system::error_code{};
        }

        // 未获取到连接，且连接数达到最大
        if(!ctx->c_ && ps_[type] >= max_) {
            // 此处可能由于释放的连接已关闭
            // 导致此处连接为空，而需要重建连接
            ctx->wait();
            // 在此处release连接会导致连接数只增不减
            // 此处超时不应该减连接数
            if(!ctx->c_ && !ctx->timeout())
                --ps_[type];
        }
DONE:
        // 引处不由于超时，并未获取到连接，所以无需释放连接
        if(ctx->timeout())
            throw php::exception(zend_ce_exception, (boost::format("timeout to acquire new connection, host %1%, connection limit %2%") % (url->host + std::to_string(url->port)) % max_).str());
        if(!ctx->c_)
            create(url, ctx);
    }

    void _connection_pool::create(std::shared_ptr<url> u, std::shared_ptr<_connection_pool::context_t> ctx) {
        auto c    = std::make_shared<tcp::socket>(gcontroller->context_x);
        auto type = u->schema + u->host + std::to_string(u->port);
        tcp::resolver::results_type eps_;
        ++ps_[type];
        resolver_.async_resolve(
            u->host, 
            std::to_string(u->port), 
            [&eps_, ctx, c] (const boost::system::error_code& ec, tcp::resolver::results_type eps) {
                if(ctx->timeout()) return ;
                ctx->ec_ = ec;
                eps_ = std::move(eps);
                ctx->ch_.resume();
        });
        ctx->ch_.suspend();
        if(ctx->ec_ || ctx->timeout())
            goto END;

        ctx->to_ |= context_t::CONNECT;
        boost::asio::async_connect(*c, eps_, [c, ctx] (const boost::system::error_code& ec, const tcp::endpoint& ep) {
            if(ec == boost::asio::error::operation_aborted || ctx->timeout()) return ;
            ctx->ec_ = ec;
            ctx->ch_.resume();
        });
        ctx->ch_.suspend();
END:
        if(ctx->timeout()) {
            if(ctx->to_ & context_t::CONNECT)
                c->cancel();
            release(type, nullptr);
            throw php::exception(zend_ce_exception, "timeout to create new connection");
        } 
        if(ctx->ec_) {
            release(type, nullptr);
            ctx->tm_.cancel();
            throw php::exception(zend_ce_exception, (boost::format("failed to create new connection: (%1%) %2%") % ctx->ec_.value() % ctx->ec_.message()).str(), ctx->ec_.value());
        }
        ctx->c_ = c;
    }

    php::value _connection_pool::execute(client_request* req, int32_t timeout, coroutine_handler &ch) {
        auto type = req->url_->schema + req->url_->host + std::to_string(req->url_->port);
        auto ctx  = std::make_shared<_connection_pool::context_t>(ch, wl_[type], timeout);
        ctx->expire();
        acquire(req->url_, ctx);

        // 构建并发送请求
        boost::beast::http::async_write(*ctx->c_, req->ctr_, [ctx](const boost::system::error_code& ec, std::size_t n) {
            if(ec == boost::asio::error::operation_aborted || ctx->timeout()) return ;
            ctx->ec_ = ec;
            ctx->ch_.resume();
        });
        ctx->ch_.suspend();
        if(ctx->timeout()) {
            ctx->c_->cancel();
            release(type, nullptr);
            throw php::exception(zend_ce_exception, "failed to write request: timeout");
        }
        if(ctx->ec_) {
            ctx->tm_.cancel();
            release(type, nullptr);
            throw php::exception(zend_ce_exception, (boost::format("failed to write request: (%1%) %2%") % ctx->ec_.value() % ctx->ec_.message()).str(), ctx->ec_.value());
        }

        // 构建并读取响应
        auto res_ref = php::object(php::class_entry<client_response>::entry());
        auto res_    = static_cast<client_response*>(php::native(res_ref));
        boost::beast::flat_buffer b;
        boost::beast::http::async_read(*ctx->c_, b, res_->ctr_, [ctx] (const boost::system::error_code& ec, std::size_t n) {
            if(ec == boost::asio::error::operation_aborted || ctx->timeout()) return ;
            ctx->ec_  = ec;
            ctx->to_ |= context_t::DONE;
            ctx->ch_.resume();
        });

        ctx->ch_.suspend();
        if(ctx->timeout()) {
            ctx->c_->cancel();
            release(type, nullptr);
            throw php::exception(zend_ce_exception, "failed to read response: timeout");
        }
        if(ctx->ec_) {
            ctx->tm_.cancel();
            release(type, nullptr);
            throw php::exception(zend_ce_exception, (boost::format("failed to read response: (%1%) %2%") % ctx->ec_.value() % ctx->ec_.message()).str(), ctx->ec_.value());
        }

        res_->build_ex();
        ctx->tm_.cancel();

        if(res_->ctr_.keep_alive() && req->ctr_.keep_alive())
            release(type, ctx->c_);
        else
            release(type, nullptr);
        return res_ref;
    }

    void _connection_pool::release(std::string type, std::shared_ptr<tcp::socket> s) {
        // read / write timeout，释放空连接
        // 队列中有等待，导致callback->resume会在io run 排队，
        // 导致其他协程通过 connections < max 
        if (wl_[type].empty()) {
            // 无等待分配的请求
            if(s)
                p_.insert(std::make_pair(type, std::make_pair(s, std::chrono::steady_clock::now())));
            else
                --ps_[type];
        } else {
            // 立刻分配使用
            auto ctx = wl_[type].front();
            wl_[type].pop_front();
            // 释放空连接时，连接池计数在cb->resume后减小
            (*ctx)(s);
        }
    }

    void _connection_pool::sweep() {
        tm_.expires_from_now(std::chrono::seconds(30));
        tm_.async_wait([this] (const boost::system::error_code &ec) {
            auto now = std::chrono::steady_clock::now();
            if(ec) return; // 当前对象销毁时会发生对应的 abort 错误

            for (auto i = p_.begin(); i != p_.end();) {
                auto itvl = std::chrono::duration_cast<std::chrono::seconds>(now - i->second.second);
                if(itvl.count() > 30) {
                    //--ps_[i->first];
                    release(i->first, nullptr);
                    i = p_.erase(i);
                } else
                    ++i;
            }
            for(auto i = wl_.begin(); i != wl_.end(); ) {
                if(i->second.empty())
                    i = wl_.erase(i);
                else
                    ++i;
            }
            // 再次启动
            sweep();
        });
    }

    _connection_pool::context_t::context_t(coroutine_handler &ch, _connection_pool::queue_t& q, int32_t to) 
    : ch_(ch)
    , to_(NORMAL)
    , tm_(gcontroller->context_x)
    , q_(q)
    , tp_(std::chrono::steady_clock::now() + std::chrono::milliseconds(to) ){
    }
    void _connection_pool::context_t::expire() {
        boost::system::error_code ec;
        tm_.expires_at(tp_);
        tm_.async_wait([this, ctx = shared_from_this()](const boost::system::error_code &ec) {
            if(ec == boost::asio::error::operation_aborted || (to_ & DONE)) return;
            if(to_ & WAIT) {
                for(auto i = q_.begin(); i != q_.end(); ++i ) {
                    if( i->get() == this) {
                        q_.erase(i);
                        break;
                    }
                }
                to_ &= ~WAIT;
            }
            to_ |= TIMEOUT;
            ec_  = ec;
            ch_.resume();
        });
    }
    void _connection_pool::context_t::wait() {
        to_ |= WAIT;
        q_.push_back(shared_from_this());
        ch_.suspend();
    }
    bool _connection_pool::context_t::timeout() {
        return to_ & TIMEOUT;
    }
    void _connection_pool::context_t::operator()(std::shared_ptr<tcp::socket> c) {
        // timeout的resume与此处的resume可能会在io_context上排队
        // 在timeout时，需要先清理queue，再做resume
        if(timeout()) return ;
        to_ &= ~WAIT;
        c_ = c;
        ch_.resume();
    }
}


