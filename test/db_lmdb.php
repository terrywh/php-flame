<?php
dl("flame.so");

$db = new flame\db\lmdb("/tmp/php-lmdb.mdb");
echo "iterator through before1:\n";
foreach($db as $key=>$val) {
	echo "[$key] => ($val)\n";
}
echo "----------------------------\n";
echo "test_key1: ";
$db->set("test_key1", false);
var_dump($db->get("test_key1"));
echo "test_key2: ";
$db->set("test_key2", 123456);
var_dump($db->get("test_key2"));
echo "test_key3: ";
$db->set("test_key3", "test_value");
var_dump($db->get("test_key3"));
echo "test_key4: ";
var_dump($db->incr("test_key4", 1));
echo "test_key5: ";
var_dump($db->get("test_key5"));
var_dump($db->has("test_key5"));

echo "iterator through before2:\n";
foreach($db as $key=>$val) {
	echo "[$key] => ($val)\n";
	if($key == "test_key2") {
		$db->del($key); 
	}
}
echo "iterator through after1:\n";
foreach($db as $key=>$val) {
	echo "[$key] => ($val)\n";
}
$db->flush();
echo "iterator through after2:\n";
foreach($db as $key=>$val) {
	echo "[$key] => ($val)\n";
}
echo microtime(true),"\n";
for($i=0;$i<10000000;++$i) {
	$db->set("test_key1", false);
	$db->get("test_key1");
	$db->set("test_key2", 123456);
	$db->get("test_key2");
	$db->set("test_key3", "test_value");
	$db->get("test_key3");
	$db->incr("test_key4", 1);
}
echo microtime(true),"\n";
var_dump($db);
