<?php
/**
 * 提供基本的 HTTP(S) 客户端、服务端封装；服务端仅支持 HTTP/1.1 协议; 客户端支持 (TLS) HTTP/1.x HTTP/2 协议;
 * 注意：
 * * 默认状态客户端对请求连接进行 KeepAlive 复用；若需要使用短连接，请自行制定 Connection: close 头；
 */
namespace flame\http;

/**
 * 使用默认客户端, 执行请求
 */
function exec(client_request $req): client_response {
    return new client_response();
}
/**
 * 使用默认客户端, 构建并执行一个 GET 请求
 */
function get(string $url, int $timeout = 3000): client_response {
    return new client_response();
}
/**
 * 使用默认客户端, 构建并执行一个 POST 请求
 */
function post(string $url, $data, int $timeout = 3000): client_response {
    return new client_response();
}
/**
 * 使用默认客户端, 构建并执行一个 PUT 请求
 */
function put(string $url, $data, int $timeout = 3000): client_response {
    return new client_response();
}
/**
 * 使用默认客户端, 构建并执行一个 DELETE 请求
 */
function delete(string $url, int $timeout = 3000): client_response {
    return new client_response();
}

/**
 * 客户端，用于执行请求
 */
class client {
    /**
     * 构建新的客户端对象
     * @param array $options 选项, 目前支持的选项如下:
     *  * "connection_per_host" - 单个 HOST 目标连接限制, 默认 16;
     */
    function __construct(array $options = []) {}
    /**
     * 执行请求, 返回响应
     */
    function exec(client_request $req): client_response {
        return new client_response();
    }
    /**
     * 构建并执行一个 GET 请求
     */
    function get(string $url, int $timeout = 3000): client_response {
        return new client_response();
    }
    /**
     * 构建并执行一个 POST 请求
     */
    function post(string $url, $data, int $timeout = 3000): client_response {
        return new client_response();
    }
    /**
     * 构建并执行一个 PUT 请求
     */
    function put(string $url, $data, int $timeout = 3000): client_response {
        return new client_response();
    }
    /**
     * 构建并执行一个 DELETE 请求
     */
    function delete(string $url, int $timeout = 3000): client_response {
        return new client_response();
    }
}
/**
 * 客户端请求对象，用于拼装生成一个客户端请求（待执行）
 */
class client_request {
    /**
     * 由框架自行选择版本
     */
    const HTTP_VERSION_NONE = 0;
    /**
     * 强制使用 HTTP/1.0 协议
     */
    const HTTP_VERSION_1_0   = 1;
    /**
     * 强制使用 HTTP/1.1 协议
     */
    const HTTP_VERSION_1_1   = 2;
    /**
     * 尝试使用 HTTP/2 协议，自动退回 HTTP/1.1 版本
     */
    const HTTP_VERSION_2     = 3;
    /**
     * 尝试在 TLS (HTTPS) 使用 HTTP/2 协议，自动退化 HTTP/1.1 版本
     */
    const HTTP_VERSION_2_TLS = 4;
    /**
     * 强制在 non-TLS 状态进行 HTTP/2 请求，TLS (HTTPS) 时同 HTTP_VERSION_2_TLS
     */
    const HTTP_VERSION_2_PRI = 5;

    const SSL_VERIFY_NONE    = 0;
    /**
     * 验证对端证书
     */
    const SSL_VERIFY_PEER    = 1;
    /**
     * 验证证书域名
     */
    const SSL_VERIFY_HOST    = 2;
    /*
     * 验证证书状态
     */
    const SSL_VERIFY_STATUS  = 4;
    /**
     * 相当于 SSL_VERIFY_PEER | SSL_VERIFY_HOST | SSL_VERIFY_STATUS
     */
    const SSL_VERIFY_ALL     = 7;
    /**
     * @var int
     */
    public $timeout;
    /**
     * @var string
     */
    public $method;
    /**
     * @var string
     */
    public $url;
    /**
     * 请求头；可通过指定 "Connection" => "close" 防止连接复用；
     * @var array
     */
    public $header;
    /**
     * @var array
     */
    public $cookie;
    /**
     * @var mixed
     */
    public $body;
    /**
     * 构建请求, 可选的指定请求体 `$body` (将自动设置为 POST 请求方法)及超时时间 `$timeout`
     */
    function __construct(string $url, $body = null, int $timeout = 3000) {}
    /**
     * @param string $cert_file 证书文件路径
     * @param string $pkey_file 密钥文件路径
     * @param string $pkey_pass 密钥密码
     */
    function ssl_pem(string $cert_file, string $pkey_file = "", string $pkey_pass = "") {}
    /**
     * 请使用 client_request::SSL_VERIFY_* 相关常量进行设置（可按位组合）
     */
    function ssl_verify(int $verify) {}
    /**
     * 请选择使用 client_request::HTTP_VERSION_* 常量设置版本
     */
    function http_version(int $version) {}
}
/**
 * 客户端响应对象，执行请求后得到
 */
class client_response {
    /**
     * 对应请求实际的 HTTP 版本
     * @var string
     */
    public $version;
    /**
     * 响应码
     * @var int
     */
    public $status;
    /**
     * 注意: 目前使用了数组形式, 切暂不支持多个同名 Header 数据的访问;
     * @var array
     */
    public $header;
    /**
     * @var mixed 以下几种 Content-Type 将自动进行解析并生成关联数组:
     *  * "application/json"
     *  * "application/x-www-form-urlencoded"
     *  * "multipart/form-data"
     * 其他类型保持文本的原始数据(与 $raw_body 相同);
     */
    public $body;
    /**
     * 原始请求体数据
     * @var string
     */
    public $raw_body;
}
/**
 * HTTP 服务器
 */
class server {
    /**
     * 服务端本地监听地址
     */
    public $address;
    /**
     * 创建服务器并绑定地址
     */
    function __construct(string $address) {}
    /**
     * 每次 HTTP 请求在执行对应 path 的处理器前, 先执行下述设置的回调;
     * 回调形式如下:
     *  function callback(server_request $req, server_response $res, bool $match) {}
     * 其中 $match 参数标识此请求是否匹配了定义的 path 处理器;
     * 当 callback 函数返回 false 时, 将**不再**执行后续处理器
     */
    function before(callable $cb):server {
        return $this;
    }
    /**
     * 每次 HTTP 请求在执行对应 path 处理器后, 会执行下述设置的回调;
     * 回调形式同 before();
     * 注意: 若 before 回调或 path 回调返回了 false, 此 after 回调将不再执行;
     */
    function after(callable $cb):server {
        return $this;
    }
    /**
     * 设置一个处理 "PUT $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function put(string $path, callable $cb):server {
        return $this;
    }
    /**
     * 设置一个处理 "DELETE $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function delete(string $path, callable $cb):server {
        return $this;
    }
    /**
     * 设置一个处理 "POST $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function post(string $path, callable $cb):server {
        return $this;
    }
    /**
     * 设置一个处理 "PATCH $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function patch(string $path, callable $cb):server {
        return $this;
    }
    /**
     * 设置一个处理 "GET $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function get(string $path, callable $cb):server {
        return $this;
    }
    /**
     * 设置一个处理 "HEAD $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function head(string $path, callable $cb):server {
        return $this;
    }
    /**
     * 设置一个处理 "OPTIONS $path HTTP/1.1" 请求的路径 path 处理器;
     * 注意: 当 before 回调返回 false 时, 此处理器将不再被执行;
     */
    function options(string $path, callable $cb):server {
        return $this;
    }
    /**
     * 启动服务器, 监听请求并执行对应回调
     * 注意: 运行服务器将阻塞当前协程;
     */
    function run() {}
    /**
     * 停止服务器, 不再接收新的连接; 上述 run() 阻塞协程将恢复执行;
     * 注意: 正在处理中的请求不收影响(继续处理);
     */
    function close() {}
}
/**
 * 服务端请求对象（由 server 对应回调获得）
 */
class server_request {
    /**
     * 请求方法
     * @var string
     */
    public $method = "GET";
    /**
     * 请求路径
     * @var string
     */
    public $path = "/";
    /**
     * 请求参数, 使用 PHP 内置 parse_str() 函数解析得到;
     * @var array
     */
    public $query;
    /**
     * 请求头; 所有头信息字段名被转换为**小写**形式;
     * 注意：暂不支持多项同名 Header 字段;
     * @var array
     */
    public $header;
    /**
     * 请求 Cookie
     * @var array
     */
    public $cookie;
    /**
     * 请求体, 以下几种 Content-Type 时自动解析为关联数组:
     *  * "application/json"
     *  * "application/x-www-form-urlencoded"
     *  * "multipart/form-data"
     * 其他类型时与 $raw_body 相同;
     * @var mixed
     */
    public $body;
    /**
     * 原始请求体
     * @var string
     */
    public $raw_body;
    /**
     * 上传文件，形如：
     *  [
     *      "file_field_1"=>
     *      [
     *          "content-type" => "mime_type",
     *          ... // other header field lowercased if available
     *          "filename" => "original_file_name",
     *          "size" => file_data_size,
     *          "data" => "file_data", // content of the uploaded file
     *      ],
     *      "file_filed_2"=>
     *      ...
     *  ]
     * 若无上传文件时可能为空
     * @var array
     */
    public $file;
    /**
     * 用户可设置的请求关联数据;
     * 例如: 在 before 回调中从 Cookie 中提取用户信息放在 $req->data["user"] 中, 后续处理器即可访问 $req->data["user"] 获取户信息;
     * @var array
     */
    public $data;
}

/**
 * 服务端响应对象（由 server 对应处理回调获得）提供接口向客户端返回数据
 */
class server_response {
    /**
     * 响应状态码, 可以直接设置; 也可以在 Transfer-Encoding: chunked 模式时使用 write_header 指定;
     * @var int
     */
    public $status = 200;
    /**
     * 响应头信息, Key/Val 形式
     * @var array
     */
    public $header;
    // public $cookie;
    /**
     * 响应体, 用于在 Content-Length: xxx 响应模式下返回数据;
     * @var mixed 任意数据, 实际响应会序列化此数据; 以下几种 Content-Type 存在内置的序列化:
     *  * "application/json" - 使用 json_encode 进行序列化;
     *  * "application/x-www-form-urlencoded" - 使用 http_build_query 进行序列化;
     * 其他类型强制转换为文本类型后响应;
     */
    public $body;
    /**
     * 设置返回的 Set-Cookie 项;
     * 参数功能与 PHP 内置 set_cookie 函数类似;
     */
    function set_cookie(string $name, string $value = null, int $expire = 0, string $path = "/", string $domain = "", bool $secure = false, bool $http_only = false) {}
    /**
     * 返回响应头, 用于启用 Transfer-Encoding: chunked 模式, 准备持续响应数据;
     * @param int $status 当参数存在时, 覆盖上述 $res->status 属性响应码;
     *
     * Transfer-Encoding: chunked 模式可用于 EventSource 等持续响应;
     */
    function write_header($status = 0) {}
    /**
     * 在 Transfer-Encoding: chunked 模式下, 返回一个 chunk 数据块;
     */
    function write(string $chunk) {}
    /**
     * 在 Transfer-Encoding: chunked 模式下, 结束响应过程 (返回最后一个结束块);
     * @param string $chunk 可选, 调用上述 write() 返回一个数据块;
     */
    function end(string $chunk = null) {}
    /**
     * 响应一个文件的内容; 一般需要自行设置对应文件的 Content-Type 以保证浏览器能正常渲染;
     * 下述参数中的 $path 将会被正常化处理, 以保证其不超出 $root 指定的根目录范围;
     * 注意: 此功能一般仅用于调试或少量文件访问使用, 大量生产环境请考虑使用 nginx 代为处理静态文件;
     */
    function file(string $root, string $path) {}
}
