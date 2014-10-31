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
#	MORE_WARNINGS	enable additional warnings (gcc/clang only)
#	NO_SSE			disable SSE floating point instructions, force x87 fpu
#

debug = int( ARGUMENTS.get( 'debug', 0 ) )
force32 = int( ARGUMENTS.get( 'force32', False ) )

def cmp_version( v1, v2 ):
	def normalise( v ):
		import re
		return [int(x) for x in re.sub( r'(\.0+)*$', '', v ).split( '.' )]
	return cmp( normalise( v1 ), normalise( v2 ) )

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
elif bits == 64:
	arch = 'x86_64'

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
if plat != 'Windows':
	env['ENV']['TERM'] = os.environ['TERM']

# prettify the compiler output
import sys
def cc_output( s, target, src, env ):
	if len( src ) > 1:
		sys.stdout.write( '  --> %s\n' % (''.join([str(x) for x in target])) )
	else:
		sys.stdout.write( '  compiling %s\n' % (''.join([str(x) for x in src])) )
env['PRINT_CMD_LINE_FUNC'] = cc_output

# obtain the compiler version
import commands
if plat == 'Windows':
	ccversion = env['MSVC_VERSION']
else:
	status, ccrawversion = commands.getstatusoutput( env['CC'] + ' -dumpversion' )
	ccversion = None if status else ccrawversion

# git revision
status, rawrevision = commands.getstatusoutput( 'git rev-parse HEAD' )
revision = None if status else rawrevision

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
	if revision:
		msg += '\ngit revision: ' + revision
	print( msg )

# compiler switches
if plat == 'Linux':
	env['CPPDEFINES'] = []
	env['CFLAGS'] = []
	env['CCFLAGS'] = []
	env['CXXFLAGS'] = []

	# set job/thread count
	#status, res = commands.getstatusoutput( 'cat /proc/cpuinfo | grep processor | wc -l' )
	#env.SetOption( 'num_jobs', res * 2 if status == 0 else 1 )

	# c warnings
	env['CFLAGS'] += [ '-Wdeclaration-after-statement', '-Wnested-externs', '-Wold-style-definition',
		'-Wstrict-prototypes'
	]

	# c/cpp warnings
	env['CCFLAGS'] += [ '-Wall', '-Wextra', '-Wno-missing-braces', '-Wno-missing-field-initializers',
		'-Wno-sign-compare', '-Wno-unused-parameter', '-Waggregate-return', '-Winit-self', '-Winline',
		'-Wmissing-include-dirs', '-Woverlength-strings', '-Wpointer-arith', '-Wredundant-decls', '-Wundef',
		'-Wuninitialized', '-Wwrite-strings'
	]

	# strict c/cpp warnings
	if 'MORE_WARNINGS' in os.environ:
		env['CCFLAGS'] += [ '-Wbad-function-cast', '-Wcast-qual', '-Wdouble-promotion', '-Wfloat-equal', '-Wlong-long',
			'-Wshadow', '-Wsign-conversion', '-Wsuggest-attribute=const', '-Wswitch-default', '-Wunreachable-code',
			'-Wunsuffixed-float-constants'
		]

	# gcc-specific warnings
	if env['CC'] == 'gcc' and arch != 'arm':
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
	env['CCFLAGS'] += [ '-fvisibility=hidden', '-fomit-frame-pointer' ]

	# c flags
	env['CFLAGS'] += [ '-std=gnu99' ]

	# c++ flags
	env['CXXFLAGS'] += [ '-fvisibility-inlines-hidden', '-std=c++11' ]

elif plat == 'Windows':
	# assume msvc
	env['CCFLAGS'] = [ '/nologo', '/WX-', '/GS', '/fp:precise', '/Zc:wchar_t', '/Zc:forScope', '/Gd', '/GF',
		'/TC', '/errorReport:prompt', '/EHs', '/EHc', '/Ot', '/MP', '/c'
	]
	env['LINKFLAGS'] = [ '/SUBSYSTEM:WINDOWS', '/MACHINE:'+arch, '/LTCG' ]
	env['CPPDEFINES'] = [ '_WINDLL', '_MSC_EXTENSIONS', '_INTEGRAL_MAX_BITS=64', '_WIN32', '_MT', '_DLL',
		'_M_FP_PRECISE'
	]
	if bits == 32:
		env['CCFLAGS'] += [ '/Zp8', '/Gs', '/Oy-' ]
		env['CPPDEFINES'] += [ '_M_IX86=600' ]
		if 'NO_SSE' in os.environ:
			env['CPPDEFINES'] += [ '_M_IX86_FP=0' ]
			if cmp_version( ccversion, '11.0' ) >= 0:
				env['CCFLAGS'] += [ '/arch:IA32' ]
		else:
			env['CPPDEFINES'] += [ '_M_IX86_FP=2' ]
			env['CCFLAGS'] += [ '/arch:SSE2' ]
	elif bits == 64:
		env['CCFLAGS'] += [ '/Zp16' ]
		env['CPPDEFINES'] += [ '_M_AMD64=100', '_M_X64=100', '_WIN64' ]

	# strict c/cpp warnings
	if 'MORE_WARNINGS' in os.environ:
		env['CPPDEFINES'] += [ '/W4', '/Wall' ]
	else:
		env['CPPDEFINES'] += [ '/W3', '/wd4996' ]

# debug / release
if debug == 0 or debug == 2:
	if plat == 'Linux':
		env['CCFLAGS'] += [ '-O3' ]
		if debug == 0:
			env['LINKFLAGS'] += [ '-s' ]
	elif plat == 'Windows':
		env['CCFLAGS'] += [ '/GL', '/Gm-', '/MD', '/O2', '/Oi' ]
		if bits == 64:
			env['CCFLAGS'] += [ '/Oy' ]
	if debug == 0:
		env['CPPDEFINES'] += [ 'NDEBUG' ]
if debug:
	if plat == 'Linux':
		env['CCFLAGS'] += [ '-g3' ]
	elif plat == 'Windows':
		env['CPPDEFINES'] += [ '__MSVC_RUNTIME_CHECKS' ]
		env['CCFLAGS'] += [ '/Gm', '/FD', '/MDd', '/Od', '/RTC1', '/RTCs', '/RTCu' ]
		if bits == 32:
			env['CCFLAGS'] += [ '/FC', '/ZI' ]
	env['CPPDEFINES'] += [ '_DEBUG' ]

if revision:
	env['CPPDEFINES'] += [ 'REVISION=\\"' + revision + '\\"' ]

env['CPPDEFINES'] += [ 'SCONS_BUILD' ]
env['CPPPATH'] = [ '#', '../game' ]

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
