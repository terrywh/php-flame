"use strict";

const net = require("net");

net.createServer(conn => {
	console.log("conn");
	let data = 0, intv = setInterval(function() {
		conn.write((++data).toString());
	},1000);
	conn.on("data", chunk => {
		console.log("==>", chunk.toString());
	}).on("error", err => {
		console.log("err", err.stack);
	}).on("close", () => {
		console.log("close");
		clearInterval(intv);
	});
}).listen(7678);