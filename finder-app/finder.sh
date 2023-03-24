#!/bin/sh

if [ $# -lt 2 ]; then
	echo "Usage - $0 <directory path> <string to search for>"
	exit 1
fi

search_path=$1
search_term=$2

if [ ! -d "$search_path" ]; then
	echo "<directory path> does not appear to be a valid directory on the file system"
	exit 1
fi

total_files=$(grep -rc $search_term $search_path | awk '{tot+=1;}  END{print tot}' )
total_matches=$(grep -rc $search_term $search_path | awk -F: '{tot+=$2;} END{print tot}' )

echo "The number of files are $total_files and the number of matching lines are $total_matches"


