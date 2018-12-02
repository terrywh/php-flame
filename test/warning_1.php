<?php
flame\init("warning_1");

flame\go(function() {
    flame\time\sleep(1000);
    $a = $b; // WARNING
});

flame\run();