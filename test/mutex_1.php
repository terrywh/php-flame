<?php
flame\init("mutex_1");

flame\go(function() {
    $mutex = new flame\mutex();
    flame\go(function() use($mutex) {
        echo "(2) awaiting lock\n";
        $guard = new flame\guard($mutex);
        echo "(2) lock acquired\n";
        flame\time\sleep(5000);
        echo "(2) protected echo\n";
        echo "(2) lock releasing\n";
    });
    flame\go(function() use($mutex) {
        echo "(3) awaiting lock\n";
        $mutex->lock();
        echo "(3) lock acquired\n";
        flame\time\sleep(1000);
        echo "(3) protected echo\n";
        $mutex->unlock();
        echo "(3) lock released\n";
    });
    echo "(1) awaiting lock\n";
    $mutex->lock();
    echo "(1) lock acquired\n";
    flame\time\sleep(2000);
    echo "(1) protected echo\n";
    $mutex->unlock();
    echo "(1) lock released\n";
});

flame\run();
