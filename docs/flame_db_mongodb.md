## `namespace flame\db\mongodb`
提供基本的异步 mongodb 协程式客户端封装；

#### `flame\db\mongodb\READ_PREFS_PRIMARY`
#### `flame\db\mongodb\READ_PREFS_SECONDARY`
#### `flame\db\mongodb\READ_PREFS_PRIMARY_PREFERRED`
#### `flame\db\mongodb\READ_PREFS_SECONDARY_PREFERRED`
#### `flame\db\mongodb\READ_PREFS_NEAREST`
用于在特定的操作中指定读取倾向, 例如可以在查询请求中指定去主节点读取:
``` PHP
yield $collection->find_one([....], [
	"readPreference" => flame\db\mongodb\READ_PREFS_PRIMARY,
]);
```
一般当上述选项未指定时, 遵循 `client` 建立连接是的选项 (`?readPreference=....`); 若连接选项未指定, 默认为 `READ_PREFS_PRIMARY`;

### `class flame\db\mongodb\client`
mongodb 客户端（连接）；
**示例**：
``` PHP
<?php
$cli = new new flame\db\mongodb\client();
yield $cli->connect("mongodb://127.0.0.1:27017/database");
// 以下两行代码功能相同
$collection = $cli->col_abc;
$collection = $cli->collection("col_abc");
```

#### `yield client::collection(string $name)`
获取指定名称的 collection 集合对象；

**注意**:
* 由于异步过程获取 `collection` 对象, **不能**串联操作:
``` php
// !!! 错误的写法 !!!
yield $cli->collection("test1")->find_one([.....]);
```

#### `client::close()`
立刻关闭当前数据库连接；

**注意**：
* 当连接关闭后，由 `client` 对象生成出的相关对象均不能继续使用，例如由 `collection` 对象；

### `class flame\db\mongodb\object_id`
#### `object_id::__toString()`
#### `object_id::jsonSerialize()`
#### `object_id::timestamp()`
秒级时间戳，表示当前 `object_id` 的创建时间

### `class flame\db\mongodb\date_time`

#### `date_time::__toString()`

#### `date_time::jsonSerialize()`

#### `date_time::timestamp()`
时间戳（单位：秒）

#### `date_time::timestamp_ms()`
时间戳（单位：毫秒）

#### `date_time::to_datetime()`
转换为 PHP 内置 [`DateTime`](http://php.net/manual/en/class.datetime.php) 类型对象；

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
* 在使用 operator 时，可考虑使用 `'` 单引号，防止功能符号被转义，例如 `$gt = "aaa"; $filter = ["a" => ['$gt'=>1234]]`；或使用转义符号；

#### `yield collection::count([array $query[, array $options])`
获取当前集合中（符合条件的）文档数量; 目前可用的选项如下:
* `readPreference` - 读取倾向, 参见上面常量定义的说明;

#### `yield collection::insert_one(array $doc)`
向当前集合插入一条新的 `$doc` 文档，并返回 `write_result` 结果对象；

**注意**：
* 顶级 `$doc` 必须是关联数组，但其成员可以存在下标数组；
* 可以自行为 `$doc` 创建 `_id` 字段（参考 `object_id` 类型）；

#### `yield collection::insert_many(array $docs[, mixed $opts])`
向当前集合插入 `$docs` 若干文档，并返回 `write_result` 结果对象；目前可用选项如下：
* `"ordered" => false` 尝试写入所有文档（遇到错误时继续）；

**注意**：
* 顶级 `$docs` 为下标数组，每个元素为关联数组标识一个文档；

#### `yield collection::remove_one(array $query)`
从当前集合中删除第一个符合 `$query` 查询的文档，并返回 `write_result` 结果对象；

#### `yield collection::delete_many(array $query)`
从当前集合中删除所有符合 `$query` 查询的文档，并返回 `write_result` 结果对象；

#### `yield collection::update_one(array $query, array $data[, mixed $options])`
从当前集合中查询第一个符合 `$query` 的文档，使用 `$data` 更新它，并返回 `write_result` 结果对象；目前可用选项如下：
* `"upsert" => true` 没有匹配的文档时，创建新的文档；

**注意**：
* `$options = true;` 时相当于设置 `$options = ["upsert"=>true]`;

#### `yield collection::update_many(array $query, array $data[, mixed $options])`
从当前集合中查询所有符合 `$query` 的文档，使用 `$data` 更新它们，并返回 `write_result` 结果对象；目前可用选项如下：
* `"upsert" => true` 没有匹配的文档时，创建新的文档；

**注意**：
* `$options = true;` 时相当于设置 `$options = ["upsert"=>true]`;

#### `yield collection::find_many(array $query[, array $options])`
查询当前集合，返回结果集指针 `cursor` 对象；目前可用 $options 选项如下：
* `readPreference` - 读取倾向, 参见上面常量定义的说明;
* `sort`       - 排序，例如 `["a"=>1,"b"=>-1]` 即 按照 `a` 字段正序后，再按照 `b` 字段逆序；
* `skip`       - 结果集起始跳过；
* `count`      - 限制结果集数量；
* `projection` - 仅返回特定字段，例如 `["a"=>1, "b"=>1, "c"=>0]`  即 包含 `a` | `b` 字段，去除 `c` 字段；
* `batchSize`  - 每次从服务端获取的数据量;

#### `yield collection::find_one(array $query[, array $options])`
查询当前集合，返回单条文档记录（关联数组）；选项 $options 与 `find_many` 相同；

#### `yield collection::aggregate(array $pipeline[, array $options])`
在当前集合上进行聚合操作并返回数据结果游标指针（同 find_many）；目前 $options 选项可用值如下:
* `readPreference` - 读取倾向, 参见上面常量定义的说明;
* `batchSize` - 每次从服务端获取的数据量;

### `class flame\db\mongodb\reply`
上述 `collection` 更新型成员函数的返回值；不同的动作结果属性会有不同的值，存在的属性如下：

* `integer reply::$inserted_count` 新增数量；（插入）
* `integer reply::$deleted_count` 删除数量；（删除）
* `integer reply::$modified_count` 修改数量；（更新）
* `integer reply::$matched_count` 匹配数量；（更新）
* `object  reply::$upserted_id` 新文档ID；（更新插入）

**注意**：
* 如果需要在插入时获得新增文档的 ID 请直接在客户端生成 `object_id` 并传递 `_id` 字段；

#### `array reply::$write_errors` 或 `array reply::$write_concern_errors`
当服务端发生错误时，将返回上述数组描述错误信息；

#### `boolean reply::has_error()`
若存在 `writeErrors` 或 `writeConcernErrors` 时返回 true 否则返回 false；

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
遍历底层指针，返回结果集中的所有文档（关联数组）组成的数组；

**注意**：
* 如可能，请不要混合使用 `next()` 和 `to_array()` 两种读取方式（某些高并发奇拿况下可能导致数据问题）；
