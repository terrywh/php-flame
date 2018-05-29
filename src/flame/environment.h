#pragma once

namespace flame {
	class environment {
	public:
		environment(bool inherits = true);
		void add(const std::string& key, const std::string& val);
		operator char**();
	private:
		std::vector<char*>    envs_;
		std::map<std::string, std::string> vars_;
	};
}
