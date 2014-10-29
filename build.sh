#!/bin/bash

# fetch arguments
ARGS=($@)
ARGSLEN=${#ARGS[@]}

# options
DEBUG=0
FORCE32=0
build='scons -Q'

for (( i=0; i<${ARGSLEN}; i++ ));
do
	case ${ARGS[$i]} in
	"debug")
		DEBUG=1
		;;
	"fastdebug")
		DEBUG=2
		;;
	"analyse")
		build='scan-build scons -Q'
		;;
	"force32")
		FORCE32=1
		;;
	*)
		;;
	esac
done

$build debug=$DEBUG force32=$FORCE32
