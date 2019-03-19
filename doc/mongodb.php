<?php
/**
 * 提供基本的 MongoDB 基本客户端封装，使用连接池维护与服务器连接
 * 注意:
 * 1. 同一协程中连续进行 MongoDB 查询操作, 且游标不读取数据不销毁 (或 不读取完不销毁) 可能导致进程死锁; (请将游标读取完毕 或 主动 unset 释放游标对象)
 * 2. 对应 MongoDB ObjectID 映射类型 class flame\mongodb\object_id;
 * 3. 对应 MongoDB DateTime 映射类型 class flame\mongodb\date_time;
 * 4. 单客户端内连接池上限大小为 6（单进程）；
 */
namespace flame\mongodb;

/**
 * 连接 Mongodb 并返回客户端对象
 * @param string $url 连接地址, 形如:
 *  mongodb://{username}:{password}@{host1}:{port1},{host2}:{port2}/{database}?{option1}={rs1}&{option2}=...
 */
function connect(string $url):client {}
/**
 * MongoDB 客户端（内部使用连接池）
 */
class client {
    /**
     * 执行以 $command 描述的命令;
     * 请参考：https://docs.mongodb.com/master/reference/command/ 编写指令；
     * 注意: 更新型指令需要自行指定 $write = true
     */
    function execute(array $command, bool $write = false):mixed {}
    /**
     * 获取指定名称的 colleciton 对象
     */
    function collection($name):collection {}
    /**
     * 与 `collection()` 相同, 返回指定的 collection 对象
     */
    function __get($name):collection {}
    /**
     * @return bool 永远为 true ;
     */
    function __isset($name):bool {
        return true;
    }
}
class collection {
    /**
     * 执行插入
     * @param array $data 一维关联数组插入单个文档; 二维时插入多个；
     * @param bool $ordered 插入多个文档时，是否顺序执行（在遇到的第一个插入错误出停止）
     * @return array 返回插入结果，形如：
     *  array(
     *      "n" => 1,  // 插入数量
     *      "ok" => 1, // 插入结果
     *      ...
     *  )
     */
    function insert(array $data, bool $ordered = true):array {}
    function insert_many(array $data, bool $ordered = true):array {}
    function insert_one(array $data):array {}
    /**
     * 执行删除
     * @return array 形式同 insert() 类似：
     *  array(
     *      "n" => 1,  // 删除数量
     *      "ok" => 1, // 删除结果
     *      ...
     *  )
     */
    function delete(array $query, int $limit = 0):array {}
    function delete_many(array $query, int $limit = 0):array {}
    function delete_one(array $query):array {}
    /**
     * 执行更新动作
     * @param array $update 一般使用类似如下形式进行更新设置：
     *  ['$set'=>['a'=>'b'],'$inc'=>['c'=>2]]
     * @return array 形式同 insert() 类似：
     *  array(
     *      "n" => 1,  // 更新数量
     *      "ok" => 1, // 更新结果
     *      ...
     *  )
     */
    function update(array $query, array $update, $upsert = false):array {}
    function update_many(array $query, array $update, $upsert = false):array {}
    function update_one(array $query, array $update, $upsert = false):array {}
    /**
     * 执行指定查询，返回游标对象
     * @param array $sort 一般使用如下形式表达排序：
     *  ['a'=>1,'b'=>-1] // a 字段升序，b 字段降序
     * @param mixed $limit 可以为 int 设置 `limit` 值，或 array 设置 `[$skip, $limit]`
     */
    function find(array $query, array $projection = null, array $sort = null, mixed $limit = null):cursor {}
    function find_many(array $query, array $projection = null, array $sort = null, mixed $limit = null):cursor {}

    /**
     * 查询并返回单个文档
     * @param array $sort 一般使用如下形式表达排序：
     *  ['a'=>1,'b'=>-1] // a 字段升序，b 字段降序
     */
    function find_one(array $query, array $sort = null): array {}
    function one(array $query, array $sort = null): array {}
    /**
     * 查询并返回单个文档的指定字段值
     * @param array $sort 一般使用如下形式表达排序：
     *  ['a'=>1,'b'=>-1] // a 字段升序，b 字段降序
     */
    function get(array $query, string $field, array $sort = null): mixed {}
    /**
     * 查询并返回匹配文档数量
     */
    function count(array $query): int {}
    /**
     * 执行指定聚合操作，返回游标对象；
     * 请参考 https://docs.mongodb.com/master/reference/operator/aggregation-pipeline/ 编写；
     */
    function aggregate(array $pipeline): cursor {}
    /**
     * 查找满足条件的第一个文档，删除并将其返回
     */
    function find_and_delete(array $query, array $sort = null, $upsert = false, $fields = null) {}
    /**
     * 查找满足条件的第一个文档，对其进行更新，并返回
     * @param $new boolean 返回更新后的文档
     */
    function find_and_update(array $query, array $update, array $sort = null, $upsert = false, array $fields = null, $new = false) {}
}
/**
 * 游标对象
 */
class cursor {
    /**
     * 读取当前 cursor 返回一个文档; 若读取已完成, 返回 null;
     */
    function fetch_row():mixed {}
    /**
     * 读取当前 cursor 返回所有文档 (二维); 若读取已完成, 返回 null;
     */
    function fetch_all(): array {}
}
/**
 * 对应 MongoDB 日期时间字段，毫秒级精度
 */
class date_time implements \JsonSerializable {
    /**
     * 构建日期时间对象
     * @param int $milliseconds 指定毫秒级时间，默认 null 使用当前时间
     */
    function __construct(int $milliseconds = null) {}
    function __toString() {}
    function __toDateTime(): \DateTime {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    /**
     * 返回秒级时间戳
     */
    function unix(): int {}
    /**
     * 返回毫秒级时间戳
     */
    function unix_ms(): int {}
    /**
     * 返回标准的 YYYY-MM-DD hh:mm:ss 文本形式的时间
     */
    function iso(): string {}
}
/**
 * 对应 MongoDB 对象ID
 */
class object_id implements \JsonSerializable {
    /**
     * 构建
     * @param string $object_id 从文本形式构建（恢复）一个对象，默认 null，构建一个新的对象
     */
    function __construct(string $object_id = null) {}
    function __toString() {}
    function __toDateTime(): \DateTime {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    /**
     * 秒级时间戳
     */
    function unix(): int {}
    /**
     * 当前对象与另一 object_id 对象是否**值相等**
     */
    function equal(object_id $oid): bool {}
}
