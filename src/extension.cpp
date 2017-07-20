#include "flame/flame.h"
#include "flame/time/time.h"
#include "flame/net/net.h"

void extension_init(php::extension_entry& ext) {
	ext.init(EXT_NAME, EXT_VER);
	// 核心功能
	flame::init(ext);
	// time
	flame::time::init(ext);
	// net
	flame::net::init(ext);
}
