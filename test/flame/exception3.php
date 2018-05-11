<?php
flame\init("excpetion3");
flame\log\set_output("/tmp/exception.log");

require("this file does not exist");
