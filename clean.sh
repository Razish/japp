#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# fetch arguments
ARGS=("$@")
ARGSLEN=${#ARGS[@]}

# options
PROJECT=game,cgame,ui
TOOLS=default
DEBUG=0
FORCE32=0
NOSQL=1
NONOTIFY=0
NOCRASHHANDLER=0
NOGEOIP=0
USE_ASAN=0
export NO_SSE=1

clean="scons -Qc"

for ((i = 0; i < ARGSLEN; i++)); do
	case ${ARGS[$i]} in
	"debug")
		DEBUG=1
		;;
	"fastdebug")
		DEBUG=2
		;;
	"use_asan")
		USE_ASAN=1
		;;
	"force32")
		FORCE32=1
		;;
	"nosql")
		NOSQL=1
		;;
	"nonotify")
		NONOTIFY=1
		;;
	"nocrashhandler")
		NOCRASHHANDLER=1
		;;
	"nogeoip")
		NOGEOIP=1
		;;
	*) ;;

	esac
done

$clean \
	"debug=$DEBUG" \
	"force32=$FORCE32" \
	"no_crashhandler=$NOCRASHHANDLER" \
	"no_geoip=$NOGEOIP" \
	"no_notify=$NONOTIFY" \
	"no_sql=$NOSQL" \
	"project=$PROJECT" \
	"tools=$TOOLS" \
	"use_asan=$USE_ASAN"

# remove any lingering object files
find . -type f -name "*.os" -delete
