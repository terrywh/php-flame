<?php

namespace flame\http;

function exec(client_request $req): client_response {}
function get(string $url, int $timeout = 3000): client_response {}
function post(string $url, mixed $data, int $timeout = 3000): client_response {}
function put(string $url, mixed $data, int $timeout = 3000): client_response {}
function delete(string $url, int $timeout = 3000): client_response {}

class client_request {
    /**
     * @property int
     */
    public $timeout;
    /**
     * @property string
     */
    public $method;
    /**
     * @property string
     */
    public $url;
    /**
     * @property array
     */
    public $header;
    /**
     * @property array
     */
    public $cookie;
    /**
     * @property mixed
     */
    public $body;
    /**
     * 构建请求, 可选的指定请求体 `$body` (将自动设置为 POST 请求方法)及超时时间 `$timeout`
     */
    function __construct(string $url, mixed $body = null, int $timeout = 3000) {}
}

class client_response {
    /**
     * 响应码
     * @property int
     */
    public $status;
    /**
     * 注意: 目前使用了数组形式, 切暂不支持多个同名 Header 数据的访问;
     * @property array
     */
    public $header;
    /**
     * @property mixed 以下几种 Content-Type 将自动进行解析并生成关联数组:
     *  * "application/json"
     *  * "application/x-www-form-urlencoded"
     *  * "multipart/form-data"
     * 其他类型保持文本的原始数据(与 $rawBody 相同);
     */
    public $body;
    /**
     * 原始请求体数据
     * @property string
     */
    public $rawBody;
}