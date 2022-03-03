#!/bin/bash

./$1 +set fs_basepath "ja" +set fs_homepath "homepath" +set fs_cdpath "cdpath" +set fs_game "japlus" +exec "$2"
