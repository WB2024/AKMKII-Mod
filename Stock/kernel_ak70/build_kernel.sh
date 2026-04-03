#!/bin/bash

job=""
clean="nkc"

if [ $# -ne 0 ]
then
for param in $*; do
	if [ $param = "c" ]
	then
		clean=""
	elif [ $param = "nj" ]
	then
		job="nj"
		echo "building with single jobs"

        elif [ $param = "km" ]
        then
                job="$job km"
                echo "building bootimage include whth kernel ko"
	else
		echo "ex) build_kernel.sh [c nk km]"
		exit
	fi
done
fi

cd ..
args="$clean nlk na $job"
echo $args

./build_base.sh $args
