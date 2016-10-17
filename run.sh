#!/bin/bash

name=$1

if [ -z "$name" ]; 
	then echo "Give input .fna input file (without .fna extension) as a command line argument";
	exit
fi

# Clean up
rm -f testcases/${name}.sdsl
rm -f testcases/${name}.sorted

# Build index
./tribble/find-superstring/find-superstring -C -f testcases/${name}.fna -i testcases/${name}.sdsl -s testcases/${name}.sorted

# Compute superstring
./tribble/find-superstring/find-superstring --find-superstring -i testcases/${name}.sdsl -s testcases/${name}.sorted
