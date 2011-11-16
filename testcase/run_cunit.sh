#!/bin/bash

cd CUnit
echo "start to run test_lrumc.c"
gcc -g -Wall test_lrumc.c ../../buff.c ../../pary.c ../../rbtree.c ../../serialize.c ../../assignment.c -lm  -lcunit -o test_lrumc && ./test_lrumc


#echo "start to run a.c"
#gcc -g -Wall a.c -lcunit -o a && ./a

