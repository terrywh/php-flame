<?php
flame\init("hash_1");

flame\go(function() {
    $x = flame\encoding\bson_encode(["a"=>1234567890123,"x"=>"中文"]);
    echo "(",strlen($x),") ", bin2hex($x), "\n";
    $y = flame\encoding\bson_decode($x);
    var_dump( $y );
});

flame\run();
