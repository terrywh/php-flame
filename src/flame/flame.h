#pragma once

namespace flame {
	void init(php::extension_entry& ext);
	// “协程” -> “继续”
	void coroutine(php::generator gen);
	// “协程” -> 启动
	php::value go(php::parameters& params);
	// 事件循环 -> 启动
	php::value run(php::parameters& params);
}
