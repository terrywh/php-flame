### `namespace flame\db`
提供基本的 Redis 协程式客户端封装；

### `class flame\db\redis_client`

	封装 HTTP 协议的请求，前置yield来调用接口；
	大部分的命令和redis接口相同，如：
	$cli = new flame\db\redis_client();
	$cli->connect(["host"=>"127.0.0.1","port"=6379]);
	yield $cli->set("key","value");
	$res = yield $cli->get("key");
	var_dump($res); // string(5) "value"
	除了个别命令，会格式化返回结果方便使用；
	会把参数作为key，把返回作为value返回array的接口：
	`hmget`
	会把返回的array的value成对格式化为key=>value形式的接口：
	`hgetall` `[zrange*] WHITSCORES`
	

#### `redis_client::connect(array args)`
**功能**：
	配置 `redis` 连接相关属性。相关参数有：
	[
		"host"=>"127.0.0.1",
		"port"=>6379,
		"auth"=>"123456",
		"select"=>1
	]
**注意**：
* 参数中 `host` 和 `port` 是必选项，其他可以没有；
* connect不会立刻完成对redis的连接，函数也不会阻塞。但是会保证第一次请求肯定是在连接成功后才发起。

#### `redis_client::close()`
**功能**：
	立刻主动关闭和 `redis` 的连接。
**注意**：
* 不调用close连接也会在client被析构的时候释放；

#### `redis_client::getlasterror()`
**功能**：
	当 `redis` 发生错误的时候，使用 `getlasterror` 可以获得上一次命令的错误信息。
**注意**：
* 像其他getlasterror一样，再次发起请求会清空上一次的错误信息；


