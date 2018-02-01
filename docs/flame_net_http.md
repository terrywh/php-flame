### `namespace flame\net`
提供基本的 HTTP/1 协议服务器封装，协程式客户端封装（底层使用 curl 并内置使用 nghttp2 提供了 HTTP/2 支持）；

#### `yield flame\net\http\get(string $url, integer $timeout = 2500)`
简单的 `GET` 请求方法。

#### `yield flame\net\http\post(string $url, mixed $post, integer $timeout = 2500)`
简单的 `POST` 请求方法；
* 当 `$post` 为数组时自动拼装为 `K1=V1&K2=V2` 形式进行发送；
* 当 `$post` 为文本时直接进行发送；

**示例**：
``` PHP
<?php
$ret = yield flame\net\http\post("http://www.panda.tv", ["arg1"=>"val1","arg2"=>"val2"]);
var_dump($ret);
```

#### `yield flame\net\http\put(string $url, mixed $put, integer $timeout = 2500)`
简单的 `PUT` 请求方法。
* 当 `$put` 为数组时自动拼装为 `K1=V1&K2=V2` 形式进行发送；
* 当 `$put` 为文本时直接进行发送；

#### `yield flame\net\http\remove(string $url, integer $timeout = 2500)`
简单的 `DELETE` 请求方法。

#### `yield flame\net\http\exec(client_request $request)`
执行指定的请求（使用默认的客户端）；


### `class flame\net\http\client_request`
封装 HTTP 协议的客户端请求请求

#### `client_request::__construct(string $url[, mixed $data[, integer $timeout]])`
初始化并生成客户端请求对象：

* `$url` ：请求的地址；
* `$data`：可选，请求体，若存在，将自动设置当前请求 `$req->method='POST'`，可以为 `null`；
* `$timeout`：可选，请求超时，单位 `ms`（毫秒），默认为 2500ms；

**示例**：
``` PHP
<?php
$req = new flame\net\http\client_request("http://www.baidu.com", null, 5);
```

#### `client_request::ssl(array $options)`
设置 ssl 相关选项，可选的支持如下选项：
**示例**：
``` PHP
<?php
// $req  = ....
$request->ssl([
	"verify" => "none", // 可选，peer - 验证对端证书真实性 / host - 验证服务器 / both - 前两者均验证 / none - 前两者均不验证
	"cert"   => "/data/htdocs/flame.terrywh.net/etc/cert.pem", // 可选，证书，类型自动按照后缀名设定 ( pem / der )
	"key"    => "/data/htdocs/flame.terrywh.net/etc/key.pem",  // 可选，私钥，类型自动按照后缀名设定 ( pem / der / eng )
	"pass"   => "123456", // 可选，私钥密钥
]);
```

#### `client_request::version(integer $version)`
设置请求使用的 HTTP 版本，请使用如下常量进行设置：
* `flame\\net\\http\\HTTP_VERSION_AUTO` - 自动适配，由框架自行决定使用合适的版本；
* `flame\\net\\http\\HTTP_VERSION_1_0` - 强制 HTTP/1.0 协议
* `flame\\net\\http\\HTTP_VERSION_1_1` - 强制 HTTP/1.1 协议
* `flame\\net\\http\\HTTP_VERSION_2_0` - 尝试协商 HTTP/2 请求，若无法实现时使用 HTTP/1.1 协议
* `flame\\net\\http\\HTTP_VERSION_2TLS` - 尝试在 HTTPS 下协商 HTTP/2 请求，若无法实现时，使用 HTTP/1.1 协议
* `flame\\net\\http\\HTTP_VERSION_2_PRIOR_KNOWLEDGE` - 强制 HTTP/2 协议（若服务端不支持 HTTP/2 将发生错误）

**注意**：
* 默认情况下，请求使用 `HTTP_VERSION_2TLS` 选项功能；

#### `string client_request::$method`
HTTP 的方法，目前支持的有 GET , POST , PUT；

#### `string client_request::$url`
HTTP 的请求地址，不能为空；

#### `integer client_request::$timeout`
请求超时，单位 `ms` (毫秒)；

#### `array client_request::$header`
请求头部，可以用于定制请求头部（KEY/VALUE 关联数组）；

**示例**：
``` PHP
<?php
// ...
// 单一设置
$req->header["content-type"] = "application/x-www-form-urlencoded";
// 统一设置
$req->header = ["content-type"=>"application/x-www-form-urlencoded"];
```

**注意**：
* 设置 Cookie 时请示用专用的 $cookie 属性数组，否则可能导致相互覆盖；
* 当用户未设置 `Content-Type` 时，默认会使用 "application/x-www-form-urlencoded" 类型；当发送含有 BODY 的请求时，请求体首个字符为 '[' 或 '{' 时，默认会使用 "application/json" 类型；

#### `array client_request::$cookie`
请求头部定制 Cookie 项；

**注意**：
* 设置 Cookie 不要使用 $header ，否则可能导致相互覆盖；

#### `mixed client_request::$body`
Array / String 当 $method = `PUT` 或 `POST` 时的请求体；

**示例**：
``` PHP
<?php
// ...
$req->body = "aaaaaaaaaa";
// 支持直接使用 Array 类型，自动序列化为 application/x-www-form-urlencoded 格式
$req->body = ["a"=> "b", "c"=> "d"];
```

### `class flame\net\http\client`
封装 libcurl 相关的 HTTP 处理功能，如无特殊需求，请尽量使用同一个 `client` 发送若干个请求（而非创建多个 `client` 对象），以减少资源占用；

#### `client::__construct(array $options)`
创建请求客户端对象，支持的属性如下示例：

**示例**：
``` PHP
<?php
$cli = new flame\net\http\client([
	"conn_share" => "pipe", // "none" => 不进行排队或并行
	                 // "pipe" => 管道传输 HTTP/1.1 请求（同一连接）
	                 // "plex" => 并行传输 HTTP/2 请求（STREAM）
					 // "both"  => 默认，PIPE + PLEX 
	"conn_per_host" => 4,  // 默认 2，同 HOST:PORT 请求建立的最大连接数 必须 <= 512
	"pipe_per_conn" => 2,  // 默认 4，单连接 HTTP/1.1 管道排队数量 必须 <= 256
]);
```

#### `yield client::exec(client_request $request)`
执行指定请求并返回响应对象 `flame\net\http\client_response`；

**示例**：
``` PHP
<?php
$cli = new flame\net\http\client();
$req1 = new flame\net\http\client_request("http://www.panda.tv");
$req2 = new flame\net\http\client_request("http://www.baidu.tv");
$res1 = yield $cli->exec($req1);
var_dump($res1);
$res2 = yield $cli->exec($req2);
var_dump($res2);
```

**注意**：
* 执行请求失败可能抛出异常（例如：超时，无法连接等）；
* 下述简化请求方法，实际也相当于调用 `exec` 故可能抛出异常；

### `class flame\net\http\client_response`
客户端请求的响应，可自动转换为文本 (`toString()`)，也可以按属性访问对应数据；

#### `integer client_response::$status`
响应码，例如 `200`；

#### `array client_response::$header`
HTTP 响应头，K/V 数组；

**注意**：
* 所有 KEY 统一被处理为小写；

#### `array client_response::$cookie`
COOKIE，对应 HTTP 响应头中 Set-Cookie 的部分；每项元素的 KEY 为该项 Cookie 的名称，VAL 为该项 Cookie 的所有属性（例如 expire / path 等）对应的 K/V 关联数组；

#### `string client_response::$body`
HTTP 响应体；

#### `string client_response::toString()`
HTTP 响应体，方便直接将对象作为文本使用；

### `class flame\net\http\server_request`
作为服务端时，收到的来自客户端、浏览器的请求封装对象；

#### `array server_request::$method`
请求方法，如 'GET' 'POST' 等，统一处理为大写；

#### `string server_request::$uri`
请求路径；

**注意**：
* 请求路径中 **不包含** 查询字符串 `query string`；

#### `array server_request::$query`
请求 `GET` 参数；数据来源于请求 `URL` 中 `PATH` 之后 `?` 与 `#` 之间 或 `URL` 结束之前的文本，并通过 `parse_str()` 进行解析得到 K/V 数组；

#### `array server_request::$header`
请求头信息，注意：
* 所有请求头的字段名称均被处理为**小写**，字段值保持不变；
* 所有请求头的字段名中包含 “\_” （下划线），将会被替换为 “-” (减号)；
* 仅存在字段名而无字段值的请求头将被忽略（不存在于 `$header` 属性中）;

#### `array server_request::$cookie`
请求附带的 `Cookie` 信息，K/V 数组；

**注意**：
* 原始的 `Cookie` 字符串可以使用 `$request->header["cookie"]` 获得；

#### `mixed server_request::$body`
请求体；存在如下可能：

* 当请求类型 `Content-Type` 标记为 `application/x-www-form-urlencoded` 时，会通过内置解析器得到 K/V 数组；
* 当请求类型 `Content-Type` 标记为 `multipart/form-data` 时，会通过内置解析器得到 K/V 数组 ；
* 当请求类型 `Content-Type` 标记为 `application/json` 时，会通过 PHP 引擎的 `json_decode` 解析得到 K/V 数组；
* 其他请求类型仅能得到原始数据 `string`；

**注意**：
* 文件上传请求的请求体类型为 `multipart/form-data` ，提取 `name` 作为 KEY 文件内容数据作为 VAL，**不会** 生成 `PHP` 中类似 `$_FILES` 的结构；

#### `server_request::$data`
默认为 null，可用于在 before / handle / after 之间传递数据；

### `class flame\net\http\handler`
HTTP/1 协议处理器，用于 tcp_server / unix_server 解析 `HTTP/1` 协议，并提供 HTTP 服务；

参考：`test/flame/net/http_server.php`

#### `handler::get/post/put/remove(string $path, callable $cb)`
分别用于设置 GET / POST / PUT / DELETE 请求方法对应路径的处理回调；

#### `handler::handle(callable $cb)`
设置默认处理回调（未匹配路径回调），或设置指定路径的请求处理回调（`$path` 参数可选）；回调函数接收两个参数：
* `$request` - 类型 `class flame\net\http\server_request` 的实例，请参考 `flame\net\http` 命名空间中的相关说明；
* `$response` - 类型 `class flame\net\fastcgi\server_response` 的实例，请参考下文；

**示例**：
``` PHP
<?php
$handler->get("/hello", function($req, $res) {
	var_dump($req->header["host"]);
	yield $res->write("hello world\n");
	yield $res->end();
})->handle(function($req, $res) {
	yield $res->end('{"error":"router not found"}');
});
```

**注意**：
* `handler` 会为每次回调启用新的协程；

#### `handler::before(callable $cb)`
设置一个在每次请求处理过程开始前被执行的回调，可以用于处理诸如登录判定、权限控制等；

**注意**：
* `handler` 会为每次回调启用新的协程；

#### `handler::after(callable $cb)`
设置一个在每次请求处理过程接手后（请求、响应对象还未被销毁前）被执行的回调，可以用于处理诸如日志记录、格式输出等；

**注意**：
* `handler` 会为每次回调启用新的协程；

### `class flame\net\http\server_response`
由上述 `handler` 生成，并回调传递给处理函数使用，用于返回响应数据给 Web 服务器；

**注意**：
* 默认情况下，服务端输出响应将自动按照 `Transfer-Encoding: chunked` 描述的方式进行；可自行设置 `Content-Length` 头信息使用标准方式输出；

#### `array server_response::$header`
响应头部 KEY/VAL 数组，默认包含 `Content-Type: text/plain`（）；其他响应头请酌情考虑添加、覆盖；

**示例**：
``` PHP
<?php
$res->header["Content-Type"] = "text/html";
$res->header["X-Server"] = "Flame/0.7.0";
```

**注意**：
* 所有输出的 HEADER 数据 **区分大小写**；

#### `server_response::$data`
默认为 null，可用于在 before / handle / after 之间传递数据；（参考 http_server2.php 示例）；

#### `server_response::set_cookie(string $name [, string $value = "" [, int $expire = 0 [, string $path = "" [, string $domain = "" [, bool $secure = false [, bool $httponly = false ]]]]]])`

定义一个 Cookie 并通过在响应头 `Set-Cookie` 下发（实际发送在 `writer_header` 时触发）；效果与 PHP 内置函数 [setcookie](http://php.net/manual/en/function.setcookie.php) 类似；

#### `yield server_response::write_header(integer $status_code)`
设置并输出响应头部（含上述响应头 KEY/VAL 及响应状态行），请参照标准 HTTP/1.1 STATUS_CODE 设置数值；

#### `yield server_response::write(string $data)`
输出指定响应内容，若还未发送相应头，将自动调用 `write_header` 发送相应头（默认 `200 OK`）；请参考 `handle()` 的相关示例；

**注意**：
* 请谨慎在多个协程中使用 `write`/`end` 函数时，防止输出顺序混乱；

#### `yield server_response::end([string $data])`
结束请求，若还未发送响应头，将自动调用 `write_header` 发送相应头（默认 `200 OK`）；可选输出响应内容；输出完成后结束响应；

**注意**：
* 当 `server_response` 对象被销毁时，会自动结束响应 `end()` ；
