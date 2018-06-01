<?php
function test1() {
	// throw new Exception("this is a exception");
	// yield flame\do_exception(false);
	yield test2();
}
function test2() {
	yield 2;
	throw new Exception("this is a exception");
	// yield flame\do_exception(false);
	yield 2;
}

function test() {
	try{
		yield test1();
	}catch(Exception $ex) {
		var_dump($ex);
	}
}

flame\init("test");
flame\log\set_output("/tmp/exception.log");
flame\go("test");
flame\run();

