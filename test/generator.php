<?php
function g() {
	echo 1, "\n";
	yield 1;
	echo 2, "\n";
	yield 2;
	echo 3, "\n";
}

$g = g();

$g->current();