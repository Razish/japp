# - Linux Makefile
# Compiler: gcc
# Target: Linux i386
# Project: JA++

CC = gcc
TARGET = ../../japlus/jampgamei386.so

# Compiler Flags - swap CONFIG for release/debug builds
#FLAGS += -O2 -DFINAL_BUILD -DNDEBUG
FLAGS += -g3 -D_DEBUG
FLAGS += -I../ -I./ -I../shared/
FLAGS += -DMISSION_PACK -D_GAME -D_JK2 -D__GCC__
FLAGS += -masm=intel -m32 -fno-strict-aliasing -march=k8

# Files
FILES += ai_main.o ai_util.o ai_wpnav.o
FILES += bg_g2_utils.o bg_misc.o bg_panimate.o bg_pmove.o bg_saber.o bg_saberLoad.o bg_saga.o bg_slidemove.o bg_vehicleLoad.o bg_weapons.o
FILES += g_active.o g_arenas.o g_bot.o g_client.o g_cmds.o g_combat.o g_exphysics.o g_ICARUScb.o g_items.o g_log.o g_main.o g_mem.o g_misc.o \
			g_missile.o g_mover.o g_nav.o g_navnew.o g_object.o g_saga.o g_session.o g_spawn.o g_strap.o g_svcmds.o g_syscalls.o g_target.o g_team.o \
			g_timer.o g_trigger.o g_turret.o g_turret_G2.o g_utils.o g_vehicles.o g_vehicleTurret.o g_weapon.o
FILES += NPC.o NPC_AI_Atst.o NPC_AI_Default.o NPC_AI_Droid.o NPC_AI_GalakMech.o NPC_AI_Grenadier.o NPC_AI_Howler.o NPC_AI_ImperialProbe.o NPC_AI_Interrogator.o \
			NPC_AI_Jedi.o NPC_AI_Mark1.o NPC_AI_Mark2.o NPC_AI_MineMonster.o NPC_AI_Rancor.o NPC_AI_Remote.o NPC_AI_Seeker.o NPC_AI_Sentry.o \
			NPC_AI_Sniper.o NPC_AI_Stormtrooper.o NPC_AI_Utils.o NPC_AI_Wampa.o NPC_behavior.o NPC_combat.o NPC_goal.o NPC_misc.o NPC_move.o NPC_reactions.o \
			NPC_senses.o NPC_sounds.o NPC_spawn.o NPC_stats.o NPC_utils.o AnimalNPC.o FighterNPC.o SpeederNPC.o WalkerNPC.o
FILES += q_math.o q_shared.o tri_coll_test.o w_force.o w_saber.o
FILES += g_engine.o g_admin.o g_playerbans.o g_unlagged.o g_clientModification.o g_lua.o g_luaevent.o g_crash.o
FILES += ../shared/qcommon/q_engine.o ../shared/JAPP/jp_tokenparser.o ../shared/json/cJSON.o ../shared/JAPP/jp_promode.o
FILES += ../shared/libudis86/decode.o ../shared/libudis86/input.o ../shared/libudis86/itab.o ../shared/libudis86/syn.o \
		../shared/libudis86/syn-att.o ../shared/libudis86/syn-intel.o ../shared/libudis86/udis86.o

# "make"
$(TARGET):  $(FILES)
	$(CC) $(FLAGS) -shared -o $(TARGET) $(FILES)
	chmod +x $(TARGET)

# "make .c.o"
.c.o:
	$(CC) $(FLAGS) -c $*.c -o $*.o

# "make clean"
clean:
	rm -f $(FILES) $(TARGET)

