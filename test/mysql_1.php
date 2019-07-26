<?php

flame\init("mysql_1");

for($i=0;$i<5;++$i) {
    flame\go(function() {
        $cli = flame\mysql\connect("mysql://user:password@host:port/database");
        var_dump( $cli->escape('a.b', '`') );
        $rs = $cli->query("SELECT * FROM `user` WHERE `uid`=?", 10000);
        $row = $rs->fetch_row();
        var_dump($row);
        $row = $rs->fetch_row();
        var_dump($row);
        $data = $rs->fetch_all();
        var_dump($data);
        $tx = $cli->begin_tx();
        try{
            $tx->insert("test_0", ["key"=>123, "val"=>"456"]);
            $tx->update("test_0", ["key"=>123], ["val"=>"567"]);
            $tx->commit();
        }catch(Exception $ex) {
            $tx->rollback();
            return;
        }    
    });
}

flame\run();
