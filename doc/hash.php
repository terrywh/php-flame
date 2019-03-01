<?php
/**
 * 补充部分在框架中存在的哈希算法函数
 */
namespace flame\hash;
/**
 * 默认情况 kafka 内部使用的哈希算法（返回 )
 * @return mixed 当 $raw = true 时返回 uint32 整数，否则返回对应的定长 HEX 串；
 */
function murmur2(string $data, bool $raw = false): mixed {}
/**
 * 一种效率极高且哈希结果理想度高的哈希算法（返回 uint64)
 * @param int $seed 用于对哈希结果进行预期内的调整
 * @return mixed 当 $raw = true 时返回 uint32 整数，否则返回对应的定长 HEX 串；
 */
function xxh64(string $data, int $seed = 0, bool $raw = false): mixed {}
/**
 * 根据 EMCA 定义实现的 crc64 哈希（与 Golang 对应 hash/crc64 ECMA 计算值一直）
 * @return mixed 当 $raw = true 时返回 uint32 整数，否则返回对应的定长 HEX 串；
 */
function crc64(string $data, bool $raw = false): mixed {}
