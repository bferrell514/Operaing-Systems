#!/usr/bin/env bash
### check input parameters
dpath=$1
ipath=$2
opath=$3
cpath=$(pwd)
### append trailing slash to paths if necessary
[[ "${dpath}" != */ ]] && dpath="${dpath}/"
[[ "${ipath}" != */ ]] && ipath="${ipath}/"
[[ "${opath}" != */ ]] && opath="${opath}/"
[[ "${cpath}" != */ ]] && cpath="${cpath}/"
### create folders if necessary
if [[ ! -d "$dpath" ]]; then
	mkdir "$dpath"
fi
if [[ ! -d "$ipath" ]]; then
	mkdir "$ipath"
fi
if [[ ! -d "$opath" ]]; then
	mkdir "$opath"
fi
if [[ ! -d "$opath"Logs ]]; then
	mkdir "$opath"Logs
fi
if [[ ! -d "$opath"Completed ]]; then
	mkdir "$opath"Completed
fi

### verify folders are different
if [[ "$dpath" == "$ipath" ]] || [[ "$dpath" == "$opath" ]] \
 || [[ "$ipath" == "$opath" ]]; then
	echo "Please use different folders."
else
	### wait for file change
	inotifywait -m "$dpath" -e create -e moved_to |
		while read path action file; do
			# handle tga file
			if [[ "$file" =~ .*tga$ ]]; then
				mv "$dpath$file" "$ipath"
			fi
			# handle job file
			if [[ "$file" =~ .*job$ ]]; then
				sleep 0.1
				echo "Processing $file"
				sleep 0.1
				"$cpath"dispatcher_v1 "$dpath$file" "\"$ipath\"" "\"$opath\"" "\"$cpath\""
				echo "Processed $file"
				echo ""
				mv "$dpath$file" "$opath"Completed
			fi
		done
fi