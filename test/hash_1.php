<?php
flame\init("hash_1");

flame\go(function() {
    var_dump( flame\hash\murmur2("abcdef", true) );
    var_dump( flame\hash\xxh64("abcdef", 0, true) );
    var_dump( flame\hash\crc64("abcdef", true) );

    var_dump( flame\hash\murmur2("abcdef") );
    var_dump( flame\hash\xxh64("abcdef") );
    var_dump( flame\hash\crc64("abcdef") );
});

flame\run();
