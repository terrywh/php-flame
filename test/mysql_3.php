<?php
flame\init("mysql_3");
flame\go(function() {
	ob_start();
	$client = yield flame\mysql\connect("mysql://user:pass@11.22.33.44:3306/database");

	$rs = yield $client->delete("test_0", 1);
	$rs = yield $client->insert("test_0", ["id"=> NULL, "text" => 1111]);
	assert($rs->affected_rows == 1);
	assert($rs->insert_id > 0);
	$target_id = $rs->insert_id;
	$rs = yield $client->insert("test_0", [["id"=> NULL, "text" => 2222], ["id"=> NULL, "text" => 3333]]);
	assert($rs->affected_rows == 2);
	$rs = yield $client->query("SELECT * FROM `test_0`");
	$data = yield $rs->fetch_all();
	assert(count($data) == 3);
	assert($data[0]["text"] == "1111");
	$rs = yield $client->delete("test_0", ["text" => 1111]);
	assert($rs->affected_rows == 1);
	$rs = yield $client->delete("test_0", ["text" => ["{<}" => 4444]], ["id"=>1], 1);
	assert($rs->affected_rows == 1);
	$rs = yield $client->update("test_0", ["text" => "3333"], ["text"=>4444]);
	assert($rs->affected_rows == 1);
	$rs = yield $client->select("test_0", ["id", "text"], ["id"=>["{>}"=>$target_id]]);
	$row = yield $rs->fetch_row();
	assert($row["id"] == $target_id + 2 && $row["text"] == "4444");
	$row = yield $client->one("test_0", ["text"], ["id"=>["{>}"=>$target_id]]);
	assert("4444" == $row["text"]);
	$text = yield $client->get("test_0", "text", ["id"=>["{>}"=>$target_id]]);
	assert("4444" == $text);
	echo "done1.\n";
	$output = ob_get_flush();
	assert($output == "done1.\n");
});
flame\run();
