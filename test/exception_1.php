<?php
flame\init("exception_1");

function test() {
    flame\time\sleep(1000);
    throw new Exception("Aaaaaaa");
}
function test2() {
    test();
}

flame\go(function() {
    flame\time\sleep(1000);
    test2();
});

// 可选
flame\on("exception", function($ex) {
    // echo $ex, "\n";
    flame\quit();
});

flame\run();