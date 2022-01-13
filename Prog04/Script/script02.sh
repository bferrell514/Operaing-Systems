#!/usr/bin/env bash
# get input parameters
if [[ -d "$1" ]]; then
	ipath=$1
	opath=$2
	maxIndex=0
	# read all files and get maxIndex
	for pfile in "$ipath"/*
	do
		if [[ -f "$pfile" ]]; then
			output=$(grep -o "^[0-9][0-9]*" "$pfile")
			output=$(($output+0))
			maxIndex=$(( maxIndex < output ? output : maxIndex ))
		fi
	done
	maxIndex=$((maxIndex+1))
	# run server process
	"../Source/Version2/prog04v02" "$maxIndex" "\"$ipath\"" "\"$opath\""
else
	echo "$ipath is not a directory."
fi