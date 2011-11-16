<?php
$dir = "./test1/";

if (is_dir($dir))
{
	if ($dh = opendir($dir))
	{
		while (($file = readdir($dh)) !== false) 
		{
			if("." == $file || ".." == $file)
			{
				continue;
			}
			$filename = $dir.$file;
			$cmd = "php ".$filename."";
			echo "\nrun cmd ".$cmd."\n";
			passthru($cmd);
		}
		closedir($dh);
	}
}

