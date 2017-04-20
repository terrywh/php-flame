<?php
dl("mill.so");

function test1() {
	$b = "bbbbbbbbb";
 $a = "aaaaaaaaa";
	return $a."<->".$b;
}
function test2() {
	$b = "bbbbbbbbb";
	return $b;
}

$a = test1();
$b = test2();
$c = $b."a";
debug_zval_dump($a, $b, $c);



$socket = new mill\net\udp_socket("127.0.0.1", 6676);

for($i=0;$i<10;++$i) {
	$addr = null;
	$addr = $socket->local_addr();
}
debug_zval_dump($addr);
$packet = $socket->recv();
$a = test();
debug_zval_dump($socket->recv(), $a);
echo $socket->remote_addr(), " => ", $packet, "\n";
$str = $addr->__toString();
debug_zval_dump($str);
