<?php
flame\init("core_0");
function async_fn() {
	yield 1;
	yield flame\trigger_error();
	yield 2;
}
flame\go(function() {
	// yield flame\aaaaa\trigger_error();
	// yield flame\trigger_error();
	yield async_fn();
	// assert("111.111.111.111");
	// $server = new flame\udp\socket("127.0.0.1:17679");
	// $from;
	// $data = yield $server->receive_from($from);
	echo "done1.\n";
});
flame\run();
