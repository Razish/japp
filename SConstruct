#
# JA++ SCons project file
# written by Raz0r
#
# projects:
#	game
#	cgame
#	ui
#
# options:
#	debug		generate debug information. value 2 also enables optimisations
#	force32		force 32 bit target when on 64 bit machine
#	compiler=?	use an alternate compiler
#	analyse		run static analysis
#
# example:
#	scons project=game debug=1 force32=1 compiler=clang
#

import platform
import os
import commands

plat = platform.system() # Windows or Linux
try:
	bits = int( platform.architecture()[0][:2] ) # 32 or 64
except( ValueError, TypeError ):
	bits = None
arch = None # platform-specific, set manually

print( 'Configuring build environment...' )

analyse = int( ARGUMENTS.get( 'analyse', 0 ) )
compiler = ARGUMENTS.get( 'compiler', 'gcc' )
debug = int( ARGUMENTS.get( 'debug', 0 ) )
force32 = int( ARGUMENTS.get( 'force32', 0 ) )
project = ARGUMENTS.get( 'project', '' )

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
	#TODO: Mac
elif bits == 64:
	arch = 'x86_64'

# Notify the user of the build configuration
print( 'Building for ' + plat + ' (' + str(bits) + ' bits, treated as \'' + arch + '\')' )
if debug:
	print( 'With debugging symbols' )
if not debug or debug == 2:
	print( 'Optimisation enabled' )
if force32:
	print( 'Forcing 32 bit compile' )
if analyse:
	print( 'WARNING: Running static analysis mode. Will not produce binaries' )
print( '' )

env = Environment( TARGET_ARCH = arch )
if plat != 'Windows':
	env['ENV']['TERM'] = os.environ['TERM']

files = {}
libs = {}

files['lua'] = [
	'lua/lapi.c',
	'lua/lauxlib.c',
	'lua/lbaselib.c',
	'lua/lbitlib.c',
	'lua/lcode.c',
	'lua/lcorolib.c',
	'lua/lctype.c',
	'lua/ldblib.c',
	'lua/ldebug.c',
	'lua/ldo.c',
	'lua/ldump.c',
	'lua/lfunc.c',
	'lua/lgc.c',
	'lua/linit.c',
	'lua/liolib.c',
	'lua/llex.c',
	'lua/lmathlib.c',
	'lua/lmem.c',
	'lua/loadlib.c',
	'lua/lobject.c',
	'lua/lopcodes.c',
	'lua/loslib.c',
	'lua/lparser.c',
	'lua/lstate.c',
	'lua/lstring.c',
	'lua/lstrlib.c',
	'lua/ltable.c',
	'lua/ltablib.c',
	'lua/ltm.c',
	'lua/lua.c',
	'lua/lundump.c',
	'lua/lvm.c',
	'lua/lzio.c'
	]

files['udis86'] = [
	'libudis86/decode.c',
	'libudis86/input.c',
	'libudis86/itab.c',
	'libudis86/syn-att.c',
	'libudis86/syn-intel.c',
	'libudis86/syn.c',
	'libudis86/udis86.c'
	] if bits == 32 else []

files['game'] = [
	'qcommon/q_math.c',
	'qcommon/q_shared.c',
	'JAPP/jp_crash.c',
	'JAPP/jp_promode.c',
	'JAPP/jp_stack.c',
	'JAPP/jp_tokenparser.c',
	'json/cJSON.c',
	'game/ai_main.c',
	'game/ai_util.c',
	'game/ai_wpnav.c',
	'game/AnimalNPC.c',
	'game/bg_g2_utils.c',
	'game/bg_lua.c',
	'game/bg_luacvar.c',
	'game/bg_luaevent.c',
	'game/bg_lualogger.c',
	'game/bg_luaplayer.c',
	'game/bg_luaserialiser.c',
	'game/bg_luavector.c',
	'game/bg_misc.c',
	'game/bg_panimate.c',
	'game/bg_pmove.c',
	'game/bg_saber.c',
	'game/bg_saberLoad.c',
	'game/bg_saga.c',
	'game/bg_slidemove.c',
	'game/bg_vehicleLoad.c',
	'game/bg_weapons.c',
	'game/FighterNPC.c',
	'game/g_active.c',
	'game/g_admin.c',
	'game/g_arenas.c',
	'game/g_bot.c',
	'game/g_chatFilters.c',
	'game/g_client.c',
	'game/g_clientModification.c',
	'game/g_cmds.c',
	'game/g_combat.c',
	'game/g_exphysics.c',
	'game/g_ICARUScb.c',
	'game/g_items.c',
	'game/g_log.c',
	'game/g_main.c',
	'game/g_mem.c',
	'game/g_misc.c',
	'game/g_missile.c',
	'game/g_mover.c',
	'game/g_nav.c',
	'game/g_navnew.c',
	'game/g_object.c',
	'game/g_playerbans.c',
	'game/g_saga.c',
	'game/g_session.c',
	'game/g_spawn.c',
	'game/g_svcmds.c',
	'game/g_syscalls.c',
	'game/g_target.c',
	'game/g_team.c',
	'game/g_timer.c',
	'game/g_trigger.c',
	'game/g_turret.c',
	'game/g_turret_G2.c',
	'game/g_unlagged.c',
	'game/g_utils.c',
	'game/g_vehicles.c',
	'game/g_vehicleTurret.c',
	'game/g_weapon.c',
	'game/NPC.c',
	'game/NPC_AI_Atst.c',
	'game/NPC_AI_Default.c',
	'game/NPC_AI_Droid.c',
	'game/NPC_AI_GalakMech.c',
	'game/NPC_AI_Grenadier.c',
	'game/NPC_AI_Howler.c',
	'game/NPC_AI_ImperialProbe.c',
	'game/NPC_AI_Interrogator.c',
	'game/NPC_AI_Jedi.c',
	'game/NPC_AI_Mark1.c',
	'game/NPC_AI_Mark2.c',
	'game/NPC_AI_MineMonster.c',
	'game/NPC_AI_Rancor.c',
	'game/NPC_AI_Remote.c',
	'game/NPC_AI_Seeker.c',
	'game/NPC_AI_Sentry.c',
	'game/NPC_AI_Sniper.c',
	'game/NPC_AI_Stormtrooper.c',
	'game/NPC_AI_Utils.c',
	'game/NPC_AI_Wampa.c',
	'game/NPC_behavior.c',
	'game/NPC_combat.c',
	'game/NPC_goal.c',
	'game/NPC_misc.c',
	'game/NPC_move.c',
	'game/NPC_reactions.c',
	'game/NPC_senses.c',
	'game/NPC_sounds.c',
	'game/NPC_spawn.c',
	'game/NPC_stats.c',
	'game/NPC_utils.c',
	'game/SpeederNPC.c',
	'game/tri_coll_test.c',
	'game/w_force.c',
	'game/w_saber.c',
	'game/WalkerNPC.c'
	] + files['lua'] + files['udis86']

files['cgame'] = [
	'qcommon/q_math.c',
	'qcommon/q_shared.c',
	'game/AnimalNPC.c',
	'game/bg_g2_utils.c',
	'game/bg_lua.c',
	'game/bg_luacvar.c',
	'game/bg_luaevent.c',
	'game/bg_lualogger.c',
	'game/bg_luaplayer.c',
	'game/bg_luaserialiser.c',
	'game/bg_luavector.c',
	'game/bg_misc.c',
	'game/bg_panimate.c',
	'game/bg_pmove.c',
	'game/bg_saber.c',
	'game/bg_saberLoad.c',
	'game/bg_saga.c',
	'game/bg_slidemove.c',
	'game/bg_vehicleLoad.c',
	'game/bg_weapons.c',
	'game/FighterNPC.c',
	'game/SpeederNPC.c',
	'game/WalkerNPC.c',
	'JAPP/jp_crashExtra.c',
	'JAPP/jp_promode.c',
	'JAPP/jp_stack.c',
	'JAPP/jp_tokenparser.c',
	'json/cJSON.c',
	'ui/ui_shared.c',
	'cgame/cg_chatbox.c',
	'cgame/cg_consolecmds.c',
	'cgame/cg_draw.c',
	'cgame/cg_drawtools.c',
	'cgame/cg_effects.c',
	'cgame/cg_ents.c',
	'cgame/cg_event.c',
	'cgame/cg_info.c',
	'cgame/cg_light.c',
	'cgame/cg_localents.c',
	'cgame/cg_luaserver.c',
	'cgame/cg_main.c',
	'cgame/cg_marks.c',
	'cgame/cg_media.c',
	'cgame/cg_newDraw.c',
	'cgame/cg_newScoreboard.c',
	'cgame/cg_players.c',
	'cgame/cg_playerstate.c',
	'cgame/cg_predict.c',
	'cgame/cg_saga.c',
	'cgame/cg_scoreboard.c',
	'cgame/cg_servercmds.c',
	'cgame/cg_serverModification.c',
	'cgame/cg_smartentities.c',
	'cgame/cg_snapshot.c',
	'cgame/cg_syscalls.c',
	'cgame/cg_teambinds.c',
	'cgame/cg_trueview.c',
	'cgame/cg_turret.c',
	'cgame/cg_view.c',
	'cgame/cg_weaponinit.c',
	'cgame/cg_weapons.c',
	'cgame/fx_blaster.c',
	'cgame/fx_bowcaster.c',
	'cgame/fx_bryarpistol.c',
	'cgame/fx_demp2.c',
	'cgame/fx_disruptor.c',
	'cgame/fx_flechette.c',
	'cgame/fx_force.c',
	'cgame/fx_heavyrepeater.c',
	'cgame/fx_rocketlauncher.c'
	] + files['lua']

files['ui'] = [
	'qcommon/q_math.c',
	'qcommon/q_shared.c',
	'JAPP/jp_crash.c',
	'JAPP/jp_tokenparser.c',
	'json/cJSON.c',
	'game/bg_misc.c',
	'game/bg_saberLoad.c',
	'game/bg_saga.c',
	'game/bg_vehicleLoad.c',
	'game/bg_weapons.c',
	'ui/ui_atoms.c',
	'ui/ui_cvar.c',
	'ui/ui_force.c',
	'ui/ui_gameinfo.c',
	'ui/ui_main.c',
	'ui/ui_saber.c',
	'ui/ui_shared.c',
	'ui/ui_syscalls.c'
	] + files['udis86']

# set up libraries to link with
if plat == 'Linux':
	libs['game'] = [ 'm', 'readline' ]
	libs['cgame'] = [ 'm', 'readline' ]
	libs['ui'] = [ 'm' ]
elif plat == 'Windows':
	libs['game'] = [ 'user32' ]
	libs['cgame'] = [ 'user32', 'Advapi32' ]
	libs['ui'] = [ 'user32' ]

# compiler options
if plat == 'Linux':
	env['CC'] = compiler
	env['CPPDEFINES'] = []
	env['CFLAGS'] = [
		'-Wdeclaration-after-statement',
		'-Wnested-externs',
		'-Wold-style-definition',
		'-Wstrict-prototypes',
		]
	env['CCFLAGS'] = [
		'-Wall', '-Wextra',
		'-Wno-missing-braces',
		'-Wno-missing-field-initializers',
		'-Wno-sign-compare',
		'-Wno-unused-parameter',
		'-Waggregate-return',
	#	'-Wbad-function-cast',
	#	'-Wcast-qual',
	#	'-Wdouble-promotion',
	#	'-Wfloat-equal',
		'-Winit-self',
		'-Winline',
	#	'-Wlong-long',
		'-Wmissing-include-dirs',
		'-Woverlength-strings',
		'-Wpointer-arith',
		'-Wredundant-decls',
	#	'-Wshadow',
	#	'-Wsign-conversion',
	#	'-Wsuggest-attribute=const',
	#	'-Wswitch-default',
		'-Wundef',
		'-Wuninitialized',
	#	'-Wunreachable-code',
	#	'-Wunsuffixed-float-constants',
		'-Wwrite-strings',
		]
	if compiler == 'gcc' and arch != 'arm':
		env['CCFLAGS'] += [
			'-mfpmath=sse',
			'-Wlogical-op' ]
		status, ver = commands.getstatusoutput( 'gcc -dumpversion' )
		if float(ver) >= 4.7:
			env['CCFLAGS'] += [ '-Wstack-usage=32768' ]
	env['CXXFLAGS'] += [ '-fvisibility-inlines-hidden', '-std=c++11' ]
	if analyse:
		env['CC'] = 'clang'
		env['CCFLAGS'] += [ '--analyze' ]
	if arch == 'arm':
		env['CCFLAGS'] += [ '-fsigned-char' ]
	else:
		env['CCFLAGS'] += [ '-msse2', '-mstackrealign' ]
		if arch == 'i386':
			env['CCFLAGS'] += [ '-m32' ]
			env['LINKFLAGS'] += [ '-m32' ]
	env['CCFLAGS'] += [ '-fvisibility=hidden' ]
elif plat == 'Windows':
	# assume msvc
	env['CCFLAGS'] = [
		'/nologo',
		'/W4',
		'/WX-',
		'/GS',
		'/fp:precise',
		'/Zc:wchar_t',
		'/Zc:forScope',
		'/Gd',
		'/GF',
		'/TC',
		'/errorReport:prompt',
		'/EHs',
		'/EHc',
		'/Ot',
		'/Zi',
		'/MP'
		]
	env['LINKFLAGS'] = [ '/SUBSYSTEM:WINDOWS', '/MACHINE:'+arch, '/LTCG' ]
	env['CPPDEFINES'] = [
		'_WINDLL',
		'_MSC_EXTENSIONS',
		'_INTEGRAL_MAX_BITS=64',
		'_WIN32',
		'_MT',
		'_DLL',
		'_M_FP_PRECISE'
		]
	if bits == 32:
		env['CCFLAGS'] += [
			'/analyze-',
			'/Zp8',
			'/Gs',
			'/Oy-'
			]
		env['CPPDEFINES'] += [ '_M_IX86=600', '_M_IX86_FP=2' ]
	elif bits == 64:
		env['CCFLAGS'] += [ '/Zp16' ]
		env['CPPDEFINES'] += [ '_M_AMD64=100', '_M_X64=100', '_WIN64' ]

# debug / release
if not debug or debug == 2:
	if plat == 'Linux':
		env['CCFLAGS'] += [ '-O3' ]
	elif plat == 'Windows':
		env['CCFLAGS'] += [ '/GL', '/Gm-', '/MD', '/O2', '/Oi' ]
		if bits == 64:
			env['CCFLAGS'] += [ '/Oy' ]
	if not debug:
		env['CPPDEFINES'] += [ 'NDEBUG' ]
if debug:
	if plat == 'Linux':
		env['CCFLAGS'] += [ '-g3' ]
	elif plat == 'Windows':
		env['CPPDEFINES'] += [ '__MSVC_RUNTIME_CHECKS' ]
		env['CCFLAGS'] += [
			'/Gm',
			'/FD',
			'/MDd',
			'/Od',
			'/RTC1',
			'/RTCs',
			'/RTCu'
			]
		if bits == 32:
			env['CCFLAGS'] += [ '/FC', '/ZI' ]
	env['CPPDEFINES'] += [ '_DEBUG' ]

# get git revision
status, rev = commands.getstatusoutput( 'git rev-parse HEAD' )
if status == 0:
	env['CPPDEFINES'] += [ 'REVISION=\\"'+rev+'\\"' ]

env['CPPDEFINES'] += [ 'SCONS_BUILD' ]
env['LIBPREFIX'] = ''
env['CPPPATH'] = [ '.', './game' ]
env['LIBS'] = libs[project]

# targets
if project == 'game':
	env['CPPDEFINES'] += [ '_GAME', 'JPLUA' ]
	if plat == 'Linux':
		env['CPPDEFINES'] += [ 'LUA_USE_LINUX' ]
	env.SharedLibrary( 'jampgame'+arch, files[project] )

elif project == 'cgame':
	env['CPPPATH'] += [ './cgame' ]
	env['CPPDEFINES'] += [ '_CGAME', 'JPLUA' ]
	if plat == 'Linux':
		env['CPPDEFINES'] += [ 'LUA_USE_LINUX' ]
	env.SharedLibrary( 'cgame'+arch, files[project] )

elif project == 'ui':
	env['CPPPATH'] += [ './ui' ]
	env['CPPDEFINES'] += [ '_UI' ]
	env.SharedLibrary( 'ui'+arch, files[project] )

else:
	print( 'no project specified' )
