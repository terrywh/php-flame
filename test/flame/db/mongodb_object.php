<?php
flame\go(function() {
	$cli = new flame\db\mongodb\client("mongodb://username:password@127.0.0.1:27017/test");
	var_dump($cli);
	// 获取 collection
	$col = $cli->test1;
	var_dump($col);
	$col = $cli->collection("test1");
	var_dump($col);
	// count
	var_dump(yield $col->count(["a"=> "aaa"]));
	// insert
	$r = yield $col->insert_one([
		"_id" => new flame\db\mongodb\object_id(),
		"a"=>"aaa",
		"b"=>["bb","bbb"],
		"c"=>123,
		"d"=> new flame\db\mongodb\date_time(),
		"e"=>["a"=>"b","c"=>"d"],
	]);
	$r = yield $col->insert_many([["a"=>"aaa"], ["b"=>"bbb"], ["c"=>"ccc"]]);
	var_dump($r, $r->success());
	// object_id
	$oid = new flame\db\mongodb\object_id();
	var_dump($oid);
	echo $oid, "\n";
	echo json_encode($oid), "\n";
	var_dump($oid->timestamp());
	// date_time
	$dt = new flame\db\mongodb\date_time();
	var_dump($dt);
	echo $dt,"\n";
	echo json_encode($dt), "\n";
	var_dump($dt->timestamp_ms());
	var_dump($dt->to_datetime());
});
flame\run();
