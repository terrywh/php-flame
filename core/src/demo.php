<?php
echo "hello (#", demo\which, ") (world)!\n";
hello1();
hello2("world", 2);
echo hello3(),"\n";

class Demo {
    public $name;
    function __construct() {
        $this->name = "world";
    }
};

echo hello4(new DateTime(), new Demo()), "\n";
$obj = new hello5();
$obj->name = "world";
$obj->hello();
echo "hello (#", hello5::which, ") (world)!\n";

echo ini_get("demo.hello"), "\n";
