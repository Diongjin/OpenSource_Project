#!/bin/bash
#expr.sh
a=1
a=`expr $a + 1`
echo $a
b=`expr $a + 10/2`
echo $b
c=`expr $a \> $b`
echo $c
