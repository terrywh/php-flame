<?php
flame\init("http_1");

for($i=0;$i<10;++$i) {
    flame\go(function() {
        flame\http\get("http://www.baidu.com/", 8000);
        flame\http\get("https://www.baidu.com/", 8000);
        flame\http\get("https://ss1.bdstatic.com/5eN1bjq8AAUYm2zgoY3K/r/www/cache/static/protocol/https/home/img/qrcode/zbios_efde696.png", 3000);
        // var_dump( flame\http\put("http://127.0.0.1:8687/", "this is the data for put request", 800000) );
        // var_dump( flame\http\get("http://127.0.0.1:8687/") );
        $req = new flame\http\client_request('https://www.baidu.com/');
        // $req = new flame\http\client_request('http://127.0.0.1:8687/');
        $req->method = "POST";
        $req->header["test1"] = "string";
        $req->header["test2"] = 123456;
        // $req->http_version(flame\http\client_request::HTTP_VERSION_2_PRI);
        var_dump(flame\http\exec($req));
        echo "done.\n";
    });
}
flame\run();
