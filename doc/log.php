<?php
/**
 * 日志相关
 */
namespace flame\log;
/**
 * 对默认日志进行重载（目标文件被重新打开或创建）
 */
function reload() {}
/**
 * 用于生成一个 Structure Data 日志区段
 * @return object
 * @example 例如：
 * ```
 * flame\log\debug(flame\log\tagkv(["a"=>"aaaa"]), "b", ["c"=>"cccc"]);
 */
function tagkv($tags) {}
/**
 * 在默认日志中，记录 DEBUG 级别日志, 自动进行 JSON 形式的序列化;
 */
function debug($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 INFO 级别日志, 自动进行 JSON 形式的序列化;
 */
function info($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 NOTICE 级别日志, 自动进行 JSON 形式的序列化;
 */
function notice($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
 */
function warning($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
 */
function warn($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 ERROR 级别日志, 自动进行 JSON 形式的序列化;
 */
function error($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 CRITICAL 级别日志, 自动进行 JSON 形式的序列化;
 */
function critical($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 CRITICAL 级别日志, 自动进行 JSON 形式的序列化;
 */
function fatal($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 ALERT 级别日志, 自动进行 JSON 形式的序列化;
 */
function alert($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 EMERGENCY 级别日志, 自动进行 JSON 形式的序列化;
 */
function emergency($x/*, $y, $z  ... */) {}
/**
 * 在默认日志中，记录 EMERGENCY 级别日志, 自动进行 JSON 形式的序列化;
 */
function emerg($x/*, $y, $z  ... */) {}
/**
 * 日志，用于构建额外的日志记录
 */
class logger {
    /**
     * 对当前进行重载（目标文件被重新打开或创建）
     */
    function reload() {}
    /**
     * 在当前日志中，记录 DEBUG 级别日志, 自动进行 JSON 形式的序列化;
     */
    function debug($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 INFO 级别日志, 自动进行 JSON 形式的序列化;
     */
    function info($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 NOTICE 级别日志, 自动进行 JSON 形式的序列化;
     */
    function notice($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
     */
    function warning($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
     */
    function warn($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 ERROR 级别日志, 自动进行 JSON 形式的序列化;
     */
    function error($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 CRITICAL 级别日志, 自动进行 JSON 形式的序列化;
     */
    function critical($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 CRITICAL 级别日志, 自动进行 JSON 形式的序列化;
     */
    function fatal($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 ALERT 级别日志, 自动进行 JSON 形式的序列化;
     */
    function alert($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 EMERGENCY 级别日志, 自动进行 JSON 形式的序列化;
     */
    function emergency($x/*, $y, $z  ... */) {}
    /**
     * 在当前日志中，记录 EMERGENCY 级别日志, 自动进行 JSON 形式的序列化;
     */
    function emerg($x/*, $y, $z  ... */) {}
};
