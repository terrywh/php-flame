<?php
flame\init("worker_1");

flame\go(function() {
    flame\time\sleep(1000);
    flame\log\trace("WORKING:", intval(getenv("FLAME_CUR_WORKER")) - 1);
    flame\time\sleep(60000);
});

flame\on("quit", function() {
    flame\log\trace("QUITING:", intval(getenv("FLAME_CUR_WORKER")) - 1);
    flame\quit();
});

flame\run();
