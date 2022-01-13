#!/usr/bin/env bash
NAME=$1
ARG1=$2
ARG2=$3
ARG3=$4
ARG4=$5
ARG5=$6
ARG6=$7
if [ "$ARG4" == "" ]; then
	echo "usage: ./script01EC4.sh arg1 arg2 arg3 arg4 [arg5] [arg6]"
elif [ "$8" != "" ]; then
	echo "usage: ./script01EC4.sh arg1 arg2 arg3 arg4 [arg5] [arg6]"
else
	gcc -o $NAME prog01.c
	./$NAME $ARG1 $ARG2
	./$NAME $ARG2 $ARG3
	./$NAME $ARG3 $ARG4
	if [ "$ARG5" != "" ]; then
		./$NAME $ARG4 $ARG5
	fi
	if [ "$ARG6" != "" ]; then
		./$NAME $ARG5 $ARG6
	fi
fi