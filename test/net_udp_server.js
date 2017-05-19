"use strict";

const dgram = require("dgram");

const server = dgram.createSocket("udp6");
server.bind(6676);
server.on("message", function(data, rinfo) {
	console.log(rinfo, data);
});
