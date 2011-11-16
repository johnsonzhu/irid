#!/bin/php
<?php
$in_filename = "in.txt";
$out_filename = "out.txt";
$clean_out_filename = "clean_out.txt";
$content = file_get_contents($in_filename);
$arr = explode("\n",$content);

$start = true;
$memaddr = "";
$out = "";
$clean_out = "";
$tmp_out = "";
$is_alloc = true;
foreach($arr as $item)
{
	$item = trim($item);
	if(empty($item))
	{
		continue;
	}
	$tmp_arr = explode(" ",$item);
	if(false === strpos($tmp_arr[0],"0x"))
	{
		continue;	
	}
	if($start)
	{
		$memaddr = $tmp_arr[0];
		$start = false;
		$tmp_out = $item."\n";
		if(in_array($tmp_arr[1],array("malloc","calloc")))
		{
			$is_alloc = true;	
		}
	}
	else
	{
		if($memaddr == $tmp_arr[0])
		{
			$tmp_out = "";
			$out .= "\n\n";
			$start = true;
		}	
		else
		{
			$memaddr = $tmp_arr[0];
			$out .= $tmp_out;
			$clean_out .= $tmp_out;
			$tmp_out = $item."\n";
			$start = false;
		}
	}
}
file_put_contents($out_filename,$out);
file_put_contents($clean_out_filename,$clean_out);
