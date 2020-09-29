<?php
flame\init("core");
$logger = new flame\logger("test.log", ["fifo"=> true]);
$logger->write("abc", 123, array("x"=>"xyz"));
flame\run();