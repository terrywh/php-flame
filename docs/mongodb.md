## `namespace flame\mongodb`

<!-- TOC depthFrom:3 -->

- [`yield flame\mongodb\connect(string $url) -> flame\mongodb\client`](#yield-flame\mongodb\connectstring-url---flame\mongodb\client)
- [`class flame\mongodb\client`](#class-flame\mongodb\client)
    - [`flame\mongodb\collection client::__get(string $name)`](#flame\mongodb\collection-client__getstring-name)
    - [`flame\mongodb\collection client::collection(string $name)`](#flame\mongodb\collection-clientcollectionstring-name)
    - [`yield client::execute(array $command[, boolean $update = false]) -> array | flame\mongodb\cursor`](#yield-clientexecutearray-command-boolean-update--false---array--flame\mongodb\cursor)
- [`class flame\mongodb\collection`](#class-flame\mongodb\collection)
    - [`yield collection::insert(array $docs[, boolean $ordered = true]) -> array`](#yield-collectioninsertarray-docs-boolean-ordered--true---array)
    - [`yield collection::delete(array $query[, Integer $limit = 0]) -> array`](#yield-collectiondeletearray-query-integer-limit--0---array)
    - [`yield collection::update(array $query, array $data[, boolean $upsert = false]) -> array`](#yield-collectionupdatearray-query-array-data-boolean-upsert--false---array)
    - [`yield collection::find(array $query[, array $projections[, array $sort[, array|integer $limit]]]) -> flame\mongodb\cursor`](#yield-collectionfindarray-query-array-projections-array-sort-arrayinteger-limit---flame\mongodb\cursor)
    - [`yield collection::one(array $query[, array $sort]) -> array | null`](#yield-collectiononearray-query-array-sort---array--null)
    - [`yield collection::get(string $field, array $query[, array $sort]) -> mixed`](#yield-collectiongetstring-field-array-query-array-sort---mixed)
    - [`yield collection::count([array $query)`](#yield-collectioncountarray-query)
    - [`yield collection::aggregate(array $pipeline) -> cursor`](#yield-collectionaggregatearray-pipeline---cursor)
- [`class flame\mongodb\object_id`](#class-flame\mongodb\object_id)
    - [`object_id::__toString()`](#object_id__tostring)
    - [`object_id::jsonSerialize()`](#object_idjsonserialize)
    - [`object_id::unix()`](#object_idunix)
    - [`date_time::__toDateTime()`](#date_time__todatetime)
- [`class flame\db\mongodb\date_time`](#class-flame\db\mongodb\date_time)
    - [`date_time::__toString()`](#date_time__tostring)
    - [`date_time::jsonSerialize()`](#date_timejsonserialize)
    - [`date_time::unix()`](#date_timeunix)
    - [`date_time::unix_ms()`](#date_timeunix_ms)
    - [`date_time::__toDateTime()`](#date_time__todatetime-1)
- [`class flame\mongodb\cursor`](#class-flame\mongodb\cursor)
    - [`yield cursor::fetch_row() -> array | null`](#yield-cursorfetch_row---array--null)
    - [`yield cursor::fetch_all() -> array`](#yield-cursorfetch_all---array)

<!-- /TOC -->

提供基本的异步 mongodb 协程式客户端封装；

**注意**：
* 对象标识 `ObjectID` 字段需要使用单独的类型 `flame\mongodb\object_id`；
* 日期时间 `DateTime` 字段需要使用单独的类型 `flame\mongodb\date_time`；
* 暂不支持 时间戳、正则 等在 Web 开发中不常用的字段类型；
* 其他字段对应相关 PHP 内置类型；
* 在使用 MongoDB 提供的 `operator` 时，可考虑使用 `'` 单引号，防止功能符号被转义，例如 `$gt = "aaa"; $filter = ["a" => ['$gt'=>1234]]`；或使用转义符号 `"\$gt"`；
* 实际指令格式请参考 [MongoDB 文档](https://docs.mongodb.com/manual/reference/command/) 中相关说明;

### `yield flame\mongodb\connect(string $url) -> flame\mongodb\client`
连接 MongoDB 数据库, 并返回数据库客户端对象; 连接地址 `$url` 形式如下:

```
mongodb://{user}:{pass}@{host}:{port}[,{host}:{port}, ...]/{database}?{option1=value1}[&option2=value2& ...]
```

**示例**：
``` PHP
// ...
$cli = yield flame\mongodb\connect("mongodb://127.0.0.1:27017/database");
// 以下两行代码功能相同
$collection = $cli->col_abc;
$collection = $cli->collection("col_abc");
// 执行指令
$cli->execute(["find"=> ..., ...]/*, false*/);
$cli->execute(["update"=> ..., ...], true); // 更新型指令回去副本集主节点执行
// collection 中存在简化接口
$collection->find(...);
$collection->update(...);
```

**注意**:
* 连接选项中请一定确认制定 `readPreference` 选项:
	* `primary`
	* `primaryPreferred`
	* `secondary`
	* `secondaryPreferred`
	* `nearest` 
	请参考 [MongoDB 相关文档](https://docs.mongodb.com/manual/core/read-preference/) 确认相关选项含义;

### `class flame\mongodb\client`

mongodb 客户端（连接）；

#### `flame\mongodb\collection client::__get(string $name)`
#### `flame\mongodb\collection client::collection(string $name)`
获取指定名称的 collection 集合对象；

#### `yield client::execute(array $command[, boolean $update = false]) -> array | flame\mongodb\cursor`
执行指定的命令; 若指定 `$update = true` 表示更新型指令, 此种指令会强制去副本集主节点执行; 执行完毕后返回执行结果或结果集指针对象;


### `class flame\mongodb\collection`
封装访问 mongodb 数据集合的基础 API；

#### `yield collection::insert(array $docs[, boolean $ordered = true]) -> array`
向当前集合插入一个 (一维K/V) 或 多个文档 (二位多个K/V)，并返回 `write_result` 结果；`$ordered = true` 时多项写入发生异常时, 停止后续写入 (否则继续);

**注意**：
* 可以考虑为写入的文档创建 `_id` 字段（参考 `flame\mongodb\object_id` 类型）以提前获知插入的 `_id`；

#### `yield collection::delete(array $query[, Integer $limit = 0]) -> array`
从当前集合中删除符合查询条件 `$query` 的文档, 可选的限制删除的数量 `$limit`;

#### `yield collection::update(array $query, array $data[, boolean $upsert = false]) -> array`
从当前集合中查询符合 `$query` 的文档，使用 `$data` 更新它；当 `$upsert = true` 时, 不存在符合条件的文档, 插入一个新的文档;

#### `yield collection::find(array $query[, array $projections[, array $sort[, array|integer $limit]]]) -> flame\mongodb\cursor`
使用 `$query` 查询当前集合, 可选的设置返回的字段 `$projection`, 排序 `$sort`, 限制结果集大小 `$limit`，完成后返回结果集指针 `cursor` 对象;

其中 `$projection` 可用如下形式:
* 数组 `["field1", "field2"]` 返回 `field1` `field2` 两个字段;
* 数组 `["field1"=>1, "field2"=>-1]` 返回 `field1` 但不返回 `field2` 字段;
* 字符串 `"field1"` 仅返回 `field1` 字段;
* NULL 返回所有字段;

其中 `$limit` 可用如下形式:
* 数值 `100` 限制仅返回 100 条记录;
* 数组 `[100, 200]` 跳过 100 条后返回 200 条记录;

#### `yield collection::one(array $query[, array $sort]) -> array | null`
按 `$query` 查询当前集合, 可选的按照 `$sort` 排序, 若存在则返回该文档关联数组, 否则返回 NULL;

#### `yield collection::get(string $field, array $query[, array $sort]) -> mixed`
按 `$query` 查询当前集合, 可选的按照 `$sort` 排序, 若存在则返回该文档中对应 `$field` 字段的值, 否则返回 NULL;

#### `yield collection::count([array $query)`
获取当前集合中（符合条件的）文档数量; 

#### `yield collection::aggregate(array $pipeline) -> cursor`
在当前集合上进行由 `$pipeline` 定义的一些列聚合操作, 并返回数据结果游标；

**注意**:
* 聚合操作 `$pipeline` 应为二维数组, 例如: `[ ['$group'=> ...], ['$lookup'=> ...], ...])`;

### `class flame\mongodb\object_id`

#### `object_id::__toString()`
#### `object_id::jsonSerialize()`
序列化及调试输出;

#### `object_id::unix()`
秒级时间戳，表示当前 `object_id` 的创建时间;

#### `date_time::__toDateTime()`
转换为 PHP 内置 [`DateTime`](http://php.net/manual/en/class.datetime.php) 类型对象；

### `class flame\db\mongodb\date_time`

#### `date_time::__toString()`
#### `date_time::jsonSerialize()`
序列化及调试输出;

#### `date_time::unix()`
时间戳（单位：秒）

#### `date_time::unix_ms()`
时间戳（单位：毫秒）

#### `date_time::__toDateTime()`
转换为 PHP 内置 [`DateTime`](http://php.net/manual/en/class.datetime.php) 类型对象；

### `class flame\mongodb\cursor`
封装结果集指针类型，用于访问查询结果数据；

#### `yield cursor::fetch_row() -> array | null`
遍历底层指针，返回下一个文档；当遍历结束时，返回 `NULL`;

**示例**：
``` PHP
<?php
// ... 通过 yield collection->find() 获取 $cursor
while($doc = yield $cursor->fetch_row()) {
	echo json_encode($doc);
}
```

#### `yield cursor::fetch_all() -> array`
遍历底层指针，返回结果集中的所有文档（关联数组）组成的数组；
