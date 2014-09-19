#!/bin/bash

# fetch arguments
ARGS=($@)
ARGSLEN=${#ARGS[@]}

# options
DEBUG=0
FORCE32=0

# targets
GAME=0
CGAME=0
UI=0

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
	"all")
		GAME=1
		CGAME=1
		UI=1
		;;
	"game")
		GAME=1
		;;
	"cgame")
		CGAME=1
		;;
	"ui")
		UI=1
		;;
	*)
		;;
	esac
done

if [ $GAME -eq 0 ] && [ $CGAME -eq 0 ] &&  [ $UI -eq 0 ]
then
	echo 'please specify a project to build: game, cgame, ui (or "all")'
	exit
fi

if [ $GAME -eq 1 ]
then
	$build project=game debug=$DEBUG force32=$FORCE32
fi

if [ $CGAME -eq 1 ]
then
	$build project=cgame debug=$DEBUG force32=$FORCE32
fi

if [ $UI -eq 1 ]
then
	$build project=ui debug=$DEBUG force32=$FORCE32
fi
