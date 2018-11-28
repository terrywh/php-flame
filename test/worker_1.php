<?php
flame\init("worker_1", [
    // "logger" => "/tmp/output.txt",
]);

flame\go(function() {
    flame\time\sleep(1000);
    echo getenv("FLAME_CUR_WORKER").": abc\n";
    flame\time\sleep(1000);
});

flame\run();
