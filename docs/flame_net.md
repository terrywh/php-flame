### `namespace flame\net`
1. 提供基本的TCP UDP 协议网络协程式客户端、服务器封装；
2. 提供以下协议的支持内部包：
	* [HTTP](/php-flame/flame_net_http)
	HTTP 协议支持（目前仅提供了客户端支持）；
	* [FastCGI](/php-flame/flame_net_fastcgi)
	精简版本的应用服务支持，可以挂接 Nginx 等 Web 服务器使用；

### `class flame\net\udp_socket`
封装 UDP 协议网络服务器、客户端连接

#### `string udp_socket::$local_address`
本地网络地址

#### `string udp_socket::$remote_address`
当成功接收数据后 `yield recv()`，保存数据来源地址

#### `string udp_socket::$remote_port`
当成功接收数据后 `yield recv()`，保存数据来源端口

#### `udp_socket::bind(string addr, long port)`
将当前对象绑定到指定的地址、端口；绑定后，可以接收来自这个地址、端口的数据；

**注意**：
* 下述 `recv()` 和 `send()` 函数会在未绑定地址、端口时，自动进行绑定；

#### `yield udp_socket::recv()`
接收数据，设置 udp_socket::$remote_address/$remote_port 属性，并返回接收到的数据；若当前套接字被关闭，将返回 `NULL`；

**注意**：
* 仅允许唯一的协程调用上述 `recv()` 过程；多个协程调用可能引起未定义的错误；
* 若当前对象还未绑定地址、端口，系统将自动绑定 `0.0.0.0` 的随机端口；

#### `yield udp_socket::send(string $data, string $addr, integer $port)`
向指定 `$addr` 地址，`$port` 端口发送指定 `$data` 内容

**注意**:
* 若当前对象还未绑定地址、端口，系统将自动绑定 `0.0.0.0` 的随机端口；

#### `udp_socket::close()`
关闭当前 `udp_socket`，所有异步过程将被终止；

### `class flame\net\unix_server`
实现基于 UnixSocket 的网络服务器；

#### `string unix_server::$local_address`
本地监听地址

#### `unix_server::handle(callable $cb)`
设置服务处理回调，在服务端接收到新连接时回调该函数；原型如下：

``` PHP
<?php
// $socket instanceof flame\net\unix_socket
function callback($socket) {
	// yield $socket->read()
}
```

**示例**：
``` PHP
<?php
// ...
$server = new flame\net\unix_server();
@unlink("/data/sockets/flame.sock");
$server->bind("/data/sockets/flame.sock");
$server->handle(function($sock) {
	var_dump($sock->read(4));
});
yield $server->run();
// ...
```

**注意**：
* `$cb` 必须是 `Generator Function`，即包含 `yield` 关键字；

#### `unix_server::bind(string $path)`
绑定到指定路径并生成 UnixSocket 文件；

**注意**：
* 若指定路径文件已存在，则无法绑定（即禁止同时监听同一 Socket 文件，）；请先删除现有监听文件；
* 生成的文件遵循默认的文件权限，如果需要请使用 `chmod()` 等函数自行更改；
* 如需多进程处理 `unix_server` 可以考虑使用 `os` 命名空间提供的相关方法，将 `accept` 后的 `socket` 传递到单独启动的工作进程；

#### `yield unix_server::run()`
启动并运行当前服务器

**注意**：
* 运行服务器会阻塞当前协程；

#### `unix_server::close()`
若当前服务器在运行中，关闭服务器

**注意**：
* 被阻塞在 `run()` 的协程会恢复执行；

### `class flame\net\unix_socket`
封装基于 UnixSocket 的客户端 API

#### `unix_socket::$remote_address`
远端地址，即连接的目标服务端 UnixSocket 路径地址；

#### `yield unix_socket::connect(string $path)`
连接指定路径对应的远端 UnixSocket；

#### `yield unix_socket::read([mixed $completion])`
从当前对象中读取数据，可选的指定读取数据的“结束条件”，支持以下三种用法：
1. 不指定结束条件，读取随机长度的数据（一般不会超过 2048 字节）；
2. 数值，读取指定长度的数据；
3. 字符串，读取到指定的结束符号；

**示例**：
``` PHP
<?php
// ...
$data = yield $sock->read(4); // $data = "abcd";
$data = yield $sock->read("\n"); // $data = "aaaaaaa\n";
$data = yield $sock->read(); // $data = "aaaaa";
```

**注意**：
* 除正常的网络关闭外，读取动作发生错误时可能抛出异常；

#### `yield unix_socket::write(string $data)`
想当前套接字写入（发送）指定数据；

**注意**：
* 当网络连接已断开等错误状态时，调用 `write` 会抛出异常；

#### `yield unix_socket::close()`
关闭当前套接字对象；（已启用的异步动作会继续进行，完成后关闭）

### `class flame\net\tcp_socket`
提供 TCP 协议的网络连接对象的封装

#### `string tcp_socket::$local_address`
本地网络地址

#### `string tcp_socket::$remote_address`
远端网络地址

#### `yield tcp_socket::connect(string addr, long port)`
连接指定的网络地址和端口

**注意**：
* `addr` 仅允许合法的 IPv4 或 IPv6 地址；若希望使用域名，请先对域名进行解析；

#### `yield tcp_socket::read([mixed $completion])`
从当前网络套接字中读取数据，可选的指定读取数据的“结束条件”（同 `unix_socket::read`）;

**注意**：
* 每次读取到的内容**长度不固定**，但不会过大（一般不会超过 2048 字节）；

**注意**：
* 除正常的网络关闭外，读取动作发生错误时可能抛出异常；

#### `yield tcp_socket::write(string data)`
进行一次网络写入操作（发送）；

**注意**：
* 当网络连接已断开等错误状态时，调用 `write` 会抛出异常；

#### `yield tcp_socket::close()`
关闭网络连接；

**注意**：
* 执行本函数会导致还未完成的 `read()` 和 `write()` 动作立刻结束并抛出错误；

### `class flame\net\tcp_server`
	封装 TCP 协议的网络服务器相关功能、接口；
#### `string tcp_server::$local_address`
	服务器本地监听地址
#### `tcp_server::bind(string addr, long port)`
绑定服务器地址端口，绑定后调用 `run()` 启动服务器开始监听指定的地址、端口；

**注意**：
* 框架默认使用 `REUSE_PORT` 可直接支持多进程监听同一端口同时处理；

#### `yield tcp_server::run()`
启动、运行服务器；

**注意**：
* 本函数会阻塞当前协程的运行，在服务器关闭 `close()` 后恢复；

#### `tcp_server::handle(callable cb) -> tcp_server`
设置连接处理回调函数，将新的连接 `tcp_socket` 对象回调传递给处理函数；

**注意**：
* `cb` 必须是 `Generator Function` 即 函数定义中包含 `yield` 表达式；
* 每个连接的处理过程在单独的协程中运行；

#### `yield tcp_socket::close()`
关闭服务器；

**注意**：
* 阻塞在 `run()` 函数的服务器协程会在调用本函数后恢复运行；
