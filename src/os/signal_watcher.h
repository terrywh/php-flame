#pragma once

class signal_watcher {
public:
	signal_watcher(std::initializer_list<int> signals);
	int next();
private:
	sigset_t mask_;
	int      fd_;
	char     buffer_[sizeof(struct signalfd_siginfo)];
};