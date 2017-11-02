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

#### `redis::connect(string $host, integer $port)`
配置 `redis` 连接相关属性；

**注意**：
* `connect()` 不会立刻完成对 `Redis` 的连接，函数也不会阻塞（实际会在第一次命令请求时进行连接）；

#### `redis::close()`
立刻主动关闭和 `redis` 的连接；

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
封装简单的“伪异步” mongodb 客户端；由于 MongoDB 官方不提供 C/C++ 异步版本的驱动，此封装实质是在额外的线程中进行 MongoDB 相关操作“模拟异步”来实现的；

**示例**：
``` PHP
<?php
flame::go(function() {
	$cli = new flame\db\mongo();
	yield $cli->connect("mongodb://127.0.0.1:27017/database");
	// 以下两行代码功能相同
	$collection = $cli->col_abc;
	$collection = $cli->collection("col_abc");
});
```

#### `yield client::connect([string $uri])`
连接 mongodb 数据库，不传递参数标识“重连”；

连接字符串请参考：[Connection String URI Format](https://docs.mongodb.com/manual/reference/connection-string/)

#### `yield client::collection(string $name)`
获取指定名称的 collection 集合对象；

#### `yield client::close()`
关闭当前数据库连接；

**注意**：
* 当连接关闭后，由 `client` 对象生成出的相关对象均不能继续使用，例如由 `collection` 对象；

### `class flame\db\mongodb\object_id`
#### `object_id::__toString()`
#### `object_id::jsonSerialize()`
#### `object_id::timestamp()`
秒级时间戳，表示当前 `object_id` 的创建时间

### `class flame\db\mongodb\date_time`
#### `date_time::__toString() | object_id::toString()`
#### `date_time::jsonSerialize()`
#### `date_time::timestamp()`
秒级时间戳
#### `date_time::timestamp_ms()`
毫秒级时间戳
#### `date_time::to_datetime()`
转换为 [`DateTime`](http://php.net/manual/en/class.datetime.php) PHP 内置类型对象；

### `class flame\db\mongodb\bulk_result`
#### `boolean bulk_result::success()`
检查批量操作的总体结果，`true` 全部成功，`false` 失败或部分失败；

#### `integer bulk_result::$nInserted`
#### `integer bulk_result::$nMatched`
#### `integer bulk_result::$nModified`
#### `integer bulk_result::$nRemoved`
#### `integer bulk_result::$nUpserted`

### `class flame\db\mongodb\collection`
封装访问 mongodb 数据集合的基础 API；

**示例**：
``` PHP
<?php
// ... 通过 $client 获得 $collection 对象
$result = yield $collection->insert_one(['a'=>'b']);
$result = yield $collection->insert_many([['a'=>'b1'], ['a'=>'b2']]);
$result = yield $collection->delete_one(['a'=>['$ne'=>'b']]);
$result = yield $collection->delete_many(['a'=>'b']);
$result = yield $collection->update_one(['a'=>'b'],['$set'=>['a'=>'c']]);
$result = yield $collection->update_many(['a'=>'b'],['a'=>'c']);
$cursor = yield $collection->find_many(['a'=>'b']);
$doc = yield $collection->find_one(['a'=>['$gt'=>'b']]);
$keys  = yield $collection->distinct('a', ['x'=>'y']);
$count = yield $collection->count(['x'=>'y']);
```

**注意**：
* 对象标识 `ObjectID` 字段需要使用单独的类型 `object_id`；
* 日期时间 `DateTime` 字段需要使用单独的类型 `date_time`；
* 暂不支持 时间戳、正则 等在 Web 开发中不常用的字段类型；
* 其他字段对应相关 PHP 内置类型；
* 请尽量使用 `'` 单引号，防止功能符号被转义，例如 `$gt = "aaa"; $filter = ["a" => ["$gt"=>1234]]`；

#### `yield collection::count([array $query])`
获取当前集合中（符合条件的）文档数量；

#### `yield collection::insert_one(array $doc)`
向当前集合插入一条新的 `$doc` 文档；

**注意**：
* `$doc` 必须是关联数组；
* 可以自行为 `$doc` 创建 `_id` 字段（参考 `object_id` 类型）；

#### `yield collection::insert_many(array $docs)`
向当前集合插入 `$docs` 若干文档；返回批量结果对象 `bulk_result`；

**注意**：
* `$docs` 为二维数组，其中第一维度为下标数组，每个元素标识一个文档（关联数组）；

#### `yield collection::remove_one(array $query)`
从当前集合中删除第一个符合 `$query` 查询的文档；删除成功返回 `true`，否则抛出发生的错误；

#### `yield collection::delete_many(array $query)`
从当前集合中删除所有符合 `$query` 查询的文档；删除成功返回 `true`，否则抛出发生的错误；

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

#### `yield cursor::to_array()`
遍历底层指针，返回结果集中的所有文档（关联数组）组成的数组

**注意**：
* 混合使用 `next()` 和 `toArray()` 两种读取方式可能导致未知错误；

### `class flame\db\mysql\client`
封装简单的“伪异步” mysql 客户端；由于 MySQL 官方不提供 C/C++ 异步版本的驱动（非异步协议），此封装实质是在额外的工作县城中进行 MySQL 相关操作“模拟异步”来实现的；

**示例**：
``` PHP
<?php
flame\go(function() {
	$cli = new flame\db\mysql\client();
	yield $cli->connect("mysql://username:password@127.0.0.1:3306/database");
});
```

#### `yied client::connect([string $uri])`
连接 mysql 数据库；不传递参数时表示“重连”；

连接字符串形式如下：
```
mysql://{username}:{password}@{host}:{port}/{database}
```

#### `yield client::query(string $sql) | yield client::query(string $format[, mixed $arg1, mixed $arg2, ...])`
进行“查询”操作并返回对应的 `result_set` 结果集对象 或 “更新”操作返回 `result_info` 执行结果；注意后一种函数原型，允许进行“格式化”参数替换即：将 $arg1 / $arg2 等参数替换到 $format 定义的格式化字符串中，自动进行转义和包裹，例如：

``` PHP
$rs = $cli->query("SELECT * FROM `test`");
$aa = "aaaa";
$bb = 123456;
$rs = $cli->query("SELECT * FROM `test` WHERE `a`=? AND `b`=?", $aa, $bb);
$rb = $cli->query("DELETE FROM `test` WHERE `a`=?", $aa);
```

**注意**：
* 请不要直接拼接 二进制数据 进行 SQL 语句，使用 `prepare` -> `bind` 代替；

#### `yield client::insert_many(string $table, array $datas)`
#### `yield client::insert_one(string $table, array $data)`

#### 条件描述 `$conditions`
下述 `delete_*` / `update_*` / `select_*` 相关函数使用的条件匹配均为如下描述的形式；形式参照了 MongoDB 查询运算符，并定义了一些 SQL 特例的实现：

* `['a'=>'aaaa','b'=>'bbbb']`
``` SQL
`a`='aaaa' AND `b`='bbbb'
```
* `['$or'=>['a'=>'aaaa','b'=>'bbbb'], '$and'=>['c'=>'cccc','d'=>'dddd']]`
``` SQL
(`a`='aaaa' OR `b`='bbbb') AND (`c`='cccc' AND `d`='dddd')
```
* `['a'=>['$in'=>['aaaa','bbbb','cccc']]]`
``` SQL
`a` IN ('aaaa','bbbb','cccc')
```
* `['a'=>['$gt'=>1111], 'b'=>['$gte'=>2222], 'c'=>['$lt'=>3333], 'd'=>['$lte'=>4444], 'e'=>['$ne'=>5555]]`
``` SQL
`a` > 1111 AND `b` >= 2222 AND `c` < 3333 AND `d` <= 4444 AND `e` != 5555
```
* `['a'=>['$like'=>'aaaa%']]`
``` SQL
`a` LIKE 'aaaa%'
```
* `['a'=>['$range'=>[1234, 5678]]]`
``` SQL
`a` BETWEEN 1234 AND 5678
```

**注意**：
* 请尽量使用 `'` 单引号，防止功能符号应含有 `$` 被转义；

#### `yield client::delete(string $table, array $conditions[, mixed $order[, mixed $limit]])`
删除匹配条件的行；

**示例**：
``` PHP
<?php
// $mysql = ....
$mysql->delete("test", ["id"=>123], ["id"=>-1, "key" => 1], [0, 100]);
// DELETE FROM `test` WHERE `id`=123 ORDER BY `id` DESC, `key` ASC LIMIT 0, 100
$mysql->delete("test", ["id"=>123], '`id` DESC, `key` ASC', 100);
$mysql->delete("test", ["id"=>123], ["id"=>false, "key"=> true], "0,100");
```

#### `yield client::update(string $table, array $conditions, array $modify[, array $order[, mixed $limit]])`
更新匹配条件的所有行，并应用由 `$modify` 描述的更改；

**示例**：
``` PHP
<?php
// $mysql = ....
$mysql->update("test", ["$or"=>["id"=>['$gt'=>123], "key"=>"xxxxx"]], ["key"=>"kkkkk", "val"=>"vvvvv"]);
// UPDATE `test` SET `key`='kkkkk', `val`='vvvvv' WHERE (`id`>123 OR `key`='xxxxx')
$mysql->update("test", ["key"=>['$like'=>"xxx%"]], ["val"=>"aaaaa"]);
// UPDATE `test` SET `val`='aaaaa' WHERE `key` LIKE 'xxx%'
```

#### `yield client::select(string $table[, mixed $columns[, array $conditions[, mixed $order[, mixed $limit]]]])`
执行查询并返回结果集 result_set 对象；

**示例**：
``` PHP
<?php
// $mysql = ....
$mysql->select("test", "*", ["id"=>['$gt'=>123]]);
// SELECT * FROM `test` WHERE `id`>123
$mysql->select("test", ["id", "key","val"], ["key"=>['$ne'=>"aaaaa"]], ["key"=>-1], [0, 10]);
// SELECT `id`,`key`,`val` FROM `test` WHERE `key`!='aaaaa' ORDER BY `key` DESC LIMIT 0, 10
```

**注意**：
* 如需要在字段名称上使用 SQL 函数，请使用文本形式自行指定 $fields 参数；数组形式不支持此种语法；

#### `yield client::one(string $table, array $conditions)`
执行查询并限定结果集大小为1，若找到匹配数据，直接返回数据（关联数组）；若未找到，返回 null；
#### `yield client::found_rows()`
简化调用 `FOUND_ROWS()` SQL 函数的过程，直接返回行数量；
#### `yield client::begin_transaction()`
简化调用 `START TRANSACTION;` SQL 语句的过程；
#### `yield client::commit()`
简化调用 `COMMIT;` SQL 语句的过程；
#### `yield client::rollback()`
简化调用 `ROLLBACK;` SQL 语句的过程；

### `class flame\db\mysql\result_set`
数据查询结果集对象

**示例**：
``` PHP
<?php
// ... 通过 client 获得 result_set 对象 $rs
$rows = yield $rs->fetch_all(); // 获得所有数据对应的关联数组
while($row = yield $rs->fetch_assoc()) { // 依次获取每行数据关联数组
	var_dump($row);
}
while($row = yield $rs->fetch_array()) { // 依次获取每行数据关联数组
	var_dump($row);
}
```

#### `yield result_set::fetch_assoc()`
获取结果集中的所有数据，返回二位数组，其元素为每行数据的关联数组；
#### `yield result_set::fetch_array()`
获取结果集中的下一数据，返回该行数据的关联数组；
#### `yield result_set::fetch_all([integer $type = flame\db\mysql\FETCH_ASSOC])`
获取所有结果集数据，默认获取关联数组(`FETCH_ASSOC`)；也可自行指定获取数值下标数组(`FETCH_ARRAY`)；
