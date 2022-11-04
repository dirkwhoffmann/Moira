#!/bin/bash
echo "Creating ADF for test case $1"
echo "Example: createadf ADD.B"

rm $1.adf
echo "cputest $1" > startup-sequence

xdftool $1.adf format "$1"
xdftool $1.adf makedir s
xdftool $1.adf write startup-sequence s
xdftool $1.adf makedir data
xdftool $1.adf makedir data/68000
xdftool $1.adf write ../data/68000/$1 data/68000/
xdftool $1.adf write ../data/68000/lmem.dat data/68000
xdftool $1.adf write ../data/68000/tmem.dat data/68000
xdftool $1.adf write ../cputest
xdftool $1.adf boot install

rm startup-sequence

xdftool $1.adf list
