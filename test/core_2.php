<?php
ob_start();

flame\init("core_3", [
	"debug"  => false, // 默认 debug = true 时不会自动重启工作进程
	"worker" => 2,
]);
function ex1() {
	yield flame\trigger_error();
}
function ex2() {
	throw new Exception();
}
flame\go(function() {
	$ex = 0;
	try{
		yield ex1();
	}catch(Exception $e){
		++$ex;	
	}
	try {
		yield ex2();
	}catch(Exception $e){
		++$ex;	
	}
	assert($ex == 2);
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\n");
}
