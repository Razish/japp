# ja++

ja++ modification for jedi academy - best used with [openjk](http://github.com/JACoders/OpenJK)  
see [japp.jkhub.org](http://japp.jkhub.org) for more information

## compilation

* Python 3.9.1
* [Scons](https://github.com/SCons/scons)

ex: `scons debug=1 force32=1`

Windows

* [TDM-GCC](https://jmeubank.github.io/tdm-gcc/) for non-MSVC
* If you wish to use MSVC with JA++, specify `tools=default` to scons

Linux

* `sudo apt-get install git scons gcc g++ libreadline-dev`

Options

* `force32` 1 to build a 32-bit binary on a 64-bit machine
* `debug` 1 to generate debug information, 2 to also optimise code
* `no_sql` 1 to disable MySQL/SQLite support 
* `no_crashhandler` 1 to disable the crash handler/logger functionality

Environment Variables

* `NO_SSE` 1 to not generate SSE2 instructions - closer to basejka. This is used for official builds
* `MORE_WARNINGS` 1 to enable more compiler warnings

## builders
====
Travis-CI Linux 32-bit: [![Build Status](https://travis-ci.org/Razish/japp.svg?branch=master)](https://travis-ci.org/Razish/japp)  
Buildbot Linux 32-bit: [![Build Status](http://japp.jkhub.org:10101/png?builder=japp-linux32)](http://japp.jkhub.org:10101/builders/japp-linux32)  
Buildbot Linux 64-bit: [![Build Status](http://japp.jkhub.org:10101/png?builder=japp-linux64)](http://japp.jkhub.org:10101/builders/japp-linux64)

## contributors
* Raz0r (lead)
* AstralSerpent
* Ensiform
* EpicLoyd
* Morabis
* teh

## credits
* JK2MV
* loda
* OJP
* OpenJK
