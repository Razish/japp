#!/bin/bash

# fetch arguments
ARGS=($@)
ARGSLEN=${#ARGS[@]}

# options
DEBUG=0
ANALYSE=0
FORCE32=0
COMPILER='gcc'

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
	"clang")
		COMPILER='clang'
		;;
	"analyse")
		ANALYSE=1
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
	scons project=game debug=$DEBUG compiler=$COMPILER analyse=$ANALYSE force32=$FORCE32 >/dev/null
fi

if [ $CGAME -eq 1 ]
then
	scons project=cgame debug=$DEBUG compiler=$COMPILER analyse=$ANALYSE force32=$FORCE32 >/dev/null
fi

if [ $UI -eq 1 ]
then
	scons project=ui debug=$DEBUG compiler=$COMPILER analyse=$ANALYSE force32=$FORCE32 >/dev/null
fi
