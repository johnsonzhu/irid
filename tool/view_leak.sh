#!/bin/bash

if [ "" == "$1" ]
then
	filename="1.txt"
else
	filename="$1"
fi

in="in.txt"
out="out.txt"
clean_filename="clean_out.txt"
sort $filename > $in 
./format.php
cat "$clean_filename" | more


