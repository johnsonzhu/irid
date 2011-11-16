<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11511);


$key = "bool_123";
$ret = $mc->set($key,true);
if(true !== $ret)
{
	echo "mc set key ".$key." fail\n";
}

$ret = $mc->get($key);
if(true !== $ret)
{
	echo "mc set get key ".$key." fail\n";
}

$key = "bool_123";
$ret = $mc->set($key,false);
if(true !== $ret)
{
	echo "mc set key ".$key." fail\n";
}

$ret = $mc->get($key);
if(false !== $ret)
{
	echo "mc get key ".$key."fail\n";
}

$value = array(
	"a"=>true,
	"b"=>false,
	"c"=>true,
	"d"=>false,
);

$ret = $mc->set($key,$value);
if(true !== $ret)
{
	echo "mc set key ".$key." fail\n";
}

$ret = $mc->get($key);
if(!is_array($ret))
{
	echo "mc get key ".$key." fail\n";
}
if(true !== $ret['a'] || false !== $ret['b'] || true !== $ret['c'] || false !== $ret['d'] )
{
	echo "mc match key ".$key." fail\n";	
}

echo "done";
