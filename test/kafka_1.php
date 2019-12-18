<?php
flame\init("kafka_1");

flame\go(function() {
    $producer = flame\kafka\produce([
        "bootstrap.servers" => "host1:port1, host2:port2",
    ], ["flame_test"]);

    for($i=0;$i<100;++$i) {
        echo $i, "\n";
        $producer->publish("flame_test", "x:".strval($i));
        flame\time\sleep(50);
        $producer->publish("flame_test", "y:".strval($i), "y:".strval($i));
        flame\time\sleep(50);
        $message = new flame\kafka\message("z:".strval($i), "z:".strval($i));
        $message->header["key1"] = "string";
        $message->header["key2"] = 123456; // convert to string
        $producer->publish("flame_test", $message);
        flame\time\sleep(50);
    }
    $producer->flush();
});

flame\run();
