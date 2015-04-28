#
# JA++ SCons project file
# written by Raz0r
#
# options:
#	debug		generate debug information. value 2 also enables optimisations
#	force32		force 32 bit target when on 64 bit machine
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
elif bits == 64:
	if plat == 'Windows':
		arch = 'x64'
	else:
		arch = 'x86_64'

clangHack = plat == 'Darwin'

# fatal error if the cpu architecture can't be detected
if arch is None:
	raise Exception( 'could not determine architecture' )
if bits is None:
	raise Exception( 'could not determine architecture width' )

# create the build environment
import os
env = Environment( TARGET_ARCH = arch )
env['CC'] = os.getenv( 'CC' ) or env[ 'CC' ]
env['CXX'] = os.getenv( 'CXX' ) or env[ 'CXX' ]
env['ENV'].update( x for x in os.environ.items() if x[0].startswith( 'CCC_' ) )
if 'TERM' in os.environ:
	env['ENV']['TERM'] = os.environ['TERM']

# prettify the compiler output
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
if plat == 'Windows':
	ccversion = env['MSVC_VERSION']
else:
	status, ccrawversion = commands.getstatusoutput( env['CC'] + ' -dumpversion' )
	ccversion = None if status else ccrawversion

# git revision
status, rawrevision = commands.getstatusoutput( 'git rev-parse --short HEAD' )
revision = None if status else rawrevision

# set job/thread count
if plat == 'Linux' or plat == 'Darwin':
	# works on recent mac/linux
	status, num_cores = commands.getstatusoutput( 'getconf _NPROCESSORS_ONLN' )
	if status != 0:
		# only works on linux
		status, num_cores = commands.getstatusoutput( 'cat /proc/cpuinfo | grep processor | wc -l' )
	env.SetOption( 'num_jobs', int(num_cores) * 3 if status == 0 else 1 )
elif plat == 'Windows':
	num_cores = int( os.environ['NUMBER_OF_PROCESSORS'] )
	env.SetOption( 'num_jobs', num_cores * 3 )

# notify the user of the build configuration
if not env.GetOption( 'clean' ):
	msg = 'Building for ' + plat + ' ' + str(bits) + ' bits (' + env['CC'] + ' ' + ccversion + ', python ' + platform.python_version() + ')'
	if debug:
		msg += ', debug symbols'
	if debug == 0 or debug == 2:
		msg += ', optimised'
	msg += ', x87 fpu' if 'NO_SSE' in os.environ else ', SSE'
	if force32:
		msg += ', forcing 32 bit build'
	msg += ', ' + str(env.GetOption( 'num_jobs' )) + ' threads'
	if revision:
		msg += '\ngit revision: ' + revision
	print( msg )

# compiler switches
if plat == 'Linux' or plat == 'Darwin':
	env['CPPDEFINES'] = []
	env['CFLAGS'] = []
	env['CCFLAGS'] = []
	env['CXXFLAGS'] = []

	# c warnings
	env['CFLAGS'] += [ '-Wdeclaration-after-statement', '-Wnested-externs', '-Wold-style-definition',
		'-Wstrict-prototypes'
	]

	# c/cpp warnings
	env['CCFLAGS'] += [ '-Wall', '-Wextra', '-Wno-missing-braces', '-Wno-missing-field-initializers',
		'-Wno-sign-compare', '-Wno-unused-parameter', '-Winit-self', '-Winline', '-Wmissing-include-dirs',
		'-Woverlength-strings', '-Wpointer-arith', '-Wredundant-decls', '-Wundef', '-Wuninitialized', '-Wwrite-strings'
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
			'-Wunreachable-code'
		]
		if not clangHack:
			env['CCFLAGS'] += [
				'-Wdouble-promotion',
				'-Wsuggest-attribute=const',
				'-Wunsuffixed-float-constants'
			]

	# gcc-specific warnings
	if env['CC'] == 'gcc' and arch != 'arm' and not clangHack:
		env['CCFLAGS'] += [ '-Wlogical-op' ]

		# requires gcc 4.7 or above
		if cmp_version( ccversion, '4.7' ) >= 0:
			env['CCFLAGS'] += [ '-Wstack-usage=32768' ]

	# disable warnings
	env['CCFLAGS'] += [ '-Wno-char-subscripts' ]

	# c/cpp flags
	if arch == 'arm':
		env['CCFLAGS'] += [ '-fsigned-char' ]
	else:
		env['CCFLAGS'] += [ '-mstackrealign' ]
		if 'NO_SSE' in os.environ:
			env['CCFLAGS'] += [ '-mfpmath=387', '-mno-sse2', '-ffloat-store' ]
			if env['CC'] == 'gcc':
				env['CCFLAGS'] += [ '-fexcess-precision=standard' ]
		else:
			env['CCFLAGS'] += [ '-mfpmath=sse', '-msse2' ]
		if arch == 'i386':
			env['CCFLAGS'] += [ '-m32', '-march=i686' ]
			env['LINKFLAGS'] += [ '-m32' ]
		elif arch == 'x86_64':
			env['CCFLAGS'] += [ '-mtune=generic' ]
	env['CCFLAGS'] += [ '-fvisibility=default', '-fomit-frame-pointer' ]

	# c flags
	env['CFLAGS'] += [ '-std=gnu99' ]

	# c++ flags
	env['CXXFLAGS'] += [ '-fvisibility-inlines-hidden', '-std=c++11' ]

elif plat == 'Windows':
	# assume msvc
	env['CFLAGS'] = [ '/TC' ] # compile as c
	env['CCFLAGS'] = [
		'/EHsc',				# exception handling
	#	'/errorReport:none',	# don't send error reports for internal compiler errors
	#	'/FC',					# display full path of source code in error messages
	#	'/Gd',					# use cdecl calling convention
	#	'/GS',					# buffer security check
		'/nologo',				# remove watermark
	]

	env['LINKFLAGS'] = [
		'/ERRORREPORT:none',	# don't send error reports for internal linker errors
	#	'/MACHINE:' + arch,		# set the linker architecture
		'/NOLOGO',				# remove watermark
	]
	if bits == 64:
		env['LINKFLAGS'] += [ '/SUBSYSTEM:WINDOWS' ]	# graphical application
	else:
		env['LINKFLAGS'] += [ '/SUBSYSTEM:WINDOWS,5.1' ]	# graphical application

	env['CPPDEFINES'] = [ '_WIN32' ]
	if bits == 64:
		env['CPPDEFINES'] += [ '_WIN64' ]

	# multi-processor compilation
	if num_cores > 1:
		env['CCFLAGS'] += [
		#	'/FS',							# force synchronous writes to PDB file
		#	'/cgthreads' + str(num_cores),	# compiler threads to use for optimisation and code generation
		]
		env['LINKFLAGS'] += [
		#	'/CGTHREADS:' + str(num_cores),	# linker threads to use for optimisations and code generation
		]

	# fpu control
	if 'NO_SSE' in os.environ:
		env['CCFLAGS'] += [ '/fp:precise' ] # precise FP
		if bits == 32:
			env['CCFLAGS'] += [ '/arch:IA32' ] # no sse, x87 fpu
	else:
		env['CCFLAGS'] += [ '/fp:strict' ] # strict FP
		if bits == 32:
			env['CCFLAGS'] += [ '/arch:SSE2' ] # sse2

	# strict c/cpp warnings
	if 'LESS_WARNINGS' in os.environ:
		env['CPPDEFINES'] += [ '/W2' ]
	else:
		env['CPPDEFINES'] += [
			'/W4',
			'/Wall',
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
			'/we 4700'
		]
	if 'MORE_WARNINGS' not in os.environ:
		env['CPPDEFINES'] += [
			'/wd 4100',
			'/wd 4127',
			'/wd 4244',
			'/wd 4706',
			'/wd 4131',
			'/wd 4996'
		]

	env['LINKFLAGS'] += [
		'/NODEFAULTLIB:LIBCMTD',
		'/NODEFAULTLIB:MSVCRT',
	]

if plat == 'Darwin':
	env['CPPDEFINES'] += [ 'MACOS_X' ]

# debug / release
if debug == 0 or debug == 2:
	if plat == 'Linux' or plat == 'Darwin':
		env['CCFLAGS'] += [ '-O3' ]
		if debug == 0 and not clangHack:
			env['LINKFLAGS'] += [ '-s' ]
	elif plat == 'Windows':
		env['CCFLAGS'] += [
		#	'/c',	# compile without linking
		#	'/GL',	# whole program optimisation
		#	'/Gw',	# optimise global data
		#	'/MP',	# multiple process compilation
			'/O2',	# maximise speed
		]
		env['LINKFLAGS'] += [
		#	'/INCREMENTAL:NO',	# don't incrementally link
		#	'/LTCG',			# link-time code generation
			'/OPT:REF',			# remove unreferenced functions/data
			'/STACK:32768',		# stack size
		]

	if debug == 0:
		env['CPPDEFINES'] += [ 'NDEBUG' ]

if debug:
	if plat == 'Linux' or plat == 'Darwin':
		env['CCFLAGS'] += [ '-g3' ]
	elif plat == 'Windows':
		env['CCFLAGS'] += [
		#	'/GF',		# string pooling
		#	'/Gy',		# function level linking
			'/Od',		# disable optimisations
		#	'/Oy-',		# disable frame pointer omission
		#	'/RTC1',	# runtime checks
			'/Z7',		# emit debug information
		    '/MDd',
		]
		env['LINKFLAGS'] += [
			'/DEBUG',		# generate debug info
		#	'/INCREMENTAL',	# incrementally link
		]

	env['CPPDEFINES'] += [ '_DEBUG' ]

if revision:
	env['CPPDEFINES'] += [ 'REVISION=\\"' + revision + '\\"' ]

env['CPPDEFINES'] += [ 'SCONS_BUILD' ]
env['CPPPATH'] = [ '#', '../game' ]
env['LIBPATH'] = [ '#/libs/' + plat + '/' + str(bits) + '/' ]

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
		exports = [ 'arch', 'bits', 'configuration', 'env', 'plat' ]
	)
