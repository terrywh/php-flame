<?php
flame\go(function() {
	$cli = new flame\db\mongodb\client("mongodb://notify:wzysy2dcaateSjcb2rlY@10.20.6.71:27017,10.20.6.72:27017/notify_test?replicaSet=devel_repl");
	echo "1\n";
	$test1 = $cli->collection("test1");
	// yield $test1->insert_one(["a"=>"aaaaa"]);
	// yield $test1->insert_one(["a"=>"aaaaa"]);
	// yield $test1->insert_one(["a"=>"aaaaa"]);
	// yield $test1->remove_one(["a"=>"aaaaa"]);
	// yield $test1->remove_many(["a"=>"aaaaa"]);
	// yield $test1->update_one(["a"=>"aaaaa"], ["\$set"=>["a"=>"bbbbb","c"=>"ddddd"]]);
	// yield $test1->update_many(["a"=>"aaaaa"], ["\$set"=>["a"=>"bbbbb","c"=>"ddddd"]]);
	// yield $test1->find_one(["a"=>"bbbbb"]);
	yield $test1->find_many(["a"=>"bbbbb"]);
});
flame\run();
