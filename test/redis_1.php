<?php
flame\init("redis_1");
flame\go(function() {
	ob_start();
	$cli = yield flame\redis\connect("redis://auth:123456@11.22.33.44:6379/0");
	yield $cli->set("test", 0);
	for($i=0;$i<1000;++$i) {
		flame\go(function() use($cli, $i) {
			yield flame\time\sleep(rand(10, 15));
			yield $cli->incr("test");
			yield $cli->get("test");
		});
	}
	yield flame\time\sleep(5000);
	assert( "1000" == yield $cli->get("test") );
	assert( "OK" == yield $cli->set("test", "this is a string") );
	assert( "this is a string" == yield $cli->get("test") );
	assert( 1 == yield $cli->del("myzset") );
	assert( 1 == yield $cli->zadd("myzset", 1, "one") );
	assert( 1 == yield $cli->zadd("myzset", 2, "two") );
	assert( 1 == yield $cli->zadd("myzset", 3, "three") );
	for($i=0;$i<100;++$i) {
		flame\go(function() use($cli, $i) {
			yield flame\time\sleep(rand(10, 15));
			echo "incr:".(yield $cli->zincrby("myzset", 10, "one"))."\n";
			yield flame\time\sleep(rand(0, 5));
			echo "scor:".(yield $cli->zscore("myzset", "one"))."\n";
		});
	}
	$r = yield $cli->zrevrangebyscore("myzset", "+inf", "-inf", "WITHSCORES");
	assert($r["one"]);
	assert($r["two"]);
	assert($r["three"]);
	$r = yield $cli->zscan("myzset", 0);
	assert($r[0] == 0);
	assert($r[1]["one"]);
	echo "done1.\n";
	$output = ob_get_flush();
	echo $output;
	// assert($output == "done1.\n");
});
flame\run();
