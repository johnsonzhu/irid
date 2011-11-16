<?php
$mc = new memcached("test");
$mc->addServer("127.0.0.1",11511);
$mc->setOption(Memcached::OPT_COMPRESSION, false);

$key = "user_".rand(1,999);
$value1 = "11111";
$value2 = "22222";
$value3 = "33333";

$mc->delete($key);

$ret = $mc->add($key,$value1);
if(true !== $ret)
{
	printf("mc add key ".$key." fail\n");
}

$ret = $mc->prepend($key,$value2);
if(true !== $ret)
{
	printf("mc prepend key ".$key." fail\n");
}

$ret = $mc->get($key);
if(false === $ret)
{
	printf("mc get key ".$key." fail\n");
}

if($ret != $value2.$value1)
{
	printf("mc prepend not match\n");
}

$ret = $mc->append($key,$value3);
if(true !== $ret)
{
	printf("mc append key ".$key." fail\n");
}

$ret = $mc->get($key);
if(false === $ret)
{
	printf("mc get key ".$key." fail\n");
}

if($ret != $value2.$value1.$value3)
{
	printf("mc append not match\n");
}


echo "done";
