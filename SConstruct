#
# JA++ SCons project file
# written by Raz0r
#
# options:
#	debug			generate debug information. value 2 also enables optimisations
#	force32			force 32 bit target when on 64 bit machine
#	no_sql			don't include any SQL dependencies
#	no_notify		don't include libnotify etc dependencies
#	no_crashhandler	don't include the crash logger (x86 only)
#
# example:
#	scons -Q debug=1 force32=1
#
# envvars:
#	NO_SSE			disable SSE floating point instructions, force x87 fpu
#	MORE_WARNINGS	enable additional warnings
#

debug = int( ARGUMENTS.get( 'debug', 0 ) )
configuration = { 0: lambda x: 'release', 1: lambda x: 'debug', 2: lambda x: 'optimised-debug' }[debug](debug)
force32 = int( ARGUMENTS.get( 'force32', 0 ) )
no_sql = int( ARGUMENTS.get( 'no_sql', 0 ) )
no_notify = int( ARGUMENTS.get( 'no_notify', 0 ) )
no_crashhandler = int( ARGUMENTS.get( 'no_crashhandler', 0 ) )
toolStr = ARGUMENTS.get( 'tools', 'gcc,g++,ar,as,gnulink' )
tools = toolStr.split( ',' )
proj = ARGUMENTS.get( 'project', 'game,cgame,ui' )

# compare semantic versions (1.0.2 < 1.0.10 < 1.2.0)
def cmp_version( v1, v2 ):
	def normalise( v ):
		import re
		return [int(x) for x in re.sub( r'(\.0+)*$', '', v ).split( '.' )]

	return cmp(
		normalise( v1 ),
		normalise( v2 )
	)

def run_command( cmd ):
	import subprocess
	p = subprocess.Popen(
		cmd.split( ' ' ),
		stdout=subprocess.PIPE,
		stderr=subprocess.PIPE
	)
	out, err = p.communicate()
	out = out.strip('\n') # bah why is this necessary
	if err:
		print( 'run_command: ' + err )
	return 0 if not err else 1, out

import platform
plat = platform.system() # Windows or Linux
try:
	bits = int( platform.architecture()[0][:2] ) # 32 or 64
except( ValueError, TypeError ):
	bits = None
arch = None # platform-specific, set manually

# architecture settings, needed for binary names, also passed as a preprocessor definition
if force32:
	bits = 32
if bits == 32:
	if plat == 'Windows':
		arch = 'x86'
	elif plat == 'Linux':
		if platform.machine()[:3] == 'arm':
			arch = 'arm'
		else:
			arch = 'i386'
	elif plat == 'Darwin':
		arch = 'i386'
	else:
		raise Exception( 'unexpected platform: ' + plat )
elif bits == 64:
	if plat == 'Windows':
		arch = 'x64'
	elif plat == 'Linux':
		arch = 'x86_64'
	elif plat == 'Darwin':
		arch = 'x86_64'
	else:
		raise Exception( 'unexpected platform: ' + plat )
else:
	raise Exception( 'could not determine architecture width: ' + str(bits) )

clangHack = plat == 'Darwin'

# create the build environment
#FIXME: also consider LD, AS, AR in the toolset
import os
def get_env( key, default_value = None ):
	return default_value if key not in os.environ else os.environ[key]

env = Environment(
	TARGET_ARCH = arch,
	tools = tools,
	ENV = { 'TMP' : get_env( 'TMP' ), 'PATH' : get_env( 'PATH' ) } # import basic shell environment
)

# set the proper compiler name
realcc = ARGUMENTS.get( 'CC', None )
realcxx = ARGUMENTS.get( 'CXX', None )

if 'CC' in os.environ:
	if 'ccc' in os.environ['CC']:
		env['CC'] = os.environ['CC']
		env['CXX'] = os.environ['CXX']
		# running a scan-build
		if realcc is None or realcxx is None:
			raise Exception( 'please specify CC/CXX as such: scons CC=gcc CXX=g++' )
	else:
		# not a scan-build, do not allow inheriting CC/CXX from outside environment
		raise Exception( 'please specify CC/CXX as such: scons CC=gcc CXX=g++' )
elif realcc is None and realcxx is None:
	realcc = env['CC']
	realcxx = env['CXX']
	#FIXME: realcxx is "$CC" on Windows/MSVC

env['ENV'].update( x for x in os.environ.items() if x[0].startswith( 'CCC_' ) )

# prettify the compiler output
if 'TERM' in os.environ:
	env['ENV']['TERM'] = os.environ['TERM']
import sys
colours = {}
enableColours = sys.stdout.isatty()
colours['white'] = '\033[1;97m' if enableColours else ''
colours['cyan'] = '\033[96m' if enableColours else ''
colours['orange'] = '\033[33m' if enableColours else ''
colours['green'] = '\033[92m' if enableColours else ''
colours['end'] = '\033[0m' if enableColours else ''

env['SHCCCOMSTR'] = env['SHCXXCOMSTR'] = env['CCCOMSTR'] = env['CXXCOMSTR'] = \
	'%s compiling: %s$SOURCE%s' % (colours['cyan'], colours['white'], colours['end'])
env['ARCOMSTR'] = \
	'%s archiving: %s$TARGET%s' % (colours['orange'], colours['white'], colours['end'])
env['RANLIBCOMSTR'] = \
	'%s  indexing: %s$TARGET%s' % (colours['orange'], colours['white'], colours['end'])
env['ASCOMSTR'] = \
	'%sassembling: %s$TARGET%s' % (colours['orange'], colours['white'], colours['end'])
env['SHLINKCOMSTR'] = env['LINKCOMSTR'] = \
	'%s   linking: %s$TARGET%s' % (colours['green'], colours['white'], colours['end'])

# obtain the compiler version
if realcc == 'cl':
	# msvc
	ccversion = env['MSVC_VERSION']
elif 'gcc' in realcc or 'clang' in realcc:
	# probably gcc or clang
	status, ccrawversion = run_command( realcc + ' -dumpversion' )
	ccversion = None if status else ccrawversion

# scons version
import SCons
sconsversion = SCons.__version__

# git revision
status, rawrevision = run_command( 'git rev-parse --short HEAD' )
revision = None if status else rawrevision

if revision:
	status, dummy = run_command( 'git diff-index --quiet HEAD' )
	if status:
		revision += '*'

# set job/thread count
def GetNumCores():
	if plat == 'Linux' or plat == 'Darwin':
		# works on recent mac/linux
		status, num_cores = run_command( 'getconf _NPROCESSORS_ONLN' )
		if status == 0:
			return int(num_cores)

		# only works on linux
		status, num_cores = run_command( 'cat /proc/cpuinfo | grep processor | wc -l' )
		if status == 0:
			return int(num_cores)

		return 1;

	elif plat == 'Windows':
		# exists since at-least XP SP2
		return int( os.environ['NUMBER_OF_PROCESSORS'] )
env.SetOption( 'num_jobs', GetNumCores() )

# notify the user of the build configuration
if not env.GetOption( 'clean' ):
	# build tools
	msg = 'Building '
	if revision:
		msg += revision + ' '
	msg += 'for ' + plat + ' '
	if force32:
		msg += 'forced '
	msg += str(bits) + ' bits using ' + str(env.GetOption( 'num_jobs' )) + ' threads\n'\
		+ '\t' + realcc + '/' + realcxx + ': ' + ccversion + '\n'\
		+ '\tpython: ' + platform.python_version() + '\n'\
		+ '\tscons: ' + sconsversion + '\n'

	# build options
	msg += 'options:\n'\
		+ '\tconfiguration: ' + configuration + '\n'\
		+ '\tinstruction set: ' + arch\
			+ ((' with x87 fpu' if 'NO_SSE' in os.environ else ' with SSE') if arch != 'arm' else '') + '\n'\
		+ '\tsql support: ' + ('dis' if no_sql else 'en') + 'abled\n'\
		+ '\tnotify support: ' + ('dis' if no_notify else 'en') + 'abled\n'\
		+ '\tcrash logging: ' + ('dis' if no_crashhandler else 'en') + 'abled\n'

	# build environment
	if 'SCONS_DEBUG' in os.environ:
		msg += realcc + ' located at ' + run_command( 'where ' + realcc )[1].split( '\n' )[0] + '\n'
		if 'AR' in env:
			msg += env['AR'] + ' located at ' + run_command( 'where ' + env['AR'] )[1].split( '\n' )[0] + '\n'
		if 'AS' in env:
			msg += env['AS'] + ' located at ' + run_command( 'where ' + env['AS'] )[1].split( '\n' )[0] + '\n'
		msg += 'python located at ' + sys.executable + '\n'
		msg += 'scons' + ' located at ' + run_command( 'where ' + 'scons' )[1].split( '\n' )[0] + '\n'

	print( msg )

# clear default compiler/linker switches
def emptyEnv( env, e ):
	if 'SCONS_DEBUG' in os.environ:
		if e in env:
			if env[e]:
				print( 'discarding ' + e + ': ' + env[e] )
			else:
				print( 'env[' + e + '] is empty' )
		else:
			print( 'env[' + e + '] does not exist' )
	env[e] = []
emptyEnv( env, 'CPPDEFINES' )
emptyEnv( env, 'CFLAGS' )
emptyEnv( env, 'CCFLAGS' )
emptyEnv( env, 'CXXFLAGS' )
emptyEnv( env, 'LINKFLAGS' )
emptyEnv( env, 'ARFLAGS' )
emptyEnv( env, 'LIBS' )

# compiler switches
if 'gcc' in realcc or 'clang' in realcc:
	env['CCFLAGS'] += [
		#'-M',	# show include hierarchy
	]
	# c warnings
	env['CFLAGS'] += [
		#'-Wdeclaration-after-statement',
		'-Wnested-externs',
		'-Wold-style-definition',
		'-Wstrict-prototypes',
	]

	# c/cpp warnings
	env['CCFLAGS'] += [
		'-Wall',
		'-Wextra',
		'-Wno-missing-braces',
		'-Wno-missing-field-initializers',
		'-Wno-sign-compare',
		'-Wno-unused-parameter',
		'-Winit-self',
		'-Winline',
		'-Wmissing-include-dirs',
		'-Woverlength-strings',
		'-Wpointer-arith',
		'-Wredundant-decls',
		'-Wundef',
		'-Wuninitialized',
		'-Wwrite-strings',
	]

	# strict c/cpp warnings
	if 'MORE_WARNINGS' in os.environ:
		env['CFLAGS'] += [
			'-Wbad-function-cast',
		]
		env['CCFLAGS'] += [
			'-Waggregate-return',
			'-Wcast-qual',
			#'-Wfloat-equal',
			#'-Wlong-long',
			'-Wshadow',
			#'-Wsign-conversion',
			'-Wswitch-default',
			'-Wunreachable-code',
		]
		if not clangHack:
			env['CFLAGS'] += [
				'-Wunsuffixed-float-constants',
			]
			env['CCFLAGS'] += [
				'-Wdouble-promotion',
				#'-Wsuggest-attribute=const',
			]

	# gcc-specific warnings
	if 'gcc' in realcc and cmp_version( ccversion, '4.6' ) >= 0 and arch != 'arm':
		env['CCFLAGS'] += [
			'-Wlogical-op',
		]

		# requires gcc 4.7 or above
		if cmp_version( ccversion, '4.7' ) >= 0:
			env['CCFLAGS'] += [
				'-Wstack-usage=32768',
			]

	# disable warnings
	env['CCFLAGS'] += [
		'-Wno-char-subscripts',
	]

	# c/cpp flags
	if arch == 'arm':
		env['CCFLAGS'] += [
			'-fsigned-char',
		]
	else:
		env['CCFLAGS'] += [
			'-mstackrealign',
			'-masm=intel',
		]
		env['ASFLAGS'] += [
			'-msyntax=intel',
			'-mmnemonic=intel',
		]
		if 'NO_SSE' in os.environ:
			env['CCFLAGS'] += [
				'-mfpmath=387',
				'-mno-sse2',
				'-ffloat-store',
			]
			if 'gcc' in realcc:
				env['CFLAGS'] += [
					'-fexcess-precision=standard',
				]
		else:
			env['CCFLAGS'] += [
				'-mfpmath=sse',
				'-msse2',
			]
		if arch == 'i386':
			env['CCFLAGS'] += [
				'-march=i686',
			]
		elif arch == 'x86_64':
			env['CCFLAGS'] += [
				'-mtune=generic',
			]

		if bits == 32:
			env['CCFLAGS'] += [
				'-m32',
			]
			env['LINKFLAGS'] += [
				'-m32',
			]
	env['CCFLAGS'] += [
		'-fvisibility=hidden',
	]

	# misc settings
	#if 'gcc' in realcc and cmp_version( ccversion, '4.9' ) >= 0:
	#	env['CCFLAGS'] += [
	#		'-fdiagnostics-color',
	#	]

	# c flags
	env['CFLAGS'] += [
		'-std=gnu99',
	]

	# c++ flags
	env['CXXFLAGS'] += [
		'-fvisibility-inlines-hidden',
		'-std=c++14',
	]

	# archive flags
	env['ARFLAGS'] = 'rc'

elif realcc == 'cl':
	# msvc
	env['CCFLAGS'] += [
		#'/showIncludes',
	]
	env['CFLAGS'] += [
		'/TC',	# compile as c
	]
	env['CXXFLAGS'] += [
		'/TP',	# compile as c++
	]
	env['CCFLAGS'] += [
		'/EHsc',	# exception handling
		'/nologo',	# remove watermark
	]

	env['LINKFLAGS'] += [
		'/ERRORREPORT:none',	# don't send error reports for internal linker errors
		'/NOLOGO',				# remove watermark
		'/MACHINE:' + arch,		# 32/64 bit linking
	]
	if bits == 64:
		env['LINKFLAGS'] += [
			'/SUBSYSTEM:WINDOWS',		# graphical application
		]
	else:
		env['LINKFLAGS'] += [
			'/SUBSYSTEM:WINDOWS,5.1',	# graphical application, XP support
		]

	env['CPPDEFINES'] += [
		'_WIN32',
	]
	if bits == 64:
		env['CPPDEFINES'] += [
			'_WIN64',
		]

	# fpu control
	if 'NO_SSE' in os.environ:
		env['CCFLAGS'] += [
			'/fp:precise',	# precise FP
		]
		if bits == 32:
			env['CCFLAGS'] += [
				'/arch:IA32',	# no sse, x87 fpu
			]
	else:
		env['CCFLAGS'] += [
			'/fp:strict',	# strict FP
		]
		if bits == 32 and cmp_version( ccversion, '14.0' ) < 0:
			env['CCFLAGS'] += [
				'/arch:SSE2',	# sse2
			]

	# strict c/cpp warnings
	if 'LESS_WARNINGS' in os.environ:
		env['CCFLAGS'] += [
			'/W2',
		]
	else:
		env['CCFLAGS'] += [
			'/W4',
			'/we 4013',
			'/we 4024',
			'/we 4026',
			'/we 4028',
			'/we 4029',
			'/we 4033',
			'/we 4047',
			'/we 4053',
			'/we 4087',
			'/we 4098',
			'/we 4245',
			'/we 4305',
			'/we 4700',
		]
	if 'MORE_WARNINGS' in os.environ:
		env['CCFLAGS'] += [
			'/Wall',
		]
	else:
		env['CCFLAGS'] += [
			'/wd 4100',
			'/wd 4127',
			'/wd 4244',
			'/wd 4706',
			'/wd 4131',
			'/wd 4996',
		]

	env['LINKFLAGS'] += [
		#'/NODEFAULTLIB:LIBCMTD',
		#'/NODEFAULTLIB:MSVCRT',
	]

if plat == 'Darwin':
	env['CPPDEFINES'] += [ 'MACOS_X' ]
	env['LINKFLAGS'] += [
		'-framework', 'CoreFoundation',
		'-framework', 'ApplicationServices'
	]

# debug / release
if debug == 0 or debug == 2:
	if 'gcc' in realcc or 'clang' in realcc:
		env['CCFLAGS'] += [
			'-O2', # O3 may not be best, due to cache size not being big enough for the amount of inlining performed
			'-fomit-frame-pointer',
		]
		if debug == 0 and 'gcc' in realcc:
			env['LINKFLAGS'] += [
				'-s',	# strip unused symbols
			]
	elif realcc == 'cl':
		env['CCFLAGS'] += [
			'/O2',	# maximise speed
			'/MD',	# multi-threaded runtime for DLLs
		]
		env['LINKFLAGS'] += [
			'/OPT:REF',			# remove unreferenced functions/data
			'/STACK:32768',		# stack size
		]

	if debug == 0:
		env['CPPDEFINES'] += [
			'NDEBUG',
		]

if debug:
	if 'gcc' in realcc or 'clang' in realcc:
		env['CCFLAGS'] += [
			'-g3',
			#'-pg',
			#'-finstrument-functions',
			'-fno-omit-frame-pointer',
			#'-fsanitize=address',
		]
		#env['LINKFLAGS'] += [
		#	'-pg',
		#	'-fsanitize=address',
		#]
	elif realcc == 'cl':
		env['CCFLAGS'] += [
			'/Od',		# disable optimisations
			'/Z7',		# emit debug information
			'/MDd',		# multi-threaded debug runtime for DLLs
		]
		env['LINKFLAGS'] += [
			'/DEBUG',			# generate debug info
			'/OPT:ICF',			# enable COMDAT folding
			'/INCREMENTAL:NO',	# no incremental linking
		]

	env['CPPDEFINES'] += [
		'_DEBUG',
	]

if revision:
	env['CPPDEFINES'] += [
		'REVISION=\\"' + revision + '\\"'
	]

if no_crashhandler:
	env['CPPDEFINES'] += [
		'NO_CRASHHANDLER',
	]

if no_sql:
	env['CPPDEFINES'] += [
		'NO_SQL',
	]

if no_notify:
	env['CPPDEFINES'] += [
		'NO_NOTIFY',
	]

# build-time settings
env['CPPDEFINES'] += [
	'JAPP_COMPILER=\\"' + realcc + ' ' + ccversion + '\\"',
	'ARCH_STRING=\\"' + arch + '\\"',
]

env['CPPPATH'] = [
	'#',
	'..' + os.sep + 'game',
]
env['LIBPATH'] = [
	'#' + os.sep + 'libs' + os.sep + plat + os.sep + realcc + os.sep + str(bits) + os.sep
]

# invoke the per-project scripts
projects = [
	'game',
	'cgame',
	'ui',
]

for project in [p for p in projects if not proj or p in proj.split(',')]:
	env.SConscript(
		os.path.join( project, 'SConscript' ),
		exports = [
			'arch',
			'bits',
			'configuration',
			'env',
			'no_crashhandler',
			'no_sql',
			'no_notify',
			'plat',
			'realcc',
		]
	)
