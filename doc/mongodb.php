<?php
/**
 * MongoDB 基本客户端
 */
namespace flame\mongodb;

function connect(string $url):client {}

class client {
    function execute(array $command, boolean $write = false): mixed {}
    function collection(): collection {}
    function __get(): collection {}
    /**
     * @return boolean 永远为 true ;
     */
    function __isset(): boolean {}
}
class collection {
    function insert(array $data, boolean $ordered = true):array {}
    function delete(array $query, integer $limit = 0):array {}
    function update(array $query, array $update, $upsert = false):array {}
    function find(array $query, array $projection = null, array $sort = null, mixed $limit):cursor {}
    function one(array $query, array $sort = null): array {}
    function get(array $query, string $field, array $sort = null): mixed {}
    function count(array $query): integer {}
    function aggregate(array $pipeline): cursor {}
}
class cursor {
    function fetch_row() {}
    function fetch_all() {}
}
class date_time implements JsonSerializable {
    function __construct(integer $milliseconds = null) {}
    function __toString() {}
    function __toDateTime() {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    function unix(): integer {}
    function unix_ms(): integer {}
}
class object_id implements JsonSerializable {
    function __construct(string $object_id = null) {}
    function __toString() {}
    function __toDateTime() {}
    function __debugInfo() {}
    function jsonSerialize(): array {}
    function unix(): integer {}
    function equal(object_id $oid): boolean {}
}
