<?php
flame\go(function() {
	$cli = new flame\db\mongodb\client("mongodb://notify:wzysy2dcaateSjcb2rlY@10.20.6.71:27017,10.20.6.72:27017/notify_test?replicaSet=devel_repl");
	var_dump($cli);
	$col = $cli->profile;
	var_dump($col);
	var_dump(yield $col->count());
});
flame\run();
