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

if [ $GAME -eq 0 ] && [ $CGAME -eq 0 ] && [ $UI -eq 0 ]
then
	GAME=1
	CGAME=1
	UI=1
fi

if [ $GAME -eq 1 ]
then
	scons project=game debug=$DEBUG force32=$FORCE32 -c >/dev/null
fi

if [ $CGAME -eq 1 ]
then
	scons project=cgame debug=$DEBUG force32=$FORCE32 -c >/dev/null
fi

if [ $UI -eq 1 ]
then
	scons project=ui debug=$DEBUG force32=$FORCE32 -c >/dev/null
fi

# remove any lingering object files
find . -name '*.o' -print0 | xargs -0 rm
