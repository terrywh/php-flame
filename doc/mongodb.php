<?php
/**
 * MongoDB 基本客户端
 */
namespace flame\mongodb;

/**
 * 连接 Mongodb 并返回客户端对象
 * @param string $url 连接地址, 形如:
 *  mongodb://{username}:{password}@{host1}:{port1},{host2}:{port2}/{database}?{option1}={rs1}&{option2}=...
 */
function connect(string $url):client {}

class client {
    /**
     * 执行以 $command 描述的命令;
     * 注意: 更新型指令需要自行指定 $write = true
     */
    function execute(array $command, bool $write = false):mixed {}
    function collection():collection {}
    /**
     * 与 collection() 相同, 返回指定的 collection 对象
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
    function insert(array $data, bool $ordered = true):array {}
    function delete(array $query, int $limit = 0):array {}
    function update(array $query, array $update, $upsert = false):array {}
    function find(array $query, array $projection = null, array $sort = null, mixed $limit):cursor {}
    /**
     * 查询并返回单个文档
     */
    function one(array $query, array $sort = null): array {}
    /**
     * 查询并返回单个文档的指定字段值
     */
    function get(array $query, string $field, array $sort = null): mixed {}
    function count(array $query): int {}
    function aggregate(array $pipeline): cursor {}
}
class cursor {
    /**
     * 读取当前 cursor 返回一个文档; 若返回已完成, 返回 null;
     */
    function fetch_row():mixed {}
    /**
     * 读取当前 cursor 返回所有文档 (二维)
     */
    function fetch_all(): array {}
}
class date_time implements \JsonSerializable {
    function __construct(int $milliseconds = null) {}
    function __toString() {}
    function __toDateTime(): \DateTime {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    function unix(): int {}
    function unix_ms(): int {}
}
class object_id implements \JsonSerializable {
    function __construct(string $object_id = null) {}
    function __toString() {}
    function __toDateTime(): \DateTime {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    function unix(): int {}
    function equal(object_id $oid): bool {}
}
