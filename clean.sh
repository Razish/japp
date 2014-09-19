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

$clean project=game debug=$DEBUG force32=$FORCE32 -c
$clean project=cgame debug=$DEBUG force32=$FORCE32 -c
$clean project=ui debug=$DEBUG force32=$FORCE32 -c

# remove any lingering object files
#find . -name '*.o' -print0 | xargs -0 rm
