#!/usr/bin/env bash
# Build executable
cd ../Version3
g++ -Wall main.cpp gl_frontEnd.cpp -lm -lGL -lglut -lpthread -o cell
# Read from command line
i=0
while read line
do
	# Split
	vals=(${line//:/ })
	# Check if this is a cell command
	if [[ "${vals[0]}" == "cell" ]]; then
		# Increase counter
		i=$((i+1))
		# Create a named pipe if it does not exists
		pipe[$i]="/tmp/pipe_ca_$i"
		rm "${pipe[$i]}" || true
		mkfifo "${pipe[$i]}"
		if [[ ! -p "${pipe[$i]}" ]]; then
			echo "Cannot create a named pipe"
			exit 1
		fi
		# Run executable
		./cell "${vals[1]}" "${vals[2]}" "${vals[3]}" <<<"$i" &
	# Otherwise
	else
		# Check if the first word is an integer
		if [[ "${vals[0]}" =~ ^[0-9][0-9]*$ ]]; then
			# Check if the first word is in valid range
			if [[ "${vals[0]}" -le "$i" ]]; then
				# Extract command
				line="${line//${vals[0]}:\ /}"
				# Write to pipe
				echo "$line" >"${pipe[${vals[0]}]}"
			# Otherwise
			else
				echo "${vals[0]} exceeds current number of processes."
			fi
		# Otherwise
		else
			echo "Invalid command"
		fi
	fi
done < /dev/stdin