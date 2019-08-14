<?php

flame\init("mysql_1");

flame\go(function() {
    $cli = flame\mysql\connect("mysql://user:password@host:port/database");
    var_dump( $cli->escape('a.b', '`') );
    $tx = $cli->begin_tx();
    try{
        $tx->insert("jtest", ["id"=>123, "data"=>'{"x":123}']);
        $tx->update("jtest", ["id"=>123], ["val"=>'{"y":456}']);
        $tx->commit();
    }catch(Exception $ex) {
        $tx->rollback();
        return;
    }
    try{
        $r = $cli->insert("jtest", [
            ["id"=>1, "data"=>["a"=>1]],  // 自动对 data 数据进行 JSON 编码
            ["id"=>2, "data"=>["b"=>2]],
            ["id"=>3, "data"=>["c"=>3]]
        ]);
    }catch(Exception $ex) {
        var_dump($cli->last_query());
    }
    $rs = $cli->select("jtest", "*", ["id"=>["{>}"=>0]]);
    $r0 = $rs->fetch_row();
    $rs = $cli->query("SELECT * from `jtest` where `id`=?", $r0["id"]); // Format + Escape
    var_dump($r0->fetch_all());
});

flame\run();
