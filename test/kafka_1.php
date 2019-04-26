<?php
flame\init("kafka_1");

flame\go(function() {
    $producer = flame\kafka\produce([
        "bootstrap.servers" => "host1:port1,host2:port2",
    ], ["test"]);

    for($i=0;$i<10000;++$i) {
        echo $i, "\n";
        $message = new flame\kafka\message("this is the payload", "this is the key");
        $message->header["key"] = "value";
        $producer->publish("test", $message);
        flame\time\sleep(50);
        $producer->publish("test", "this is the payload", "this is the key");
        flame\time\sleep(50);
    }
    $producer->flush();
});

flame\run();
