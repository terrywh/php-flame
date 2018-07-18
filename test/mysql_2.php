<?php
flame\init("mysql_2");
flame\go(function() {
	ob_start();
	$client = yield flame\mysql\connect("mysql://user:pass@11.22.33.44:3306/database");

	$sql = $client->where(["a"=>"1", "b"=>[2,"3","4"], "c"=>null, "d"=>["{!=}"=>5]]);
	assert($sql == " WHERE (`a`='1' && `b`  IN ('2','3','4') && `c` IS NULL && `d`!='5')");
	$sql = $client->where(["{OR}"=>["a"=>["{!=}"=>1], "b"=>["{><}"=>[1, 10, 3]], "c"=>["{~}"=>"aaa%"]]]);
	assert($sql == " WHERE (`a`!='1' || `b` NOT BETWEEN 1 AND 10 || `c` LIKE 'aaa%')");
	$sql = $client->order("`a` ASC, `b` DESC");
	assert($sql == " ORDER BY `a` ASC, `b` DESC");
	$sql = $client->order(["a"=>1, "b"=>-1, "c"=>true, "d"=>false, "e"=>"ASC", "f"=>"DESC"]);
	assert($sql == " ORDER BY `a` ASC, `b` DESC, `c` ASC, `d` DESC, `e` ASC, `f` DESC");
	$sql = $client->limit(10);
	assert($sql == " LIMIT 10");
	$sql = $client->limit([10,10]);
	assert($sql == " LIMIT 10, 10");
	$sql = $client->limit("20, 300");
	assert($sql == " LIMIT 20, 300");
	echo "done1.\n";
	$output = ob_get_flush();
	assert($output == "done1.\n");
});
flame\run();
