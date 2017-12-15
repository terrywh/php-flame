<?php
flame\init("fastcgi-server");
flame\go(function() {
	// 创建 fastcgi 处理器
	$handler = new flame\net\fastcgi\handler();
	// 设置处理程序
	$a = $handler->get("/hello", function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req->method, $req->cookie); // GET
		$res->set_cookie("test", "bbbb", 1000, "/hello", null, false, true);
		yield $res->write("hello ");
		yield $res->end("world\n");
	})
	->post("/hello", function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req->method, $req->header, $req->body); // POST ....
		yield $res->write("hello ");
		yield $res->end("world\n");
	})
	->get("/server/events/source", function($req, $res) {
		$res->header["Content-Type"]  = "text/event-stream";
		$res->header["Cache-Control"] = "no-cache";
		yield $res->write_header(200);

		flame\time\tick(1000, function($tick) use($res) {
			$r = yield $res->write("data: this is a message containing randomg data, ". rand() ."\n\n");
			if(!$r) $tick->stop();
		});
	})
	->get("/server/events", function($req, $res) {
		$html = <<<HTMLCODE
<!doctype html><html><head><title>Server Side Events</title></head><body>
<pre id="data"></pre>
<script>
	var evtSource = new EventSource("/server/events/source"), evtResult = document.getElementById("data");
	evtSource.onmessage = function(e) {
		var el = document.createElement("code");
		el.innerText = e.data + "\\n";
		evtResult.appendChild(el);
	}
</script>
</body></html>
HTMLCODE;
		yield $res->end($html);
	})
	// 默认处理程序（即：不匹配上述路径形式时调用）
	->handle(function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req);
		$data = json_encode($req);
		yield $res->end($data);
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定服务程序
	$server->bind("127.0.0.1", 19001);
	yield $server->run();
});
flame\run();
