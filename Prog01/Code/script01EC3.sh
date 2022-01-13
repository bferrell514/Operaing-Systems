#!/usr/bin/env bash
NAME=${1?usage: ./script01EC3.sh arg1 arg2}
ARG1=${2?usage: ./script01EC3.sh arg1 arg2}
ARG2=${3?usage: ./script01EC3.sh arg1 arg2}
if [ "$4" != "" ]; then
	echo "usage: ./script01EC3.sh arg1 arg2"
else
	gcc -o $NAME prog01.c
	./$NAME $ARG1 $ARG2
fi