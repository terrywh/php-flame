--TEST--
start framework by `flame\run()`;
--FILE--
<?php
flame\run([
    "service_name"=> "abc",
], function() {
    flame\go(function() {
        flame\time\sleep(100);
        echo "2\n";
    });
    echo "1\n";
    flame\time\sleep(200);
    echo "3\n";
});
?>
--EXPECT--
1
2
3
