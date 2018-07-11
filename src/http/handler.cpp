#include "../coroutine.h"
#include "handler.h"
#include "value_body.h"
#include "server_request.h"
#include "server_response.h"
#include "server.h"
#include "http.h"

namespace flame {
namespace http {
	handler::handler(server* svr, tcp::socket&& sock, std::shared_ptr<flame::coroutine> co)
	: svr_(svr)
	, socket_(std::move(sock))
	, co_(co) {

	}
	void handler::handle(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {
		do {
			req_ref = php::object(php::class_entry<server_request>::entry());
			res_ref = php::object(php::class_entry<server_response>::entry());
			req_ = static_cast<server_request*>(php::native(req_ref));
			res_ = static_cast<server_response*>(php::native(res_ref));

			BOOST_ASIO_CORO_YIELD boost::beast::http::async_read(socket_, buffer_, req_->ctr_,
				std::bind(&handler::handle, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			if(error == boost::beast::http::error::end_of_stream || error == boost::asio::error::eof || error == boost::asio::error::broken_pipe) {
				return;
			}else if(error) {
				co_->fail(error);
				return;
			}
			req_->build_ex();
			res_->ctr_.keep_alive(req_->ctr_.keep_alive());
			BOOST_ASIO_CORO_YIELD {
				auto ptr = this->shared_from_this();
				res_->handler_ = this->shared_from_this();
				// 启动协程执行请求处理流程(before/route/after)
				// 注意: 在此流程中发生异常, 可能导致请求或响应对象"泄露" (并非真的泄露, 指示此流程可能无法回收这两个对象)
				std::make_shared<flame::coroutine>()
					->stack(php::callable([this, ptr] (php::parameters& params) -> php::value {
						if(!(res_->status_ & server_response::STATUS_ALREADY_BUILT) && !res_ref.get("body").typeof(php::TYPE::NULLABLE)) {
							// 1. Content-Length: xxxxx 用法
							res_->build_ex();
							boost::beast::http::async_write(socket_, res_->sr_,
								std::bind(&handler::handle, ptr, std::placeholders::_1, std::placeholders::_2));
						}else{
							// 2. Transfer-Encoding: chunked 用法
							boost::asio::post(context, std::bind(&handler::handle, ptr, boost::system::error_code(), 0));
						}
						// 两种方式都需要在完成时恢复当前 handler 协程 handler::handle 以期服用连接
						return nullptr;
					}))
					// after 处理
					->stack(php::callable([this, ptr] (php::parameters& params) -> php::value {
						auto iafter = svr_->cb_.find("after");
						if(iafter != svr_->cb_.end()) {
							// 确认是否匹配了目标处理器
							std::string route = req_ref.get("method");
							route.push_back(':');
							route.append(req_ref.get("path"));
							auto ipath = svr_->cb_.find(route);
							return iafter->second.call({req_ref, res_ref, ipath != svr_->cb_.end()});
						}
						return nullptr;
					}))
					// path 处理
					->stack(php::callable([this, ptr] (php::parameters& params) -> php::value {
						// 执行前计算并重新查找目标处理器(允许用户在 before 中进行更改)
						std::string route = req_ref.get("method");
						route.push_back(':');
						route.append(req_ref.get("path"));

						auto ipath = svr_->cb_.find(route);
						if(ipath != svr_->cb_.end())  {
							return ipath->second.call({req_ref, res_ref});
						}
						return nullptr;
					}))
					// before 处理
					->start(php::callable([this, ptr] (php::parameters& params) -> php::value {
						auto ibefore = svr_->cb_.find("before");
						if(ibefore != svr_->cb_.end()) {
							// 确认是否匹配了目标处理器
							std::string route = req_ref.get("method");
							route.push_back(':');
							route.append(req_ref.get("path"));
							auto ipath = svr_->cb_.find(route);
							return ibefore->second.call({req_ref, res_ref, ipath != svr_->cb_.end()});
						}
						return nullptr;
					}));
			};
			// 此处恢复可能存在两种可能:
			// 1. 上述 stack 中回调 (Content-Length 响应);
			// 2. server_response::end() 后回调 (Chunked 响应);
			if(error == boost::asio::error::eof || error == boost::asio::error::broken_pipe) {
				return;
			}else if(error) {
				co_->fail(error);
				return;
			}
		} while(!res_->ctr_.need_eof()); // KeepAlive
	}}
}
}
