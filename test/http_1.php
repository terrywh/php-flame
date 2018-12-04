<?php
flame\init("http_1");

/*flame\go(function() {*/
    //flame\http\get("http://127.0.0.1:8082/", 3000);
    //flame\http\post("http://127.0.0.1:8082/", "post", 3000);
    //flame\http\put("http://127.0.0.1:8082/", "put", 3000);
    //flame\http\delete("http://127.0.0.1:8082/", 3000);
/*});*/
// 使用独立连接测试
flame\go(function() {
    $opt["connection_per_host"] = 32;
    $cli = new flame\http\client($opt);
    for($i = 0; $i < 2000; $i++) {
        flame\go(function() use($i, $cli) {
            for($j = 0; $j < 5; ++$j) {
                try {
                    var_dump('------ ' . $i . ' --- ' . $j . ' 1000');
                    //$port = rand(8081, 8082);
                    $port = 8082;
                    $req = new flame\http\client_request('http://127.0.0.1:' . $port . '/get', null, 1500);
                    $req->method = "GET";
                    $cli->exec($req);
                } catch(Exception $e) {
                    echo $e;
                }
            }
        });
    }
    flame\go(function() {
        $opt2["connection_per_host"] = 16;
        $cli2 = new flame\http\client($opt2);
        for($i = 0; $i < 100; $i++) {
            flame\go(function() use($i, $cli2) {
                for($j = 0; $j < 50; ++$j) {
                    try {
                        var_dump('++++++ ' . $i . ' + ' . $j . ' 1000');
                        //$port = rand(8081, 8082);
                        $port = 8082;
                        $req = new flame\http\client_request('http://127.0.0.1:' . $port . '/get', null, 2000);
                        $req->method = "GET";
                        $cli2->exec($req);
                    } catch(Exception $e) {
                        echo $e;
                    }
                }
            });
        }
    });
});
// 直接使用全局连接测试
/*flame\go(function() {*/
    //for($i = 0; $i < 50; $i++) {
        //flame\go(function() use($i) {
            //for($j = 0; $j < 40; ++$j) {
                //var_dump('====== ' . $i . ' === ' . $j . ' 1000');
                //flame\http\get("http://127.0.0.1:8081/get", 300);
            //}
        //});
    //}
    //flame\time\sleep(3000);
//});

//flame\go(function() {
    //for($i = 1000; $i < 1500; $i++) {
        //flame\go(function() use($i) {
            //var_dump('****** ' . $i . ' 1000');
            //$port = rand(8081, 8082);
            //flame\http\get('http://127.0.0.1:' . $port . '/get', 300);
        //});
    //}
/*});*/

 //连接超时测试
/*flame\go(function() {*/
    //$opt["connection_per_host"] = 1;
    //$cli = new flame\http\client($opt);
    //flame\go(function() use($cli) {
        //try {
            //var_dump("client ------ 0 4s " . microtime());
            //$req = new flame\http\client_request("http://127.0.0.1:8081/get", null, 4000);
            //$req->method = "GET";
            //$cli->exec($req);
        //} catch(Exception $e) {
            //echo $e;
        //}
        //var_dump("client ------- 0 4s " . microtime());
    //});
    //flame\go(function() use($cli) {
        //try {
            //var_dump("client ++++++ 1 1s " . microtime());
            //$req = new flame\http\client_request("http://127.0.0.1:8081/get", null, 1000);
            //$req->method = "GET";
            //$cli->exec($req);
        //} catch(Exception $e) {
            //echo $e;
        //}
        //var_dump("client ++++++ 1 1s " . microtime());
    //});
    //flame\go(function() use($cli) {
        //try {
            //var_dump("client ------ 2 15s " . microtime());
            //$req = new flame\http\client_request("http://127.0.0.1:8081/get", null, 15000);
            //$req->method = "GET";
            //$cli->exec($req);
        //} catch(Exception $e) {
            //echo $e;
        //}
        //var_dump("client ------- 2 15s " . microtime());
    //});

    //flame\time\sleep(20000);
/*});*/

flame\run();
