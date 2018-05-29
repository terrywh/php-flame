#include "deps.h"
#include "environment.h"

extern char** environ;

namespace flame {
	environment::environment(bool inherits) {
		if(inherits) {
			for(char** e = environ; *e != nullptr; ++e) {
				envs_.push_back(*e);
			}
		}
	}
	void environment::add(const std::string& key, const std::string& val) {
		std::string& var = vars_[key] = fmt::format("{0}={1}", key, val);
		envs_.push_back(const_cast<char*>(var.data()));
	}
	environment::operator char**() {
		if(envs_.back() != nullptr) {
			envs_.push_back(nullptr);
		}
		return envs_.data();
	}
}
