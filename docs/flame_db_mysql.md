## `namespace flame\db\mysql`
提供基本的异步 mysql 协程十客户端；

### `class flame\db\mysql\client`
MySQL 客户端（连接）；

**示例**：
``` PHP
// ...
$cli = new flame\db\mysql\client();
yield $cli->connect(mysql://username:password@127.0.0.1:3306/database_name");
```

#### `client::__construct([array $options])`
可用选项如下：
* `debug` - 调试开关，输出实际执行的 SQL 语句，默认 `false`；
* `ping`  - 周期性执行 `SELECT 1` 语句，以此保持连接活跃，默认 `60000` ms；

#### `yield client::connect(string $url)`
连接 MySQL 服务器，`$url` 形式如下：

```
mysql://{username}:{password}@{host}:{port}/{database}
```

#### `client::$affected_rows`
更新型 SQL 语句影响到的行数

#### `client::$insert_id`
单条插入语句执行后 插入行生成的 自增 ID（若存在）

#### `yield client::query(string $sql) | yield client::query(string $format[, mixed $arg1, mixed $arg2, ...])`
进行“查询”操作并返回对应的 `result_set` 结果集对象 或 “更新”操作返回 `result_info` 执行结果；

后一种函数原型，允许进行“格式化”参数替换即：将 $arg1 / $arg2 等参数替换到 $format 定义的格式化字符串中，并自动进行转义和包裹；例如：

``` PHP
$rs = $cli->query("SELECT * FROM `test`");
$aa = "aaaa";
$bb = 123456;
$rs = $cli->query("SELECT * FROM `test` WHERE `a`=? AND `b`=?", $aa, $bb); // SELECT * FROM `test` WHERE `a`='aaaa' AND `b`=123456
$aa = "aaa'aa"
$rs = $cli->query("DELETE FROM `test` WHERE `a`=?", $aa); // DELETE FROM `test` WHERE `a`='aaa\'aa';
```

**注意**：
* 后一种函数原型支持的包裹转义流程不支持二进制数据，请使用 `Base64` 等编码手段预先处理；

#### `yield client::insert(string $table, array $data)`
插入一行或多行数据，对应 `INSERT INTO ....`：
* 当 $data 为关联数组时，插入一行记录，`array_keys($data)` 作为插入字段；
* 当 $data 为多行关联数组时，插入多行记录，`array_keys($data[0])` 作为插入字段；

#### 条件描述 `$conditions`
下述 `delete` / `update` / `select` / `one` 相关函数使用的条件匹配均为如下描述的形式；形式参照了 MongoDB 查询运算符，并定义了一些 SQL 特例的实现：

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

**注意**：
* 请尽量使用 `'` 单引号，防止功能符号被转义；
#### 排序描述 `$sort`
对应 `ORDER BY ...`，存在如下情况：
1. 当 `$sort` 为字符串时，直接拼接，例如：`a ASC` => `ORDER BY a ASC`；
2. 当 `$sort` 为关联数组时，对应项 VAL `>0` 时，表示对应 KEY 升序 `ASC`；否则 降序排列 `DESC`；例如： `["a" => 1, "b" => -1]` => `ORDER BY ``a`` ASC, ``b`` DESC`；

#### 限制描述 `$limit`
对应 `LIMIT ...`，存在如下情况：
1. 当 `$limit` 为字符串时，直接拼接，例如：`"10,20"` => `LIMIT 10,20`；
2. 当 `$limit` 为数值时，直接拼接，例如 `10` -> `LIMIT 10`；
3. 当 `$limit` 为数组时，其首项 和 次项 分别作为 LIMIT 的首个和次个参数，例如：`[10, 20]` => `LIMIT 10, 20`;

#### `yield client::delete(string $table, array $conditions[, mixed $sort[, mixed $limit]])`
删除匹配条件的所有行；

**注意**：
* 当需要指定 `$limit` 但无 `$sort` 请在 `$sort` 字段填写 `NULL`；

#### `yield client::update(string $table, array $conditions, array $modify[, mixed $sort[, mixed $limit]])`
更新匹配条件的所有行，并应用由 `$modify` 描述的更改；

**注意**：
* 当需要指定 `$limit` 但无 `$sort` 请在 `$sort` 字段填写 `NULL`；

#### `yield client::select(string $table, mixed $fields[, array $conditions[, mixed $sort[, mixed $limit]]])`
执行查询并返回结果集 result_set 对象；其中 `$fields` 参数可以使用下述两种形式：
``` PHP
<?php
	// 1. 文本形式直接指定
	$fields = '*';
	$fields = 'SQL_CALC_FOUND_ROWS `a`, ABS(`b`), RAND()';
	// 2. 使用数组
	$fields = ['a','b']; // 注意：不能填入 SQL 函数
```

**注意**：
* 如需要在字段名称上使用 SQL 函数，请使用文本形式自行指定 `$fields` 参数；数组形式不支持此种语法；
* 当需要指定 `$sort` 但无 `$conditions` 请在 `$conditions` 字段填写 `NULL`；`$limit` 与 `$sort` 类似；

#### `yield client::one(string $table, array $conditions[, mixed $sort])`
执行查询并限定结果集大小为1，若找到匹配数据，直接返回数据（关联数组）；若未找到，返回 null；

#### `yield client::found_rows()`
简化调用 `FOUND_ROWS()` SQL 函数的过程，直接返回行数量；

### `class flame\db\mysql\result_set`
数据查询结果集对象

**示例**：
``` PHP
<?php
// ... 通过 client 获得 result_set 对象 $rs
$rows = yield $rs->fetch_all(); // 获得所有数据对应的关联数组
while($row = yield $rs->fetch_row()) { // 依次获取每行数据关联数组
	var_dump($row);
}
```

#### `yield result_set::fetch_all([$type = flame\\db\\mysql\\FETCH_ASSOC])`
获取结果集中的所有数据，返回二位数组，其元素为每行数据的关联数组；
* `flame\db\mysql\FETCH_ASSOC` - 获取关联数组
* `flame\db\mysql\FETCH_ENUMS` - 获取下标数组

#### `yield result_set::fetch_row([$type = flame\\db\\mysql\\FETCH_ASSOC])`
获取结果集中的下一数据，返回该行数据的关联数组；
