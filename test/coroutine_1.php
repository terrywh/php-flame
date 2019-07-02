<?php
flame\init("coroutine_1");

flame\go(function() {
    flame\time\sleep(100);
    echo "c1:", flame\co_id(), "\n";
    flame\time\sleep(200);
    echo "c1:", flame\co_id(), "\n";
});


flame\go(function() {
    flame\time\sleep(200);
    echo "c2:", flame\co_id(), "\n";
    flame\time\sleep(100);
    echo "c2:", flame\co_id(), "\n";
});


flame\run();