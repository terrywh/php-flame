<?php
flame\init("mongodb_1");

flame\go(function() {
    $cli = flame\mongodb\connect("mongodb://username:password@host1:port1,host2:port2/database?replicaSet=rs1");
    $cs = $cli->execute([
        "find"   => "photo",
        "filter" => ["uid" => 25865119921602568],
        "limit"  => 10,
    ]);
    var_dump( $cs->fetch_row() );
    var_dump( $cs->fetch_all() );

    $cs = $cli->photo->find(["uid" => 25865119921602568]);
    var_dump( $cs->fetch_all() );
});


flame\run();
