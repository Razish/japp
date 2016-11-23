#!/bin/bash

# fetch arguments
ARGS=($@)
ARGSLEN=${#ARGS[@]}

# options
PROJECT=game,cgame,ui
TOOLS=default
DEBUG=0
FORCE32=0
NOSQL=1
export NO_SSE=1

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
		build='scan-build $build'
		;;
	"force32")
		FORCE32=1
		;;
	"nosql")
		NOSQL=1
		;;
	*)
		;;
	esac
done

$build debug=$DEBUG force32=$FORCE32 no_sql=$NOSQL project=$PROJECT tools=$TOOLS
