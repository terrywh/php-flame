### `namespace flame\net\fastcgi`
提供对 `fastcgi/1.1` 协议的简化封装，支持挂接到 nginx 等 Web 服务器提供应用服务。

### `class flame\net\fastcgi\server`
监听 `unix socket` 提供后端服务，提供 `fastcgi/1.1` 简化协议的应用服务器封装。

#### `server::bind(string $path)`
绑定到指定的 `unix socket` 路径（以接收来自 Web 服务器的连接）；

**示例**：
``` PHP
<?php
$server = new flame\net\fastcgi\server();
$server->bind("/data/sockets/uinx_socket_1.sock");
```

**注意**：
* 当将 `unix socket` 设置到 `/tmp` 路径时可能导致 `nginx` 无法连接，建议路径设置为自定义（非 `/tmp`）路径；

#### `server::handle([string $path,] callable $cb)`
设置默认处理回调（未匹配路径回调），或设置指定路径的请求处理回调（`$path` 参数可选）；回调函数接收两个参数：
* `$request instanceof class flame\net\http\server_request` 请参考 `flame\net\http` 命名空间中的相关说明；
* `$response instanceof class flame\net\fastcgi\server_response`

**示例**：
``` PHP
<?php
$server->handle("/hello/world", function($req, $res) {
	var_dump($req->header["host"]);
	yield $res->write("hello world\n");
	yield $res->end();
});
$server->handle(function($req, $res) {
	yield $res->end('{"error":"router not found"}');
});
```
**注意**：
* `$cb` 必须为 `Generator Function`（含有 `yield` 关键字）而非普通回调函数；

#### `server::run()`
监听由上述 `server::bind()` 函数指定的 `unix socket` 并开始服务；

**注意**：
* 此函数会阻塞运行，直到 `server::close()` 被调用或发生严重错误中断；

#### `server::close()`
关闭应用服务器，并恢复被 `server::run()` 阻塞的协程继续运行；
