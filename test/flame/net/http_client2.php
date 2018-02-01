<?php
flame\init("http_client_test");

flame\go(function() {
	$conn = 0;
	$cli = new flame\net\http\client(["conn_per_host" => 4]);
	
	for($i=0;$i<1000000;++$i) {
		echo $i, " conn: ", $conn, "\n";
		flame\go(function() use($cli, &$conn) {
			++$conn;
			$req = new flame\net\http\client_request("https://10.110.32.193/t.php");
			$req->header["Host"] = "i1.cdn.xiongmaoxingyan.com";
			// $req = new flame\net\http\client_request("https://http2.github.io");
			$req->ssl(["verify" => "none"]);
			$res = yield $cli->exec($req);
			var_dump($res);
			--$conn;
		});
		while($conn >= 1) {
			yield flame\time\sleep(10);
		}
	}
});

flame\run();
