<?php
/**
 * 提供对 TOML 格式文本、文件的解析；
 * TOML 格式定义请参考：https://github.com/toml-lang/toml
 */
namespace flame\toml;

/**
 * 解析 TOML 格式的字符串并返回关联数组
 * @param string $toml 文本 TOML 格式
 * 注意：若文本不完整或不正确会抛出异常；
 */
function parse_string(string $toml): array {
    return [];
}

/**
 * 解析指定 TOML 格式的数据文件，并返回关联数组
 * @param string $file 待解析文件路径
 */
function parse_file($file): array {
    return [];
}
