<?php


$x = [
    "a" => [
        "b" => "111111",
    ]
];
$y = flame\get($x, "a.b");
var_dump($y);
flame\set($x, "a.c", "222222");
flame\set($x, "a.x.d", "333333");
flame\set($x, "e.f", "444444");
flame\set($x, "g.h.i", "555555");
var_dump($x);


$r = flame\toml\parse_file(__DIR__."/toml_1.txt");
var_dump($r);
$data = file_get_contents(__DIR__."/toml_1.txt");
$r = flame\toml\parse_string($data);
var_dump($r);
