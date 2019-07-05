<?php

/**
 * 日志模块, 可通过框架配置设置日志等级以过滤日志输出;
 * @see flame\init()
 */
namespace flame\log;

/**
 * 创建（连接到 IPC 通道）一个新的日志记录器（记录到指定文件）
 */
function connect(string $filepath): logger {
    return new logger();
}
/**
 * 记录 无前缀（无时间戳 、无警告级别）的日志数据
 */
function write($x/*, $y, $z  ... */) {}
/**
 * 记录 TRACE 级别日志, 自动进行 JSON 形式的序列化;
 */
function trace($x/*, $y, $z  ... */) {}
/**
 * 记录 DEBUG 级别日志, 自动进行 JSON 形式的序列化;
 */
function debug($x/*, $y, $z  ... */) {}
/**
 * 记录 INFO 级别日志, 自动进行 JSON 形式的序列化;
 */
function info($x/*, $y, $z  ... */) {}
/**
 * 记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
 */
function warn($x/*, $y, $z  ... */) {}
/**
 * 记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
 */
function warning($x/*, $y, $z  ... */) {}
/**
 * 记录 ERROR 级别日志, 自动进行 JSON 形式的序列化;
 */
function error($x/*, $y, $z  ... */) {}
/**
 * 同 fatal()
 * @see fatal()
 */
function fail($x/*, $y, $z  ... */) {}
/**
 * 记录 FATAL 级别日志, 自动进行 JSON 形式的序列化;
 */
function fatal($x/*, $y, $z  ... */) {}

class logger {
    /**
     * 禁止手动创建日志对象，请使用 flame\log\connect() 函数
     * @see flame\log\connect()
     */
    private function __construct() {}
    /**
     * 记录 无前缀（无时间戳 、无警告级别）的日志数据
     */
    function write($x/*, $y, $z  ... */) {}
    /**
     * 记录 TRACE 级别日志, 自动进行 JSON 形式的序列化;
     */
    function trace($x/*, $y, $z  ... */) {}
    /**
     * 记录 DEBUG 级别日志, 自动进行 JSON 形式的序列化;
     */
    function debug($x/*, $y, $z  ... */) {}
    /**
     * 记录 INFO 级别日志, 自动进行 JSON 形式的序列化;
     */
    function info($x/*, $y, $z  ... */) {}
    /**
     * 记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
     */
    function warn($x/*, $y, $z  ... */) {}
    /**
     * 记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
     */
    function warning($x/*, $y, $z  ... */) {}
    /**
     * 记录 ERROR 级别日志, 自动进行 JSON 形式的序列化;
     */
    function error($x/*, $y, $z  ... */) {}
    /**
     * 同 fatal()
     * @see fatal()
     */
    function fail($x/*, $y, $z  ... */) {}
    /**
     * 记录 FATAL 级别日志, 自动进行 JSON 形式的序列化;
     */
    function fatal($x/*, $y, $z  ... */) {}
};
