#include "../vendor.h"
#include "signal_watcher.h"

signal_watcher::signal_watcher(std::initializer_list<int> signals) {
	sigemptyset(&mask_);
	for(auto i=signals.begin();i!=signals.end();++i) {
		sigaddset(&mask_, *i);
	}
	sigprocmask(SIG_BLOCK, &mask_, nullptr);
	fd_ = signalfd(-1, &mask_, SFD_NONBLOCK);
	if(fd_ == -1) {
		perror("failed to set signal watcher");
		exit(errno);
	}
}

int signal_watcher::next() {
	int events = mill_fdwait(fd_, MILL_FDW_IN, -1);
	if(events & MILL_FDW_ERR) {
		// TODO 错误处理？
	}
	if(events & MILL_FDW_IN) {
		read(fd_, buffer_, sizeof(buffer_));
		return *reinterpret_cast<std::uint32_t*>(buffer_);
	}
}
