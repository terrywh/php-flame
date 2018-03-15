<?php
flame\init("http_client_test");

flame\go(function() {
	$conn = 0;
	$cli = new flame\net\http\client();
	
	for($i=0;$i<1000;++$i) {
		echo $i, " conn: ", $conn, "\n";
		flame\go(function() use($cli, &$conn) {
			++$conn;
			$req = new flame\net\http\client_request("https://10.110.32.193/");
			$req->host["Host"] = "i1.cdn.xiongmaoxingyan.com";
			// $req = new flame\net\http\client_request("https://http2.akamai.com/demo");
			// $req = new flame\net\http\client_request("https://http2.github.io");
			// $req = new flame\net\http\client_request("https://api.push.apple.com/");
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
