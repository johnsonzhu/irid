<?php
$mc = new memcached();
$mc->addServer("127.0.0.1",11111);


$max = 1000000000;
for($i=0;$i<$max;$i++)
{
	$key = "key".rand(1,25000);	
	$value = "value".$key."";
	$value1 = "value".$key."";
	switch(rand(100,999)%10)
	{
		case 4:
		case 0:
			$ret = $mc->get($key);
			if(false === $ret)
			{
				$ret1 = $mc->add($key,$value);
				if(false === $ret1)
				{
					echo "add key ".$key." fail\n";
				}
				else
				{
					$ret2 = $mc->get($key);	
					if($ret2 != $value)
					{
						echo "add get key ".$key." fail\n";
					}
				}
			}
			else
			{
				$ret4 = $mc->add($key,$value);	
				if(false !== $ret4)
				{
					echo "get add key ".$key." fail\n";
				}
			}
			if(1 == rand(1,999)%2)
			{
				$ret5 = $mc->delete($key);
				if(false === $ret5)
				{
					$ret6 = $mc->add($key,$value);
					if(false === $ret6)
					{
						echo "delete add key ".$key." fail\n";
					}
				}
				else
				{
					$ret7 = $mc->get($key);
					if(false !== $ret7)
					{
						echo "delete get key ".$key." fail\n";
					}	
				}
			}
		break;	
		case 5:
		case 1:
			//echo "[".$i."]set key ".$key."\n";
			$ret1 = $mc->set($key,$value);
			if(false === $ret1)
			{
				echo "set key ".$key." fail\n";
			}
			else
			{
				$ret2 = $mc->replace($key,$value1);		
				if(false === $ret2)
				{
					echo "set replace key ".$key." fail\n";
				}
				else
				{
					$ret3 = $mc->get($key);		
					if($ret3 != $value1)
					{
						echo "set replace get key ".$key." fail\n";
					}
				}
			}
		break;	
		case 6:
		case 2:
			//echo "[".$i."]replace key ".$key."\n";
			$ret1 = $mc->get($key);
			if(false === $ret1)
			{
				$ret2 = $mc->add($key,$value);
				if(false === $ret2)
				{
					echo "get add key ".$key." fail\n";
				}
			}
			else
			{
				$ret3 = $mc->replace($key,$value1);
				if(false === $ret3)
				{
					echo "get replace key ".$key." fail\n";
				}
				else
				{
					$ret4 = $mc->delete($key);	
					if(false === $ret4)
					{
						echo "get replace delete key ".$key." fail\n";
					}
				}
			}
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

