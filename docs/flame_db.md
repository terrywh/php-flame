## `namespace flame\db`
提供基本的数据库资源客户端的异步协程式端封装：
1. [Redis](/php-flame/flame_db)
2. [MySQL](/php-flame/flame_db_mysql)；
3. [Mongodb](/php-flame/flame_db_mongodb)；
4. [Kafka](/php-flame/flame_db_kafka)；
5. [RabbitMQ](/php-flame/flame_db_rabbitmq)；

### `class flame\db\redis`
封装 REDIS 协议的请求、命令，注意需要前置 `yield` 来调用命令，例如：

**示例**：
``` PHP
<?php
// ...
$cli = new flame\db\redis();
yield $cli->connect("127.0.0.1",6379);
// 可选
yield $cli->auth("123456");
yield $cli->select(31);
yield $cli->set("key","val");
$val = yield $cli->get("key");
```

实际功能函数基于 `__call()` 魔术函数实现，包含大部分 Reids 指令，函数名称与指定名相同（忽略大小写），如：`hgetall` `lpush` 等等；

存在下面特殊返回：
* `hmget` | `hgetall` | `mget` 返回结果格式化为 k=>v 数组；
* `zrange*` 等命令补充 `WHITSCORES` 参数时，返回 k=>v 数组；

**注意**：
* 所有命令函数均需前置 `yield` 关键字进行调用；

#### `yield redis::connect(string $host, integer $port)`
连接 `redis` 服务器；

#### `redis::close()`
立刻主动关闭和 `redis` 的连接；

**注意**：
* 不调用 `close()` 与 `Redis` 建立的连接**也会**在对象析构时自动释放；

#### `yield redis::subscribe(callable $cb, string $chan[, string $chan, ...])`
监听（订阅）指定通道 `$chan`，当另外的 redis 连接向该通道进行 `publish` 推送时，调用 `$cb` 回调函数；可指定多个 `$chan` 参数，同时订阅；
回调函数原型如下：
``` PHP
<?php
function callback($chan, $data) {
	// ...
}
```

**注意**：
* 不能（在多个协程中）同时建立订阅监听过程（`subscribe` / `psubscribe` 也不能同时使用），否则会引起；
* 监听会“阻塞”当前协程，并在单独的“协程”中调用回调函数；
* 当发生错误时，`subscribe` 的流程将立刻结束（即恢复运行）并抛出错误；

#### `yield redis::psubscribe(callable $cb, string $chan[, string $chan, ...])`
监听（订阅）指定通道模式匹配的所有通道，参考 `redis::subscribe()`；

**注意**：
* 不能（在多个协程中）同时建立订阅监听过程（`subscribe` / `psubscribe` 也不能同时使用），否则会引起；

#### `redis::stop_all()`
结束已建立的监听过程（恢复监听协程的运行）；

**注意**：
* 此函数与 REDIS 操作 `UNSUBSCRIBE` | `PUNSUBSCRIBE` 对应，但不支持传递参数（关闭所有当前连接建立的订阅监听）；
