<?php
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
$obj->hello();


