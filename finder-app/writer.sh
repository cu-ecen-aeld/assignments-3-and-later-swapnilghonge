#!/bin/sh
# Assignment 1 : Basics of Bash Scripting
# Author: Swapnil Ghonge
# Description: This file contains functon of writer.sh


# Error Checking for number of arguements
if [ $# -ne 2 ]
then
	echo "Wrong Number of Arguements"
	
	exit 1
fi

echo $2>$1

































