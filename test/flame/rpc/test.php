<?php
$client = new flame\rpc\client("http://xxx.pdtv.io:8360");
$data = yield $client->invoke("/hello", $a, $b, $c);
$data = yield $client->hello($a, $b, $c);
