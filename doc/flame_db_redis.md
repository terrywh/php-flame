### `namespace flame\db`
提供基本的 Redis 协程式客户端封装；

### `class flame\db\redis`
封装 HTTP 协议的请求，前置 `yield` 来调用接口，例如：

**示例**：
``` PHP
<?php
$cli = new flame\db\redis_client();
$cli->connect(["host"=>"127.0.0.1","port"=6379]);
yield $cli->set("key","val");
$val = yield $cli->get("key");
```

实际功能函数基于 `__call()` 魔术函数实现，包含大部分 Reids 指令，函数名称与指定名相同（忽略大小写），如：`hgetall` `lpush` 等等；

存在下面特殊返回：
* `hmget` | `hgetall` | `mget` 返回结果格式化为 k=>v 数组；
* `zrange*` 等命令补充 `WHITSCORES` 参数时，返回 k=>v 数组；

#### `redis::connect(array $opts)` | `redis::connect(string $uri)`
配置 `redis` 连接相关属性；当提供参数为数组时，需要以下结构：
```
	[
		"host"=>"127.0.0.1",
		"port"=>6379,
		"auth"=>"123456", // 可选
		"select"=>1, // 可选
	]
```
当提供参数为字符串时需要如下形式：
```
redis://127.0.0.1:6379/1?auth=123456
```

**注意**：
* `connect()` 不会立刻完成对 `Redis` 的连接，函数也不会阻塞（在第一次请求时完成连接）；

#### `redis::close()`
立刻主动关闭和 `redis` 的连接。

**注意**：
* 不调用 `close()` 与 `Redis` 建立的连接也会在对象析构时自动释放；
