#!/usr/bin/env bash
# Check input arguments
width=${1?usage: ./script1.sh [width] [height] [threads]}
height=${2?usage: ./script1.sh [width] [height] [threads]}
threads=${3?usage: ./script1.sh [width] [height] [threads]}
if [ "$4" != "" ]; then
	echo "usage: ./script1.sh [width] [height] [threads]"
else
	# Build executable
	cd ../Version3
	g++ -Wall main.cpp gl_frontEnd.cpp -lm -lGL -lglut -lpthread -o cell
	# Create a named pipe if it does not exists
	pipe="/tmp/pipe_ca_1"
	rm "$pipe" || true
	mkfifo "$pipe"
	if [[ ! -p $pipe ]]; then
		echo "Cannot create a named pipe"
		exit 1
	fi
	# Run executable
	./cell "$width" "$height" "$threads" <<<"1" &
	# Read from command line
	while read line
	do
		# Write to the pipe
		echo "$line" >$pipe
	done < /dev/stdin
fi