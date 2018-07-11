<?php
ob_start();

flame\init("os_1");
flame\go(function() {
	// $info = flame\os\interfaces();
	// assert(count($info) > 0);
	// foreach($info as $name=>$addrs) {
	// 	assert(count($addrs) > 0);
		assert("10.31.148.34");
		// assert($addrs[0]["family"]);
		// assert($addrs[0]["address"]);
		// assert(!empty($addrs[0]["family"]));
		// assert(!empty($addrs[0]["address"]));
	// }
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\n");
}

