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

clean='scons -Qc'

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
	"nosql")
		NOSQL=1
		;;
	*)
		;;
	esac
done

$clean debug=$DEBUG force32=$FORCE32 no_sql=$NOSQL project=$PROJECT tools=$TOOLS

# remove any lingering object files
find . -type f -name "*.os" -delete
