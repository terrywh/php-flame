<?php
flame\go(function() {
	$cli = new flame\db\mongodb\client("mongodb://username:password@127.0.0.1:27017,10.20.6.72:27017/test");
	var_dump($cli);
	$col = $cli->test_collection;
	var_dump($col);
	$col = $cli->collection("test_collection");
	var_dump(yield $col->count());
	$oid = new flame\db\mongodb\object_id();
	var_dump($oid);
	echo $oid, "\n";
	echo json_encode($oid), "\n";
	var_dump($oid->timestamp());
	$dt = new flame\db\mongodb\date_time();
	var_dump($dt);
	echo $dt,"\n";
	echo json_encode($dt), "\n";
	var_dump($dt->timestamp_ms());
	var_dump($dt->to_datetime());
});
flame\run();
