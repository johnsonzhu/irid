<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11111);
$mc->setOption(Memcached::OPT_COMPRESSION, false);

$key = "abc";
$value = "valueabc";

$ret = $mc->set($key,$value);
if(true !== $ret )
{
	echo "set key ".$key." fail\n";
	die;
}

$ret = $mc->append($key,"123456789");
if($ret !== true)
{
	echo "append key ".$key." fail\n";
	//die;
}

$ret = $mc->prepend($key,"987654321");
if($ret !== true)
{
	echo "prepend key ".$key." fail\n";
	//die;
}
$ret = $mc->get($key);
var_dump($ret);
$mc->set("aaa",0);
$ret = $mc->increment("aaa",10);
var_dump($ret);
$ret = $mc->increment("aaa",5);
var_dump($ret);
$ret = $mc->increment("aaa");
var_dump($ret);
$ret = $mc->decrement("aaa",7);
var_dump($ret);
$ret = $mc->decrement("aaa");
var_dump($ret);
$ret = $mc->decrement("aaa",100);
var_dump($ret);
$ret = $mc->decrement("bbb");
var_dump($ret);
