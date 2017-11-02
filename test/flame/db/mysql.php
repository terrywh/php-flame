<?php
flame\init("mysql_test");
$cli = new flame\db\mysql\client();
flame\go(function() use(&$cli) {
	yield $cli->connect("mysql://bullet:titcNmgt9if8gshkfKrz@10.20.6.69:3336/bullet_room_user_alpha");
	flame\go(function() use(&$cli) {
		var_dump( $cli->format("SELECT * FROM `test_0` WHERE `id`=?", true) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->delete("test_0", [
			"text" => ['$ne'=>"123"]
		], ["text"=>-1], 1) );
		$rs = yield $cli->select("test_0", ["id","text"], ["id"=>['$ne'=>NULL]], ["text"=>-1], 3);
		var_dump( $rs );
		var_dump( yield $rs->fetch_all(flame\db\mysql\FETCH_ASSOC) );
		var_dump( yield $cli->one("test_0", ["id"=>['$lt'=>10000]], ["id"=>1], 1) );
	});
	flame\go(function() use(&$cli)  {
		var_dump( $cli->format("SELECT * FROM `test_0` WHERE `id`=?", true) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]) );
		var_dump( yield $cli->delete("test_0", [
			"text" => ['$ne'=>"123"]
		], ["text"=>-1], 1) );
		$rs = yield $cli->select("test_0", ["id","text"], ["id"=>['$ne'=>NULL]], ["text"=>-1], 3);
		var_dump( $rs );
		var_dump( yield $rs->fetch_all(flame\db\mysql\FETCH_ASSOC) );
		var_dump( yield $cli->one("test_0", ["id"=>['$lt'=>10000]], ["id"=>1], 1) );
	});
});
flame\run();
