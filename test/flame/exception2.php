<?php
function test1() {
	echo "3\n";
	yield test2();
	echo "4\n";
	// test2();
}
function test2() {
	yield 2;
	echo "5\n";
	// throw new Exception("this is a exception");
	yield flame\do_exception(false);
	echo "6\n";
	yield 2;
}

function test() {
	echo "1\n";
	try{
		yield test1();
	}catch(Exception $ex) {
		var_dump($ex);
	}
	echo "2\n";
}

// function run($gn, $depth = 0) {
// 	echo "depth = ", $depth, "\n";
// 	try{
// 		$v = $gn->valid();
// 	}catch(Exception $ex) {
// 		var_dump($ex);
// 	}
// 	while($v) {
// 		$nn = $gn->current();
// 		if($nn instanceof Generator) {
// 			run($nn, $depth + 1);
// 		}
// 		$gn->next();
// 	}
// 	var_dump($gn);
// }
// run(test());

flame\init("test");
flame\go(test);
flame\run();
