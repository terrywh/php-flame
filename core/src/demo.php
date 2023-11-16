<?php
echo demo\hello0, "\n";
hello1();
hello2("world", 2);
echo "hello (#3) [", hello3(),"]!\n";

class Demo {
    public $name;
    function __construct() {
        $this->name = "world";
    }
};

echo hello4(new DateTime()), "\n";
function demo_hello5() {
    $obj = new hello5();
    $obj->name = "world";
    $obj->hello();
}
demo_hello5();
echo hello6::hello, "\n";
echo ini_get("demo.hello7"), "\n";
$cb = hello8();
call_user_func($cb);
echo hello9(new Demo(), "name"), "\n";
