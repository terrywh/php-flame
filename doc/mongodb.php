<?php
/**
 * MongoDB 基本客户端
 */
namespace flame\mongodb;

function connect(string $url):client {}

class client {
    function execute(array $command, bool $write = false):mixed {}
    function collection():collection {}
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
    function one(array $query, array $sort = null): array {}
    function get(array $query, string $field, array $sort = null): mixed {}
    function count(array $query): int {}
    function aggregate(array $pipeline): cursor {}
}
class cursor {
    function fetch_row() {}
    function fetch_all() {}
}
class date_time implements JsonSerializable {
    function __construct(int $milliseconds = null) {}
    function __toString() {}
    function __toDateTime() {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    function unix(): int {}
    function unix_ms(): int {}
}
class object_id implements JsonSerializable {
    function __construct(string $object_id = null) {}
    function __toString() {}
    function __toDateTime() {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    function unix(): int {}
    function equal(object_id $oid): bool {}
}
