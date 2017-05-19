"use strict";

const net = require("net");

net.createServer(function(conn) {
	conn.on("data", function(chunked) {
		console.log("->", chunked);
		conn.write(chunked);
	}).on("end", function() {
		console.log("(end)");
	});
}).listen(6676);
