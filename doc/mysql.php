<?php
/**
 * 提供基本的 MySQL 客户端封装，内部使用连接池
 * 注意:
 * 1. 同一协程中连续进行 MySQL 查询操作, 且结果集数据不读取且不销毁(或不读取完且不销毁)可能导致进程死锁; (请将结果集读取完 或 主动 unset 释放结果集对象)
 * 2. MySQL 读取数据时数值型字段读取映射为 PHP 对应数值类型；`DATETIME` 映射为 PHP 内置类型 `DateTime`;
 * 3. 客户端提供的简化方法如 `insert()` `update()` 也同时支持与上述反向的映射;
 * 4. 单客户端内连接池上限大小为 6（单进程），即在并行访问超过 6 时须排队等待；
 */
namespace flame\mysql;
/**
 * 建立（连接）到 MySQL 服务器的客户端对象
 * @param string $url 服务端地址, 形如:
 *  mysql://{user}:{pass}@{host}:{port}/{database}?opt1=val1
 *  目前可用的选项如下:
 *      * "charset" => 字符集
 *      * "auth" => 认证方式，默认 "mysql_native_password" 可用 "caching_sha2_password"（MySQL 8.0 服务器默认） / "sha256_parssword"
 *      * "proxy" => 当前地址是否是代理服务（已提供连接复用机制） "1" - 是，已提供连接复用机制 (禁用框架提供的复用重置机制)， "0" - 否，未提供（默认）
 * 注意：若服务端配置默认字符集与上述字符集不同，且未指定 proxy=1 参数，那么：
 *      每次连接使用（复用）连接重置，字符集被恢复为服务器默认，框架将再次进行字符集设置（这里可能会产生少量额外消耗）；
 * 注意：若指定了 proxy 参数框架不再进行任何”连接复用“的清理工作（CONNECTION_RESET / CHANGE_USER / CHARSET_RESET)；
 * @return client 客户端对象
 */
function connect($url): client {
    return new client();
}

/**
 * WHERE 字句语法规则:
 *
 * @example
 *  $where = ["a"=>"1", "b"=>[2,"3","4"], "c"=>null, "d"=>["{!=}"=>5]];
 *  // " WHERE (`a`='1' AND `b`  IN ('2','3','4') AND `c` IS NULL AND `d`!='5')"
 * @example
 *  $where = ["{OR}"=>["a"=>["{!=}"=>1], "b"=>["{><}"=>[1, 10, 3]], "c"=>["{~}"=>"aaa%"]]];
 *  // $sql == " WHERE (`a`!='1' OR `b` NOT BETWEEN 1 AND 10 OR `c` LIKE 'aaa%')"
 * @example
 *  $where = "`a`=1";
 *  // $sql == " WHERE `a`=1";
 *
 * @param 特殊的符号均以 "{}" 进行包裹, 存在以下可用项:
 *  * `{NOT}` / `{!}` - 逻辑非, 对逻辑子句取反, 生成形式: `NOT (.........)`;
 *  * `{OR}` / `{||}` - 逻辑或, 对逻辑子句进行逻辑或拼接, 生成形式: `... OR ... OR ...`;
 *  * `{AND}` / `{&&}` - 逻辑与, 对逻辑子句进行逻辑与拼接 (默认拼接方式), 生成形式: `... AND ... AND ...`;
 *  * `{!=}` - 不等, 生成形式: `...!='...'` / ` ... IS NOT NULL`;
 *  * `{>}` - 大于, 生成形式: `...>...`;
 *  * `{<}` - 小于, 生成形式: `...<...`;
 *  * `{>=}` - 大于等于, 生成形式: `...>=...`;
 *  * `{<=}` - 小于等于, 生成形式: `...<=...`;
 *  * `{<>}` - 区间内, 生成形式: `... BETWEEN ... AND ...`, 目标数组至少存在两个数值;
 *  * `{><}` - 区间外, 生成形式: `... NOT BETWEEN ... AND ...`, 目标数组至少存在两个数值;
 *  * `{~}` - 模糊正匹配, 生成形式: `... LIKE ...`;
 *  * `{!~}` - 模糊非匹配, 生成形式: `... NOT LIKE ...`;
 */

/**
 * ORDER 子句语法规则:
 * @example
 *  $order = "`a` ASC, `b` DESC";
 *  // $sql == " ORDER BY `a` ASC, `b` DESC");
 * @example
 *  $order = ["a"=>1, "b"=>-1, "c"=>true, "d"=>false, "e"=>"ASC", "f"=>"DESC"];
 *  // $sql == " ORDER BY `a` ASC, `b` DESC, `c` ASC, `d` DESC, `e` ASC, `f` DESC"
 *
 * @param
 *  * 当 value 位正数或 `true` 时, 生成 `ASC`; 否则生成 `DESC`;
 *  * 当 value 为文本时, 直接拼接;
 */

/**
 * LIMIT 子句语法规则:
 * @example
 *  $limit = 10;
 *  // $sql == " LIMIT 10"
 * @example
 *  $limit = [10,10];
 *  // $sql == " LIMIT 10, 10"
 * @example
 *  $limit = "20, 300";
 *  // $sql == " LIMIT 20, 300"
 *
 * @param
 *  * 当 $limit 值位文本或数值时, 直接拼接;
 *  * 当 $limit 为数组时, 拼接前两个值;
 */

/**
 * MySQL 客户端（内部使用连接池）
 */
class client {
    /**
     * 将数据进行转移，防止可能出现的 SQL 注入等问题；
     */
    function escape($value):string {
        return "escaped string";
    }
    /**
     * 返回事务对象 (绑定在一个连接上)
     */
    function begin_tx(): ?tx {
        return new tx();
    }
    /**
     * 执行制定的 SQL 查询, 并返回结果
     * @param string $sql
     * @return mixed SELECT 型语句返回结果集对象 `result`; 否则返回关联数组, 一般包含 affected_rows / insert_id 两项;
     */
    function query(string $sql): result {
        return new result();
    }
    /**
     * 返回最近一次执行的 SQL 语句
     * 注意：由于协程的“并行”（穿插）执行，本函数返回的语句可能与当前上下问代码不对应；
     */
    function last_query(): string {
        return "last executed sql";
    }
    /**
     * 向指定表插入一行或多行数据 (自动进行 ESCAPE 转义)
     * @param string $table 表名
     * @param array $data 待插入数据, 多行关联数组插入多行数据(以首行 KEY 做字段名); 普通关联数组插入一行数据;
     * @return array 关联数组, 一般包含 affected_rows / insert_id 两项;
     */
    function insert(string $table, array $data): result {
        return new result();
    }
    /**
     * 从指定表格删除匹配的数据
     * @param string $table 表名
     * @param mixed $where WHERE 子句, 请参考上文;（数组形式描述将自动进行 ESCAPE 转义)
     * @param mixed $order ORDER 子句, 请参考上文;（数组形式描述将自动进行 ESCAPE 转义)
     * @param mixed $limit LIMIT 子句, 请参考上文;（数组形式描述将自动进行 ESCAPE 转义)
     * @return array 关联数组, 一般包含 affected_rows / insert_id 两项;
     */
    function delete(string $table, $where, $order = null, $limit = null): result {
        return new result();
    }
    /**
     * 更新指定表
     * @param string $table 表名
     * @param mixed $where WHERE 子句, 请参考上文;
     * @param mixed $modify 当 $modify 为字符串时, 直接拼接 "UPDATE ... SET $modify", 否则按惯例数组进行 KEY = VAL 拼接;
     * @param mixed $order ORDER 子句, 请参考上文;
     * @param mixed $limit LIMIT 子句, 请参考上文;
     * @return array 关联数组, 一般包含 affected_rows / insert_id 两项;
     */
    function update(string $table, $where, $modify, $order = null, $limit = null): result {
        return new result();
    }
    /**
     * 从指定表筛选获取数据
     * @param string $table 表名
     * @param mixed $fields 待选取字段, 为数组时其元素表示各个字段; 文本时直接拼接在 "SELECT $fields FROM $table";
     *  关联数组的 KEY 表示对应函数, 仅可用于简单函数调用, 例如:
     *      "SUM" => "a"
     *  表示:
     *      SUM(`a`)
     * @param mixed $where WHERE 子句, 请参考上文;
     * @param mixed $order ORDER 子句, 请参考上文;
     * @param mixed $limit LIMIT 子句, 请参考上文;
     */
    function select(string $table, $fields, $where, $order = null, $limit = null): result {
        return new result();
    }
    /**
     * 从指定表筛选获取一条数据, 并立即返回该行数据
     */
    function one(string $table, $where, $order = null): array {
        return [];
    }
    /**
     * 从指定表获取一行数据的指定字段值
     * @return mixed 字段类型映射请参见顶部说明
     */
    function get(string $table, string $field, array $where, $order) {
        return null;
    }
    /**
     * 获取服务端版本
     */
    function server_version():string {
        return "5.5.5";
    }
};

/**
 * 事务对象, 与 client 对象接口基本相同
 */
class tx {
    /**
     * 提交当前事务
     */
    function commit() {}
    /**
     * 回滚当前事务
     */
    function rollback() {}
    /**
     * @see client::query()
     * @return mixed cursor/array
     */
    function query(string $sql) {}
    /**
     * @see client::insert()
     */
    function insert(string $table, array $data): array {
        return [];
    }
    /**
     * @see client::delete()
     */
    function delete(string $table, $where, $order, $limit): array {
        return [];
    }
    /**
     * @see client::update()
     */
    function update(string $table, $where, $modify, $order = null, $limit = null): array {
        return [];
    }
    /**
     * @see client::select()
     */
    function select(string $table, $fields, $where, $order = null, $limit = null): result {
        return new result();
    }
    /**
     * @see client::one()
     */
    function one(string $table, $where, $order = null): array {
        return [];
    }
    /**
     * @see client::get()
     * @return mixed 字段类型映射参见顶部说明；
     */
    function get(string $table, string $field, $where, $order = null) {
        return null;
    }
};
/**
 * 结果集
 */
class result {
    /**
     * @var 结果集包含的记录行数
     */
    public $fetched_rows;
    /**
     * 读取下一行
     * @return 下一行数据关联数组;
     *  若读取已完成, 返回 NULL
     */
    function fetch_row():?array {
        return [];
    }
    /**
     * 读取 (剩余) 全部行
     * @return 二维数组, 可能为空数组;
     *  若读取已完成, 返回 NULL
     */
    function fetch_all():?array {
        return [];
    }
};
