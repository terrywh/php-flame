<?php
flame\init("notify_1");

flame\on("message", function($msg) {
    var_dump($msg);
});

flame\run(function() {
    for($i=0;$i<10;++$i) {
        flame\time\sleep(1000);
        flame\send(1, ["time" => flame\time\now()]);
    }
    flame\time\sleep(1000);
    flame\off("message");
});