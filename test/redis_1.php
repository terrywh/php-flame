<?php
flame\init("redis_1");

flame\go(function() {
    $cli = flame\redis\connect("redis://auth:password@host:port/database");
    var_dump( $cli->set("test1", "this is a text") );
    var_dump( $cli->get("test1") );
    var_dump( $cli->get("test1") );
    var_dump( $cli->set("test2", 123456) );
    var_dump( $cli->get("test2") );
    var_dump( $cli->incr("test2") );

    var_dump( $cli->zrange("rank:relay_20181122", 0, 30, "WITHSCORES") );
    var_dump( $cli->hgetall("rank:relay_201811") );

    $rv = $cli->multi()
        ->get("test1")
        ->get("test2")
        ->del("test1")
        ->del("test2")
        ->exec();
    var_dump( $rv );
});

flame\run();
