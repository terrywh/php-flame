### flame\net
基础网络功能的协程式封装

#### flame\net\tcp_socket
功能：
	封装 TCP 协议的网络连接
原型：
```
	class tcp_socket {}
```
##### flame\net\tcp_socket::connect
原型：
```
	connect(string addr, long port);
```
##### flame\net\tcp_socket::read
原型：
```
	read();
```
##### flame\net\tcp_socket::write
原型：
```
	write(string data);
```
##### flame\net\tcp_socket::close
原型：
```
	close();
```
#### flame\net\tcp_server
功能：
	封装 TCP 协议的网络服务器
原型：
```
	class tcp_server {}
```
##### flame\net\tcp_server::bind
原型：
```
	bind(string addr, long port);
```
##### flame\net\tcp_server::run
原型：
```
	run()
```
##### flame\net\tcp_socket::handle
原型：
```
	handle(callable cb);
```
注：`cb` 必须是 `Generator Function`
##### flame\net\tcp_socket::close
原型：
```
	close();
```