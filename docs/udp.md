### `namespace flame\udp`
提供 UDP 协议的网络连接对象的封装;

### `class flame\udp\socket`
封装 UDP 协议网络服务器、客户端对象;

#### `socket::__construct([string $address])`
构建一个可选绑定到指定地址端口 `$address` 的 UDP 套接字对象;

#### `string socket::$local_address`
本地网络地址;

#### `yield socket::read()`
接收数据，并返回接收到的`udp_packet` 类型 数据包对象；若当前套接字被关闭，将返回 `NULL`；

**示例**：
``` PHP
// ....
$remote_address;
$packet = yield $socket->read($remote_address);
echo "packet data: ", $packet, "\n";
echo "from: ", $remote_address "\n";
```

**注意**：
* 若当前对象还未绑定, 框架将自动绑定 `0.0.0.0` 的随机端口；

#### `yield socket::send(string $data, string $address)`
向指定 `$address` 地址端口, 发送指定 `$packet` 内容;

**注意**:
* 若当前对象还未绑定, 框架将自动绑定 `0.0.0.0` 的随机端口；

#### `socket::close()`
关闭当前套接字对象;

**注意**:
* 执行本函数会导致还在进行中的接收和发送动作立刻结束；
