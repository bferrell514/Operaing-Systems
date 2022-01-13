#!/usr/bin/env bash
# build
# version 1
gcc -o ../Source/Version1/prog04v01 ../Source/Version1/prog04v01.c
# version 2
gcc -o ../Source/Version2/prog04v02 ../Source/Version2/prog04v02.c
# version 3
gcc -o ../Source/Version3/prog04v03 ../Source/Version3/prog04v03.c
gcc -o ../Source/Version3/prog04v03_distribute ../Source/Version3/prog04v03_distribute.c
gcc -o ../Source/Version3/prog04v03_process ../Source/Version3/prog04v03_process.c