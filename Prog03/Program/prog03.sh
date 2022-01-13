#!/usr/bin/env bash
# get input parameters
pname=$1
ipath=$2
ppath=$3
opath=$4
# remove temporary folder and recreate
temp_dir_path="prog03_temp"
temp_file_path="prog03.temp"
rm -rf "$temp_dir_path"
# copy all files from pattern folder to the temporary folder
cp -R "$ppath" "$temp_dir_path"
# replace line endings
for pfile in "$temp_dir_path"/*.pat
do
	while IFS= read -r line; do
		#echo $(xxd -pu <<< "$line")
		if [[ "$line" =~ $'\r' ]]; then
			echo "${line:0:${#line}-1}"
		else
			echo "$line"
		fi
	done < "$pfile" > "$temp_file_path"
	rm "$pfile"
	mv "$temp_file_path" "$pfile"
done
# copy all files back to pattern folder
rm -rf "$ppath"
mv "$temp_dir_path" "$ppath"
# build c program
gcc -o "$pname" "prog03.c"
# launch separate processes for each pattern
rm -rf "$opath"
mkdir "$opath"
for pfile in "$ppath"/*.pat
do
	"./$pname" "\"$pfile\"" "\"$ipath\"" "\"$opath\"" &
done