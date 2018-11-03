#include "../coroutine.h"
#include "value_body.h"
#include "server_request.h"
#include "server_response.h"
#include "server.h"
#include "handler.h"
#include "http.h"

namespace flame {
namespace http {
	handler::handler(server* svr, tcp::socket&& sock, std::shared_ptr<flame::coroutine> co)
	: svr_(svr)
	, socket_(std::move(sock))
	, co_(co) {

	}
	handler::~handler() {

	}
	void handler::handle(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {
		do {
			// 准备接收请求
			req_.reset(new boost::beast::http::message<true, value_body<true>>());
			BOOST_ASIO_CORO_YIELD boost::beast::http::async_read(socket_, buffer_, *req_,
				std::bind(&handler::handle, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			// 服务端逻辑 (暂不报错)
			if(error) return;
			// 准备拼装响应
			res_.reset(new boost::beast::http::message<false, value_body<false>>());
			res_->keep_alive(req_->keep_alive());
			BOOST_ASIO_CORO_YIELD {
				php::object req = php::object(php::class_entry<server_request>::entry());
				php::object res = php::object(php::class_entry<server_response>::entry());
				server_request*  req_ptr = static_cast<server_request*>(php::native(req));
				server_response* res_ptr = static_cast<server_response*>(php::native(res));
				// 将 C++ 请求对象展开到 PHP 中
				req_ptr->build_ex(*req_);
				auto ptr = req_ptr->handler_ = res_ptr->handler_ = this->shared_from_this();
				// 启动协程执行请求处理流程(before/route/after)
				// 注意: 在此流程中发生异常, 可能导致请求或响应对象"泄露" (并非真的泄露, 指示此流程可能无法回收这两个对象)
				std::make_shared<flame::coroutine>()
					->stack(php::callable([this, req, res, res_ptr, ptr] (php::parameters& params) {
						if(res_ptr->status_ & RESPONSE_STATUS_FINISHED) {
							// 1. 若协程结束时, 响应已经完毕, 则直接恢复复用连接
							boost::asio::post(context, std::bind(&handler::handle, ptr, boost::system::error_code(), 0));
						}else{
							// 2. 还未完成, 标记脱离响应 (在各 writer 中需要对此种情况恢复 HANDLER 复用连接)
							res_ptr->status_ |= RESPONSE_STATUS_DETACHED;
						}
						return nullptr;
					}))
					// after 处理
					->stack(php::callable([this, req, res, ptr] (php::parameters& params) -> php::value {
						auto iafter = svr_->cb_.find("after");
						if(iafter != svr_->cb_.end()) {
							// 确认是否匹配了目标处理器
							std::string route = req.get("method");
							route.push_back(':');
							route.append(req.get("path"));
							auto ipath = svr_->cb_.find(route);
							return iafter->second.call({req, res, ipath != svr_->cb_.end()});
						}
						return nullptr;
					}))
					// path 处理
					->stack(php::callable([this, req, res, ptr] (php::parameters& params) -> php::value {
						// 执行前计算并重新查找目标处理器(允许用户在 before 中进行更改)
						std::string route = req.get("method");
						route.push_back(':');
						route.append(req.get("path"));

						auto ipath = svr_->cb_.find(route);
						if(ipath != svr_->cb_.end())  {
							return ipath->second.call({req, res});
						}
						return nullptr;
					}))
					// before 处理
					->start(php::callable([this, req, res, ptr] (php::parameters& params) -> php::value {
						auto ibefore = svr_->cb_.find("before");
						if(ibefore != svr_->cb_.end()) {
							// 确认是否匹配了目标处理器
							std::string route = req.get("method");
							route.push_back(':');
							route.append(req.get("path"));
							auto ipath = svr_->cb_.find(route);
							return ibefore->second.call({req, res, ipath != svr_->cb_.end()});
						}
						return nullptr;
					}));
			};
			// 此处恢复可能存在两种可能:
			// 1. 上述 stack 中回调 (Content-Length 响应);
			// 2. server_response::end() 后回调 (Chunked 响应);
			if(error == boost::asio::error::eof || error == boost::asio::error::broken_pipe || error == boost::asio::error::connection_reset) {
				n = 0; // 客户端连接关闭, 标志无需 keepalive
			}else if(error) {
				n = 0;
				co_->fail(error);
			}
		} while(n > 0); //
	}}
}
}
