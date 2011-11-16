<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11511);


$key = "user_info_12345678";
$path = "username";
$value = array("a"=>"123","b"=>"哈哈哈");

$ret = $mc->set($key."@".$path.".".$path,$value);
if(true !==$ret)
{
	echo "mc set fail\n";
}



$ret = $mc->get($key);
if(empty($ret))
{
	echo "mc get key ".$key." fail\n";
}
if(!is_array($ret))
{
	echo "mc get ret is not a array\n";
}

if(!isset($ret[$path][$path]))
{	
	echo "ret is not match\n";	
	var_dump($ret);
}


$ret = $mc->get($key."@".$path);
if(false == $ret)
{
	echo "mc get key ".$key."@".$path." fail\n";	
}

$ret = $mc->delete($key."@".$path.".".$path);
if(false === $ret)
{
	echo "mc delete key ".$key."@".$path.".".$path." fail\n";
}

$ret = $mc->get($key);
if(false === $ret)
{
	echo "mc get key ".$key." fail\n";
}

echo "done";
