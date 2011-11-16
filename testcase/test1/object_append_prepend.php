<?php
$mc = new memcached("test");
$mc->addServer("127.0.0.1",11511);
$mc->setOption(Memcached::OPT_COMPRESSION, false);

$mood = "今天天气真好啊!";

$arr = array(
	"userId"=>12345678,
	"username"=>"朱文飞",
	"sex"=>"男",
	"email"=>"zhuwenfei008@163.com",
	"mood"=>$mood,
	"status"=>1
);

$key = "user_12345678";
//$mc->delete($key);

$ret = $mc->set($key,$arr);
if(true !== $ret)
{
	printf("mc set key ".$key." fail\n");
}

$str = "人会笑，猫会叫，狗子在蹦蹦地跳";
$ret = $mc->append($key."@mood",$str);
if(true !== $ret)
{
	printf("mc append key ".$key." fail\n");
}

$ret = $mc->get($key);
if(false === $ret)
{
	echo "mc get key ".$key." fail\n";
}

if(!is_array($ret))
{
	echo "mc get key ".$key." is not a array\n";
}

if($mood.$str != $ret['mood'])
{
	echo "mc append key ".$key." fail\n";	
}

echo "done";
