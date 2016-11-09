#!/bin/bash

name=$1

if [ -z "$name" ]; 
	then echo "Give input .fna input file (without .fna extension) as a command line argument";
	exit
fi

# Clean up
rm -f "testcases/${name}.sdsl"
rm -f "testcases/${name}.sorted"
rm -f "testcases/${name}-index_breakdown.html"
rm -f "testcases/${name}-create_index.html"
rm -f "testcases/${name}-find_superstring.html"

# Build index
./tribble/find-superstring/find-superstring -C -f testcases/${name}.fna -i testcases/${name}.sdsl -s testcases/${name}.sorted -m testcases/${name}-create_index.html

# Create the pie chart
./tribble/find-superstring/find-superstring -I -i testcases/${name}.sdsl -c testcases/${name}-index_breakdown.html

# Compute superstring
./tribble/find-superstring/find-superstring --find-superstring -i testcases/${name}.sdsl -s testcases/${name}.sorted -m testcases/${name}-find_superstring.html > testcases/${name}.superstring
