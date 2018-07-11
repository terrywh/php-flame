<?php
ob_start();

flame\init("mongodb_2");
flame\go(function() {
	$client = yield flame\mongodb\connect("mongodb://user:pass@11.22.33.44:27017,44.33.22.11:27017/database?replicaSet=replicaSetName&readPreference=secondaryPreferred");
	
	$col = $client->collection("test_0");
	$r = yield $col->delete([]);
	assert($r["ok"]);
	$r = yield $col->insert(["a"=>"1", "b"=>2, "c"=>["x"=>"y"]]);
	assert($r["ok"] && $r["n"] == 1);
	$r = yield $col->insert([["a"=>"2", "b"=>3, "c"=>["x"=>"z"]], ["a"=>"3", "b"=>NULL, "c"=>["x"=>"m"]]]);
	assert($r["ok"] && $r["n"] == 2);
	$r = yield $col->delete(["a"=>"2"]);
	assert($r["ok"] && $r["n"] == 1);
	$r = yield $col->delete(["a"=>['$lt' => "4"]], 1);
	assert($r["ok"] && $r["n"] == 1);
	$r = yield $col->update(["a"=>"3"], ['$set'=>["b"=>1234]]);
	assert($r["ok"] && $r["nModified"] == 1);
	$r = yield $col->update(["a"=>"4"], ['$set'=>["b"=>1234]], true); // upsert
	assert($r["ok"] && $r["upserted"] && $r["upserted"][0]["_id"]);
	$r = yield $col->find(["a"=>['$gt'=>"2"]], ["a", "b"]);
	$r = yield $r->__toArray();
	assert(count($r) == 2 && count($r[0]) == 2);
	$r = yield $col->one(["a"=>['$ne'=>"3"]]);
	assert($r["_id"] && $r["a"] && $r["b"]);
	$r = yield $col->get("b", ["a"=>"4"]);
	assert($r === 1234);
	$r = yield $col->count([]);
	assert($r == 2);
	$r = yield $col->aggregate([
		['$match' => ['a'=>['$ne'=>"5"]]],
		['$group' => ['_id' => '$b']],
	]);
	assert(gettype($r) == "object");
	$r = yield $r->next();
	assert($r["_id"] == 1234);
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\n");
}