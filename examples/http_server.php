<?php
// 框架初始化 (自动设置进程标题)
flame\init("http_server", [
	"worker" => 4, // 使用 4 个工作进程
	"logger" => "/tmp/http_server.log", // 日志输出路径
]);
flame\go(function() {
	// 创建服务器
	$server = new flame\http\server("127.0.0.1:7678");
	// 前置处理器
	$server->before(function($req, $res, $r) {
		// 对 /hello 开头的请求进行 REWRITE 重写
		if(substr($req->path, 0, 6) == "/hello") {
			$req->method = "GET";
			$req->path = "/";
		}
		// 使用 $req->data 保存数据（在路径处理器或后置处理器中均可访问）
		$req->data["start"] = flame\time\now();
	})->get("/stream", function($req, $res) { // 路径处理器
		// 使用 Chunked 方式持续返回数据，设置为事件流方式响应数据
		$res->header["Content-Type"]  = "text/event-stream";
		$res->header["Cache-Control"] = "no-cache";
		yield $res->write_header(200);
		// 启动额外的协程 “异步” 继续处理流程
		flame\go(function() use($res) {
			// 每秒响应一个 “时间事件”
			for($i=0;$i<10;++$i) {
				yield flame\time\sleep(1000);
				yield $res->write("event: time\n");
				yield $res->write("data: ". flame\time\now() . "\n\n");
			}
			yield $res->end(); // 结束请求
		});
	})->get("/", function($req, $res) { // 路径处理器
		// 使用 chunked 方式返回数据
		yield $res->write("hello, ");
		yield $res->end("world!\n"); // 最后一个响应
	})->post("/", function($req, $res) { // 路径处理器
		// 使用 content-length 方式返回数据
		$res->body = json_encode($req);
		// 若设置了 content-type 对应类型 application/json 框架可以自动序列化为 JSON
	})->after(function($req, $res, $r) { // 后置处理器
		if($r) { // 匹配了并执行了对应路径处理程序
			// 日志：[xxxx-xx-xx xx:xx:xx] (INFO) ......
			flame\log\info($req->method, $req->path, "elapsed:", (flame\time\now() - $req->data["start"]), "ms");
		}else{  // 未匹配自定义路径处理器
			yield $res->header["content-type"] = "text/html";
			// 在当前路径响应静态文件
			yield $res->file(__DIR__, $req->path);
			// 注意：这里提供的静态文件响应方式效率较低，生产环境建议使用专业的 Web 服务器
		}
	});
	// 运行服务器
	yield $server->run();
});
// 框架异步调度启动、运行入口
flame\run();
