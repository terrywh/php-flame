#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "cluster.h"
#include "messenger.h"

namespace flame {
namespace os {
namespace cluster {
	messenger* default_msg = nullptr;
	static php::value handle(php::parameters& params) {
		if(default_msg == nullptr) {
			default_msg = new messenger();
			default_msg->start();
		}
		default_msg->cb_socket = params[0];
		if(!default_msg->cb_socket.is_closure() && default_msg->cb_socket.is_object()
			&& static_cast<php::object&>(default_msg->cb_socket).prop("__CONNECTION_HANDLER__", 22).is_true()) {

			default_msg->cb_type |= 0x04;
		}else if(default_msg->cb_socket.is_callable()) {
			default_msg->cb_type |= 0x02;
		}else{
			throw php::exception("callale is required as socket handler");
		}
		return nullptr;
	}
	static php::value ondata(php::parameters& params) {
		if(default_msg == nullptr) {
			default_msg = new messenger();
			default_msg->start();
		}
		default_msg->cb_string = params[0];
		if(default_msg->cb_string.is_callable()) {
			default_msg->cb_type |= 0x01;
		}else{
			throw php::exception("callale is required as string handler");
		}
		return nullptr;
	}
	static php::value send(php::parameters& params) {
		if(default_msg != nullptr) {
			if(!params[0].is_string()) {
				throw php::exception("failed to send: only string is allowed");
			}
			default_msg->send(params);
		}else{
			throw php::exception("failed to send: ipc handler not set");
		}
		return flame::async();
	}
	void init(php::extension_entry& ext) {
		ext.on_module_shutdown([] (php::extension_entry& ext) -> bool {
			if(default_msg) {
				delete default_msg; // 这里没有使用 ->close()
				// 因为 flame::loop 已经停止
			}
			return true;
		});
		ext.add<handle>("flame\\os\\cluster\\handle");
		ext.add<ondata>("flame\\os\\cluster\\ondata");
		ext.add<send>("flame\\os\\cluster\\send");
	}
}
}
}
