# ja++

ja++ modification for jedi academy - best used with [openjk](http://github.com/JACoders/OpenJK)  
see [japp.jkhub.org](http://japp.jkhub.org) for more information

## supports

| | Windows | Linux | macOS |
| - | - | - | - |
| x86 | ✅ | ✅ | ❓ |
| x86_64 | ✅ | ✅ | ✅ |
| Arm (RPi) | ❌ | ✅ | ❌ |
| Apple Silicon | ❌ | ❌ | ✅ |

## development requirements (general)

- Python 3.11
- [Scons](https://github.com/SCons/scons) 4.4

### windows

[TDM-GCC](https://jmeubank.github.io/tdm-gcc/) or MSVC (if you pass `tools=default` to scons)

### linux (debian-based)

`git scons gcc g++ libreadline-dev`

## compiling

just run `scons` or `build.sh`

Options:

- `force32` 1 to build a 32-bit binary on a 64-bit machine
- `debug` 1 to generate debug information, 2 to also optimise code
- `no_sql` 1 to disable MySQL/SQLite support
- `no_crashhandler` 1 to disable the crash handler/logger functionality

Environment Variables

- `NO_SSE` 1 to not generate SSE2 instructions - closer to basejka. This is used for official builds
- `MORE_WARNINGS` 1 to enable more compiler warnings

## builders

⚠️ oops they're all gone! please replace me with Github actions!

## contributors ([full list](https://github.com/Razish/japp/graphs/contributors))

- Raz0r (lead)
- AstralSerpent
- Ensiform
- EpicLoyd
- Morabis
- teh

## credits

- JK2MV
- loda
- OJP
- OpenJK
