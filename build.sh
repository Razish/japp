#!/bin/bash

# fetch arguments
ARGS=($@)
ARGSLEN=${#ARGS[@]}

# options
BUILD_ALL=1
DEBUG=0
ANALYSE=0
FORCE32=0

# targets
GAME=0
CGAME=0
UI=0

for (( i=0; i<${ARGSLEN}; i++ ));
do
	case ${ARGS[$i]} in
	"debug")
		DEBUG=1
		;;
	"analyse")
		ANALYSE=1
		;;
	"force32")
		FORCE32=1
		;;
	"game")
		GAME=1
		BUILD_ALL=0
		;;
	"cgame")
		CGAME=1
		BUILD_ALL=0
		;;
	"ui")
		UI=1
		BUILD_ALL=0
		;;
	*)
		;;
	esac
done

if [ $BUILD_ALL -eq 1 ]
then
	GAME=1
	CGAME=1
	UI=1
fi

if [ $GAME -eq 1 ]
then
	scons game=1 debug=$DEBUG analyse=$ANALYSE force32=$FORCE32 > /dev/null
fi

if [ $CGAME -eq 1 ]
then
	scons cgame=1 debug=$DEBUG analyse=$ANALYSE force32=$FORCE32 > /dev/null
fi

if [ $UI -eq 1 ]
then
	scons ui=1 debug=$DEBUG analyse=$ANALYSE force32=$FORCE32 > /dev/null
fi
