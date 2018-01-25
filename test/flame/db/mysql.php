<?php
flame\init("mysql_test");

flame\go(function() {
	$cli = new flame\db\mysql\client();
	yield $cli->connect("mysql://bullet:titcNmgt9if8gshkfKrz@10.20.6.69:3336/bullet_room_user_alpha");
	var_dump( $cli->format("SELECT * FROM `test_0` WHERE `id`=?", true) );
	for($i=0;$i<1000;++$i) {
		yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]);
	}
	var_dump( yield $cli->delete("test_0", [
		"text" => ['$ne'=>"123"]
	], ["text"=>-1], 1) );

	for($i=0;$i<1000;++$i) {
		// $rs = yield $cli->select("test_0", ["id","text"], ["id"=>['$ne'=>NULL]], ["text"=>-1], 30);
		$rs = yield $cli->query("SELECT * FROM `test_0` WHERE `id` IS NOT NULL ORDER BY `text` DESC LIMIT 30");
		$rw = yield $rs->fetch_all(flame\db\mysql\FETCH_ASSOC);
	}
	debug_zval_dump( $rs );
	debug_zval_dump( $rw );
	var_dump( yield $cli->one("test_0", ["id"=>['$gt'=>10000]], ["id"=>1], 1) );
	var_dump( yield $cli->found_rows() );
	yield $cli->delete("test_0", "true");
});
flame\run();


// $mysqli = new mysqli("10.20.6.69", "bullet", "titcNmgt9if8gshkfKrz", "bullet_room_user_alpha", 3336);
// $rs = $mysqli->query("SELECT * FROM `test_0` WHERE `id` IS NOT NULL ORDER BY `id` DESC LIMIT 30");
// $rw = $rs->fetch_all(MYSQLI_ASSOC);
// debug_zval_dump($rw);