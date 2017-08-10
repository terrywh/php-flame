### `namespace flame\net`
提供基本的TCP UDP 协议网络协程式客户端、服务器封装；
内部包：
* http - HTTP 协议服务端、客户端支持；
* fastcgi - FASTCGI/1.1 精简版本的应用服务支持，可以挂接 nginx 等 Web 服务器使用；

### `class flame\net\tcp_socket`

	封装 TCP 协议的网络连接

#### `yield tcp_socket::connect(string addr, long port)`
**功能**：
	封装 TCP 协议的网络连接

**注意**：
* `addr` 仅允许合法的 IPv4 或 IPv6 地址，若使用域名，请先对域名进行解析（参见 `namespace flame\net\dns` 提供的相关接口）；

#### `yield tcp_socket::read()`
**功能**：
	进行一次网络读取操作（接收），若网络已关闭（EOF）则返回 NULL；否则返回读取到的内容；

**注意**：
* 每次读取到的内容**长度不固定**，但不会过大（一般不会超过 2048 字节）；

#### `yield tcp_socket::write(string data)`
**功能**：
	进行一次网络写入操作（发送）；

#### `yield tcp_socket::close()`
**功能**：
	关闭网络连接；

**注意**：
* 执行本函数会导致还未完成的 `read()` 和 `write()` 动作立刻结束并抛出错误；

### `class flame\net\tcp_server`
	封装 TCP 协议的网络服务器相关功能、接口；

#### `tcp_server::bind(string addr, long port)`
**功能**：
	绑定服务器地址端口，绑定后调用 `run()` 启动服务器开始监听指定的地址、端口；

#### `yield tcp_server::run()`
**功能**：
	启动、运行服务器；

**注意**：
* 本函数会阻塞当前协程的运行，在服务器关闭 `close()` 后恢复；

#### `tcp_server::handle(callable cb)`
**功能**：
	设置连接处理回调函数，将新的连接 `tcp_socket` 对象回调传递给处理函数；

**注意**：
* `cb` 必须是 `Generator Function` 即 函数定义中包含 `yield` 表达式；
* 每个连接的处理过程在单独的协程中运行；

#### `yield tcp_socket::close()`
**功能**：
	关闭服务器；

**注意**：
* 阻塞在 `run()` 函数的服务器协程会在调用本函数后恢复运行；

### `class flame\net\udp_socket`

	封装 UDP 协议网络服务器、客户端连接

#### `udp_socket::bind(string addr, long port)`
将当前对象绑定到指定的地址、端口；绑定后，可以接收来自这个地址、端口的数据；

**注意**：
* 下述 `recv_from()` 和 `send_to()` 函数会在未绑定地址、端口时，自动进行绑定；

#### `yield udp_socket::recv_from(string& addr, long& port)`
接收来自某地址、端口的数据，来源方地址信息保存在 `addr`, `port` 中；两项参数均可选；

**注意**：
* 若当前对象还未绑定地址、端口，系统将自动绑定 `0.0.0.0` 的随机端口；

#### `yield udp_socket::send_to(string data, string addr, long port)`
向指定 `addr` 地址，`port` 端口发送指定 `data` 内容

**注意**:
* 若当前对象还未绑定地址、端口，系统将自动绑定 `0.0.0.0` 的随机端口；
