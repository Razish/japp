#!/bin/bash

# fetch arguments
ARGS=($@)
ARGSLEN=${#ARGS[@]}

# options
DEBUG=0
FORCE32=0
clean='scons -Q'

for (( i=0; i<${ARGSLEN}; i++ ));
do
	case ${ARGS[$i]} in
	"debug")
		DEBUG=1
		;;
	"fastdebug")
		DEBUG=2
		;;
	"force32")
		FORCE32=1
		;;
	*)
		;;
	esac
done

$clean debug=$DEBUG force32=$FORCE32 -c

# remove any lingering object files
find . -type f -name "*.os" -delete
