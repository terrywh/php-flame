### `namespace flame\tcp`
提供 TCP 协议的网络连接对象的封装;

#### `yield flame\tcp\connect(string $address) -> flame\tcp\socket`
连接到指定的远端地址, 完成后返回连接套接字对象实例;

**注意**：
* `$address` 仅允许合法的 IPv4 或 IPv6 地址 ( 不能直接使用域名 )；

### `class flame\tcp\socket`
提供 TCP 客户端 Socket 接口封装;

**注意**:
* 当 `socket` 套接字对象销毁时将会关闭对应的网络连接;

#### `String socket::$local_address`
本地网络地址

#### `String socket::$remote_address`
远端网络地址

#### `yield socket::read([mixed $completion]) -> String`
从当前网络套接字中读取数据，可选的指定读取数据的“结束条件”:
* `Integer` - 指示读取指定长度;
* `String` - 指示读到取指定结束符;
* 无 - 指示随机读取一段数据;

**注意**：
* 每次读取到的内容**长度不固定**；
* 若读取时对方关闭了连接, 会返回 `NULL`;

#### `yield socket::write(string data) -> void`
进行一次网络写入操作（发送）；

**注意**：
* 当网络连接已断开等错误状态时，调用 `write` 会抛出异常；

#### `yield socket::close() -> void`
关闭网络连接；

**注意**：
* 执行本函数会导致还在进行中的接收和发送动作立刻结束；

### `class flame\tcp\server`
封装 TCP 协议的网络服务器相关功能、接口；

#### `server::__construct(string $address)`
构建一个监听指定地址 `$address` 的服务器对象, 在实际接收到远端连接后启用新的协程执行回调 `$cb`;

**注意**：
* `$address` 仅允许合法的 IPv4 或 IPv6 地址 ( 不能直接使用域名 )；

#### `string server::$local_address`
服务器本地监听地址

#### `yield server::run(callable $cb) -> void`
运行(启动)服务器；协程回调用于接收和处理客户端连接, 原型如下:

``` PHP
function (flame\tcp\socket $socket) {}
```

**注意**：
* 本函数会阻塞当前协程的运行，在服务器关闭 `close()` 后恢复；

#### `yield server::close()`
关闭服务器, 阻塞在 `run()` 函数的协程会在调用本函数后恢复运行；
