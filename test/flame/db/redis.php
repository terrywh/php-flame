<?php
flame\init("redis_init");
flame\go(function() {
	$obj = new flame\db\redis();
	yield $obj->connect("10.20.6.73",6380);
	yield $obj->auth("1zqI9WmUdwXRl2suaSAy");
	echo "HMSET hash key1 11111 key2 22222: ";
	$res = yield $obj->hmset("hash", "key1", "11111", "key2", "22222");
	echo "HMGET hash key1 key2: ";
	$res = yield $obj->hmget("hash", "key1","key2");
	var_dump($res);
	echo "HGETALL hash: ";
	$res = yield $obj->hgetall("hash");
	var_dump($res);
	echo "SET arg_2 123: ";
	$res = yield $obj->set("arg_2", 123);
	var_dump($res);
	echo "GET arg2: ";
	$res = yield $obj->get("arg_2");
	var_dump($res);
	echo "SADD set1 321: ";
	$res = yield $obj->sadd("set1", 321);
	var_dump($res);
	echo "ZRANGE page_rank 0 -1 WITHSCORES: ";
	$res = yield $obj->zrange("sorted", 0, -1, "WITHSCORES");
	var_dump($res);
	flame\time\after(60000, function() use(&$obj) {
		$obj->stop_all(); // 60s 后停止订阅过程继续执行
		echo "\tSUBSCRIBE stopping ...\n";
		unset($obj);
	});
	echo "SUBSCRIBE foo bar:\n";
	yield $obj->subscribe("foo", "bar", function($channel, $message) {
		echo "\t[", $channel, "] -> ",$message,"\n";
	});
	echo "\tSUBSCRIBE stopped.\n";
});
flame\run();
