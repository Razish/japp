#include "cg_local.h"
#include "ui/ui_shared.h"
#include "cg_media.h"

extern displayContextDef_t cgDC;

cgMedia_t media;

#define RFL_NONE		(0x00000000u)
#define RFL_NOMIP		(0x00000001u)

typedef struct resource_s {
	void *handle;
	const char *location;
	uint32_t flags;
	uint32_t gametypes;
} resource_t;

static const resource_t sounds[] = {
//	{ &media.sounds.awards.assist, NULL, RFL_NONE, GTB_CTF },
	{ &media.sounds.awards.capture, "sound/teamplay/flagcapture_yourteam.wav", RFL_NONE, GTB_CTF },
	{ &media.sounds.awards.defense, "sound/chars/protocol/misc/40MOM024", RFL_NONE, GTB_CTF },
	{ &media.sounds.awards.denied, "sound/chars/protocol/misc/40MOM017", RFL_NONE, GTB_ALL },
	{ &media.sounds.awards.excellent, "sound/chars/protocol/misc/40MOM053", RFL_NONE, GTB_ALL },
	{ &media.sounds.awards.holyShit, "sound/chars/protocol/misc/holyshit", RFL_NONE, GTB_ALL },
	{ &media.sounds.awards.humiliation, "sound/chars/protocol/misc/40MOM019", RFL_NONE, GTB_ALL },
	{ &media.sounds.awards.impressive, "sound/chars/protocol/misc/40MOM025", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.chunk, "sound/weapons/explosions/glasslcar", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.crackle, "sound/effects/energy_crackle.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.crateBreak[0], "sound/weapons/explosions/crateBust1", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.crateBreak[1], "sound/weapons/explosions/crateBust2", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.fall, "sound/player/fallsplat.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONEWALK][0], "sound/player/footsteps/stone_step1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONEWALK][1], "sound/player/footsteps/stone_step2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONEWALK][2], "sound/player/footsteps/stone_step3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONEWALK][3], "sound/player/footsteps/stone_step4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONERUN][0], "sound/player/footsteps/stone_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONERUN][1], "sound/player/footsteps/stone_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONERUN][2], "sound/player/footsteps/stone_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_STONERUN][3], "sound/player/footsteps/stone_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALWALK][0], "sound/player/footsteps/metal_step1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALWALK][1], "sound/player/footsteps/metal_step2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALWALK][2], "sound/player/footsteps/metal_step3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALWALK][3], "sound/player/footsteps/metal_step4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALRUN][0], "sound/player/footsteps/metal_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALRUN][1], "sound/player/footsteps/metal_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALRUN][2], "sound/player/footsteps/metal_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_METALRUN][3], "sound/player/footsteps/metal_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPEWALK][0], "sound/player/footsteps/pipe_step1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPEWALK][1], "sound/player/footsteps/pipe_step2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPEWALK][2], "sound/player/footsteps/pipe_step3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPEWALK][3], "sound/player/footsteps/pipe_step4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPERUN][0], "sound/player/footsteps/pipe_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPERUN][1], "sound/player/footsteps/pipe_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPERUN][2], "sound/player/footsteps/pipe_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_PIPERUN][3], "sound/player/footsteps/pipe_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SPLASH][0], "sound/player/footsteps/water_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SPLASH][1], "sound/player/footsteps/water_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SPLASH][2], "sound/player/footsteps/water_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SPLASH][3], "sound/player/footsteps/water_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WADE][0], "sound/player/footsteps/water_walk1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WADE][1], "sound/player/footsteps/water_walk2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WADE][2], "sound/player/footsteps/water_walk3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WADE][3], "sound/player/footsteps/water_walk4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SWIM][0], "sound/player/footsteps/water_wade_01.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SWIM][1], "sound/player/footsteps/water_wade_02.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SWIM][2], "sound/player/footsteps/water_wade_03.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SWIM][3], "sound/player/footsteps/water_wade_04.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWWALK][0], "sound/player/footsteps/snow_step1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWWALK][1], "sound/player/footsteps/snow_step2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWWALK][2], "sound/player/footsteps/snow_step3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWWALK][3], "sound/player/footsteps/snow_step4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWRUN][0], "sound/player/footsteps/snow_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWRUN][1], "sound/player/footsteps/snow_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWRUN][2], "sound/player/footsteps/snow_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SNOWRUN][3], "sound/player/footsteps/snow_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDWALK][0], "sound/player/footsteps/sand_walk1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDWALK][1], "sound/player/footsteps/sand_walk2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDWALK][2], "sound/player/footsteps/sand_walk3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDWALK][3], "sound/player/footsteps/sand_walk4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDRUN][0], "sound/player/footsteps/sand_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDRUN][1], "sound/player/footsteps/sand_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDRUN][2], "sound/player/footsteps/sand_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_SANDRUN][3], "sound/player/footsteps/sand_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSWALK][0], "sound/player/footsteps/grass_step1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSWALK][1], "sound/player/footsteps/grass_step2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSWALK][2], "sound/player/footsteps/grass_step3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSWALK][3], "sound/player/footsteps/grass_step4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSRUN][0], "sound/player/footsteps/grass_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSRUN][1], "sound/player/footsteps/grass_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSRUN][2], "sound/player/footsteps/grass_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRASSRUN][3], "sound/player/footsteps/grass_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTWALK][0], "sound/player/footsteps/dirt_step1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTWALK][1], "sound/player/footsteps/dirt_step2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTWALK][2], "sound/player/footsteps/dirt_step3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTWALK][3], "sound/player/footsteps/dirt_step4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTRUN][0], "sound/player/footsteps/dirt_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTRUN][1], "sound/player/footsteps/dirt_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTRUN][2], "sound/player/footsteps/dirt_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_DIRTRUN][3], "sound/player/footsteps/dirt_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDWALK][0], "sound/player/footsteps/mud_walk1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDWALK][1], "sound/player/footsteps/mud_walk2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDWALK][2], "sound/player/footsteps/mud_walk3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDWALK][3], "sound/player/footsteps/mud_walk4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDRUN][0], "sound/player/footsteps/mud_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDRUN][1], "sound/player/footsteps/mud_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDRUN][2], "sound/player/footsteps/mud_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_MUDRUN][3], "sound/player/footsteps/mud_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELWALK][0], "sound/player/footsteps/gravel_walk1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELWALK][1], "sound/player/footsteps/gravel_walk2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELWALK][2], "sound/player/footsteps/gravel_walk3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELWALK][3], "sound/player/footsteps/gravel_walk4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELRUN][0], "sound/player/footsteps/gravel_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELRUN][1], "sound/player/footsteps/gravel_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELRUN][2], "sound/player/footsteps/gravel_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_GRAVELRUN][3], "sound/player/footsteps/gravel_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGWALK][0], "sound/player/footsteps/rug_step1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGWALK][1], "sound/player/footsteps/rug_step2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGWALK][2], "sound/player/footsteps/rug_step3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGWALK][3], "sound/player/footsteps/rug_step4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGRUN][0], "sound/player/footsteps/rug_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGRUN][1], "sound/player/footsteps/rug_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGRUN][2], "sound/player/footsteps/rug_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_RUGRUN][3], "sound/player/footsteps/rug_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODWALK][0], "sound/player/footsteps/wood_walk1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODWALK][1], "sound/player/footsteps/wood_walk2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODWALK][2], "sound/player/footsteps/wood_walk3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODWALK][3], "sound/player/footsteps/wood_walk4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODRUN][0], "sound/player/footsteps/wood_run1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODRUN][1], "sound/player/footsteps/wood_run2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODRUN][2], "sound/player/footsteps/wood_run3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.footsteps[FOOTSTEP_WOODRUN][3], "sound/player/footsteps/wood_run4.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.glassChunk, "sound/weapons/explosions/glassbreak1", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.grate, "sound/effects/grate_destroy", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.land, "sound/player/land1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.metalBounce[0], "sound/effects/metal_bounce", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.metalBounce[1], "sound/effects/metal_bounce2", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.respawn, "sound/items/respawn1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.rockBounce[0], "sound/effects/stone_bounce", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.rockBounce[1], "sound/effects/stone_bounce2", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.rockBreak, "sound/effects/wall_smash", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.roll, "sound/player/roll1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.teleIn, "sound/player/telein.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.teleOut, "sound/player/teleout.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.waterIn, "sound/player/watr_in.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.waterOut, "sound/player/watr_out.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.environment.waterUnder, "sound/player/watr_un.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.force.drain, "sound/weapons/force/drained.mp3", RFL_NONE, GTB_ALL },
	{ &media.sounds.force.drain2, "sound/weapons/force/drain.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.force.lightning, "sound/weapons/force/lightning", RFL_NONE, GTB_ALL },
	{ &media.sounds.force.noforce, "sound/weapons/force/noforce", RFL_NONE, GTB_ALL },
	{ &media.sounds.force.teamHeal, "sound/weapons/force/teamheal.wav", RFL_NONE, GTB_TEAM },
	{ &media.sounds.force.teamRegen, "sound/weapons/force/teamforce.wav", RFL_NONE, GTB_TEAM },
	{ &media.sounds.interface.deploySeeker, "sound/chars/seeker/misc/hiss", RFL_NONE, GTB_ALL },
	{ &media.sounds.interface.holocronPickup, "sound/player/holocron.wav", RFL_NONE, GTB_HOLOCRON },
	{ &media.sounds.interface.medkit, "sound/items/use_bacta.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.interface.select, "sound/weapons/change.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.interface.talk, "sound/player/talk.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.items.jetpackHover, "sound/boba/JETHOVER", RFL_NONE, GTB_ALL },
	{ &media.sounds.items.jetpackLoop, "sound/effects/fire_lp", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.hit[0], "sound/weapons/saber/saberhit1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.hit[1], "sound/weapons/saber/saberhit2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.hit[2], "sound/weapons/saber/saberhit3.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.hitWall[0], "sound/weapons/saber/saberhitwall1", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.hitWall[1], "sound/weapons/saber/saberhitwall2", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.hitWall[2], "sound/weapons/saber/saberhitwall3", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.turnOn, "sound/weapons/saber/saberon.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.saber.turnOff, "sound/weapons/saber/saberoffquick.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.team.blueFlagReturned, "sound/chars/protocol/misc/40MOM041", RFL_NONE, GTB_CTF },
	{ &media.sounds.team.blueLeads, "sound/chars/protocol/misc/40MOM045", RFL_NONE, GTB_ALL&~GTB_NOTTEAM },
	{ &media.sounds.team.blueScored, "sound/chars/protocol/misc/40MOM043", RFL_NONE, GTB_CTF },
	{ &media.sounds.team.blueTookFlag, "sound/chars/protocol/misc/40MOM039", RFL_NONE, GTB_CTF },
	{ &media.sounds.team.blueTookYsal, "sound/chars/protocol/misc/40MOM047", RFL_NONE, GTB_CTY },
	{ &media.sounds.team.blueYsalReturned, "sound/chars/protocol/misc/40MOM049", RFL_NONE, GTB_CTY },
	{ &media.sounds.team.redFlagReturned, "sound/chars/protocol/misc/40MOM042", RFL_NONE, GTB_CTF },
	{ &media.sounds.team.redLeads, "sound/chars/protocol/misc/40MOM046", RFL_NONE, GTB_ALL&~GTB_NOTTEAM },
	{ &media.sounds.team.redScored, "sound/chars/protocol/misc/40MOM044", RFL_NONE, GTB_CTF },
	{ &media.sounds.team.redTookFlag, "sound/chars/protocol/misc/40MOM040", RFL_NONE, GTB_CTF },
	{ &media.sounds.team.redTookYsal, "sound/chars/protocol/misc/40MOM048", RFL_NONE, GTB_CTY },
	{ &media.sounds.team.redYsalReturned, "sound/chars/protocol/misc/40MOM050", RFL_NONE, GTB_CTY },
	{ &media.sounds.team.teamsTied, "sound/chars/protocol/misc/40MOM032", RFL_NONE, GTB_ALL },
	{ &media.sounds.warning.count1, "sound/chars/protocol/misc/40MOM037", RFL_NONE, GTB_ALL },
	{ &media.sounds.warning.count2, "sound/chars/protocol/misc/40MOM036", RFL_NONE, GTB_ALL },
	{ &media.sounds.warning.count3, "sound/chars/protocol/misc/40MOM035", RFL_NONE, GTB_ALL },
	{ &media.sounds.warning.countFight, "sound/chars/protocol/misc/40MOM038", RFL_NONE, GTB_ALL },
	{ &media.sounds.warning.fiveMinute, "sound/chars/protocol/misc/40MOM005", RFL_NONE, GTB_ALL },
	{ &media.sounds.warning.oneFrag, "sound/chars/protocol/misc/40MOM001", RFL_NONE, GTB_NOTFLAG },
	{ &media.sounds.warning.oneMinute, "sound/chars/protocol/misc/40MOM004", RFL_NONE, GTB_ALL },
	{ &media.sounds.warning.threeFrag, "sound/chars/protocol/misc/40MOM003", RFL_NONE, GTB_NOTFLAG },
	{ &media.sounds.warning.twoFrag, "sound/chars/protocol/misc/40MOM002", RFL_NONE, GTB_NOTFLAG },
	{ &media.sounds.weapons.grenadeBounce1, "sound/weapons/thermal/bounce1.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.weapons.grenadeBounce2, "sound/weapons/thermal/bounce2.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.weapons.noAmmo, "sound/weapons/noammo.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.weapons.stunBatonIdle, "sound/weapons/baton/idle", RFL_NONE, GTB_ALL },
	{ &media.sounds.weapons.zoomEnd, "sound/interface/zoomend.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.weapons.zoomLoop, "sound/interface/zoomloop.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.weapons.zoomStart, "sound/interface/zoomstart.wav", RFL_NONE, GTB_ALL },
	{ &media.sounds.dramaticFailure, "music/badsmall.mp3", RFL_NONE, GTB_ALL },
	{ &media.sounds.happyMusic, "music/goodsmall.mp3", RFL_NONE, GTB_ALL },
	{ &media.sounds.loser, "sound/chars/protocol/misc/40MOM010", RFL_NONE, GTB_ALL },
	{ &media.sounds.winner, "sound/chars/protocol/misc/40MOM006", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup1.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup2.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup3.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup4.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup5.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup6.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup7.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhup8.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock1.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock2.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock3.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock4.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock5.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock6.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock7.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock8.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberblock9.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/bounce1.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/bounce2.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/bounce3.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/thermal/bounce1.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/thermal/bounce2.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/melee/punch1.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/melee/punch2.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/melee/punch3.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/melee/punch4.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/movers/objects/saber_slam", RFL_NONE, GTB_ALL },
	{ NULL, "sound/player/bodyfall_human1.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/player/bodyfall_human2.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/player/bodyfall_human3.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/enemy_saber_on.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/enemy_saber_off.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhum1.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saberhit.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/saber/saber_catch.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/heal.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/speed.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/see.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/rage.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/lightninghit1", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/lightninghit2", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/lightninghit3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/jumpbuild.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/distract.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/distractstop.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/pull.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/push.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/movers/switches/switch2.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/movers/switches/switch3.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/ambience/spark5.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/chars/turret/ping.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/chars/turret/startup.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/chars/turret/shutdown.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/chars/turret/move.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/player/pickuphealth.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/player/pickupshield.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/effects/glassbreak1.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/rocket/tick.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/rocket/lock.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/speedloop.wav", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/protecthit.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/protect.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/absorbhit.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/absorb.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/jump.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/weapons/force/grip.mp3", RFL_NONE, GTB_ALL },
	{ NULL, "sound/movers/objects/objectHit.wav", RFL_NONE, GTB_ALL },
};

static const resource_t gfx[] = {
	{ &media.gfx.interface.automap.frameBottom, "gfx/mp_automap/mpauto_frame_bottom", RFL_NOMIP, GTB_SIEGE },
	{ &media.gfx.interface.automap.frameLeft, "gfx/mp_automap/mpauto_frame_left", RFL_NOMIP, GTB_SIEGE },
	{ &media.gfx.interface.automap.frameRight, "gfx/mp_automap/mpauto_frame_right", RFL_NOMIP, GTB_SIEGE },
	{ &media.gfx.interface.automap.frameTop, "gfx/mp_automap/mpauto_frame_top", RFL_NOMIP, GTB_SIEGE },
	{ &media.gfx.interface.automap.playerIcon, "gfx/menus/radar/arrow_w", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.automap.rocketIcon, "gfx/menus/radar/rocket", RFL_NOMIP, GTB_SIEGE },
	{ &media.gfx.interface.binoculars.arrow, "gfx/2d/binSideArrow", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.binoculars.circle, "gfx/2d/binCircle", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.binoculars.mask, "gfx/2d/binMask", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.binoculars.overlay, "gfx/2d/binocularNumOverlay", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.binoculars.staticMask, "gfx/2d/binocularWindow", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.binoculars.tri, "gfx/2d/binTopTri", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.disruptor.charge, "gfx/2d/crop_charge", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.disruptor.insert, "gfx/2d/cropCircle", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.disruptor.insertTick, "gfx/2d/insertTick", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.disruptor.light, "gfx/2d/cropCircleGlow", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.disruptor.mask, "gfx/2d/cropCircle2", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.medals.assist, "medal_assist", RFL_NOMIP, GTB_CTF },
	{ &media.gfx.interface.medals.capture, "medal_capture", RFL_NOMIP, GTB_CTF },
	{ &media.gfx.interface.medals.defend, "medal_defend", RFL_NOMIP, GTB_CTF },
	{ &media.gfx.interface.medals.excellent, "medal_excellent", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.medals.gauntlet, "medal_gauntlet", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.medals.impressive, "medal_impressive", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.team.assault, "ui/assets/statusbar/assault", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.team.blue, "sprites/team_blue", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.team.camp, "ui/assets/statusbar/camp", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.team.defend, "ui/assets/statusbar/defend", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.team.escort, "ui/assets/statusbar/escort", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.team.flags[0], "ui/assets/statusbar/flag_in_base", RFL_NOMIP, GTB_CTF },
	{ &media.gfx.interface.team.flags[1], "ui/assets/statusbar/flag_capture", RFL_NOMIP, GTB_CTF },
	{ &media.gfx.interface.team.flags[2], "ui/assets/statusbar/flag_missing", RFL_NOMIP, GTB_CTF },
	{ &media.gfx.interface.team.follow, "ui/assets/statusbar/follow", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.team.patrol, "ui/assets/statusbar/patrol", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.team.red, "sprites/team_red", RFL_NOMIP, (GTB_ALL&~GTB_NOTTEAM) | GTB_JEDIMASTER },
	{ &media.gfx.interface.team.retrieve, "ui/assets/statusbar/retrieve", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.backTile, "gfx/2d/backtile", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.balloon, "gfx/mp/chat_icon", RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.charset, NULL, RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[0], "gfx/2d/numbers/c_zero", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[1], "gfx/2d/numbers/c_one", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[2], "gfx/2d/numbers/c_two", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[3], "gfx/2d/numbers/c_three", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[4], "gfx/2d/numbers/c_four", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[5], "gfx/2d/numbers/c_five", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[6], "gfx/2d/numbers/c_six", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[7], "gfx/2d/numbers/c_seven", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[8], "gfx/2d/numbers/c_eight", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[9], "gfx/2d/numbers/c_nine", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.chunkyNumbers[10], "gfx/2d/numbers/t_minus", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.connection, "gfx/2d/net", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[0], "gfx/2d/crosshaira", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[1], "gfx/2d/crosshairb", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[2], "gfx/2d/crosshairc", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[3], "gfx/2d/crosshaird", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[4], "gfx/2d/crosshaire", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[5], "gfx/2d/crosshairf", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[6], "gfx/2d/crosshairg", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[7], "gfx/2d/crosshairh", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.crosshairs[8], "gfx/2d/crosshairi", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.cursor, "cursor", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.cursorSelect, "ui/assets/selectcursor", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.cursorSize, "ui/assets/sizecursor", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.defer, "gfx/2d/defer", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forceCorona, "gfx/hud/force_swirl", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forceIconBackground, "gfx/hud/background_f", RFL_NONE, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_HEAL], "gfx/mp/f_icon_lt_heal", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_LEVITATION], "gfx/mp/f_icon_levitation", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_SPEED], "gfx/mp/f_icon_speed", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_PUSH], "gfx/mp/f_icon_push", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_PULL], "gfx/mp/f_icon_pull", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_TELEPATHY], "gfx/mp/f_icon_lt_telepathy", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_GRIP], "gfx/mp/f_icon_dk_grip", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_LIGHTNING], "gfx/mp/f_icon_dk_l1", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_RAGE], "gfx/mp/f_icon_dk_rage", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_PROTECT], "gfx/mp/f_icon_lt_protect", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_ABSORB], "gfx/mp/f_icon_lt_absorb", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_TEAM_HEAL], "gfx/mp/f_icon_lt_healother", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_TEAM_FORCE], "gfx/mp/f_icon_dk_forceother", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_DRAIN], "gfx/mp/f_icon_dk_drain", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_SEE], "gfx/mp/f_icon_sight", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_SABER_OFFENSE], "gfx/mp/f_icon_saber_attack", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_SABER_DEFENSE], "gfx/mp/f_icon_saber_defend", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.forcePowerIcons[FP_SABERTHROW], "gfx/mp/f_icon_saber_throw", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.hackerIcon, "gfx/mp/c_icon_tech", RFL_NOMIP, GTB_SIEGE|GTB_FFA },
	{ &media.gfx.interface.heart, "ui/assets/statusbar/selectedhealth", RFL_NONE, GTB_ALL },
//	{ &media.gfx.interface.invenIcons[HI_NONE], NULL, RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_SEEKER], "gfx/hud/i_icon_seeker", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_SHIELD], "gfx/hud/i_icon_shieldwall", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_MEDPAC], "gfx/hud/i_icon_bacta", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_MEDPAC_BIG], "gfx/hud/i_icon_big_bacta", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_BINOCULARS], "gfx/hud/i_icon_zoom", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_SENTRY_GUN], "gfx/hud/i_icon_sentrygun", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_JETPACK], "gfx/hud/i_icon_jetpack", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_HEALTHDISP], "gfx/hud/i_icon_healthdisp", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_AMMODISP], "gfx/hud/i_icon_ammodisp", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_EWEB], "gfx/hud/i_icon_eweb", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.invenIcons[HI_CLOAK], "gfx/hud/i_icon_cloak", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.inventoryIconBackground, "gfx/hud/background_i", RFL_NONE, GTB_ALL },
	{ &media.gfx.interface.lagometer, "gfx/2d/lag", RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.loadBarLED, NULL, RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.loadBarLEDCap, NULL, RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.loadBarLEDSurround, NULL, RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[0], "gfx/2d/numbers/zero", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[1], "gfx/2d/numbers/one", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[2], "gfx/2d/numbers/two", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[3], "gfx/2d/numbers/three", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[4], "gfx/2d/numbers/four", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[5], "gfx/2d/numbers/five", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[6], "gfx/2d/numbers/six", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[7], "gfx/2d/numbers/seven", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[8], "gfx/2d/numbers/eight", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[9], "gfx/2d/numbers/nine", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.numbers[10], "gfx/2d/numbers/minus", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.pain, "gfx/misc/borgeyeflare", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.painShields, "gfx/mp/dmgshader_shields", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.painShieldsAndHealth, "gfx/mp/dmgshader_shieldsandhealth", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.powerduelAlly, "gfx/mp/pduel_icon_double", RFL_NOMIP, GTB_POWERDUEL|GTB_FFA|GTB_TEAM },
	{ &media.gfx.interface.radar, "gfx/menus/radar/radar", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.rageRecovery, "gfx/mp/f_icon_ragerec", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.scoreboardLine, "gfx/menus/scoreboard", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.siegeItem, "gfx/menus/radar/goalitem", RFL_NOMIP, GTB_SIEGE },
	{ &media.gfx.interface.smallNumbers[0], "gfx/2d/numbers/t_zero", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[1], "gfx/2d/numbers/t_one", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[2], "gfx/2d/numbers/t_two", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[3], "gfx/2d/numbers/t_three", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[4], "gfx/2d/numbers/t_four", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[5], "gfx/2d/numbers/t_five", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[6], "gfx/2d/numbers/t_six", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[7], "gfx/2d/numbers/t_seven", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[8], "gfx/2d/numbers/t_eight", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[9], "gfx/2d/numbers/t_nine", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.smallNumbers[10], "gfx/2d/numbers/t_minus", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.teamStatusBar, "gfx/2d/colorbar", RFL_NOMIP, GTB_ALL&~GTB_NOTTEAM },
	{ &media.gfx.interface.vchat, "gfx/mp/vchat_icon", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconBackground, "gfx/hud/background", RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.weaponIcons[WP_NONE], NULL, RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_STUN_BATON], "gfx/hud/w_icon_stunbaton", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_MELEE], "gfx/hud/w_icon_melee", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_SABER], "gfx/hud/w_icon_lightsaber", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_BRYAR_PISTOL], "gfx/hud/w_icon_blaster_pistol", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_BLASTER], "gfx/hud/w_icon_blaster", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_DISRUPTOR], "gfx/hud/w_icon_disruptor", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_BOWCASTER], "gfx/hud/w_icon_bowcaster", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_REPEATER], "gfx/hud/w_icon_repeater", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_DEMP2], "gfx/hud/w_icon_demp2", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_FLECHETTE], "gfx/hud/w_icon_flechette", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_ROCKET_LAUNCHER], "gfx/hud/w_icon_merrsonn", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_THERMAL], "gfx/hud/w_icon_thermal", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_TRIP_MINE], "gfx/hud/w_icon_tripmine", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_DET_PACK], "gfx/hud/w_icon_detpack", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_CONCUSSION], "gfx/hud/w_icon_c_rifle", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIcons[WP_BRYAR_OLD], "gfx/hud/w_icon_briar", RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.weaponIcons[WP_EMPLACED_GUN], NULL, RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.weaponIcons[WP_TURRET], NULL, RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.weaponIconsInactive[WP_NONE], "gfx/hud/w_icon_none_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_STUN_BATON], "gfx/hud/w_icon_stunbaton_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_MELEE], "gfx/hud/w_icon_melee_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_SABER], "gfx/hud/w_icon_lightsaber_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_BRYAR_PISTOL], "gfx/hud/w_icon_blaster_pistol_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_BLASTER], "gfx/hud/w_icon_blaster_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_DISRUPTOR], "gfx/hud/w_icon_disruptor_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_BOWCASTER], "gfx/hud/w_icon_bowcaster_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_REPEATER], "gfx/hud/w_icon_repeater_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_DEMP2], "gfx/hud/w_icon_demp2_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_FLECHETTE], "gfx/hud/w_icon_flechette_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_ROCKET_LAUNCHER], "gfx/hud/w_icon_merrsonn_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_THERMAL], "gfx/hud/w_icon_thermal_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_TRIP_MINE], "gfx/hud/w_icon_tripmine_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_DET_PACK], "gfx/hud/w_icon_detpack_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_CONCUSSION], "gfx/hud/w_icon_c_rifle_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.interface.weaponIconsInactive[WP_BRYAR_OLD], "gfx/hud/w_icon_briar_na", RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.weaponIconsInactive[WP_EMPLACED_GUN], "gfx/hud/w_icon_blaster_na", RFL_NOMIP, GTB_ALL },
//	{ &media.gfx.interface.weaponIconsInactive[WP_TURRET], "gfx/hud/w_icon_blaster_na", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.footsteps.left.heavy, "footstep_heavy_l", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.footsteps.left.light, "footstep_l", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.footsteps.right.heavy, "footstep_heavy_r", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.footsteps.right.light, "footstep_r", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.red.core, "gfx/effects/sabers/red_line", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.red.glow, "gfx/effects/sabers/red_glow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.orange.core, "gfx/effects/sabers/orange_line", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.orange.glow, "gfx/effects/sabers/orange_glow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.yellow.core, "gfx/effects/sabers/yellow_line", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.yellow.glow, "gfx/effects/sabers/yellow_glow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.green.core, "gfx/effects/sabers/green_line", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.green.glow, "gfx/effects/sabers/green_glow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.blue.core, "gfx/effects/sabers/blue_line", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.blue.glow, "gfx/effects/sabers/blue_glow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.purple.core, "gfx/effects/sabers/purple_line", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.purple.glow, "gfx/effects/sabers/purple_glow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.rgb.core, "gfx/effects/sabers/RGBcore1", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb.glow, "gfx/effects/sabers/RGBglow1", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.black.core, "gfx/effects/sabers/blackcore", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.black.glow, "gfx/effects/sabers/blackglow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.black.trail, "gfx/effects/sabers/blacktrail", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb2.core, "gfx/effects/sabers/RGBcore2", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb2.glow, "gfx/effects/sabers/RGBglow2", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.rgb2.trail, "gfx/effects/sabers/RGBtrail2", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb3.core, "gfx/effects/sabers/RGBcore3", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb3.glow, "gfx/effects/sabers/RGBglow3", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.rgb3.trail, "gfx/effects/sabers/RGBtrail3", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb4.core, "gfx/effects/sabers/RGBcore4", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb4.glow, "gfx/effects/sabers/RGBglow4", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.rgb4.trail, "gfx/effects/sabers/RGBtrail4", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb5.core, "gfx/effects/sabers/RGBcore5", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.rgb5.glow, "gfx/effects/sabers/RGBglow5", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saber.rgb5.trail, "gfx/effects/sabers/swordTrail", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.sfx.blade, "gfx/sfx_sabers/saber_blade", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.sfx.blade2, "gfx/sfx_sabers/saber_blade_rgb", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.sfx.end, "gfx/sfx_sabers/saber_end", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.sfx.end2, "gfx/sfx_sabers/saber_end_rgb", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.sfx.trail, "gfx/sfx_sabers/saber_trail", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.blur, "gfx/effects/sabers/saberBlur", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.saber.swordTrail, "gfx/effects/sabers/swordTrail", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.bdecal_bodyburn1, "gfx/damage/bodyburnmark1", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.bdecal_burn1, "gfx/damage/bodybigburnmark1", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.bdecal_saberglow, "gfx/damage/saberglowmark", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.bolt, "gfx/misc/blueLine", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.boon, "powerups/boonshell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.bryarFrontFlash, "gfx/effects/bryarFrontFlash", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.cloaked, "gfx/effects/cloakedShader", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.demp2Shell, "gfx/effects/demp2shell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.disruptor, "gfx/effects/burn", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.electric, "gfx/misc/electric2", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.electricBody, "gfx/misc/electric", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.electricBody2, "gfx/misc/fullbodyelectric2", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.endarkenment, "powerups/endarkenmentshell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.enlightenment, "powerups/enlightenmentshell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.forcePush, "gfx/effects/forcePush", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.forceShell, "powerups/forceshell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.forceSightBubble, "gfx/misc/sightbubble", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.glassShard, "gfx/misc/test_crackle", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.greenFrontFlash, "gfx/effects/greenFrontFlash", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.halfShield, "halfShieldShell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.invulnerability, "powerups/invulnerabilityshell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.itemRespawningPlaceholder, "powerups/placeholder", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.itemRespawningRezOut, "powerups/rezout", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.lightningFlash, "gfx/misc/lightningFlash", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.playerShieldDamage, "gfx/misc/personalshield", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.protect, "gfx/misc/forceprotect", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.refraction, "effects/refraction", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.refraction2, "effects/refract_2", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.rivetMark, "gfx/damage/rivetmark", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.saberDamageGlow, "gfx/effects/saberDamageGlow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.shadowMark, "markShadow", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.sightShell, "powerups/sightshell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.solidWhite, "gfx/effects/solidWhite_cull", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.strafeTrail, "gfx/misc/whiteline2", RFL_NOMIP, GTB_ALL },
	{ &media.gfx.world.surfaceExplosion, "surfaceExplosion", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.wakeMark, "wake", RFL_NONE, GTB_ALL },
//	{ &media.gfx.world.whiteShader, NULL, RFL_NONE, GTB_ALL },
	{ &media.gfx.world.yellowDroppedSaber, "gfx/effects/yellow_glow", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.ysalimari, "powerups/ysalimarishell", RFL_NONE, GTB_ALL },
	{ &media.gfx.world.ysalimariBlue, "powerups/ysaliblueshell", RFL_NONE, GTB_CTY },
	{ &media.gfx.world.ysalimariRed, "powerups/ysaliredshell", RFL_NONE, GTB_CTY },
	{ NULL, "gfx/effects/saberFlare", RFL_NONE, GTB_ALL },
	{ NULL, "powerups/ysalimarishell", RFL_NONE, GTB_ALL },
	{ NULL, "gfx/misc/red_dmgshield", RFL_NONE, GTB_ALL },
	{ NULL, "gfx/misc/red_portashield", RFL_NONE, GTB_ALL },
	{ NULL, "gfx/misc/blue_dmgshield", RFL_NONE, GTB_ALL },
	{ NULL, "gfx/misc/blue_portashield", RFL_NONE, GTB_ALL },
	{ NULL, "models/map_objects/imp_mine/turret_chair_dmg", RFL_NONE, GTB_ALL },
	{ NULL, "gfx/misc/mp_light_enlight_disable", RFL_NONE, GTB_ALL },
	{ NULL, "gfx/misc/mp_dark_enlight_disable", RFL_NONE, GTB_ALL },
	{ NULL, "gfx/mp/pduel_icon_lone", RFL_NOMIP, GTB_POWERDUEL },
	{ NULL, "gfx/hud/mpi_rflag_x", RFL_NOMIP, GTB_CTF | GTB_CTY },
	{ NULL, "gfx/hud/mpi_bflag_x", RFL_NOMIP, GTB_CTF | GTB_CTY },
	{ NULL, "gfx/hud/mpi_rflag_ys", RFL_NOMIP, GTB_CTF | GTB_CTY },
	{ NULL, "gfx/hud/mpi_bflag_ys", RFL_NOMIP, GTB_CTF | GTB_CTY },
	{ NULL, "gfx/hud/mpi_rflag", RFL_NOMIP, GTB_CTF | GTB_CTY },
	{ NULL, "gfx/hud/mpi_bflag", RFL_NOMIP, GTB_CTF | GTB_CTY },
};

static const resource_t efx[] = {
	{ &media.efx.blaster.droidImpact, "blaster/droid_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.blaster.fleshImpact, "blaster/flesh_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.blaster.shot, "blaster/shot", RFL_NONE, GTB_ALL },
	{ &media.efx.blaster.wallImpact, "blaster/wall_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.bowcaster.impact, "bowcaster/explosion", RFL_NONE, GTB_ALL },
	{ &media.efx.bowcaster.shot, "bowcaster/shot", RFL_NONE, GTB_ALL },
	{ &media.efx.concussion.altRing, "concussion/alt_ring", RFL_NONE, GTB_ALL },
	{ &media.efx.concussion.impact, "concussion/explosion", RFL_NONE, GTB_ALL },
	{ &media.efx.concussion.shot, "concussion/shot", RFL_NONE, GTB_ALL },
	{ &media.efx.demp2.altDetonate, "demp2/altDetonate", RFL_NONE, GTB_ALL },
	{ &media.efx.demp2.fleshImpact, "demp2/flesh_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.demp2.projectile, "demp2/projectile", RFL_NONE, GTB_ALL },
	{ &media.efx.demp2.wallImpact, "demp2/wall_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.detpack.explosion, "detpack/explosion", RFL_NONE, GTB_ALL },
	{ &media.efx.disruptor.altHit, "disruptor/alt_hit", RFL_NONE, GTB_ALL },
	{ &media.efx.disruptor.altMiss, "disruptor/alt_miss", RFL_NONE, GTB_ALL },
	{ &media.efx.disruptor.fleshImpact, "disruptor/flesh_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.disruptor.projectile, "disruptor/projectile", RFL_NONE, GTB_ALL },
	{ &media.efx.disruptor.rings, "disruptor/rings", RFL_NONE, GTB_ALL },
	{ &media.efx.disruptor.wallImpact, "disruptor/wall_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.flechette.altBlow, "flechette/alt_blow", RFL_NONE, GTB_ALL },
	{ &media.efx.flechette.altShot, "flechette/alt_shot", RFL_NONE, GTB_ALL },
	{ &media.efx.flechette.fleshImpact, "flechette/flesh_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.flechette.shot, "flechette/shot", RFL_NONE, GTB_ALL },
	{ &media.efx.flechette.wallImpact, "flechette/wall_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.footstep.gravel, "materials/gravel", RFL_NONE, GTB_ALL },
	{ &media.efx.footstep.mud, "materials/mud", RFL_NONE, GTB_ALL },
	{ &media.efx.footstep.sand, "materials/sand", RFL_NONE, GTB_ALL },
	{ &media.efx.footstep.snow, "materials/snow", RFL_NONE, GTB_ALL },
	{ &media.efx.force.confusionOld, "force/confusion_old", RFL_NONE, GTB_ALL },
	{ &media.efx.force.drain, "mp/drain", RFL_NONE, GTB_ALL },
	{ &media.efx.force.drained, "mp/drainhit", RFL_NONE, GTB_ALL },
	{ &media.efx.force.drainWide, "mp/drainwide", RFL_NONE, GTB_ALL },
	{ &media.efx.force.lightning, "force/lightning", RFL_NONE, GTB_ALL },
	{ &media.efx.force.lightningWide, "force/lightningwide", RFL_NONE, GTB_ALL },
	{ &media.efx.landing.dirt, "materials/dirt_large", RFL_NONE, GTB_ALL },
	{ &media.efx.landing.gravel, "materials/gravel_large", RFL_NONE, GTB_ALL },
	{ &media.efx.landing.mud, "materials/mud_large", RFL_NONE, GTB_ALL },
	{ &media.efx.landing.sand, "materials/sand_large", RFL_NONE, GTB_ALL },
	{ &media.efx.landing.snow, "materials/snow_large", RFL_NONE, GTB_ALL },
	{ &media.efx.pistol.droidImpact, "bryar/droid_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.pistol.fleshImpact, "bryar/flesh_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.pistol.powerupShot, "bryar/crackleShot", RFL_NONE, GTB_ALL },
	{ &media.efx.pistol.shot, "bryar/shot", RFL_NONE, GTB_ALL },
	{ &media.efx.pistol.wallImpact, "bryar/wall_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.pistol.wallImpact2, "bryar/wall_impact2", RFL_NONE, GTB_ALL },
	{ &media.efx.pistol.wallImpact3, "bryar/wall_impact3", RFL_NONE, GTB_ALL },
	{ &media.efx.portal.blue, "effects/env/portal1", RFL_NONE, GTB_ALL },
	{ &media.efx.portal.orange, "effects/env/portal2", RFL_NONE, GTB_ALL },
	{ &media.efx.repeater.altProjectile, "repeater/alt_projectile", RFL_NONE, GTB_ALL },
	{ &media.efx.repeater.altWallImpact, "repeater/concussion", RFL_NONE, GTB_ALL },
	{ &media.efx.repeater.fleshImpact, "repeater/flesh_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.repeater.projectile, "repeater/projectile", RFL_NONE, GTB_ALL },
	{ &media.efx.repeater.wallImpact, "repeater/wall_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.rocket.explosion, "rocket/explosion", RFL_NONE, GTB_ALL },
	{ &media.efx.rocket.shot, "rocket/shot", RFL_NONE, GTB_ALL },
	{ &media.efx.splash.acid, "env/acid_splash", RFL_NONE, GTB_ALL },
	{ &media.efx.splash.lava, "env/lava_splash", RFL_NONE, GTB_ALL },
	{ &media.efx.splash.water, "env/water_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.stunbaton.fleshImpact, "stunBaton/flesh_impact", RFL_NONE, GTB_ALL },
	{ &media.efx.thermal.explosion, "thermal/explosion", RFL_NONE, GTB_ALL },
	{ &media.efx.thermal.shockwave, "thermal/shockwave", RFL_NONE, GTB_ALL },
	{ &media.efx.tripmine.explosion, "tripMine/explosion", RFL_NONE, GTB_ALL },
	{ &media.efx.tripmine.glow, "tripMine/glowbit", RFL_NONE, GTB_ALL },
	{ &media.efx.tripmine.laser, "tripMine/laserMP", RFL_NONE, GTB_ALL },
	{ &media.efx.blackSmoke, "volumetric/black_smoke", RFL_NONE, GTB_ALL },
	{ &media.efx.blasterDeflect, "blaster/deflect", RFL_NONE, GTB_ALL },
	{ &media.efx.blasterSmoke, "blaster/smoke_bolton", RFL_NONE, GTB_ALL },
	{ &media.efx.bobaJet, "boba/jet", RFL_NONE, GTB_ALL },
	{ &media.efx.disruptorDeathSmoke, "disruptor/death_smoke", RFL_NONE, GTB_ALL },
	{ &media.efx.emplacedDeadSmoke, "emplaced/dead_smoke", RFL_NONE, GTB_ALL },
	{ &media.efx.emplacedExplode, "emplaced/explode", RFL_NONE, GTB_ALL },
	{ &media.efx.emplacedMuzzleFlash, "emplaced/muzzle_flash", RFL_NONE, GTB_ALL },
	{ &media.efx.flamethrower, "effects/bespin/flamejet", RFL_NONE, GTB_ALL },
	{ &media.efx.hyperspaceStars, "ships/hyperspace_stars", RFL_NONE, GTB_ALL },
	{ &media.efx.itemCone, "mp/itemcone", RFL_NONE, GTB_ALL },
	{ &media.efx.jediSpawn, "mp/jedispawn", RFL_NONE, GTB_ALL },
	{ &media.efx.saberBlock, "saber/saber_block", RFL_NONE, GTB_ALL },
	{ &media.efx.saberBloodSparks, "saber/blood_sparks_mp", RFL_NONE, GTB_ALL },
	{ &media.efx.saberBloodSparksMid, "saber/blood_sparks_50_mp", RFL_NONE, GTB_ALL },
	{ &media.efx.saberBloodSparksSmall, "saber/blood_sparks_25_mp", RFL_NONE, GTB_ALL },
	{ &media.efx.saberCut, "saber/saber_cut", RFL_NONE, GTB_ALL },
	{ &media.efx.shipDestBurning, "ships/dest_burning", RFL_NONE, GTB_ALL },
	{ &media.efx.shipDestDestroyed, "ships/dest_destroyed", RFL_NONE, GTB_ALL },
	{ &media.efx.sparkExplosion, "sparks/spark_explosion", RFL_NONE, GTB_ALL },
	{ &media.efx.sparks, "sparks/spark_nosnd", RFL_NONE, GTB_ALL },
	{ &media.efx.sparksExplodeNoSound, "sparks/spark_exp_nosnd", RFL_NONE, GTB_ALL },
	{ &media.efx.spawn, "mp/spawn", RFL_NONE, GTB_ALL },
	{ &media.efx.turretExplode, "turret/explode", RFL_NONE, GTB_ALL },
	{ &media.efx.turretMuzzleFlash, "turret/muzzle_flash", RFL_NONE, GTB_ALL },
	{ &media.efx.turretShot, "turret/shot", RFL_NONE, GTB_ALL },
	{ NULL, "effects/mp/test_sparks", RFL_NONE, GTB_ALL },
	{ NULL, "effects/mp/test_wall_impact", RFL_NONE, GTB_ALL },
	{ NULL, "force/force_touch", RFL_NONE, GTB_ALL },
};

static const resource_t models[] = {
	{ &media.models.blueFlag, "models/flags/b_flag.md3", RFL_NONE, GTB_CTF },
	{ &media.models.blueFlag, "models/flags/b_flag_ysal.md3", RFL_NONE, GTB_CTY },
	{ &media.models.chunks[CHUNK_METAL1][0], "models/chunks/metal/metal2_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_METAL1][1], "models/chunks/metal/metal2_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_METAL1][2], "models/chunks/metal/metal2_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_METAL1][3], "models/chunks/metal/metal2_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_METAL2][0], "models/chunks/metal/metal1_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_METAL2][1], "models/chunks/metal/metal1_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_METAL2][2], "models/chunks/metal/metal1_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_METAL2][3], "models/chunks/metal/metal1_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK1][0], "models/chunks/rock/rock1_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK1][1], "models/chunks/rock/rock1_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK1][2], "models/chunks/rock/rock1_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK1][3], "models/chunks/rock/rock1_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK2][0], "models/chunks/rock/rock2_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK2][1], "models/chunks/rock/rock2_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK2][2], "models/chunks/rock/rock2_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK2][3], "models/chunks/rock/rock2_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK3][0], "models/chunks/rock/rock3_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK3][1], "models/chunks/rock/rock3_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK3][2], "models/chunks/rock/rock3_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_ROCK3][3], "models/chunks/rock/rock3_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE1][0], "models/chunks/crate/crate1_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE1][1], "models/chunks/crate/crate1_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE1][2], "models/chunks/crate/crate1_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE1][3], "models/chunks/crate/crate1_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE2][0], "models/chunks/crate/crate2_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE2][1], "models/chunks/crate/crate2_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE2][2], "models/chunks/crate/crate2_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_CRATE2][3], "models/chunks/crate/crate2_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_WHITE_METAL][0], "models/chunks/metal/wmetal1_1.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_WHITE_METAL][1], "models/chunks/metal/wmetal1_2.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_WHITE_METAL][2], "models/chunks/metal/wmetal1_3.md3", RFL_NONE, GTB_ALL },
	{ &media.models.chunks[CHUNK_WHITE_METAL][3], "models/chunks/metal/wmetal1_4.md3", RFL_NONE, GTB_ALL },
	{ &media.models.demp2Shell, "models/items/sphere.md3", RFL_NONE, GTB_ALL },
	{ &media.models.explosion, "models/map_objects/mp/sphere.md3", RFL_NONE, GTB_ALL },
	{ &media.models.forceHolocrons[FP_HEAL], "models/map_objects/mp/lt_heal.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_LEVITATION], "models/map_objects/mp/force_jump.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_SPEED], "models/map_objects/mp/force_speed.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_PUSH], "models/map_objects/mp/force_push.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_PULL], "models/map_objects/mp/force_pull.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_TELEPATHY], "models/map_objects/mp/lt_telepathy.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_GRIP], "models/map_objects/mp/dk_grip.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_LIGHTNING], "models/map_objects/mp/dk_lightning.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_RAGE], "models/map_objects/mp/dk_rage.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_PROTECT], "models/map_objects/mp/lt_protect.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_ABSORB], "models/map_objects/mp/lt_absorb.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_TEAM_HEAL], "models/map_objects/mp/lt_healother.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_TEAM_FORCE], "models/map_objects/mp/dk_powerother.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_DRAIN], "models/map_objects/mp/dk_drain.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_SEE], "models/map_objects/mp/force_sight.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_SABER_OFFENSE], "models/map_objects/mp/saber_attack.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_SABER_DEFENSE], "models/map_objects/mp/saber_defend.md3", RFL_NONE, GTB_CTY },
	{ &media.models.forceHolocrons[FP_SABERTHROW], "models/map_objects/mp/saber_throw.md3", RFL_NONE, GTB_CTY },
	{ &media.models.halfShield, "models/weaphits/testboom.md3", RFL_NONE, GTB_ALL },
	{ &media.models.itemHolo, "models/map_objects/mp/holo.md3", RFL_NONE, GTB_ALL },
	{ &media.models.redFlag, "models/flags/r_flag.md3", RFL_NONE, GTB_CTF },
	{ &media.models.redFlag, "models/flags/r_flag_ysal.md3", RFL_NONE, GTB_CTY },
	{ &media.models.seeker, "models/items/remote.md3", RFL_NONE, GTB_ALL },
	{ NULL, "models/map_objects/mp/sphere.md3", RFL_NONE, GTB_ALL },
};

static const size_t	numSounds = ARRAY_LEN( sounds ),
numGraphics = ARRAY_LEN( gfx ),
numEffects = ARRAY_LEN( efx ),
numModels = ARRAY_LEN( models );

static size_t numLoadedResources = 0;
static qboolean loadedMap = qfalse;
static void CG_UpdateLoadBar( void ) {
	const size_t numResources = numSounds + numGraphics + numEffects + numModels;
	const float mapPercent = 0.25f; // let's say maps take 25% of load times

	numLoadedResources++;

	// factor in map load time
	cg.loadFrac = loadedMap ? mapPercent : 0.0f;
	cg.loadFrac += (((float)numLoadedResources / (float)numResources) * (1.0f - mapPercent));
	trap->UpdateScreen();
}

static void CG_LoadResource( const resource_t *resource, qhandle_t( *registerFunc )(const char *location) ) {
	qhandle_t handle;

#ifdef _DEBUG
	CG_LogPrintf( cg.log.debug, "Loading resource: %s\n", resource->location );
#endif
	CG_UpdateLoadBar();

	if ( !(resource->gametypes & (1 << cgs.gametype)) )
		return;

	handle = registerFunc( resource->location );

	if ( !handle ) {
		CG_LogPrintf( cg.log.debug, "Missing resource: %s\n", resource->location );
	}
	if ( resource->handle ) {
		*(qhandle_t *)resource->handle = handle;
	}
}

// the server says this item is used on this level
static void CG_RegisterItemSounds( int itemNum ) {
	const gitem_t *item = &bg_itemlist[itemNum];
	char data[MAX_QPATH];
	const char *s, *start;
	int len;

	if ( item->pickup_sound ) {
		trap->S_RegisterSound( item->pickup_sound );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if ( !s || !s[0] ) {
		return;
	}

	while ( *s ) {
		start = s;
		while ( *s && *s != ' ' ) {
			s++;
		}

		len = s - start;
		if ( len >= MAX_QPATH || len < 5 ) {
			trap->Error( ERR_DROP, "PrecacheItem: %s has bad precache string", item->classname );
			return;
		}
		memcpy( data, start, len );
		data[len] = '\0';
		if ( *s ) {
			s++;
		}

		trap->S_RegisterSound( data );
	}

	// parse the space seperated precache string for other media
	s = item->precaches;
	if ( !s || !s[0] )
		return;

	while ( *s ) {
		start = s;
		while ( *s && *s != ' ' ) {
			s++;
		}

		len = s - start;
		if ( len >= MAX_QPATH || len < 5 ) {
			trap->Error( ERR_DROP, "PrecacheItem: %s has bad precache string", item->classname );
			return;
		}
		memcpy( data, start, len );
		data[len] = '\0';
		if ( *s )
			s++;

		if ( !strcmp( data + len - 3, "efx" ) )
			trap->FX_RegisterEffect( data );
	}
}

static void CG_RegisterSounds( void ) {
	const resource_t *resource = NULL;
	size_t i;
	const char *soundName = NULL;

	for ( i = 0, resource = sounds; i < numSounds; i++, resource++ ) {
		CG_LoadResource( resource, trap->S_RegisterSound );
	}

	for ( i = 1; i < MAX_SOUNDS; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS + i );

		if ( !soundName[0] ) {
			break;
		}

		// custom sounds
		if ( soundName[0] == '*' ) {
			if ( soundName[1] == '$' ) {
				CG_PrecacheNPCSounds( soundName );
			}
			continue;
		}
		cgs.gameSounds[i] = trap->S_RegisterSound( soundName );
	}

	// load the ambient sets
	trap->AS_AddPrecacheEntry( "#clear" );

	for ( i = 1; i < MAX_AMBIENT_SETS; i++ ) {
		soundName = CG_ConfigString( CS_AMBIENT_SET + i );
		if ( !soundName || !soundName[0] )
			break;

		trap->AS_AddPrecacheEntry( soundName );
	}
	soundName = CG_ConfigString( CS_GLOBAL_AMBIENT_SET );
	if ( soundName && soundName[0] && Q_stricmp( soundName, "default" ) )
		trap->AS_AddPrecacheEntry( soundName );

	trap->AS_ParseSets();
}

static void CG_RegisterEffects( void ) {
	const resource_t *resource = NULL;
	size_t i;
	const char *effectName = NULL;

	for ( i = 0, resource = efx; i < numEffects; i++, resource++ ) {
		CG_LoadResource( resource, trap->FX_RegisterEffect );
	}

	for ( i = 1; i < MAX_FX; i++ ) {
		effectName = CG_ConfigString( CS_EFFECTS + i );

		if ( !effectName[0] ) {
			break;
		}

		if ( effectName[0] == '*' ) {
			// it's a special global weather effect
			CG_ParseWeatherEffect( effectName );
			cgs.gameEffects[i] = 0;
		}
		else {
			cgs.gameEffects[i] = trap->FX_RegisterEffect( effectName );
		}
	}

	CG_InitGlass();
}

static void CG_RegisterGraphics( void ) {
	const resource_t *resource = NULL;
	size_t i;

	for ( i = 0, resource = gfx; i < numGraphics; i++, resource++ ) {
		if ( resource->flags & RFL_NOMIP ) {
			CG_LoadResource( resource, trap->R_RegisterShaderNoMip );
		}
		else {
			CG_LoadResource( resource, trap->R_RegisterShader );
		}
	}

	for ( i = 1; i < MAX_ICONS; i++ ) {
		const char *iconName = CG_ConfigString( CS_ICONS + i );

		if ( !iconName[0] ) {
			break;
		}

		cgs.gameIcons[i] = trap->R_RegisterShaderNoMip( iconName );
	}

	//check for cg_drawRewards
	if (cg_drawRewards.integer){
		if (!(media.gfx.interface.medals.assist &&
			media.gfx.interface.medals.capture &&
			media.gfx.interface.medals.defend &&
			media.gfx.interface.medals.excellent &&
			media.gfx.interface.medals.gauntlet &&
			media.gfx.interface.medals.impressive)){
			trap->Cvar_Set("cg_drawRewards", "0");
			trap->Cvar_Update(&cg_drawRewards);
		}
	}
}

static void CG_RegisterModels( void ) {
	const resource_t *resource = NULL;
	size_t i;
	const char *terrainInfo = NULL;
	int breakPoint;

	for ( i = 0, resource = models; i < numModels; i++, resource++ ) {
		CG_LoadResource( resource, trap->R_RegisterModel );
	}

	CG_LoadingString( "Inline models" );
	breakPoint = cgs.numInlineModels = (size_t)trap->CM_NumInlineModels();
	for ( i = 1; i < cgs.numInlineModels; i++ ) {
		char name[10];
		vector3 mins, maxs;
		int j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap->R_RegisterModel( name );
		if ( !cgs.inlineDrawModel[i] ) {
			breakPoint = i;
			break;
		}

		trap->R_ModelBounds( cgs.inlineDrawModel[i], &mins, &maxs );
		for ( j = 0; j < 3; j++ )
			cgs.inlineModelMidpoints[i].raw[j] = mins.raw[j] + 0.5f * (maxs.raw[j] - mins.raw[j]);
	}

	CG_LoadingString( "Server models" );
	for ( i = 1; i < MAX_MODELS; i++ ) {
		const char *cModelName;
		char modelName[MAX_QPATH];

		cModelName = CG_ConfigString( CS_MODELS + i );
		if ( !cModelName[0] )
			break;

		Q_strncpyz( modelName, cModelName, sizeof(modelName) );

		// Check to see if it has a custom skin attached.
		if ( strstr( modelName, ".glm" ) || modelName[0] == '$' ) {
			CG_HandleAppendedSkin( modelName );
			CG_CacheG2AnimInfo( modelName );
		}

		// don't register vehicle names and saber names as models.
		if ( modelName[0] != '$' && modelName[0] != '@' )
			cgs.gameModels[i] = trap->R_RegisterModel( modelName );
		else
			cgs.gameModels[i] = 0; //FIXME: register here so that stuff gets precached!!!
	}

	size_t numBSPInstances = 0;
	for ( i = 1; i < MAX_SUB_BSP; i++ ) {
		const char *bspName = CG_ConfigString( CS_BSP_MODELS + i );
		if ( bspName[0] ) {
			numBSPInstances++;
		}
	}
	for ( i = 1; i < numBSPInstances + 1; i++ ) {
		const char *bspName = CG_ConfigString( CS_BSP_MODELS + i );
		if ( !bspName[0] ) {
			break;
		}

		char loadingStr[1024];
		Com_sprintf( loadingStr, sizeof(loadingStr), "Loading BSP instance %i/%i: %s",
			i, numBSPInstances, bspName+1
		);
		CG_LoadingString( loadingStr );

		trap->CM_LoadMap( bspName, qtrue );
		cgs.inlineDrawModel[breakPoint] = trap->R_RegisterModel( bspName );
		vector3 mins, maxs;
		trap->R_ModelBounds( cgs.inlineDrawModel[breakPoint], &mins, &maxs );
		for ( int j = 0; j < 3; j++ ) {
			cgs.inlineModelMidpoints[breakPoint].raw[j] = mins.raw[j] + 0.5f * (maxs.raw[j] - mins.raw[j]);
		}
		breakPoint++;

		for ( int sub = 1; sub < MAX_MODELS; sub++ ) {
			char temp[MAX_QPATH];
			Com_sprintf( temp, MAX_QPATH, "*%d-%d", i, sub );
			cgs.inlineDrawModel[breakPoint] = trap->R_RegisterModel( temp );
			if ( !cgs.inlineDrawModel[breakPoint] ) {
				break;
			}

			trap->R_ModelBounds( cgs.inlineDrawModel[breakPoint], &mins, &maxs );
			for ( int j = 0; j < 3; j++ ) {
				cgs.inlineModelMidpoints[breakPoint].raw[j] = mins.raw[j] + 0.5f * (maxs.raw[j] - mins.raw[j]);
			}
			breakPoint++;
		}
	}

	CG_LoadingString( "Creating terrain" );
	for ( i = 0; i < MAX_TERRAINS; i++ ) {
		terrainInfo = CG_ConfigString( CS_TERRAINS + i );
		if ( !terrainInfo[0] )
			break;

		trap->RMG_Init( trap->CM_RegisterTerrain( terrainInfo ), terrainInfo );

		// Send off the terrainInfo to the renderer
		trap->RE_InitRendererTerrain( terrainInfo );
	}
}

static void CG_RegisterItems( void ) {
	char items[MAX_ITEMS + 1];
	size_t i;

	memset( cg_items, 0, sizeof(cg_items) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );

	Q_strncpyz( items, CG_ConfigString( CS_ITEMS ), sizeof(items) );

	for ( i = 1; i < bg_numItems; i++ ) {
		if ( items[i] == '1' ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
			CG_RegisterItemSounds( i );
		}
	}

	CG_InitG2Weapons();
}

void CG_LoadMedia( void ) {
	refdef_t *refdef = CG_GetRefdef();

	// sounds
	CG_LoadingString( "Sounds" );
	CG_RegisterSounds();

	// graphics
	CG_LoadingString( "Graphics" );
	CG_RegisterGraphics();
	memset( &cg.refdef[0], 0, sizeof(cg.refdef) );
	trap->R_ClearScene();
	CG_LoadingString( va( "Collision map (%s)", cgs.mapname ) );
	trap->CM_LoadMap( cgs.mapname, qfalse );
	CG_LoadingString( va( "Map (%s)", cgs.mapname ) );
	trap->R_LoadWorld( cgs.mapname );
	loadedMap = qtrue;
	CG_LoadingString( "Automap" );
	trap->R_InitializeWireframeAutomap();

	// effects
	CG_LoadingString( "Effects subsystem" );
	trap->FX_InitSystem( refdef );
	CG_LoadingString( "Effects" );
	CG_RegisterEffects();

	// models
	CG_LoadingString( "Models" );
	CG_RegisterModels();

	// items
	CG_LoadingString( "Items" );
	CG_RegisterItems();
}

void CG_PreloadMedia( void ) {
	cgDC.Assets.qhSmallFont = CG_Cvar_Get( "font_small" );
	cgDC.Assets.qhSmall2Font = CG_Cvar_Get( "font_small2" );
	cgDC.Assets.qhMediumFont = CG_Cvar_Get( "font_medium" );
	cgDC.Assets.japp.fontLarge = CG_Cvar_Get( "font_japplarge" );
	cgDC.Assets.japp.fontSmall = CG_Cvar_Get( "font_jappsmall" );
	cgDC.Assets.japp.fontMono = CG_Cvar_Get( "font_jappmono" );
	cgDC.Assets.qhBigFont = CG_Cvar_Get( "font_big" );

	media.gfx.interface.charset = trap->R_RegisterShaderNoMip( "gfx/2d/charsgrid_med" );
	media.gfx.world.whiteShader = trap->R_RegisterShader( "white" );
	media.gfx.interface.loadBarLED = trap->R_RegisterShaderNoMip( "gfx/hud/load_tick" );
	media.gfx.interface.loadBarLEDCap = trap->R_RegisterShaderNoMip( "gfx/hud/load_tick_cap" );
	media.gfx.interface.loadBarLEDSurround = trap->R_RegisterShaderNoMip( "gfx/hud/mp_levelload" );
}
