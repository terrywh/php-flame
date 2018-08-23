### `namespace flame\mysql`
提供基本的异步 mysql 协程十客户端；

#### `yield flame\mysql\connect(string $url) -> flame\mysql\client`
连接 MySQL 数据库, 并返回数据库客户端对象; 连接地址 `$url` 形式如下:

```
mysql://{user}:{pass}@{host}:{port}/{database}
```

### `class flame\mysql\client`
MySQL 客户端（连接）；

#### `string client::escape(mixed $value[, string quote="'"])`
进行数据防注入转义，可选的指定对应包裹符号，目前允许：单引号 `'` 及 反单引号 ``` ；

#### `string client::where(mixed $conditions)`
用于(测试)生成 `WHERE` 子句 `SQL`, 支持以 文本 或 数组 形式描述条件语法, 下述简化接口中 `$where` 参数均以此函数进行条件拼装生成:

**示例**:
``` PHP
// $sql == " WHERE (`a`='1' && `b`  IN ('2','3','4') && `c` IS NULL && `d`!='5')"
$sql = $client->where(["a"=>"1", "b"=>[2,"3","4"], "c"=>null, "d"=>["{!=}"=>5]]);
// $sql == " WHERE (`a`!='1' || `b` NOT BETWEEN 1 AND 10 || `c` LIKE 'aaa%')"
$sql = $client->where(["{OR}"=>["a"=>["{!=}"=>1], "b"=>["{><}"=>[1, 10, 3]], "c"=>["{~}"=>"aaa%"]]]);
```

特殊的符号均以 "{}" 进行包裹, 存在以下可用项:
* `{NOT}` / `{!}` - 逻辑非, 对逻辑子句取反, 生成形式: `NOT (.........)`;
* `{OR}` / `{||}` - 逻辑或, 对逻辑子句进行逻辑或拼接, 生成形式: `... || ... || ...`;
* `{AND}` / `{||}` - 逻辑与, 对逻辑子句进行逻辑与拼接 (默认拼接方式), 生成形式: `... && ... && ...`;
* `{!=}` - 不等, 生成形式: `...!='...'` / ` ... IS NOT NULL`;
* `{>}` - 大于, 生成形式: `...>...`;
* `{<}` - 小于, 生成形式: `...<...`;
* `{>=}` - 大于等于, 生成形式: `...>=...`;
* `{<=}` - 小于等于, 生成形式: `...<=...`;
* `{<>}` - 区间内, 生成形式: `... BETWEEN ... AND ...`, 目标数组至少存在两个数值;
* `{><}` - 区间外, 生成形式: `... NOT BETWEEN ... AND ...`, 目标数组至少存在两个数值;
* `{~}` - 模糊正匹配, 生成形式: `... LIKE ...`;
* `{!~}` - 模糊非匹配, 生成形式: `... NOT LIKE ...`;

#### `string client::order(mixed $orders)`
用于(测试)生成 `ORDER BY` 子句 `SQL`, 支持以 文本 或 数组 形式描述排序语法, 下述简化接口中 `$order` 参数均以此函数进行排序拼装生成:

**示例**:
``` PHP
// $sql == " ORDER BY `a` ASC, `b` DESC");
$sql = $client->order("`a` ASC, `b` DESC");
// $sql == " ORDER BY `a` ASC, `b` DESC, `c` ASC, `d` DESC, `e` ASC, `f` DESC"
$sql = $client->order(["a"=>1, "b"=>-1, "c"=>true, "d"=>false, "e"=>"ASC", "f"=>"DESC"]);
```
可用形式存在以下几种:
* 文本形式, 直接拼接 `ORDER BY ...`;
* 数组 "字段 => 数值/布尔" 形式, 对应字段值为 正数/`TRUE` 时, 生成形式: `ORDER BY ... ASC, ... ASC`, 否则生成 `ORDER BY ... DESC`;
* 数组 "字段 => 文本" 形式, 生成形式: `ORDER BY {KEY} {VAL}, {KEY} {VAL}, ...`;

#### `string client::limit(mixed $limits)`
用于(测试)生成 `LIMIT` 子句 `SQL`, 支持以 文本 数值 或 数组 形式描述限制语法, 下述简化接口中 `$limit` 参数均以此函数进行限制拼装生成:

**示例**:
``` PHP
// $sql == " LIMIT 10"
$sql = $client->limit(10);
// $sql == " LIMIT 10, 10"
$sql = $client->limit([10,10]);
// $sql == " LIMIT 20, 300"
$sql = $client->limit("20, 300");
```
可用形式存在以下几种:
* 文本形式/数值形式, 直接拼接 `LIMIT ...`;
* 数组形式, 拼接 `LIMIT {VAL_0}, {VAL_1}`;

#### `yield client::query(string $sql) -> flame\mysql\result`
进行“查询”操作并返回对应的 `result` 结果对象；

**示例**:
``` PHP
$rs = $cli->query("SELECT * FROM `test`");
$aa = "aaaa";
```

#### `yield client::insert(string $table, array $data) -> flame/mysql/result`
插入一行或多行数据，对应 `INSERT INTO ....`：
* 当 `$data` 为关联数组时，插入一行记录，`array_keys($data)` 作为插入字段；
* 当 `$data` 为多行关联数组时，插入多行记录，`array_keys($data[0])` 作为插入字段；

#### `yield client::delete(string $table, array $where[, mixed $order[, mixed $limit]]) -> flame/mysql/result`
删除匹配条件行；

**注意**：
* 当需要指定 `$limit` 但无 `$order` 可在 `$sort` 字段填写 `NULL`；

#### `yield client::update(string $table, array $where, mixed $modify[, mixed $order[, mixed $limit]]) -> flame/mysql/result`
更新匹配条件的所有行，并应用由 `$modify` 描述的更改：

* 当 `$modify` 为字符串时，将拼接形如 `UPDATE ``{$table}`` SET {$modify}` 语句；
* 当 `$modify` 为数组串时，将拼接形如 `UPDATE ``{$table}`` SET ``{$key}``='{$val}', ...` 语句 `foreach($modify as $key=>$val)`；

**注意**：
* 当需要指定 `$limit` 但无 `$order` 请在 `$order` 字段填写 `NULL`；

#### `yield client::select(string $table, mixed $fields[, array $where[, mixed $order[, mixed $limit]]]) -> flame/mysql/result`
执行查询并返回结果集 result_set 对象；其中 `$fields` 参数可以使用下述两种形式：
``` PHP
<?php
	// 1. 文本形式直接指定
	$fields = '*';
	$fields = '`a`, ABS(`b`), RAND()';
	// 2. 使用数组
	$fields = ['a','b']; // 注意：不能填入 SQL 函数
```

**注意**：
* 如需要在字段名称上使用 SQL 函数，请使用文本形式自行指定 `$fields` 参数；数组形式不支持此种语法；
* 当需要指定 `$order` 但无 `$where` 请在 `$where` 字段填写 `NULL` 或 `1`；
* 当需要制定 `$limit` 但无 `$order` 请在 `$order` 字段填写 `NULL`；

#### `yield client::one(string $table, array $where[, mixed $order]) -> array/null`
执行查询并限定结果集大小为1，若找到匹配数据，直接返回数据（关联数组）；若未找到，返回 null；

#### `yield client::get(string $table, string $field, array $where[, mixed $order]) -> string`
执行查询并限定结果集大小为1，若找到匹配数据，直接返回数据对应字段值；若未找到，返回 null；

### `class flame\db\mysql\result`
数据查询结果对象, 用于读取“结果集”数据项 或 获取更新型语句执行结果；

**示例**：
``` PHP
<?php
// ... 通过 client 获得 result_set 对象 $rs
$rows = yield $rs->fetch_all(); // 获得所有数据对应的关联数组
while($row = yield $rs->fetch_row()) { // 依次获取每行数据关联数组
	var_dump($row);
}
```

#### `yield result::fetch_all() -> array/null`
获取结果集中的所有数据，返回二位数组，其元素为每行数据的关联数组；若不存在结果集, 则返回 `NULL`;

#### `yield result::fetch_row() -> array/null`
获取结果集中的下一数据，返回该行数据的关联数组；若不存在结果集, 则返回 `NULL`;

#### `result::$affected_rows`
#### `result::$insert_id`
