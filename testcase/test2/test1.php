<?php
$mc = new memcached("test");
$mc->addServer("127.0.0.1",11511);
$mc->setOption(Memcached::OPT_COMPRESSION, false);

$start = 1;
$end = 45000;

$max = 1000000000;
for($i=0;$i<$max;$i++)
{
	switch(rand(100,999)%10)
	{
		case 4:
			$key = "key".rand($start,$end);	
			$mc->get($key);
		break;
		case 0:
			$key = "key".rand($start,$end);	
			$value = "value".$key."";
			$ret = $mc->add($key,$value);
		break;	
		case 5:
		case 1:
			$key = "key".rand($start,$end);	
			$value = "value".$key."";
			$ret = $mc->set($key,$value);
		break;	
		case 6:
			$key = "key".rand($start,$end);	
			$value = "value".$key."";
			$mc->append($key,$value);
		break;
		case 2:
			$key = "key".rand($start,$end);	
			$value = "value".$key."";
			$mc->replace($key,$value);
		break;	
		case 7:
			$key = "key".rand($start,$end);	
			//$mc->delete($key);
		break;	
		case 3:
		default:
			$key = "key".rand($start,$end);	
			$mc->get($key);
		break;	
	}
	if($i%10000 == 0) echo $i."\n";
}
