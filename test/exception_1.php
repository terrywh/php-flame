<?php
flame\init("exception_1", [
    "logger" => "/tmp/output.txt",
]);

function abc() {
    // flame\time\sleep(1000);
    throw new Exception("Aaaaaaa");
}

flame\go(function() {
    flame\time\sleep(5000);
    echo "Abc\n";
    throw new Exception("Aaaaaaa");
});

flame\on("exception", function($ex) {
    // echo $ex, "\n";
    flame\quit();
});

flame\run();