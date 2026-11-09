#!/bin/bash
echo "Creating HDF cputester.hdf"

rm cputester.hdf 2>/dev/null
#echo "cputest all" > startup-sequence

xdftool cputester.hdf create "cputester" size=96M
xdftool cputester.hdf format "cputester" ofs
#xdftool cputester.hdf makedir s
#xdftool cputester.hdf write startup-sequence s
#xdftool $1.hdf makedir data
xdftool cputester.hdf write data data
xdftool cputester.hdf write cputest
xdftool cputester.hdf boot install

#rm startup-sequence

xdftool cputester.hdf list
