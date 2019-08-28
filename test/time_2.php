<?php

flame\init("time_2");

flame\go(function() {
    $x = 0;
    flame\time\tick(1000, function() use(&$x) {
        flame\log\debug("tick1");
        if(++$x > 3) {
            return false;
        }
    });

    $tm = new flame\time\timer(2000, function() {
        flame\log\debug("tick2");
    });
    $tm->start();

    flame\time\after(5000, function() use($tm) {
        flame\log\debug("done3");
        $tm->close();
    });
});

flame\run();