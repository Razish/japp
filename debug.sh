#!/usr/bin/env bash
set -euo pipefail

assert_dir() {
	[ -d "$1" ] && return 0
	echo >&2 "missing ./$1 symlink"
	exit 2
}

assert_dir basepath
assert_dir cdpath
assert_dir homepath

"${1-openjk}" +set fs_basepath "basepath" +set fs_homepath "homepath" +set fs_cdpath "cdpath" +set fs_game "japlus" "${*:2}"
