<?php
flame\init("exception_1");

function test() {
    flame\time\sleep(1000);
    throw new Exception("Aaaaaaa");
}

flame\go(function() {
    flame\time\sleep(1000);
    test();
});

// 可选
flame\on("exception", function($ex) {
    // echo $ex, "\n";
    flame\quit();
});

flame\run();