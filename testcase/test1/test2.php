<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11511);

$userId = rand(1,9999999);
$key = "user_info_".$userId;

$value1 = "value1";
$value2 = "value2";
$value3 = "value3";

$mc->delete($key);

$ret = $mc->get($key);
if(false !== $ret)
{
	echo "mc delete key ".$key." fail\n";
}

$mc->add($key,$value1);

$ret = $mc->get($key); 
if($ret != $value1)
{
	echo "mc add get key ".$key." fail\n";	
	var_dump($ret);
}
$ret = $mc->get($key."@path");
if(false !== $ret)
{
	echo "mc get key ".$key."@path fail\n";
}

$arr = array(
	"key1"=>$value1,
	"key2"=>$value2,
	"key3"=>$value3
);
$ret = $mc->set($key,$arr);
if(true !== $ret)
{
	printf("11 mc set key ".$key." fail\n");
	var_dump($ret);
}
var_dump($ret);


$ret = $mc->get($key);
if($ret === false)
{
	printf("22 mc get key ".$key." fail\n");	
}
var_dump($ret);

echo "done";
