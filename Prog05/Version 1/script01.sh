#!/usr/bin/env bash
### check input parameter
if [[ -d "$1" ]]; then
	### get ImageLibrary folder
	ipath=$1
	[[ "${ipath}" != */ ]] && ipath="${ipath}/"

	### get current folder
	cpath=$(pwd)
	[[ "${cpath}" != */ ]] && cpath="${cpath}/"

	### recreate lib subfolder
	rm -rf "$ipath"lib
	mkdir "$ipath"lib

	### build static library
	# create static subfolder if necessary
	mkdir "$ipath"lib/static
	mkdir "$ipath"lib/static/build
	# build object files
	cd "$ipath"lib/static/build
	g++ -I"$ipath"include -Wall -c "$ipath"src/ImageIO.cpp \
	"$ipath"src/ImageIO_TGA.cpp "$ipath"src/utilities.cpp \
	"$ipath"src/RasterImage.cpp
	# build library
	cd "$ipath"lib/static
	ar -cvq libImageLibrary.a "$ipath"lib/static/build/ImageIO.o \
	"$ipath"lib/static/build/ImageIO_TGA.o \
	"$ipath"lib/static/build/utilities.o \
	"$ipath"lib/static/build/RasterImage.o
	# remove build folder
	rm -rf "$ipath"lib/static/build
	
	### build shared library
	# create shared subfolder if necessary
	mkdir "$ipath"lib/shared
	mkdir "$ipath"lib/shared/build
	# build object files
	cd "$ipath"lib/shared/build
	g++ -I"$ipath"include -Wall -fPIC -c "$ipath"src/ImageIO.cpp \
	"$ipath"src/ImageIO_TGA.cpp "$ipath"src/utilities.cpp \
	"$ipath"src/RasterImage.cpp
	# build library
	cd "$ipath"lib/shared
	g++ -shared -Wl,-soname,libImageLibrary.so.1 -o libImageLibrary.so.1.0 \
	"$ipath"lib/shared/build/ImageIO.o \
	"$ipath"lib/shared/build/ImageIO_TGA.o \
	"$ipath"lib/shared/build/utilities.o \
	"$ipath"lib/shared/build/RasterImage.o
	# remove build folder
	rm -rf "$ipath"lib/shared/build
	# change the folder back to current
	cd "$cpath"
	
	### build utilities against the static library
	g++ -I"$ipath"include -Wall -o "$cpath"comp "$ipath"applications/comp.cpp \
	-L"$ipath"lib/static -lImageLibrary
	g++ -I"$ipath"include -Wall -o "$cpath"crop "$ipath"applications/crop.cpp \
	-L"$ipath"lib/static -lImageLibrary
	g++ -I"$ipath"include -Wall -o "$cpath"flipH "$ipath"applications/flipH.cpp \
	-L"$ipath"lib/static -lImageLibrary
	g++ -I"$ipath"include -Wall -o "$cpath"flipV "$ipath"applications/flipV.cpp \
	-L"$ipath"lib/static -lImageLibrary
	g++ -I"$ipath"include -Wall -o "$cpath"gray "$ipath"applications/gray.cpp \
	-L"$ipath"lib/static -lImageLibrary
	g++ -I"$ipath"include -Wall -o "$cpath"rotate "$ipath"applications/rotate.cpp \
	-L"$ipath"lib/static -lImageLibrary
	
	### build dispatcher version 1
	g++ -Wall -o "$cpath"dispatcher_v1 "../Version 1/dispatcher_v1.cpp" || true

	### build dispatcher version 2
	g++ -Wall -o "$cpath"dispatcher_v2 "../Version 2/dispatcher_v2.cpp" || true

	### build dispatcher version 3
	g++ -Wall -o "$cpath"dispatcher_v3 "../Version 2/dispatcher_v3.cpp" || true
	g++ -Wall -o "$cpath"crop_dispatcher "../Version 2/crop_dispatcher.cpp" || true
	g++ -Wall -o "$cpath"flipH_dispatcher "../Version 2/flipH_dispatcher.cpp" || true
	g++ -Wall -o "$cpath"flipV_dispatcher "../Version 2/flipV_dispatcher.cpp" || true
	g++ -Wall -o "$cpath"gray_dispatcher "../Version 2/gray_dispatcher.cpp" || true
	g++ -Wall -o "$cpath"rotate_dispatcher "../Version 2/rotate_dispatcher.cpp" || true

	### print success message
	echo "Success."
else
	echo "$1 is not a directory."
fi