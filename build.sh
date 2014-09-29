#!/bin/bash

# fetch arguments
ARGS=($@)
ARGSLEN=${#ARGS[@]}

# options
DEBUG=0
FORCE32=0
XCOMPILE=0

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
	"xcompile")
		XCOMPILE=1
		export CC=i586-mingw32msvc-gcc
		export CXX=i586-mingw32msvc-c++
		export LD=i586-mingw32msvc-ld
		export AR=i586-mingw32msvc-ar
		export AS=i586-mingw32msvc-as
		export NM=i586-mingw32msvc-nm
		export STRIP=i586-mingw32msvc-strip
		export RANLIB=i586-mingw32msvc-ranlib
		export DLLTOOL=i586-mingw32msvc-dlltool
		export OBJDUMP=i586-mingw32msvc-objdump
		export RESCOMP=i586-mingw32msvc-windres
		export WINDRES=i586-mingw32msvc-windres
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
	$build project=game debug=$DEBUG force32=$FORCE32 xcompile=$XCOMPILE
fi

if [ $CGAME -eq 1 ]
then
	$build project=cgame debug=$DEBUG force32=$FORCE32 xcompile=$XCOMPILE
fi

if [ $UI -eq 1 ]
then
	$build project=ui debug=$DEBUG force32=$FORCE32 xcompile=$XCOMPILE
fi
