<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11511);


$key = "user_".rand(1,999);
$value = 1;

$ret = $mc->set($key,$value);
if(true !== $ret)
{
	printf("mc set key ".$key." fail\n");
}

$ret = $mc->increment($key);
if(2 !== intval($ret))
{
	printf("2 mc incremnt key ".$key." fail\n");	
}

$ret = $mc->increment($key);
if(3 !== intval($ret))
{
	printf("3 mc incremnt key ".$key." fail\n");	
}

$ret = $mc->increment($key,10);
if(13 !== intval($ret))
{
	printf("13 mc incremnt key ".$key." fail\n");	
}

for($i=0;$i<10;$i++)
{
	$ret = $mc->increment($key,1);
	if(empty($ret))
	{
		echo "mc increment key ".$key." fail\n"; 
	}
}

if(23 !== intval($ret))
{
	echo "mc increment 10 1 fail\n";
}

$ret = $mc->decrement($key);
if(22 !== intval($ret))
{
	echo "mc decrement key ".$key." fail\n";
}


$ret = $mc->decrement($key,10);
if(12 !== intval($ret))
{
	echo "mc decrement key ".$key." fail\n";
}

$ret = $mc->decrement($key,20);
if(0 !== intval($ret))
{
	echo "mc decrement key ".$key." fail\n";
}

echo "done";
