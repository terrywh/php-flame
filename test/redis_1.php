<?php
ob_start();

flame\init("redis_1");
flame\go(function() {
	$cli = yield flame\redis\connect("redis://auth:123456@11.22.33.44:6379/0");
	
	assert( "OK" == yield $cli->set("test", "this is a string") );
	assert( "this is a string" == yield $cli->get("test") );
	assert( 1 == yield $cli->del("myzset") );
	assert( 1 == yield $cli->zadd("myzset", 1, "one") );
	assert( 1 == yield $cli->zadd("myzset", 2, "two") );
	assert( 1 == yield $cli->zadd("myzset", 3, "three") );
	$r = yield $cli->zrevrangebyscore("myzset", "+inf", "-inf", "WITHSCORES");
	assert($r["one"]);
	assert($r["two"]);
	assert($r["three"]);
	$r = yield $cli->zscan("myzset", 0);
	assert($r[0] == 0);
	assert($r[1]["one"]);
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\n");
}