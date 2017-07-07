<?php
$server = new flame\fastcgi_server();
$server->listen(__DIR__."/../flame.sock");
flame\fork();
$server->run();
