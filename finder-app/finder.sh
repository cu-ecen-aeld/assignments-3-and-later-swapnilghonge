#!/bin/sh
# Assignment 1 : Basics of Bash Scripting
# Author: Swapnil Ghonge
# Description: This file contains functon of finder.sh

filesdir=$1
searchstr=$2

# Error Checking for number of arguements
if [ $# -ne 2 ]
then
	echo "Wrong Number of Arguements"
	
	exit 1
fi
#Error Checking for file directory
if [ ! -d $filesdir ]
then 
	echo "File Directory not present"
	exit 1
fi

#Computing the number of files in directory
	
number_of_file=$(find $1 -type f | wc -l)

#Computing the matching line

matching_line=$(grep -r $2 $1 | wc -l)

echo "The number of files are $number_of_file and the number of matching lines are $matching_line"

exit 0


























