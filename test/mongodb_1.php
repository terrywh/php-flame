<?php
flame\init("mongodb_1");
flame\go(function() {
	ob_start();
	$client = yield flame\mongodb\connect("mongodb://user:pass@11.22.33.44:27017,44.33.22.11:27017/database?replicaSet=replicaSetName&readPreference=secondaryPreferred");
	assert(gettype($client) == "object");
	$reply = yield $client->execute(["insert" => "test_0", "documents"=>[["a"=>"aaaaa", "b"=>"bbbbb"]]], true); // 更新型操作需要额外参数
	assert($reply["ok"] && $reply["n"] == 1);
	$reply = yield $client->execute(["count" => "test_0"]);
	assert($reply["ok"] && $reply["n"] > 0);
	$cursor = yield $client->execute(["find" => "test_0", "filter"=>["a"=>"aaaaa"]]);
	assert(gettype($cursor) == "object");
	$doc = yield $cursor->next();
	assert(gettype($doc) == "array" && $doc["_id"] && $doc["a"] == "aaaaa");
	$col1 = $client->test_0;
	$col2 = $client->collection("test_0");
	assert($col1->name === $col2->name);
	echo "done1.\n";
	$output = ob_get_flush();
	assert($output == "done1.\n");
});
flame\run();
