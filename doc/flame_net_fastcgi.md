### `namespace flame\net\fastcgi`
提供对 `fastcgi/1.1` 协议的简化封装，支持挂接到 nginx 等 Web 服务器提供应用服务。

### `class flame\net\fastcgi\server`
监听 `unix socket` 提供后端服务，提供 `fastcgi/1.1` 简化协议的应用服务器封装。

#### `server::bind(string $path)` | `server::bind(string $addr, integer $port)`
1. 绑定到指定的路径，以接收来自 Web 服务器的 UnixSocket 连接；
2. 绑定到指定的 地址、端口，以接受来自 Web 服务器的 TCP 连接；

**示例**：
``` PHP
<?php
$server1 = new flame\net\fastcgi\server();
// 绑定到 UnixSocket
@unlink("/data/sockets/uinx_socket_1.sock"); // 删除已存在的 UNIXSocket
$server1->bind("/data/sockets/flame.xingyan.panda.tv.sock"); // 绑定
@chmod("/data/sockets/uinx_socket_1.sock", 0777); // 权限，允许其他用户连接

$server2 = new flame\net\fastcgi\server();
// 绑定到 TCP 地址端口
$server2->bind("127.0.0.1", 19001);
```

**注意**：
* 当将 `unix socket` 设置到 `/tmp` 路径时可能导致 `nginx` 无法连接，建议路径设置为自定义（非 `/tmp`）路径；
* 不允许将上述两种绑定形式对同一个对象使用；

#### `server::handle([string $path,] callable $cb)`
设置默认处理回调（未匹配路径回调），或设置指定路径的请求处理回调（`$path` 参数可选）；回调函数接收两个参数：
* `$request` - 类型 `class flame\net\http\server_request` 的实例，请参考 `flame\net\http` 命名空间中的相关说明；
* `$response` - 类型 `class flame\net\fastcgi\server_response` 的实例，请参考下文；

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

### `class flame\net\fastcgi\server_response`
由上述 `server` 生成，并回调传递给处理函数使用，用于返回响应数据给 Web 服务器；

#### `array server_response::$header`
响应头部 KEY/VAL 数组，默认包含 `Content-Type: text/plain`（注意**区分**大小写）；其他响应头请酌情考虑添加、覆盖；

**示例**：
``` PHP
<?php
$res->header["Content-Type"] = "text/html";
$res->header["X-Server"] = "Flame/0.7.0";
```

#### `yield server_response::write_header(integer $status_code)`
设置并输出响应头部（含上述响应头 KEY/VAL 及响应状态行），请参照标准 HTTP/1.1 STATUS_CODE 设置数值；

#### `yield server_response::write(string $data)`
输出指定响应内容，请参考 `handle()` 的相关示例；

#### `yield server_response::end([string $data])`
结束请求，可选输出响应内容（输出功能与 `write()` 完全相同）；

**注意**：
* 若服务端设置保留 fastcgi 连接（NGINX 配置 `fastcgi_keep_conn on;`），在结束响应后与 Web 服务器的连接将被保留；否则将会立即关闭；
