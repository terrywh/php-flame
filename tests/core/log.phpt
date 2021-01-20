--TEST--
flame\trace()/debug()/... to log all kinds of information.
--FILE--
<?php
flame\trace("abc", 123, new DateTime("2020-10-13 15:21"));
flame\debug("abc", 123, new DateTime("2020-10-13 15:21"));
flame\info("abc", 123, new DateTime("2020-10-13 15:21"));
flame\warn("abc", 123, new DateTime("2020-10-13 15:21"));
flame\error("abc", 123, new DateTime("2020-10-13 15:21"));
flame\fatal("abc", 123, new DateTime("2020-10-13 15:21"));
?>
--EXPECT--
