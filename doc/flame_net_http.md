### `namespace flame\net`
提供基本的 HTTP 协议网络协程式客户端、服务器封装；

### `class flame\net\http_request`

	封装 HTTP 协议的请求

#### `http_request::http_request(string url)`
**属性**：
	使用单独的 `url` 来初始化一个请求；

#### `http_request::http_request(string method, string url, array postfield)`
**属性**：
	使用 `method` , `url` , `postfield` 来初始化一个请求：
	`method` ：HTTP 的方法。前支持的有 GET , POST , PUT；
	`url` ：请求的地址；
	`postfield`： `POST` 和 `PUT` 上传的数据。

#### `http_request::http_request(array args)`
**功能**：
	使用 `array` 类型的 `map` 来初始化一个请求，支持的参数包括 `url` , `method` , `timeout` , `header`：
	[
		"url"=>"www.example.com",
		"method"=>"POST",
		"timeout"=>10,
		"header"=>[
				"Accept"=>"*/*",
				"userdefine"=>"userdefine"
			]
	]

#### `http_request::url`
**属性**：
	HTTP 的请求地址，必须不为空；
**注意**：
* 如果执行 `url` 为空的请求， `http_client::exec` 会抛出异常；

#### `http_request::method`
**属性**：
	HTTP 的方法，目前支持的有 GET , POST , PUT；

#### `http_request::timeout`
**属性**：
	请求的超时时间，默认为10秒；

#### `http_request::header`
**属性**：
	`array` 类型的 `map` ，key是header的类型，value是参数；

### `class flame\net\http_client`
	封装 libcurl 相关的 HTTP 处理功能；

#### `yield http_client::exec(http_request request)`
**功能**：
	绑定服务器地址端口，绑定后调用 `run()` 启动服务器开始监听指定的地址、端口；
**注意**：
* 如果执行 `url` 为空的请求， `http_client::exec` 会抛出异常；

#### `http_client::debug(int is_open)`
**功能**：
	参数不为零时，打开 `libcurl` 的 VERBOSE 模式，可以更好的调试。

