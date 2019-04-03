<?php
/**
 * 补充部分在框架中存在的压缩算法
 */
namespace flame\compress;
/**
 * snappy 压缩（与 https://github.com/kjdev/php-ext-snappy 兼容）
 */
function snappy_compress(string $data):string {
    return "compressed data";
}
/**
 * snappy 解压（与 https://github.com/kjdev/php-ext-snappy 兼容）
 */
function snappy_uncompress(string $data):string {
    return "original string";
}
