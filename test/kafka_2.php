<?php
flame\init("kafka_1");

flame\go(function() {
    $consumer = flame\kafka\consume([
        "bootstrap.servers" => "host1:port1,host2:port2",
        "group.id" => "flame-test-consumer",
        "auto.offset.reset" => "smallest",
    ], ["test"]);
    flame\go(function() use($consumer) {
        // 60 秒后关闭消费者
        flame\time\sleep(60000);
        $consumer->close();
    });
    $consumer->run(function($msg) {
        var_dump($msg);
        flame\time\sleep(50);
    });
    echo "done.\n";
    unset($consumer);
});

flame\run();
