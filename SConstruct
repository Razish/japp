"""Module SConstruct"""

#
# JA++ SCons project file
# written by Raz0r
#
# options:
# 	target			cross-compile for "Windows", "Linux" or "Darwin"
# 	debug			generate debug information. value 2 also enables optimisations
# 	force32			force 32 bit target when on 64 bit machine
# 	no_sql			disable sqlite+mysql support (external dependencies)
# 	no_notify		disable desktop notifications (external dependencies: libnotify etc)
# 	no_crashhandler	disable x86 crash logger
# 	no_geoip		disable support for asynchronous geoip lookup
#
# 	simple example:
# 		scons -Q debug=1 force32=1
# 	cross-compile from linux64 to win64:
# 		CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ \
# 		scons -Q no_sql=1 no_crashhandler=1 no_geoip=1 target=Windows
# 	cross-compile from linux64 to win32:
# 		CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ \
# 		scons -Q no_sql=1 no_crashhandler=1 no_geoip=1 force32=1 target=Windows
# 	compile on macOS without pretending clang is gcc:
# 		CC=clang CXX=clang++ scons -Q
#
# envvars:
# 	NO_SSE			disable SSE floating point instructions, force x87 fpu
# 	MORE_WARNINGS	enable additional warnings
# 	LESS_WARNINGS	disable some default warnings
# 	SCONS_DEBUG 	debug logging for scons env/tool selection
#

import platform
import os
import re
import subprocess
import sys
import SCons

debug = int(ARGUMENTS.get("debug", 0))
force32 = int(ARGUMENTS.get("force32", 0))
no_sql = int(ARGUMENTS.get("no_sql", 0))
no_notify = int(ARGUMENTS.get("no_notify", 0))
no_crashhandler = int(ARGUMENTS.get("no_crashhandler", 0))
no_geoip = int(ARGUMENTS.get("no_geoip", 0))
use_asan = int(ARGUMENTS.get("use_asan", 0))
toolStr = ARGUMENTS.get("tools", "gcc,g++,ar,as,gnulink")
tools = toolStr.split(",")
proj = ARGUMENTS.get("project", "game,cgame,ui")

configuration = {
    0: lambda x: "release",
    1: lambda x: "debug",
    2: lambda x: "optimised-debug",
}[
    debug
](debug)


def cmp(a, b):
    return (a > b) - (a < b)


# compare semantic versions (1.0.2 < 1.0.10 < 1.2.0)
def cmp_version(v1, v2):
    def normalise(v):
        return [int(x) for x in re.split(r"[^0-9.]", v)[0].split(".")]
        # return [int(x) for x in re.sub( r'(\.0-+)*$', '', v ).split( '.' )]

    return cmp(normalise(v1), normalise(v2))


def run_command(cmd):
    with subprocess.Popen(cmd.split(" "), stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
        out, err = p.communicate()
        out = out.decode("utf-8").strip("\n")
        if err:
            print("run_command: " + err.decode("utf-8").strip("\n"))
        return 0 if not err else 1, out


def get_env(key, default_value=None):
    return default_value if key not in os.environ else os.environ[key]


host_plat = platform.system()  # Windows, Linux, Darwin
target_plat = ARGUMENTS.get("target", host_plat)  # Windows, Linux, Darwin
try:
    bits = int(platform.architecture()[0][:2])  # 32 or 64
except (ValueError, TypeError):
    bits = None
arch = None  # platform-specific, set manually

# architecture settings, needed for binary names, also passed as a preprocessor definition
if force32:
    bits = 32
if bits == 32:
    if target_plat == "Windows":
        arch = "x86"
    elif target_plat in ("Linux", "Darwin"):
        # FIXME: we need to read the TARGET platform, not the HOST platform...this breaks cross-compiling to ARM
        if platform.machine()[:3] == "arm":
            arch = "arm"
        else:
            arch = "i386"
    else:
        raise RuntimeError("unexpected platform: " + target_plat)
elif bits == 64:
    if platform.machine()[:3] == "arm" or platform.machine()[:5] == "aarch":
        arch = "arm64"
    else:
        arch = "x86_64"
else:
    raise RuntimeError("could not determine architecture width: " + str(bits))


# create the build environment
# FIXME: also consider LD, AS, AR in the toolset
env = Environment(
    TARGET_ARCH=arch,
    tools=tools,
    # import basic shell environment
    ENV={
        "TMP": get_env("TMP"),
        "PATH": get_env("PATH"),
        "CC": get_env("CC"),
        "CXX": get_env("CXX"),
        "PKG_CONFIG_PATH": get_env("PKG_CONFIG_PATH"),
    },
)
env.Tool("compilation_db")
env.CompilationDatabase("compile_commands.json")

# grab the compiler name
if "CC" in os.environ:
    env["CC"] = os.environ["CC"]
if "CXX" in os.environ:
    env["CXX"] = os.environ["CXX"]

# prettify the compiler output
if "TERM" in os.environ:
    env["ENV"]["TERM"] = os.environ["TERM"]
colours = {}
enableColours = sys.stdout.isatty()
colours["white"] = "\033[1;97m" if enableColours else ""
colours["cyan"] = "\033[96m" if enableColours else ""
colours["orange"] = "\033[33m" if enableColours else ""
colours["green"] = "\033[92m" if enableColours else ""
colours["end"] = "\033[0m" if enableColours else ""

env["SHCCCOMSTR"] = env["SHCXXCOMSTR"] = env["CCCOMSTR"] = env["CXXCOMSTR"] = (
    f"{colours['cyan']} compiling: {colours['white']}$SOURCE{colours['end']}"
)
env["ARCOMSTR"] = f"{colours['orange']} archiving: {colours['white']}$TARGET{colours['end']}"
env["RANLIBCOMSTR"] = f"{colours['orange']}  indexing: {colours['white']}$TARGET{colours['end']}"
env["ASCOMSTR"] = f"{colours['orange']}assembling: {colours['white']}$TARGET{colours['end']}"
env["SHLINKCOMSTR"] = env["LINKCOMSTR"] = f"{colours['green']}   linking: {colours['white']}$TARGET{colours['end']}"


# obtain the compiler version
def get_compiler_version():
    if env["CC"] == "cl":
        # msvc
        compiler_version = env["MSVC_VERSION"]
    elif "gcc" in env["CC"] or "clang" in env["CC"]:
        # probably gcc or clang
        cmd_status, ccrawversion = run_command(env["CC"] + " -dumpversion")
        compiler_version = None if cmd_status else ccrawversion
    return compiler_version


ccversion = get_compiler_version()


# git tag
def get_git_tag():
    cmd_status, rawtag = run_command("git describe --tags --exclude=latest")
    return None if cmd_status else rawtag


git_tag = get_git_tag()


# set job/thread count
def GetNumCores():
    if host_plat in ("Linux", "Darwin"):
        # works on recent mac/linux
        cmd_status, num_cores = run_command("getconf _NPROCESSORS_ONLN")
        if cmd_status == 0:
            return int(num_cores)

        # only works on linux
        cmd_status, num_cores = run_command("cat /proc/cpuinfo | grep processor | wc -l")
        if cmd_status == 0:
            return int(num_cores)

        return 1
    if host_plat == "Windows":
        # requires >= XP SP2
        return int(os.environ["NUMBER_OF_PROCESSORS"])
    return None


env.SetOption("num_jobs", GetNumCores())

# notify the user of the build configuration
if not env.GetOption("clean"):
    # build tools
    msg = "Building " + ((git_tag + " ") if git_tag else "")
    msg += (
        "using "
        + str(env.GetOption("num_jobs"))
        + " threads\n"
        + "\thost: "
        + host_plat
        + "\n"
        + "\ttarget: "
        + target_plat
        + " "
        + ("forced " if force32 else "")
        + str(bits)
        + "bit\n"
        + "\tC compiler: "
        + env["CC"]
        + " "
        + ccversion
        + "\n"
        + "\tC++ compiler: "
        + env["CXX"]
        + " "
        + ccversion
        + "\n"
        + "\tpython: "
        + platform.python_version()
        + " at "
        + sys.executable
        + "\n"
        + "\tscons: "
        + SCons.__version__
        + "\n"
    )

    # build options
    msg += (
        "options:\n"
        + "\tconfiguration: "
        + configuration
        + "\n"
        + "\tinstruction set: "
        + arch
        + ((" with x87 fpu" if int(get_env("NO_SSE", 0)) else " with SSE") if arch[:3] != "arm" else "")
        + "\n"
        + "\tsql support: "
        + ("dis" if no_sql else "en")
        + "abled\n"
        + "\tnotify support: "
        + ("dis" if no_notify else "en")
        + "abled\n"
        + "\tcrash logging: "
        + ("dis" if no_crashhandler else "en")
        + "abled\n"
        + "\tgeoip support: "
        + ("dis" if no_geoip else "en")
        + "abled\n"
        + "\taddress sanitizer: "
        + ("dis" if not use_asan else "en")
        + "abled\n"
    )

    # build environment
    if int(get_env("SCONS_DEBUG", 0)):
        msg += env["CC"] + " located at " + run_command("where " + env["CC"])[1].split("\n", maxsplit=1)[0] + "\n"
        if "AR" in env:
            msg += env["AR"] + " located at " + run_command("where " + env["AR"])[1].split("\n", maxsplit=1)[0] + "\n"
        if "AS" in env:
            msg += env["AS"] + " located at " + run_command("where " + env["AS"])[1].split("\n", maxsplit=1)[0] + "\n"
        msg += "python located at " + sys.executable + "\n"
        msg += "scons" + " located at " + run_command("where " + "scons")[1].split("\n", maxsplit=1)[0] + "\n"

    print(msg)


# clear default compiler/linker switches
def emptyEnv(_env, e):
    if int(get_env("SCONS_DEBUG", 0)):
        if e in _env:
            if _env[e]:
                print("discarding " + e + ": " + _env[e])
            else:
                print("env[" + e + "] is empty")
        else:
            print("env[" + e + "] does not exist")
    _env[e] = []


emptyEnv(env, "CPPDEFINES")
emptyEnv(env, "CFLAGS")
emptyEnv(env, "CCFLAGS")
emptyEnv(env, "CXXFLAGS")
emptyEnv(env, "LINKFLAGS")
emptyEnv(env, "ARFLAGS")
emptyEnv(env, "LIBS")

# compiler switches
if "gcc" in env["CC"] or "clang" in env["CC"]:
    env["CCFLAGS"] += [
        #'-M',	# show include hierarchy
    ]
    # c warnings
    env["CFLAGS"] += [
        #'-Wdeclaration-after-statement',
        "-Wnested-externs",
        "-Wold-style-definition",
        "-Wstrict-prototypes",
    ]

    # c/cpp warnings
    env["CCFLAGS"] += [
        "-Wall",
        "-Wextra",
        "-Wno-missing-braces",
        "-Wno-missing-field-initializers",
        "-Wno-sign-compare",
        "-Wno-unused-parameter",
        "-Winit-self",
        "-Winline",
        "-Wmissing-include-dirs",
        "-Woverlength-strings",
        "-Wpointer-arith",
        "-Wredundant-decls",
        "-Wundef",
        "-Wuninitialized",
        "-Wwrite-strings",
    ]

    # strict c/cpp warnings
    if int(get_env("MORE_WARNINGS", 0)):
        env["CFLAGS"] += [
            "-Wbad-function-cast",
        ]
        env["CCFLAGS"] += [
            "-Waggregate-return",
            "-Wcast-qual",
            #'-Wfloat-equal',
            #'-Wlong-long',
            "-Wshadow",
            #'-Wsign-conversion',
            "-Wswitch-default",
            "-Wunreachable-code",
        ]

    # gcc-specific warnings
    if "gcc" in env["CC"]:
        env["CFLAGS"] += [
            "-Wunsuffixed-float-constants",
        ]
        env["CCFLAGS"] += [
            "-Wdouble-promotion",
            #'-Wsuggest-attribute=const',
        ]

        if cmp_version(ccversion, "4.6") >= 0 and arch[:3] != "arm":
            env["CCFLAGS"] += [
                "-Wlogical-op",
            ]

        # requires gcc 4.7 or above
        if cmp_version(ccversion, "4.7") >= 0:
            env["CCFLAGS"] += [
                "-Wstack-usage=32768",
            ]

    # disable warnings
    env["CCFLAGS"] += [
        "-Wno-char-subscripts",
    ]

    env["CCFLAGS"] += [
        "-fvisibility=hidden",
    ]

    # c/cpp flags
    if arch[:3] == "arm":
        # arm
        env["CCFLAGS"] += [
            "-fsigned-char",
        ]
    else:
        # x86
        env["CCFLAGS"] += [
            "-mstackrealign",
            "-masm=intel",
        ]
        env["ASFLAGS"] += [
            "-msyntax=intel",
            "-mmnemonic=intel",
        ]
        if int(get_env("NO_SSE", 0)):
            env["CCFLAGS"] += [
                "-mno-sse2",
            ]
            if "clang" not in env["CC"]:
                env["CFLAGS"] += [
                    "-fexcess-precision=standard",
                    "-mfpmath=387",
                    "-ffloat-store",
                ]
            else:
                env["CCFLAGS"] += [
                    "-mfpmath=sse",
                    "-msse2",
                ]
            if arch == "i386":
                env["CCFLAGS"] += [
                    "-march=i686",
                ]
            elif arch == "x86_64":
                env["CCFLAGS"] += [
                    "-mtune=generic",
                ]

        if bits == 32:
            env["CCFLAGS"] += [
                "-m32",
            ]
            env["LINKFLAGS"] += [
                "-m32",
            ]

    # misc settings
    # if 'gcc' in env['CC'] and cmp_version( ccversion, '4.9' ) >= 0:
    # 	env['CCFLAGS'] += [
    # 		'-fdiagnostics-color',
    # 	]

    # c flags
    env["CFLAGS"] += [
        "-std=c17",
    ]

    # c++ flags
    env["CXXFLAGS"] += [
        "-fvisibility-inlines-hidden",
        "-std=c++17",
    ]

    # archive flags
    env["ARFLAGS"] = "rc"

elif env["CC"] == "cl":
    # msvc
    env["CCFLAGS"] += [
        #'/showIncludes',
    ]
    env["CFLAGS"] += [
        "/TC",  # compile as c
    ]
    env["CXXFLAGS"] += [
        "/TP",  # compile as c++
    ]
    env["CCFLAGS"] += [
        "/EHsc",  # exception handling
        "/nologo",  # remove watermark
    ]

    env["LINKFLAGS"] += [
        "/ERRORREPORT:none",  # don't send error reports for internal linker errors
        "/NOLOGO",  # remove watermark
    ]
    if arch == "x86_64":
        env["LINKFLAGS"] += [
            "/MACHINE:X64",
        ]
    else:
        env["LINKFLAGS"] += [
            "/MACHINE:" + arch,
        ]
    if bits == 64:
        env["LINKFLAGS"] += [
            "/SUBSYSTEM:WINDOWS",  # graphical application
        ]
    else:
        env["LINKFLAGS"] += [
            "/SUBSYSTEM:WINDOWS,5.1",  # graphical application, XP support
        ]

    env["CPPDEFINES"] += [
        "_WIN32",
    ]
    if bits == 64:
        env["CPPDEFINES"] += [
            "_WIN64",
        ]

    # fpu control
    if int(get_env("NO_SSE", 0)):
        env["CCFLAGS"] += [
            "/fp:precise",  # precise FP
        ]
        if bits == 32:
            env["CCFLAGS"] += [
                "/arch:IA32",  # no sse, x87 fpu
            ]
    else:
        env["CCFLAGS"] += [
            "/fp:strict",  # strict FP
        ]
        if bits == 32 and cmp_version(ccversion, "14.0") < 0:
            env["CCFLAGS"] += [
                "/arch:SSE2",  # sse2
            ]

    # strict c/cpp warnings
    if int(get_env("LESS_WARNINGS", 0)):
        env["CCFLAGS"] += [
            "/W2",
        ]
    else:
        env["CCFLAGS"] += [
            "/W4",
            "/we 4013",
            "/we 4024",
            "/we 4026",
            "/we 4028",
            "/we 4029",
            "/we 4033",
            "/we 4047",
            "/we 4053",
            "/we 4087",
            "/we 4098",
            "/we 4245",
            "/we 4305",
            "/we 4700",
        ]
    if int(get_env("MORE_WARNINGS", 0)):
        env["CCFLAGS"] += [
            "/Wall",
        ]
    else:
        env["CCFLAGS"] += [
            "/wd 4100",
            "/wd 4127",
            "/wd 4244",
            "/wd 4706",
            "/wd 4131",
            "/wd 4996",
        ]

    # c flags
    env["CFLAGS"] += [
        "/std:c17",
    ]

    # c++ flags
    env["CXXFLAGS"] += [
        "/std:c++17",
    ]

    env["LINKFLAGS"] += [
        #'/NODEFAULTLIB:LIBCMTD',
        #'/NODEFAULTLIB:MSVCRT',
    ]

# if using mingw as cross-compiler, statically link libgcc/libstdc++ to avoid missing dependency on SJLJ exception
# handling
if "mingw" in env["CC"] and host_plat != target_plat:
    env["LINKFLAGS"] += [
        "-static-libgcc",
        "-static-libstdc++",
    ]

if target_plat == "Darwin":
    env["CPPDEFINES"] += ["MACOS_X"]
    env["LINKFLAGS"] += [
        "-framework",
        "CoreFoundation",
        "-framework",
        "ApplicationServices",
    ]

# debug / release
if debug in (0, 2):
    # release or fastdebug
    if "gcc" in env["CC"] or "clang" in env["CC"]:
        env["CCFLAGS"] += [
            "-O2",  # O3 may not be best, due to cache size not being big enough for the amount of inlining performed
            "-fomit-frame-pointer",
        ]
        if debug == 0:
            # strip unused symbols
            # for macOS, we run the `strip` utility after linking
            if "gcc" in env["CC"]:
                env["LINKFLAGS"] += [
                    "-s",
                ]
    elif env["CC"] == "cl":
        env["CCFLAGS"] += [
            "/O2",  # maximise speed
            "/MD",  # multi-threaded runtime for DLLs
        ]
        env["LINKFLAGS"] += [
            "/OPT:REF",  # remove unreferenced functions/data
            "/STACK:32768",  # stack size
        ]

    if debug == 0:
        env["CPPDEFINES"] += [
            "NDEBUG",
        ]

if debug:
    if "gcc" in env["CC"] or "clang" in env["CC"]:
        env["CCFLAGS"] += [
            "-g3",
            #'-pg',
            #'-finstrument-functions',
            "-fno-omit-frame-pointer",
        ]
        if use_asan:
            env["CCFLAGS"] += [
                "-fsanitize=address",
            ]
        env["LINKFLAGS"] += [
            # 	'-pg',
        ]
        if use_asan:
            env["LINKFLAGS"] += [
                "-fsanitize=address",
            ]
    elif env["CC"] == "cl":
        env["CCFLAGS"] += [
            "/Od",  # disable optimisations
            "/Z7",  # emit debug information
            "/MDd",  # multi-threaded debug runtime for DLLs
        ]
        env["LINKFLAGS"] += [
            "/DEBUG",  # generate debug info
            "/OPT:ICF",  # enable COMDAT folding
            "/INCREMENTAL:NO",  # no incremental linking
        ]

    env["CPPDEFINES"] += [
        "_DEBUG",
    ]

if git_tag:
    env["CPPDEFINES"] += ['GIT_TAG=\\"' + git_tag + '\\"']

# override options
if target_plat != "Linux":
    # only have notification backend available for linux
    no_notify = 1
if target_plat == "Windows" and "mingw" in env["CC"]:
    # Dec 2017: cross-compiling with MinGW does not fully support std::future
    no_geoip = 1

if no_crashhandler:
    env["CPPDEFINES"] += [
        "NO_CRASHHANDLER",
    ]

if no_sql:
    env["CPPDEFINES"] += [
        "NO_SQL",
    ]

if no_notify:
    env["CPPDEFINES"] += [
        "NO_NOTIFY",
    ]

if no_geoip:
    env["CPPDEFINES"] += [
        "NO_GEOIP",
    ]

# build-time settings
env["CPPDEFINES"] += [
    'JAPP_COMPILER=\\"' + env["CC"] + " " + ccversion + '\\"',
    'ARCH_STRING=\\"' + arch + '\\"',
]

env["CPPPATH"] = [
    "#",
    ".." + os.sep + "game",
]
env["LIBPATH"] = ["#" + os.sep + "libs" + os.sep + target_plat + os.sep + str(bits) + os.sep]

if target_plat == "Windows":
    env["SHLIBSUFFIX"] = ".dll"
elif target_plat == "Linux":
    env["SHLIBSUFFIX"] = ".so"
elif target_plat == "Darwin":
    env["SHLIBSUFFIX"] = ".dylib"

# invoke the per-project scripts
projects = [
    "game",
    "cgame",
    "ui",
]

for project in [p for p in projects if not proj or p in proj.split(",")]:
    result = env.SConscript(
        os.path.join(project, "SConscript"),
        exports=[
            "arch",
            "bits",
            "configuration",
            "env",
            "no_crashhandler",
            "no_sql",
            "no_notify",
            "no_geoip",
            "host_plat",
            "target_plat",
        ],
    )
    # strip symbols
    # if not env.GetOption("clean")
    if host_plat == "Darwin" and "clang" in env["CC"]:
        for f in result:
            AddPostAction(result, Action("strip -x " + f.name))
