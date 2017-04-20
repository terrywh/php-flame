<?php
dl("mill.so");
// $app = new mill\application();
// // for($i=0;$i<mill\application::concurrency;++$i) {
// for($i=0;$i<1;++$i) {
// 	$proc = new mill\process();
// 	$svr  = new mill\udp\server("127.0.0.1", 6696);
// 	$svr->on_packet = function($packet, $remote_addr) {
// 		echo $packet, " ", $remote_addr, " ", posix_getpid(), "\n";
// 	};
// 	$proc->add($svr);
// 	$app->add($proc);
// }
// $app->run();

function server1() {
	mill\sleep(1000);
	echo "ccccc\n";
	$server = new mill\udp\server("127.0.0.1", 6696);
	$server->listen();
	echo "dddddd\n";
	while($packet = $server->recv()) {
		echo posix_getpid(), " ", $server->remote_addr, " => ", $packet,"\n";
	}
	$server->close();
}

function server2() {
	$server = new mill\udp\server("127.0.0.1", 6697);
	$server->listen();
	while($packet = $server->recv()) {
		echo posix_getpid(), " ", $server->remote_addr, " => ", $packet,"\n";
	}

	$server->close();
}

function tcp1() {
	$socket = mill\tcp\client();
	$socket->recv();
	$socket->send();
}

function http1() {
	$task_queue = [];
	mill\go(function() use($task_queue) {
		// .......
		array_push($task_queue, "task");
	});
	for($i=0;$i<100;++$i) {
		mill\go(function() use($task_queue) {
			$taks = array_pop($task_queue);
			$client = mill\http\client();
			$client->get("http://aaaaaa", ["a"=>$task], ["header"=>[]]);
			$client->post("http://bbbbbbb", ["a"=>"b"], []);
			$response = $client->request("GET", "URL", ["a"=>"b"], []);
			$response->body
			if($response->header["Content-type"] === "text/plain") {

			}
			$client->get("key", function($err, $value) {

			});
			$value = $client->get("key");
		});
	}
}

function main() {
	echo "11111\n";
	// mill\go(server1);
	// echo "22222\n";
	// mill\go(server2);
	// file_get_contents();
	// echo "33333\n";
	mill\sleep(1000000);
}

mill\run(main, ["worker_count"=>1]);

// yield target;
//
//
// 1. target -> object of Generator
// 2. target -> function() {} definition, if function return object of Generator -> 1.
// 3. target -> evaluate other
