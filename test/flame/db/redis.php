<?php
flame\init("redis_init");
flame\go(function() {
	$obj = new flame\db\redis();
	yield $obj->connect("redis://auth:123456@11.22.33.44:6379/30");
	// 与下述三行实现功能相同
	// yield $obj->connect("11.22.33.44", 6379);
	// yield $obj->auth("123456");
	// yield $obj->select(30);
	for($i=0;$i<1000;++$i) {
		if($i == 0) echo "HMSET hash key1 11111 key2 22222: ";
		$res = yield $obj->hmset("hash", "key1", "11111", "key2", "22222");
		if($i == 0) var_dump($res);
		if($i == 0) echo "HMGET hash key1 key2: ";
		$res = yield $obj->hmget("hash", "key1","key2");
		if($i == 0) var_dump($res);
		if($i == 0) echo "HGETALL hash: ";
		$res = yield $obj->hgetall("hash");
		if($i == 0) var_dump($res);
		if($i == 0) echo "SET arg_2 123: ";
		$res = yield $obj->set("arg_2", 123);
		if($i == 0) var_dump($res);
		if($i == 0) echo "GET arg2: ";
		$res = yield $obj->get("arg_2");
		if($i == 0) var_dump($res);
		if($i == 0) echo "SADD set1 321: ";
		$res = yield $obj->sadd("set1", 321);
		if($i == 0) var_dump($res);
		yield $obj->zadd("sorted", 111, "aaa");
		yield $obj->zadd("sorted", 333, "ccc");
		yield $obj->zadd("sorted", 222, "bbb");
		yield $obj->zadd("sorted", 555, "eee");
		yield $obj->zadd("sorted", 444, "ddd");
		if($i == 0) echo "ZRANGE sorted 0 -1 WITHSCORES: ";
		$res = yield $obj->zrange("sorted", 0, -1, "WITHSCORES");
		if($i == 0) var_dump($res);
		if($i == 0) echo "ZREVRANGE sorted 0 -1 WITHSCORES: ";
		$res = yield $obj->zrevrange("sorted", 0, -1, "WITHSCORES");
		if($i == 0) var_dump($res);
	}
	flame\time\after(5000, function() use(&$obj) {
		$obj->stop_all(); // 5s 后停止订阅过程继续执行
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
