#
# JA++ SCons project file
# written by Raz0r
#
# options:
#	debug		generate debug information. value 2 also enables optimisations
#	force32		force 32 bit target when on 64 bit machine
#	no_sql		don't include any SQL dependencies
#
# example:
#	scons -Q debug=1 force32=1
#
# envvars:
#	MORE_WARNINGS	enable additional warnings
#	NO_SSE			disable SSE floating point instructions, force x87 fpu
#

debug = int( ARGUMENTS.get( 'debug', 0 ) )
force32 = int( ARGUMENTS.get( 'force32', 0 ) )
no_sql = int( ARGUMENTS.get( 'no_sql', 0 ) )

def cmp_version( v1, v2 ):
	def normalise( v ):
		import re
		return [int(x) for x in re.sub( r'(\.0+)*$', '', v ).split( '.' )]

	return cmp(
		normalise( v1 ),
		normalise( v2 )
	)

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
import os
env = Environment(
	TARGET_ARCH = arch,
	tools = ['gcc', 'g++', 'ar', 'as', 'gnulink'],
	ENV = { 'PATH' : os.environ['PATH'] }
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
colours['white'] = '\033[1;97m'
colours['cyan'] = '\033[96m'
colours['orange'] = '\033[33m'
colours['green'] = '\033[92m'
colours['end']  = '\033[0m'

# if the output is not a terminal, remove the colours
if not sys.stdout.isatty():
	for key in colours.keys():
		colours[key] = ''

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
import commands
if realcc == 'cl':
	# msvc
	ccversion = env['MSVC_VERSION']
elif realcc == 'gcc' or realcc == 'clang':
	status, ccrawversion = commands.getstatusoutput( realcc + ' -dumpversion' )
	ccversion = None if status else ccrawversion

# scons version
import SCons
sconsversion = SCons.__version__

# git revision
status, rawrevision = commands.getstatusoutput( 'git rev-parse --short HEAD' )
revision = None if status else rawrevision

if revision:
	status, dummy = commands.getstatusoutput( 'git diff-index --quiet HEAD' )
	if status:
		revision += '*'

# set job/thread count
if plat == 'Linux' or plat == 'Darwin':
	# works on recent mac/linux
	status, num_cores = commands.getstatusoutput( 'getconf _NPROCESSORS_ONLN' )
	if status != 0:
		# only works on linux
		status, num_cores = commands.getstatusoutput( 'cat /proc/cpuinfo | grep processor | wc -l' )
	env.SetOption( 'num_jobs', int(num_cores) if status == 0 else 1 )
elif plat == 'Windows':
	num_cores = int( os.environ['NUMBER_OF_PROCESSORS'] )
	env.SetOption( 'num_jobs', num_cores )

# notify the user of the build configuration
if not env.GetOption( 'clean' ):
	msg = 'Building for ' + plat + ' ' + str(bits) + ' bits (' \
		+ realcc + '/' + realcxx + ' ' + ccversion + ', '\
		+ 'python ' + platform.python_version() + ', '\
		+ 'scons ' + sconsversion \
		+ ')'
	if debug:
		msg += ', debug symbols'
	if debug == 0 or debug == 2:
		msg += ', optimised'
	msg += ', x87 fpu' if 'NO_SSE' in os.environ else ', SSE'
	if force32:
		msg += ', forcing 32 bit build'
	msg += ', ' + str(env.GetOption( 'num_jobs' )) + ' threads'
	#msg += '\n' + realcc + ' located at ' + commands.getoutput( 'where ' + realcc )
	#msg += '\npython located at ' + sys.executable
	#msg += '\nscons' + ' located at ' + commands.getoutput( 'where ' + 'scons' )
	if revision:
		msg += '\ngit revision: ' + revision
	print( msg )

# compiler switches
if realcc == 'gcc' or realcc == 'clang':
	env['CPPDEFINES'] = []
	env['CFLAGS'] = []
	env['CCFLAGS'] = []
	env['CXXFLAGS'] = []

	# c warnings
	env['CFLAGS'] += [
		'-Wdeclaration-after-statement',
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
		env['CCFLAGS'] += [
			'-Waggregate-return',
			'-Wbad-function-cast',
			'-Wcast-qual',
			'-Wfloat-equal',
			'-Wlong-long',
			'-Wshadow',
			'-Wsign-conversion',
			'-Wswitch-default',
			'-Wunreachable-code',
		]
		if not clangHack:
			env['CCFLAGS'] += [
				'-Wdouble-promotion',
				'-Wsuggest-attribute=const',
				'-Wunsuffixed-float-constants',
			]

	# gcc-specific warnings
	if realcc == 'gcc' and cmp_version( ccversion, '4.6' ) >= 0 and arch != 'arm':
		env['CCFLAGS'] += [
			'-Wlogical-op'
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
			'-masm=intel'
		]
		if 'NO_SSE' in os.environ:
			env['CCFLAGS'] += [
				'-mfpmath=387',
				'-mno-sse2',
				'-ffloat-store',
			]
			if realcc == 'gcc':
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
		'-fomit-frame-pointer',
	]

	# misc settings
	#if realcc == 'gcc' and cmp_version( ccversion, '4.9' ) >= 0:
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
		'-std=c++11',
	]

elif realcc == 'cl':
	# msvc
	env['CFLAGS'] = [
		'/TC',	# compile as c
	]
	env['CCFLAGS'] = [
		'/EHsc',				# exception handling
		'/nologo',				# remove watermark
	]

	env['LINKFLAGS'] = [
		'/ERRORREPORT:none',	# don't send error reports for internal linker errors
		'/NOLOGO',				# remove watermark
	]
	if bits == 64:
		env['LINKFLAGS'] += [
			'/SUBSYSTEM:WINDOWS',		# graphical application
		]
	else:
		env['LINKFLAGS'] += [
			'/SUBSYSTEM:WINDOWS,5.1',	# graphical application, XP support
		]

	env['CPPDEFINES'] = [
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
		'/NODEFAULTLIB:LIBCMTD',
		'/NODEFAULTLIB:MSVCRT',
	]

if plat == 'Darwin':
	env['CPPDEFINES'] += [ 'MACOS_X' ]

# debug / release
if debug == 0 or debug == 2:
	if realcc == 'gcc' or realcc == 'clang':
		env['CCFLAGS'] += [
			'-O2', # O3 may not be best, due to cache size not being big enough for the amount of inlining performed
		]
		if debug == 0 and realcc == 'gcc':
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
	if realcc == 'gcc' or realcc == 'clang':
		env['CCFLAGS'] += [
			'-ggdb3',
		]
	elif realcc == 'cl':
		env['CCFLAGS'] += [
			'/Od',		# disable optimisations
			'/Z7',		# emit debug information
		    '/MDd',		# multi-threaded debug runtime for DLLs
		]
		env['LINKFLAGS'] += [
			'/DEBUG',		# generate debug info
		]
		env['PDB'] = product_name + '.pdb'

	env['CPPDEFINES'] += [
		'_DEBUG',
	]

if revision:
	env['CPPDEFINES'] += [
		'REVISION=\\"' + revision + '\\"'
	]

if no_sql:
	env['CPPDEFINES'] += [
		'NO_SQL',
	]

env['CPPDEFINES'] += [
	'JAPP_COMPILER=\\"' + realcc + ' ' + ccversion + '\\"',
	'NO_CRASHHANDLER'
]

env['CPPPATH'] = [
	'#',
	'..' + os.sep + 'game',
]
env['LIBPATH'] = [
	'#' + os.sep + 'libs' + os.sep + plat + os.sep + str(bits) + os.sep
]

if debug == 1:
	configuration = 'debug'
elif debug == 2:
	configuration = 'optimised-debug'
else:
	configuration = 'release'

# invoke the per-project scripts
projects = [
	'game',
	'cgame',
	'ui'
]
for project in projects:
	env.SConscript(
		os.path.join( project, 'SConscript' ),
		exports = [
			'arch',
			'bits',
			'configuration',
			'env',
			'no_sql',
			'plat',
			'realcc',
		]
	)
