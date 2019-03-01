<?php
/**
 * 补充部分在框架中存在的编码功能
 */
namespace flame\encoding;
/**
 * 将 PHP 数组转换为 BSON 格式，并返回实际 BSON 的原始二进制数据
 */
function bson_encode(array $data):string {}
/**
 * 还原由上述 `bson_encode` 生成的原始二进制数据，返回对应的数组
 */
function bson_decode(string $data):array {}
