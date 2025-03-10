# ja++

ja++ modification for jedi academy - best used with [openjk](https://github.com/JACoders/OpenJK)  
see [japp.jkhub.org](https://japp.jkhub.org) for more information.

assets can be found here: [Razish/japp-assets](https://github.com/Razish/japp-assets)

[![build](https://github.com/Razish/japp/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/Razish/japp/releases/tag/latest)

## supports

| | Windows | Linux | macOS |
| - | - | - | - |
| x86 | ✅ | ✅ | ❓ |
| x86_64 | ✅ | ✅ | ✅ |
| armhf | ❌ | ✅ | ❌ |
| arm64 | ❌ | ✅ | ✅ |

## development requirements (general)

- Python 3.11
- [Scons](https://github.com/SCons/scons) 4.4
- zip or 7zip on your `PATH` (for packaging)

### windows

[TDM-GCC](https://jmeubank.github.io/tdm-gcc/) or Visual Studio (if you pass `tools=default` to scons)

### linux (debian-based)

install packages: `git scons gcc g++ libreadline-dev libglib2.0-dev libgtk2.0-dev libnotify-dev`

### asdf-vm + lua setup (optional, recommended)

install [asdf-vm](https://asdf-vm.com/guide/getting-started.html):

```sh
git clone https://github.com/asdf-vm/asdf.git ~/.asdf --branch v0.14.0
```

add the following to your shell rc (e.g. `~/.bashrc`) and restart your shell:

```sh
. "$HOME/.asdf/asdf.sh"
```

```sh
asdf plugin-add python
asdf plugin-add lua https://github.com/Stratus3D/asdf-lua.git
asdf install # install required versions
luarocks install luafilesystem
luarocks install luacheck
```

## compiling

just run `scons` or `build.sh` followed by `lua package.lua`

options:

- `force32` 1 to build a 32-bit binary on a 64-bit machine
- `debug` 1 to generate debug information, 2 to also optimise code
- `no_sql` 1 to disable MySQL/SQLite support
- `no_crashhandler` 1 to disable the crash handler/logger functionality

environment variables:

- `NO_SSE` 1 to not generate SSE2 instructions - closer to basejka. This is used for official builds
- `MORE_WARNINGS` 1 to enable more compiler warnings

## contributors ([full list](https://github.com/Razish/japp/graphs/contributors))

- Raz0r (lead)
- AstralSerpent
- Ensiform
- Exmirai
- Morabis
- teh

## credits

- JK2MV
- loda
- OJP
- OpenJK
