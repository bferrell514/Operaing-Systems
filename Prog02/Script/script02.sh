#!/usr/bin/env bash
path=$1
extension=$2
if [ "$1" == "" ] || [ "$2" == "" ] || [ "$3" != "" ]; then
	echo "usage: ./script02.sh <path> <extension>"
	exit 1
fi
echo "Looking for files with extension .$extension in folder"
echo $path
count=0
for entry in "$path"/*
do
	if [[ $entry == *$extension ]]; then
		count=$((count+1))
	fi
done
if [[ $count == 0 ]]; then
	echo "No file found."
else
	if [[ $count == 1 ]]; then
		echo "1 file found:"
	else
		echo "$count files found"
	fi
	for entry in "$path"/*
	do
		if [[ $entry == *$extension ]]; then
			echo $(basename $entry)
		fi
	done
fi