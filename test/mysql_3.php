<?php
// ob_start();

flame\init("mysql_2");
flame\go(function() {
	$client = yield flame\mysql\connect("mysql://user:pass@11.22.33.44:3306/database");
	
	$rs = yield $client->delete("test_0", 1);
	$rs = yield $client->insert("test_0", ["id"=> NULL, "text" => 1111]);
	assert($rs->affected_rows == 1);
	$rs = yield $client->insert("test_0", [["id"=> NULL, "text" => 2222], ["id"=> NULL, "text" => 3333]]);
	assert($rs->affected_rows == 2);
	$rs = yield $client->delete("test_0", ["text" => 1111]);
	assert($rs->affected_rows == 1);
	$rs = yield $client->delete("test_0", ["text" => ["{<}" => 4444]], ["id"=>1], 1);
	assert($rs->affected_rows == 1);
	$rs = yield $client->update("test_0", ["text" => "3333"], ["text"=>4444]);
	assert($rs->affected_rows == 1);
	$rs = yield $client->select("test_0", ["id", "text"], ["id"=>["{>}"=>7000]]);
	assert($rs->found_rows == 1);
	$rs = yield $client->one("test_0", ["text"], ["id"=>["{>}"=>7000]]);
	assert("4444" == $rs["text"]);
	$rs = yield $client->get("test_0", "text", ["id"=>["{>}"=>7000]]);
	assert("4444" == $rs);
	echo "done1.\n";
});
flame\run();

// if(getenv("FLAME_PROCESS_WORKER")) {
// 	// $output = ob_get_flush();
// 	assert($output == "done1.\n");
// }