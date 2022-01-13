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
	while read status path; do
		if [[ "$status" == "End" ]]; then
			echo "End"
			mv "$path" "$opath"Completed
			exit 0
		fi
		if [[ "$status" == "Processed" ]]; then
			echo "Processed"
			mv "$path" "$opath"Completed
		fi
	done < <(
		"$cpath"dispatcher_v3 < <(
			while read path action file; do
				# handle tga file
				if [[ "$file" =~ .*tga$ ]]; then
					mv "$dpath$file" "$ipath"
				fi
				# handle job file
				if [[ "$file" =~ .*job$ ]]; then
					[[ "${path}" != */ ]] && path="${path}/"
					sleep 0.1
					echo "\"$path$file\"" "\"$ipath\"" "\"$opath\"" "\"$cpath\""
				fi
			done < <(inotifywait -m "$dpath" -e create -e moved_to)
			)
		)
fi