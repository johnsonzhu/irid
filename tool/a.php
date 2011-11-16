<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11111);


$key = "user_info_12345678";
$path = "username";
$value = array("a"=>"123","b"=>"哈哈哈");

$ret = $mc->set($key."@".$path.".".$path,$value);
//$ret = $mc->set($key,$value);
var_dump($ret);



$ret = $mc->get($key);
var_dump($ret);


$ret = $mc->get($key."@".$path);
var_dump($ret);

echo "delete\n\n";
$ret = $mc->delete($key."@".$path.".".$path);
var_dump($ret);

echo "after delete\n";
$ret = $mc->get($key);
var_dump($ret);

