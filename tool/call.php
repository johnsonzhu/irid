<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11111);


$max = 1000000000;
for($i=0;$i<$max;$i++)
{
	$key = "key".rand(1,25000);	
	$value = "value".$key."";
	switch(rand(100,999)%10)
	{
		case 4:
		case 0:
			//echo "[".$i."]add key ".$key."\n";
			$ret = $mc->add($key,$value);
		break;	
		case 5:
		case 1:
			//echo "[".$i."]set key ".$key."\n";
			$mc->set($key,$value);
		break;	
		case 6:
		case 2:
			//echo "[".$i."]replace key ".$key."\n";
			$mc->replace($key,$value);
		//break;	
		case 7:
		case 3:
			//echo "[".$i."]delete key ".$key."\n";
			$mc->delete($key);
		//break;	
		default:
			//echo "[".$i."]get key ".$key."\n";
			$mc->get($key);
		break;	
	}
	if($i%10000 == 0) echo $i."\n";
}
