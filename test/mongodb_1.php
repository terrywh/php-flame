<?php
flame\init("mongodb_1");

flame\go(function() {
    $cli = flame\mongodb\connect("mongodb://username:password@host1:port1,host2:port2/database?replicaSet=rs1");
    for($i=0;$i<10;++$i) {
        flame\go(function() use($cli, $i) {
            $cs = $cli->execute([
                "find"   => "photo",
                "filter" => ["uid" => 25865119921602568],
                "limit"  => 10,
            ]);
            echo "execute\n";
            // var_dump( $cs->fetch_row() );
            $cs->fetch_all();
            flame\time\sleep(100);
            // unset($cs);
            echo "sleep\n";
            $cs = $cli->photo->find(["uid" => 25865119921602568]);
            // var_dump( $cs->fetch_all() );
            echo "done: ". $i. "\n";
        });
    }
});


flame\run();
