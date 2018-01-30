#include "deps.h"
#include "flame/flame.h"
#include "flame/coroutine.h"
#include "flame/time/time.h"
#include "flame/os/os.h"
#include "flame/log/log.h"
#include "flame/db/db.h"
#include "flame/net/net.h"

void extension_init(php::extension_entry& ext) {
	ext.init(EXT_NAME, EXT_VER);
	// 核心功能
	flame::init(ext);
	// time
	flame::time::init(ext);
	// os
	flame::os::init(ext);
	// log
	flame::log::init(ext);
	// db
	flame::db::init(ext);
	// net
	flame::net::init(ext);
}
