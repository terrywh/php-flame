#include "vendor.h"
#include "keeper.h"
#include "core.h"

void keeper::init(php::extension_entry& extension) {
	extension.add<keeper::keep>("flame\\keep");
	extension.add<keeper::take>("flame\\take");
}
keeper::keeper() {
	event_assign(&ev_, core::base, -1, 0, keeper::timer_handler, this);
}
keeper::~keeper() {
	event_del(&ev_);
}
void keeper::start() {
	struct timeval timeout = { 10, 0 };
	event_add(&ev_, &timeout);
}
void keeper::stop() {
	event_del(&ev_);
}
php::value keeper::take(php::parameters& params) {
	zend_object* kw = params[0];
	core::keep->map_.erase(kw);
	return nullptr;
}
php::value keeper::keep(php::parameters& params) {
	zend_object*    key = params[0];
	keeper_wrapper& kw  = core::keep->map_[key];
	kw.conn = params[0];
	if(0 != event_base_gettimeofday_cached(core::base, &kw.time)) {
		throw php::exception("keep failed: failed to gettimeofday");
	}
	if(params.length() > 1) {
		kw.ttl = static_cast<int>(params[1]);
	}else{
		kw.ttl = 60;
	}
	kw.time.tv_sec += kw.ttl;
	if(params.length() > 2) {
		kw.ping = static_cast<std::string>(params[2]);
	}else{
		kw.ping.assign("ping", 4);
	}
	return nullptr;
}

void keeper::timer_handler(evutil_socket_t fd, short events, void* data) {
	std::printf("timer_handler\n");
	keeper* self = reinterpret_cast<keeper*>(data);
	struct timeval now;
	event_base_gettimeofday_cached(core::base, &now);
	for(auto i=self->map_.begin(); i!= self->map_.end(); ++i) {
		if(i->second.time.tv_sec <= now.tv_sec) {
			i->second.conn.call(i->second.ping);
			i->second.time = now;
			i->second.time.tv_sec += i->second.ttl;
		}
	}
	struct timeval timeout = { 10, 0 };
	event_add(&self->ev_, &timeout);
}
