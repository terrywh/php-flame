<?php
flame\init("excpetion3");
flame\log\set_output("/tmp/exception.log");
flame\go(function() {
	yield test();
});
function test() {
	yield 1;
	$a = ["A" "B"];
	yield 2;
}
flame\run()
