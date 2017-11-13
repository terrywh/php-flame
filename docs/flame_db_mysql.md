## `namespace flame\db\mysql`
提供基本的异步 mysql 协程十客户端；

### `class flame\db\mysql\client`
MySQL 客户端（连接）；

**示例**：
``` PHP
<?php
$cli = new flame\db\mysql\client();
$cli->connect("127.0.0.1", 3306, "username", "password", "database_name");
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
* `['a'=>['$between'=>[1234, 5678]]]`
``` SQL
`a` BETWEEN 1234 AND 5678
```

**注意**：
* 请尽量使用 `'` 单引号，防止功能符号被转义；

#### `yield client::delete_many(string $table, array $conditions)`
删除匹配条件的所有行；
#### `yield client::delete_one(string $table, array $conditions[, array $order])`
删除匹配条件的第一行；`$order` 参数设置请参考 `select_many()` 函数说明；
#### `yield client::update_many(string $table, array $conditions, array $modify)`
更新匹配条件的所有行，并应用由 `$modify` 描述的更改；
#### `yield client::update_one(string $table, array $conditions, array $modify[, array $order])`
更新匹配条件的第一行，并应用由 `$modify` 描述的更改；`$order` 参数设置请参考 `select_many()` 函数说明；
#### `yield client::select_many(string $table, array $conditions[, array $order[, integer $offset[, integer $limit[, string|array $fields]]]])`
执行查询并返回结果集 result_set 对象；
* `$order` 参数用于描述排序信息，可以使用如下形式：
``` PHP
<?php
	// ORDER BY `a` ASC, `b` DESC
	$order = ["a"=>1,"b"=>-1];
```
* `$fields` 参数可以使用下述两种形式：
``` PHP
<?php
	// 文本形式直接指定
	$fields = '*';
	$fields = 'SQL_CALC_FOUND_ROWS `a`, ABS(`b`), RAND()';
	// 使用数组
	$fields = ['a','b']; // 不能填入 SQL 函数
```
**注意**：
* 如需要在字段名称上使用 SQL 函数，请使用文本形式自行指定 $fields 参数；数组形式不支持此种语法；

#### `yield client::select_one(string $table, array $conditions)`
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
while($row = yield $rs->fetch_row()) { // 依次获取每行数据关联数组
	var_dump($row);
}
```

#### `yield result_set::fetch_all()`
获取结果集中的所有数据，返回二位数组，其元素为每行数据的关联数组；
#### `yield result_set::fetch_row()`
获取结果集中的下一数据，返回该行数据的关联数组；

### `class flame\db\mysql\result_info`
#### `result_info::__toBool()`
可用于判定结果是否为“成功”；
#### `yield result_info::affect_rows()`
用于获取执行影响行数量；
#### `yield result_info::last_insert_id()`
获取由 INSERT 或 UPDATE 语句写入数据生成的 AUTO_INCREMENT 字段对应数据；
