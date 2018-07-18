<?php
flame\init("os_1");
flame\go(function() {
	ob_start();
	$info = flame\os\interfaces();
	assert(count($info) > 0);
	foreach($info as $name=>$addrs) {
		assert(count($addrs) > 0);
		assert(!empty($addrs[0]["family"]));
		assert(!empty($addrs[0]["address"]));
	}
	echo "done1.\n";
	$output = ob_get_flush();
	assert($output == "done1.\n");
});
flame\run();
