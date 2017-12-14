<?php
flame\init("mongodb_test");
flame\go(function() {
	$cli = new flame\db\mongodb\client();
	yield $cli->connect("mongodb://notify:wzysy2dcaateSjcb2rlY@10.20.6.71:27017,10.20.6.72:27017/notify_test?replicaSet=devel_repl");
	$test1 = yield $cli->collection("test1");
	var_dump( yield $test1->find_one(["a"=>["\$in"=>["bbbbb"]]]) );
	yield $test1->remove_many([]);
	yield $test1->insert_one(["a"=>"aaaaa"]);
	yield $test1->insert_one(["a"=>"aaaaa"]);
	yield $test1->remove_many(["a"=>"aaaaa"]);
	yield $test1->insert_many([["a"=>"aaaaa"],["a"=>"aaaaa"],["a"=>"aaaaa"],["a"=>"aaaaa"],["a"=>"aaaaa"]]);
	yield $test1->remove_one(["a"=>"aaaaa"]);
	yield $test1->update_one(["a"=>"aaaaa"], ["\$set"=>["a"=>"bbbbb","c"=>"ddddd"]]);
	yield $test1->update_many(["a"=>"aaaaa"], ["\$set"=>["a"=>"xxxxx","c"=>"yyyyy"]]);
	$doc = yield $test1->find_one(["a"=>"bbbbb"]);
	var_dump( $doc, json_encode($doc) );
	$cursor = yield $test1->find_many(["a"=>"xxxxx"]);
	var_dump($cursor);
	var_dump(yield $cursor->next());
	var_dump(yield $cursor->to_array());
});
flame\run();
