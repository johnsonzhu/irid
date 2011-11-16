<?php
$mc = new memcached("test");
$mc->addServer("127.0.0.1",11511);

$yuwen = 78;

$key = "user_".rand(1,999);
$value = array(
	"userId"=>"12345678",
	"yuwen"=>$yuwen,
	"shuxue"=>90,
	"yingyu"=>"88"
);

$ret = $mc->set($key,$value);
if(true !== $ret)
{
	echo "mc set key ".$key." fail\n";
}

$ret = $mc->increment($key."@yuwen",10);
if(($yuwen + 10 )!= intval($ret))
{
	echo "mc increment key ".$key." fail\n";
}

$ret = $mc->get($key."@yuwen");
if(($yuwen + 10 )!= intval($ret))
{
	echo "mc increment key ".$key." not match\n";
	var_dump($ret);
}

$ret = $mc->get($key);
var_dump($ret);

echo "done";
