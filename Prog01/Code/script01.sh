#!/usr/bin/env bash
NAME=${1?Error: no name given}
gcc -o $NAME prog01.c
./$NAME 24
./$NAME 24 HelloWorld
./$NAME 24 Hello World
./$NAME 24 Hello World !