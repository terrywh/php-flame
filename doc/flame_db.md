### `namespace flame\db`
提供基本的异步 Redis 协程式客户端封装；提供基本的“伪异步” MongoDB 协程式客户端封装；

### `class flame\db\redis`
封装 REDIS 协议的请求、命令，注意需要前置 `yield` 来调用命令，例如：

**示例**：
``` PHP
<?php
$cli = new flame\db\redis();
$cli->connect("127.0.0.1",6379);
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

#### `redis::connect(string $host, integer $port)` | `redis::connect(string $uri)`
配置 `redis` 连接相关属性；

**注意**：
* `connect()` 不会立刻完成对 `Redis` 的连接，函数也不会阻塞（实际会在第一次命令请求时进行连接）；

#### `redis::close()`
立刻主动关闭和 `redis` 的连接。

**注意**：
* 不调用 `close()` 与 `Redis` 建立的连接**也会**在对象析构时自动释放；

#### `yield redis::subscribe(callable $cb, string $chan[, string $chan, ...])`
订阅指定通道 `$chan`，当另外的 redis 连接向该通道进行 `publish` 推送时，调用 `$cb` 回调函数；可指定多个 `$chan` 参数，同时订阅；
回调函数原型如下：
```
function callback($chan, $data);
```
**注意**：
* 当发生错误时，`subscribe` 的流程将立刻结束（即恢复运行）并抛出错误；

### `class flame\db\mongodb\client`
封装简单的“伪异步” mongodb 的客户端；由于 MongoDB 官方不提供 C/C++ 异步版本的驱动，此封装实质是在额外的线程中进行 MongoDB 相关操作“模拟异步”来实现的；

**示例**：
``` PHP
<?php
$cli = new flame\db\mongo("mongodb://127.0.0.1:27017/database");
// 以下两行代码功能相同
$collection = $cli->col_abc;
$collection = $cli->collection("col_abc");
```

#### `client::collection(string $name) | clieng::$collection_name`
获取指定名称的 collection 集合对象；`client` 类实现了魔术函数 `__get()` 可以直接用集合名称，通过属性形式访问对应的 collection 集合；

### `class flame\db\mongodb\object_id`
#### `object_id::__toString() | object_id::toString()`
#### `object_id::jsonSerialize()`

### `class flame\db\mongodb\timestamp`
#### `timestamp::__toString()`
#### `timestamp::toInteger()`
#### `timestamp::jsonSerialize()`

### `class flame\db\mongodb\collection`
封装访问 mongodb 数据集合的基础 API；

**示例**：
``` PHP
<?php
// ... 通过 $client 获得 $collection 对象
$result = yield $collection->insert_one(["a"=>"b"]);
$result = yield $collection->insert_many([["a"=>"b1"], ["a"=>"b2"]]);
$result = yield $collection->delete_many(["a"=>"b"]);
$result = yield $collection->delete_one(["a"=>["$ne"=>"b"]]);
$result = yield $collection->update_one(["a"=>"b"],["$set"=>["a"=>"c"]]);
$result = yield $collection->update_many(["a"=>"b"],["a"=>"c"]);
$cursor = yield $collection->find_many(["a"=>"b"]);
$doc = yield $collection->find_one(["a"=>["$gt"=>"b"]]);
$keys  = yield $collection->distinct("a", ["x"=>"y"]);
$count = yield $collection->count(["x"=>"y"]);
```

**注意**：
* `_id` 字段需要使用单独的类型 `object_id`；
* 日期时间 字段需要使用 PHP 内置类型 `DateTime`；
* 时间戳 字段需要使用单独的类型 `timestamp`；
* 其他字段对应相关 PHP 内置类型；

#### `yield collection::insert_one(array $doc)`
向当前集合插入一条新的 `$doc` 文档；

**注意**：
* `$doc` 必须是关联数组；
* 可以自行为 `$doc` 创建 `_id` 字段（参考 `object_id` 类型）；

#### `yield collection::insert_many(array $docs)`
向当前集合插入 `$docs` 若干文档；

**注意**：
* `$docs` 为二维数组，其中第一维度为下标数组，每个元素标识一个文档（关联数组）；

#### `yield collection::delete_many(array $query)`
从当前集合中删除所有符合 `$query` 查询的文档；

#### `yield collection::delete_many(array $query)`
从当前集合中删除第一个符合 `$query` 查询的文档；

#### `yield collection::update_one(array $query, array $data)`
从当前集合中查询第一个符合 `$query` 的文档，并使用 `$data` 更新它；

#### `yield collection::update_many(array $query, array $data)`
从当前集合中查询所有符合 `$query` 的文档，并使用 `$data` 更新它们；

#### `yield collection::find_many(array $query[, array $sort[, integer $skip[, integer $count[, array $fields]]]])`
查询当前集合，返回结果集指针 `cursor` 对象；
* `$query` - 查询内容
* `$sort`  - 排序
* `$skip`  - 结果集起始跳过；
* `$count` - 限制结果集数量；
* `$fields` - 仅返回特定字段；

#### `yield collection::find_one(array $query, array $sort)`
查询当前集合，返回单条文档记录（关联数组）；


### `class flame\db\mongodb\cursor`
封装结果集指针类型，用于访问查询结果数据；

#### `yield cursor::next()`
遍历底层指针，返回下一个文档；当遍历结束时，返回 null;

**示例**：
``` PHP
<?php
// ... 通过 collection->find_many() 获取 $cursor
while($doc = yield $cursor->next()) {
	echo json_encode($doc);
}
```

#### `yield cursor::toArray()`
遍历底层指针，返回结果集中的所有文档（关联数组）组成的数组；
