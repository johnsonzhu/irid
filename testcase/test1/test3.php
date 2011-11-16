<?php
$mc = new memcached("test");
$mc->addServer("127.0.0.1",11511);


$key = "user_info_12345678";
$path = "username";
$value = array("a"=>"123","b"=>'哈"哈\'哈\\"aaaa"');

$ret = $mc->set($key."@".$path.".".$path,$value);
if(true !==$ret)
{
	echo "mc set fail\n";
}



$ret = $mc->get($key);
var_dump($ret);

echo "done";
