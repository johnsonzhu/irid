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
	"status"=>1,
	"aa"=>array(
			"bb"=>array(
					"cc1"=>array(
						"dd1"=>array(
							"e1"=>"test1",
							"e2"=>"test2",
							"e3"=>"test3",
							"e4"=>"test4",
							"e5"=>"test5",
						),
						"dd2"=>array(
							"e1"=>"test6",
							"e2"=>"test7",
							"e3"=>"test8",
							"e4"=>"test9",
							"e5"=>"test10",
						),
					),
					"cc2"=>array(
						"dd1"=>array(
							"e1"=>"test11",
							"e2"=>"test12",
							"e3"=>"test13",
							"e4"=>"test14",
							"e5"=>"test15",
						),
						"dd2"=>array(
							"e1"=>"test16",
							"e2"=>"test17",
							"e3"=>"test18",
							"e4"=>"test19",
							"e5"=>"test20",
						),
					),
				),
		),
);

$key = "user_12345678";
$ret = $mc->add($key,$arr);
if(false === $ret)
{
	$ret = $mc->delete($key);
	if(false === $ret)
	{
		echo "mc add delete key ".$key." fail\n";
	}
	$ret = $mc->add($key,$arr);
	if(true !== $ret)
	{
		echo "mc add key ".$key." fail\n";
	}
}

$ret = $mc->get($key); 
if(false === $ret)
{
	echo "11 mc get key ".$key." fail\n";
}

if(!isset($ret['aa']['bb']['cc2']['dd2']['e3']))
{
	echo "22 mc get key ".$key." fail\n";
}

if("test18" != $ret['aa']['bb']['cc2']['dd2']['e3'])
{
	echo "mc get key ".$key." is not match\n";	
}

$ret = $mc->get($key."@aa.bb.cc2");
var_dump($ret);

echo "done";
