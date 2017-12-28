#pragma once

namespace flame {
	void init(php::extension_entry& ext);
	void free_handle_cb(uv_handle_t* handle);
	void free_data_cb(uv_handle_t* handle);
}
