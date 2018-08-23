<?php
flame\init("mysql_1");
flame\go(function() {
	ob_start();
	$client = yield flame\mysql\connect("mysql://user:pass@11.22.33.44:3306/database");
	assert($client->escape("a.b", '`') == '`a`.`b`');
	assert($client->escape("a'b") == "'a\\'b'");
	$rs = yield $client->query("SELECT * FROM `test_0`");
	$data = yield $rs->fetch_all();
	assert(count($data) > 0);
	$tx = yield $client->begin_tx();
	$rs = yield $tx->query("SELECT * FROM `test_0`");
	$data = yield $rs->fetch_all();
	assert(count($data) > 0);
	$rs = yield $tx->query("INSERT `test_0` VALUES(NULL, 3333)");
	assert($rs->insert_id > 0);
	yield $tx->rollback();
	echo "done1.\n";
	$output = ob_get_flush();
	assert($output == "done1.\n");
});
flame\run();
