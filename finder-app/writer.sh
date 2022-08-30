#!/bin/sh
# Assignment 1 : Basics of Bash Scripting
# Author: Swapnil Ghonge
# Description: This file contains functon of writer.sh

# References: https://www.youtube.com/watch?v=V1y-mbWM3B8
#		https://linuxconfig.org/bash-scripting-tutorial-for-beginners
#		https://linux.die.net/man/1/mkdir		

# Error Checking for number of arguements
if [ $# -ne 2 ]
then
	echo "Wrong Number of Arguements"
	
	exit 1
fi


echo $2>$1

































