// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_misc.c -- both games misc functions, all completely stateless

#include "qcommon/q_shared.h"
#include "bg_public.h"

#if defined( PROJECT_GAME )
	#include "g_local.h"
#elif defined( PROJECT_UI )
	#include "ui_local.h"
#elif defined( PROJECT_CGAME )
	#include "cg_local.h"
#endif

#include "JAPP/jp_cinfo.h"

#ifdef PROJECT_GAME
void Q3_SetParm( int entID, int parmNum, const char *parmValue );
#endif

const char *bgToggleableSurfaces[BG_NUM_TOGGLEABLE_SURFACES] = {
	"l_arm_key",					//0
	"torso_canister1",
	"torso_canister2",
	"torso_canister3",
	"torso_tube1",
	"torso_tube2",					//5
	"torso_tube3",
	"torso_tube4",
	"torso_tube5",
	"torso_tube6",
	"r_arm",						//10
	"l_arm",
	"torso_shield",
	"torso_galaktorso",
	"torso_collar",
	//	"torso_eyes_mouth",				//15
	//	"torso_galakhead",
	//	"torso_galakface",
	//	"torso_antenna_base_cap",
	//	"torso_antenna",
	//	"l_arm_augment",				//20
	//	"l_arm_middle",
	//	"l_arm_wrist",
	//	"r_arm_middle", //yeah.. galak's surf stuff is no longer auto, sorry! need the space for vehicle surfs.
	"r_wing1",						//15
	"r_wing2",
	"l_wing1",
	"l_wing2",
	"r_gear",
	"l_gear",						//20
	"nose",
	"blah4",
	"blah5",
	"l_hand",
	"r_hand",						//25
	"helmet",
	"head",
	"head_concussion_charger",
	"head_light_blaster_cann",		//29
	NULL
};

const int bgToggleableSurfaceDebris[BG_NUM_TOGGLEABLE_SURFACES] =
{
	0,					//0
	0,
	0,
	0,
	0,
	0,					//5
	0,
	0,
	0,
	0,
	0,					//10
	0,
	0,
	0,
	0, //>= 2 means it should create a flame trail when destroyed (for vehicles)
	3,					//15
	5, //rwing2
	4,
	6, //lwing2
	0, //rgear
	0, //lgear			//20
	7, //nose
	0, //blah
	0, //blah
	0,
	0,					//25
	0,
	0,
	0,
	0,					//29
	-1
};

const char	*bg_customSiegeSoundNames[MAX_CUSTOM_SIEGE_SOUNDS] = {
	"*att_attack",
	"*att_primary",
	"*att_second",
	"*def_guns",
	"*def_position",
	"*def_primary",
	"*def_second",
	"*reply_coming",
	"*reply_go",
	"*reply_no",
	"*reply_stay",
	"*reply_yes",
	"*req_assist",
	"*req_demo",
	"*req_hvy",
	"*req_medic",
	"*req_sup",
	"*req_tech",
	"*spot_air",
	"*spot_defenses",
	"*spot_emplaced",
	"*spot_sniper",
	"*spot_troops",
	"*tac_cover",
	"*tac_fallback",
	"*tac_follow",
	"*tac_hold",
	"*tac_split",
	"*tac_together",
	NULL
};

//rww - not putting @ in front of these because
//we don't need them in a cgame StringEd lookup.
//Let me know if this causes problems, pat.
const char *forceMasteryLevels[NUM_FORCE_MASTERY_LEVELS] =
{
	"MASTERY0",	//"Uninitiated",	// FORCE_MASTERY_UNINITIATED,
	"MASTERY1",	//"Initiate",		// FORCE_MASTERY_INITIATE,
	"MASTERY2",	//"Padawan",		// FORCE_MASTERY_PADAWAN,
	"MASTERY3",	//"Jedi",			// FORCE_MASTERY_JEDI,
	"MASTERY4",	//"Jedi Adept",		// FORCE_MASTERY_JEDI_GUARDIAN,
	"MASTERY5",	//"Jedi Guardian",	// FORCE_MASTERY_JEDI_ADEPT,
	"MASTERY6",	//"Jedi Knight",	// FORCE_MASTERY_JEDI_KNIGHT,
	"MASTERY7",	//"Jedi Master"		// FORCE_MASTERY_JEDI_MASTER,
};

const int forceMasteryPoints[NUM_FORCE_MASTERY_LEVELS] =
{
	0,		// FORCE_MASTERY_UNINITIATED,
	5,		// FORCE_MASTERY_INITIATE,
	10,		// FORCE_MASTERY_PADAWAN,
	20,		// FORCE_MASTERY_JEDI,
	30,		// FORCE_MASTERY_JEDI_GUARDIAN,
	50,		// FORCE_MASTERY_JEDI_ADEPT,
	75,		// FORCE_MASTERY_JEDI_KNIGHT,
	100		// FORCE_MASTERY_JEDI_MASTER,
};

int bgForcePowerCost[NUM_FORCE_POWERS][NUM_FORCE_POWER_LEVELS] = //0 == neutral
{
	{ 0, 2, 4, 6 },	// Heal			// FP_HEAL
	{ 0, 0, 2, 6 },	// Jump			//FP_LEVITATION,//hold/duration
	{ 0, 2, 4, 6 },	// Speed		//FP_SPEED,//duration
	{ 0, 1, 3, 6 },	// Push			//FP_PUSH,//hold/duration
	{ 0, 1, 3, 6 },	// Pull			//FP_PULL,//hold/duration
	{ 0, 4, 6, 8 },	// Mind Trick	//FP_TELEPATHY,//instant
	{ 0, 1, 3, 6 },	// Grip			//FP_GRIP,//hold/duration
	{ 0, 2, 5, 8 },	// Lightning	//FP_LIGHTNING,//hold/duration
	{ 0, 4, 6, 8 },	// Dark Rage	//FP_RAGE,//duration
	{ 0, 2, 5, 8 },	// Protection	//FP_PROTECT,//duration
	{ 0, 1, 3, 6 },	// Absorb		//FP_ABSORB,//duration
	{ 0, 1, 3, 6 },	// Team Heal	//FP_TEAM_HEAL,//instant
	{ 0, 1, 3, 6 },	// Team Force	//FP_TEAM_FORCE,//instant
	{ 0, 2, 4, 6 },	// Drain		//FP_DRAIN,//hold/duration
	{ 0, 2, 5, 8 },	// Sight		//FP_SEE,//duration
	{ 0, 1, 5, 8 },	// Saber Attack	//FP_SABER_OFFENSE,
	{ 0, 1, 5, 8 },	// Saber Defend	//FP_SABER_DEFENSE,
	{ 0, 4, 6, 8 }	// Saber Throw	//FP_SABERTHROW,
	//NUM_FORCE_POWERS
};

const int forcePowerSorted[NUM_FORCE_POWERS] = { //rww - always use this order when drawing force powers for any reason
	FP_TELEPATHY,
	FP_HEAL,
	FP_ABSORB,
	FP_PROTECT,
	FP_TEAM_HEAL,
	FP_LEVITATION,
	FP_SPEED,
	FP_PUSH,
	FP_PULL,
	FP_SEE,
	FP_LIGHTNING,
	FP_DRAIN,
	FP_RAGE,
	FP_GRIP,
	FP_TEAM_FORCE,
	FP_SABER_OFFENSE,
	FP_SABER_DEFENSE,
	FP_SABERTHROW
};

const int forcePowerDarkLight[NUM_FORCE_POWERS] = //0 == neutral
{ //nothing should be usable at rank 0..
	FORCESIDE_LIGHT,//FP_HEAL,//instant
	0,//FP_LEVITATION,//hold/duration
	0,//FP_SPEED,//duration
	0,//FP_PUSH,//hold/duration
	0,//FP_PULL,//hold/duration
	FORCESIDE_LIGHT,//FP_TELEPATHY,//instant
	FORCESIDE_DARK,//FP_GRIP,//hold/duration
	FORCESIDE_DARK,//FP_LIGHTNING,//hold/duration
	FORCESIDE_DARK,//FP_RAGE,//duration
	FORCESIDE_LIGHT,//FP_PROTECT,//duration
	FORCESIDE_LIGHT,//FP_ABSORB,//duration
	FORCESIDE_LIGHT,//FP_TEAM_HEAL,//instant
	FORCESIDE_DARK,//FP_TEAM_FORCE,//instant
	FORCESIDE_DARK,//FP_DRAIN,//hold/duration
	0,//FP_SEE,//duration
	0,//FP_SABER_OFFENSE,
	0,//FP_SABER_DEFENSE,
	0//FP_SABERTHROW,
	//NUM_FORCE_POWERS
};

const int WeaponReadyAnim[WP_NUM_WEAPONS] =
{
	TORSO_DROPWEAP1,//WP_NONE,

	TORSO_WEAPONREADY3,//WP_STUN_BATON,
	TORSO_WEAPONREADY3,//WP_MELEE,
	BOTH_STAND2,//WP_SABER,
	TORSO_WEAPONREADY2,//WP_BRYAR_PISTOL,
	TORSO_WEAPONREADY3,//WP_BLASTER,
	TORSO_WEAPONREADY3,//TORSO_WEAPONREADY4,//WP_DISRUPTOR,
	TORSO_WEAPONREADY3,//TORSO_WEAPONREADY5,//WP_BOWCASTER,
	TORSO_WEAPONREADY3,//TORSO_WEAPONREADY6,//WP_REPEATER,
	TORSO_WEAPONREADY3,//TORSO_WEAPONREADY7,//WP_DEMP2,
	TORSO_WEAPONREADY3,//TORSO_WEAPONREADY8,//WP_FLECHETTE,
	TORSO_WEAPONREADY3,//TORSO_WEAPONREADY9,//WP_ROCKET_LAUNCHER,
	TORSO_WEAPONREADY10,//WP_THERMAL,
	TORSO_WEAPONREADY10,//TORSO_WEAPONREADY11,//WP_TRIP_MINE,
	TORSO_WEAPONREADY10,//TORSO_WEAPONREADY12,//WP_DET_PACK,
	TORSO_WEAPONREADY3,//WP_CONCUSSION
	TORSO_WEAPONREADY2,//WP_BRYAR_OLD,

	//NOT VALID (e.g. should never really be used):
	BOTH_STAND1,//WP_EMPLACED_GUN,
	TORSO_WEAPONREADY1//WP_TURRET,
};

const int WeaponReadyLegsAnim[WP_NUM_WEAPONS] = {
	BOTH_STAND1,//WP_NONE,

	BOTH_STAND1,//WP_STUN_BATON,
	BOTH_STAND1,//WP_MELEE,
	BOTH_STAND2,//WP_SABER,
	BOTH_STAND1,//WP_BRYAR_PISTOL,
	BOTH_STAND1,//WP_BLASTER,
	BOTH_STAND1,//TORSO_WEAPONREADY4,//WP_DISRUPTOR,
	BOTH_STAND1,//TORSO_WEAPONREADY5,//WP_BOWCASTER,
	BOTH_STAND1,//TORSO_WEAPONREADY6,//WP_REPEATER,
	BOTH_STAND1,//TORSO_WEAPONREADY7,//WP_DEMP2,
	BOTH_STAND1,//TORSO_WEAPONREADY8,//WP_FLECHETTE,
	BOTH_STAND1,//TORSO_WEAPONREADY9,//WP_ROCKET_LAUNCHER,
	BOTH_STAND1,//WP_THERMAL,
	BOTH_STAND1,//TORSO_WEAPONREADY11,//WP_TRIP_MINE,
	BOTH_STAND1,//TORSO_WEAPONREADY12,//WP_DET_PACK,
	BOTH_STAND1,//WP_CONCUSSION
	BOTH_STAND1,//WP_BRYAR_OLD,

	//NOT VALID (e.g. should never really be used):
	BOTH_STAND1,//WP_EMPLACED_GUN,
	BOTH_STAND1//WP_TURRET,
};

const int WeaponAttackAnim[WP_NUM_WEAPONS] = {
	BOTH_ATTACK1,//WP_NONE, //(shouldn't happen)

	BOTH_ATTACK3,//WP_STUN_BATON,
	BOTH_ATTACK3,//WP_MELEE,
	BOTH_STAND2,//WP_SABER, //(has its own handling)
	BOTH_ATTACK2,//WP_BRYAR_PISTOL,
	BOTH_ATTACK3,//WP_BLASTER,
	BOTH_ATTACK3,//BOTH_ATTACK4,//WP_DISRUPTOR,
	BOTH_ATTACK3,//BOTH_ATTACK5,//WP_BOWCASTER,
	BOTH_ATTACK3,//BOTH_ATTACK6,//WP_REPEATER,
	BOTH_ATTACK3,//BOTH_ATTACK7,//WP_DEMP2,
	BOTH_ATTACK3,//BOTH_ATTACK8,//WP_FLECHETTE,
	BOTH_ATTACK3,//BOTH_ATTACK9,//WP_ROCKET_LAUNCHER,
	BOTH_THERMAL_THROW,//WP_THERMAL,
	BOTH_ATTACK3,//BOTH_ATTACK11,//WP_TRIP_MINE,
	BOTH_ATTACK3,//BOTH_ATTACK12,//WP_DET_PACK,
	BOTH_ATTACK3,//WP_CONCUSSION, //Raz: Fixed bryar pistol animation
	BOTH_ATTACK2,//WP_BRYAR_OLD,

	//NOT VALID (e.g. should never really be used):
	BOTH_STAND1,//WP_EMPLACED_GUN,


	BOTH_ATTACK1//WP_TURRET,
};

const stringID_table_t eTypes[ET_MAX] = {
	ENUM2STRING( ET_GENERAL ),
	ENUM2STRING( ET_PLAYER ),
	ENUM2STRING( ET_ITEM ),
	ENUM2STRING( ET_MISSILE ),
	ENUM2STRING( ET_SPECIAL ),
	ENUM2STRING( ET_HOLOCRON ),
	ENUM2STRING( ET_MOVER ),
	ENUM2STRING( ET_BEAM ),
	ENUM2STRING( ET_PORTAL ),
	ENUM2STRING( ET_SPEAKER ),
	ENUM2STRING( ET_PUSH_TRIGGER ),
	ENUM2STRING( ET_TELEPORT_TRIGGER ),
	ENUM2STRING( ET_INVISIBLE ),
	ENUM2STRING( ET_NPC ),
	ENUM2STRING( ET_TEAM ),
	ENUM2STRING( ET_BODY ),
	ENUM2STRING( ET_TERRAIN ),
	ENUM2STRING( ET_FX ),
	ENUM2STRING( ET_EVENTS ),
};

static qboolean BG_FileExists( const char *fileName ) {
	if ( fileName && fileName[0] ) {
		int fh = 0;
		trap->FS_Open( fileName, &fh, FS_READ );
		if ( fh > 0 ) {
			trap->FS_Close( fh );
			return qtrue;
		}
	}

	return qfalse;
}

#ifdef PROJECT_CGAME
char *CG_NewString( const char *string );
#endif

#ifndef PROJECT_UI

static int spawncmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((BG_field_t*)b)->name );
}

// Takes a key/value pair and sets the binary values in a gentity/centity/whatever the hell you want
void BG_ParseField( const BG_field_t *l_fields, int numFields, const char *key, const char *value, byte *ent ) {
	const BG_field_t *f;
	byte *b;
	float v;
	vector3 vec;

	f = (BG_field_t *)bsearch( key, l_fields, numFields, sizeof(BG_field_t), spawncmp );
	if ( f ) {// found it
		b = (byte *)ent;

		switch ( f->type ) {
		case F_LSTRING:
#ifdef PROJECT_GAME
			*(char **)(b + f->ofs) = G_NewString( value );
#else
			*(char **)(b + f->ofs) = CG_NewString( value );
#endif
			break;
		case F_VECTOR:
			//Raz: unsafe sscanf usage
			/* basejka code
			sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
			((float *)(b+f->ofs))[0] = vec[0];
			((float *)(b+f->ofs))[1] = vec[1];
			((float *)(b+f->ofs))[2] = vec[2];
			*/
			if ( sscanf( value, "%f %f %f", &vec.x, &vec.y, &vec.z ) == 3 ) {
				((float *)(b + f->ofs))[0] = vec.x;
				((float *)(b + f->ofs))[1] = vec.y;
				((float *)(b + f->ofs))[2] = vec.z;
			}
			else {
				Com_Printf(
					"BG_ParseField(%s): F_VECTOR (%s:%s) with incorrect amount of arguments. Using null vector\n",
					f->name, key, value
				);
				((float *)(b + f->ofs))[0] = ((float *)(b + f->ofs))[1] = ((float *)(b + f->ofs))[2] = 0.0f;
			}
			break;
		case F_INT:
			*(int *)(b + f->ofs) = atoi( value );
			break;
		case F_FLOAT:
			*(float *)(b + f->ofs) = atof( value );
			break;
		case F_ANGLEHACK:
			v = atof( value );
			((float *)(b + f->ofs))[0] = 0;
			((float *)(b + f->ofs))[1] = v;
			((float *)(b + f->ofs))[2] = 0;
			break;
#ifdef PROJECT_GAME
		case F_PARM1:
		case F_PARM2:
		case F_PARM3:
		case F_PARM4:
		case F_PARM5:
		case F_PARM6:
		case F_PARM7:
		case F_PARM8:
		case F_PARM9:
		case F_PARM10:
		case F_PARM11:
		case F_PARM12:
		case F_PARM13:
		case F_PARM14:
		case F_PARM15:
		case F_PARM16:
			Q3_SetParm( ((gentity_t *)(ent))->s.number, (f->type - F_PARM1), value );
			break;
#endif
		default:
		case F_IGNORE:
			break;
		}
		return;
	}
}

#endif

// This will take the force power string in powerOut and parse through it, then legalize it based on the supposed rank
// and spit it into powerOut, returning true if it was legal to begin with and false if not.
// fpDisabled is actually only expected (needed) from the server, because the ui disables force power selection anyway
// when force powers are disabled on the server.
qboolean BG_LegalizedForcePowers( char *powerOut, size_t powerOutSize, int maxRank, qboolean freeSaber, int teamForce, int gametype, int fpDisabled ) {
	char powerBuf[128], readBuf[128];
	qboolean maintainsValidity = qtrue;
	int powerLen = strlen( powerOut );
	int i = 0, c = 0;
	int allowedPoints = 0, usedPoints = 0, countDown = 0;
	int final_Side;
	int final_Powers[NUM_FORCE_POWERS] = { 0 };

	if ( powerLen >= sizeof(powerBuf) ) {
		trap->Print( "BG_LegalizedForcePowers: powerLen:%i >= sizeof(powerBuf):%i\n", powerLen, sizeof(powerBuf) );
		// This should not happen. If it does, this is obviously a bogus string.
		Q_strncpyz( powerBuf, DEFAULT_FORCEPOWERS, sizeof(powerBuf) );
		maintainsValidity = qfalse;
	}
	else {
		Q_strncpyz( powerBuf, powerOut, sizeof(powerBuf) ); //copy it as the original
	}

	// first of all, print the max rank into the string as the rank
	Q_strncpyz( powerOut, va( "%i-", maxRank ), powerOutSize );

	while ( i < sizeof(powerBuf) && powerBuf[i] && powerBuf[i] != '-' ) {
		i++;
	}
	i++;
	while ( i < sizeof(powerBuf) && powerBuf[i] && powerBuf[i] != '-' ) {
		readBuf[c] = powerBuf[i];
		c++;
		i++;
	}
	readBuf[c] = 0;
	i++;
	//at this point, readBuf contains the intended side
	final_Side = atoi( readBuf );

	if ( final_Side != FORCESIDE_LIGHT && final_Side != FORCESIDE_DARK ) {
		// Not a valid side. You will be dark.
		trap->Print( "BG_LegalizedForcePowers: invalid alignment: %i\n", final_Side );
		final_Side = FORCESIDE_DARK;
		maintainsValidity = qfalse;
	}

	if ( teamForce ) { //If we are under force-aligned teams, make sure we're on the right side.
		if ( final_Side != teamForce ) {
			final_Side = teamForce;
			//maintainsValidity = qfalse;
			//Not doing this, for now. Let them join the team with their filtered powers.
		}
	}

	//Now we have established a valid rank, and a valid side.
	//Read the force powers in, and cut them down based on the various rules supplied.
	c = 0;
	while ( i < sizeof(powerBuf) && powerBuf[i] && powerBuf[i] != '\n' && powerBuf[i] != '\r'  //standard sanity checks
		&& powerBuf[i] >= '0' && powerBuf[i] <= '3' && c < NUM_FORCE_POWERS ) {
		readBuf[0] = powerBuf[i];
		readBuf[1] = 0;
		final_Powers[c] = atoi( readBuf );
		c++;
		i++;
	}

	//final_Powers now contains all the stuff from the string
	//Set the maximum allowed points used based on the max rank level, and count the points actually used.
	allowedPoints = forceMasteryPoints[maxRank];

	i = 0;
	while ( i < NUM_FORCE_POWERS ) { //if this power doesn't match the side we're on, then 0 it now.
		if ( final_Powers[i] &&
			forcePowerDarkLight[i] &&
			forcePowerDarkLight[i] != final_Side ) {
			final_Powers[i] = 0;
			//This is only likely to happen with g_forceBasedTeams. Let it slide.
		}

		if ( final_Powers[i] &&
			(fpDisabled & (1 << i)) ) { //if this power is disabled on the server via said server option, then we don't get it.
			final_Powers[i] = 0;
		}

		i++;
	}

	if ( gametype < GT_TEAM ) { //don't bother with team powers then
		final_Powers[FP_TEAM_HEAL] = 0;
		final_Powers[FP_TEAM_FORCE] = 0;
	}

	usedPoints = 0;
	i = 0;
	while ( i < NUM_FORCE_POWERS ) {
		countDown = Q_clampi( 0, final_Powers[i], NUM_FORCE_POWER_LEVELS );

		while ( countDown > 0 ) {
			usedPoints += bgForcePowerCost[i][countDown]; //[fp index][fp level]
			//if this is jump, or we have a free saber and it's offense or defense, take the level back down on level 1
			if ( countDown == 1 &&
				((i == FP_LEVITATION) ||
				(i == FP_SABER_OFFENSE && freeSaber) ||
				(i == FP_SABER_DEFENSE && freeSaber)) ) {
				usedPoints -= bgForcePowerCost[i][countDown];
			}
			countDown--;
		}

		i++;
	}

	if ( usedPoints > allowedPoints ) { //Time to do the fancy stuff. (meaning, slowly cut parts off while taking a guess at what is most or least important in the config)
		int attemptedCycles = 0;
		int powerCycle = 2;
		int minPow = 0;

		trap->Print( "BG_LegalizedForcePowers: usedPoints:%i > allowedPoints:%i\n", usedPoints, allowedPoints );

		if ( freeSaber ) {
			minPow = 1;
		}

		maintainsValidity = qfalse;

		while ( usedPoints > allowedPoints ) {
			c = 0;

			while ( c < NUM_FORCE_POWERS && usedPoints > allowedPoints ) {
				if ( final_Powers[c] && final_Powers[c] < powerCycle ) { //kill in order of lowest powers, because the higher powers are probably more important
					if ( c == FP_SABER_OFFENSE &&
						(final_Powers[FP_SABER_DEFENSE] > minPow || final_Powers[FP_SABERTHROW] > 0) ) { //if we're on saber attack, only suck it down if we have no def or throw either
						int whichOne = FP_SABERTHROW; //first try throw

						if ( !final_Powers[whichOne] ) {
							whichOne = FP_SABER_DEFENSE; //if no throw, drain defense
						}

						while ( final_Powers[whichOne] > 0 && usedPoints > allowedPoints ) {
							if ( final_Powers[whichOne] > 1 ||
								((whichOne != FP_SABER_OFFENSE || !freeSaber) &&
								(whichOne != FP_SABER_DEFENSE || !freeSaber)) ) { //don't take attack or defend down on level 1 still, if it's free
								usedPoints -= bgForcePowerCost[whichOne][final_Powers[whichOne]];
								final_Powers[whichOne]--;
							}
							else {
								break;
							}
						}
					}
					else {
						while ( final_Powers[c] > 0 && usedPoints > allowedPoints ) {
							if ( final_Powers[c] > 1 ||
								((c != FP_LEVITATION) &&
								(c != FP_SABER_OFFENSE || !freeSaber) &&
								(c != FP_SABER_DEFENSE || !freeSaber)) ) {
								usedPoints -= bgForcePowerCost[c][final_Powers[c]];
								final_Powers[c]--;
							}
							else {
								break;
							}
						}
					}
				}

				c++;
			}

			powerCycle++;
			attemptedCycles++;

			if ( attemptedCycles > NUM_FORCE_POWERS ) { //I think this should be impossible. But just in case.
				break;
			}
		}

		if ( usedPoints > allowedPoints ) { //Still? Fine then.. we will kill all of your powers, except the freebies.
			i = 0;

			while ( i < NUM_FORCE_POWERS ) {
				final_Powers[i] = 0;
				if ( i == FP_LEVITATION ||
					(i == FP_SABER_OFFENSE && freeSaber) ||
					(i == FP_SABER_DEFENSE && freeSaber) ) {
					final_Powers[i] = 1;
				}
				i++;
			}
		}
	}

	if ( freeSaber ) {
		if ( final_Powers[FP_SABER_OFFENSE] < 1 ) {
			final_Powers[FP_SABER_OFFENSE] = 1;
		}
		if ( final_Powers[FP_SABER_DEFENSE] < 1 ) {
			final_Powers[FP_SABER_DEFENSE] = 1;
		}
	}
	if ( final_Powers[FP_LEVITATION] < 1 ) {
		final_Powers[FP_LEVITATION] = 1;
	}

	i = 0;
	while ( i < NUM_FORCE_POWERS ) {
		if ( final_Powers[i] > FORCE_LEVEL_3 ) {
			final_Powers[i] = FORCE_LEVEL_3;
		}
		i++;
	}

	if ( fpDisabled ) { //If we specifically have attack or def disabled, force them up to level 3. It's the way
		//things work for the case of all powers disabled.
		//If jump is disabled, down-cap it to level 1. Otherwise don't do a thing.
		if ( fpDisabled & (1 << FP_LEVITATION) ) {
			final_Powers[FP_LEVITATION] = 1;
		}
		if ( fpDisabled & (1 << FP_SABER_OFFENSE) ) {
			final_Powers[FP_SABER_OFFENSE] = 3;
		}
		if ( fpDisabled & (1 << FP_SABER_DEFENSE) ) {
			final_Powers[FP_SABER_DEFENSE] = 3;
		}
	}

	if ( final_Powers[FP_SABER_OFFENSE] < 1 ) {
		final_Powers[FP_SABER_DEFENSE] = 0;
		final_Powers[FP_SABERTHROW] = 0;
	}

	// We finally have all the force powers legalized and stored locally.
	// Put them all into the string and return the result.
	// We already have the rank there, so print the side and the powers now.
	Q_strcat( powerOut, powerOutSize, va( "%i-", final_Side ) );

	i = strlen( powerOut );
	c = 0;
	while ( c < NUM_FORCE_POWERS ) {
		Q_strncpyz( readBuf, va( "%i", final_Powers[c] ), sizeof(readBuf) );
		powerOut[i] = readBuf[0];
		c++;
		i++;
	}
	powerOut[i] = 0;

	return maintainsValidity;
}

const gitem_t bg_itemlist[] = {
	// classname	pickup_sound	icon	quantity	type	tag		view_model	world_model		precache	sounds	description
	{ NULL, NULL, NULL, 0, IT_BAD, 0, NULL, { NULL, NULL, NULL }, "", "", "" }, // leave index 0 alone
	{ "item_shield_sm_instant", "sound/player/pickupshield.wav", "gfx/mp/small_shield", 25, IT_ARMOR, 1, NULL, { "models/map_objects/mp/psd_sm.md3", NULL, NULL }, "", "", "" },
	{ "item_shield_lrg_instant", "sound/player/pickupshield.wav", "gfx/mp/large_shield", 100, IT_ARMOR, 2, NULL, { "models/map_objects/mp/psd.md3", NULL, NULL }, "", "", "" },
	{ "item_medpak_instant", "sound/player/pickuphealth.wav", "gfx/hud/i_icon_medkit", 25, IT_HEALTH, 0, NULL, { "models/map_objects/mp/medpac.md3", NULL, NULL }, "", "", "" },
	{ "item_seeker", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_seeker", 120, IT_HOLDABLE, HI_SEEKER, NULL, { "models/items/remote.md3", NULL, NULL }, "", "", "@MENUS_AN_ATTACK_DRONE_SIMILAR" },
	{ "item_shield", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_shieldwall", 120, IT_HOLDABLE, HI_SHIELD, NULL, { "models/map_objects/mp/shield.md3", NULL, NULL }, "", "sound/weapons/detpack/stick.wav sound/movers/doors/forcefield_on.wav sound/movers/doors/forcefield_off.wav sound/movers/doors/forcefield_lp.wav sound/effects/bumpfield.wav", "@MENUS_THIS_STATIONARY_ENERGY" },
	{ "item_medpac", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_bacta", 25, IT_HOLDABLE, HI_MEDPAC, NULL, { "models/map_objects/mp/bacta.md3", NULL, NULL }, "", "", "@SP_INGAME_BACTA_DESC" },
	{ "item_medpac_big", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_big_bacta", 25, IT_HOLDABLE, HI_MEDPAC_BIG, NULL, { "models/items/big_bacta.md3", NULL, NULL }, "", "", "@SP_INGAME_BACTA_DESC" },
	{ "item_binoculars", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_zoom", 60, IT_HOLDABLE, HI_BINOCULARS, NULL, { "models/items/binoculars.md3", NULL, NULL }, "", "", "@SP_INGAME_LA_GOGGLES_DESC" },
	{ "item_sentry_gun", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_sentrygun", 120, IT_HOLDABLE, HI_SENTRY_GUN, NULL, { "models/items/psgun.glm", NULL, NULL }, "", "", "@MENUS_THIS_DEADLY_WEAPON_IS" },
	{ "item_jetpack", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_jetpack", 120, IT_HOLDABLE, HI_JETPACK, NULL, { "models/items/psgun.glm", NULL, NULL }, "effects/boba/jet.efx", "sound/chars/boba/JETON.wav sound/chars/boba/JETHOVER.wav sound/effects/fire_lp.wav", "@MENUS_JETPACK_DESC" },
	{ "item_healthdisp", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_healthdisp", 120, IT_HOLDABLE, HI_HEALTHDISP, NULL, { "models/map_objects/mp/bacta.md3", NULL, NULL }, "", "", "" },
	{ "item_ammodisp", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_ammodisp", 120, IT_HOLDABLE, HI_AMMODISP, NULL, { "models/map_objects/mp/bacta.md3", NULL, NULL }, "", "", "" },
	{ "item_eweb_holdable", "sound/interface/shieldcon_empty", "gfx/hud/i_icon_eweb", 120, IT_HOLDABLE, HI_EWEB, NULL, { "models/map_objects/hoth/eweb_model.glm", NULL, NULL }, "", "", "@MENUS_EWEB_DESC" },
	{ "item_cloak", "sound/weapons/w_pkup.wav", "gfx/hud/i_icon_cloak", 120, IT_HOLDABLE, HI_CLOAK, NULL, { "models/items/psgun.glm", NULL, NULL }, "", "", "@MENUS_CLOAK_DESC" },
	{ "item_force_enlighten_light", "sound/player/enlightenment.wav", "gfx/hud/mpi_jlight", 25, IT_POWERUP, PW_FORCE_ENLIGHTENED_LIGHT, NULL, { "models/map_objects/mp/jedi_enlightenment.md3", NULL, NULL }, "", "", "" },
	{ "item_force_enlighten_dark", "sound/player/enlightenment.wav", "gfx/hud/mpi_dklight", 25, IT_POWERUP, PW_FORCE_ENLIGHTENED_DARK, NULL, { "models/map_objects/mp/dk_enlightenment.md3", NULL, NULL }, "", "", "" },
	{ "item_force_boon", "sound/player/boon.wav", "gfx/hud/mpi_fboon", 25, IT_POWERUP, PW_FORCE_BOON, NULL, { "models/map_objects/mp/force_boon.md3", NULL, NULL }, "", "", "" },
	{ "item_ysalimari", "sound/player/ysalimari.wav", "gfx/hud/mpi_ysamari", 25, IT_POWERUP, PW_YSALAMIRI, NULL, { "models/map_objects/mp/ysalimari.md3", NULL, NULL }, "", "", "" },
	{ "weapon_stun_baton", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_stunbaton", 100, IT_WEAPON, WP_STUN_BATON, "models/weapons2/stun_baton/baton.md3", { "models/weapons2/stun_baton/baton_w.glm", NULL, NULL }, "", "", "" },
	{ "weapon_melee", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_melee", 100, IT_WEAPON, WP_MELEE, "models/weapons2/stun_baton/baton.md3", { "models/weapons2/stun_baton/baton_w.glm", NULL, NULL }, "", "", "@MENUS_MELEE_DESC" },
	{ "weapon_saber", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_lightsaber", 100, IT_WEAPON, WP_SABER, "models/weapons2/saber/saber_w.md3", { "models/weapons2/saber/saber_w.glm", NULL, NULL }, "", "", "@MENUS_AN_ELEGANT_WEAPON_FOR" },
	{ "weapon_blaster_pistol", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_blaster_pistol", 100, IT_WEAPON, WP_BRYAR_PISTOL, "models/weapons2/blaster_pistol/blaster_pistol.md3", { "models/weapons2/blaster_pistol/blaster_pistol_w.glm", NULL, NULL }, "", "", "@MENUS_BLASTER_PISTOL_DESC" },
	{ "weapon_concussion_rifle", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_c_rifle", 50, IT_WEAPON, WP_CONCUSSION, "models/weapons2/concussion/c_rifle.md3", { "models/weapons2/concussion/c_rifle_w.glm", NULL, NULL }, "", "", "@MENUS_CONC_RIFLE_DESC" },
	{ "weapon_bryar_pistol", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_briar", 100, IT_WEAPON, WP_BRYAR_OLD, "models/weapons2/briar_pistol/briar_pistol.md3", { "models/weapons2/briar_pistol/briar_pistol_w.glm", NULL, NULL }, "", "", "@SP_INGAME_BLASTER_PISTOL" },
	{ "weapon_blaster", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_blaster", 100, IT_WEAPON, WP_BLASTER, "models/weapons2/blaster_r/blaster.md3", { "models/weapons2/blaster_r/blaster_w.glm", NULL, NULL }, "", "", "@MENUS_THE_PRIMARY_WEAPON_OF" },
	{ "weapon_disruptor", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_disruptor", 100, IT_WEAPON, WP_DISRUPTOR, "models/weapons2/disruptor/disruptor.md3", { "models/weapons2/disruptor/disruptor_w.glm", NULL, NULL }, "", "", "@MENUS_THIS_NEFARIOUS_WEAPON" },
	{ "weapon_bowcaster", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_bowcaster", 100, IT_WEAPON, WP_BOWCASTER, "models/weapons2/bowcaster/bowcaster.md3", { "models/weapons2/bowcaster/bowcaster_w.glm", NULL, NULL }, "", "", "@MENUS_THIS_ARCHAIC_LOOKING" },
	{ "weapon_repeater", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_repeater", 100, IT_WEAPON, WP_REPEATER, "models/weapons2/heavy_repeater/heavy_repeater.md3", { "models/weapons2/heavy_repeater/heavy_repeater_w.glm", NULL, NULL }, "", "", "@MENUS_THIS_DESTRUCTIVE_PROJECTILE" },
	{ "weapon_demp2", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_demp2", 100, IT_WEAPON, WP_DEMP2, "models/weapons2/demp2/demp2.md3", { "models/weapons2/demp2/demp2_w.glm", NULL, NULL }, "", "", "@MENUS_COMMONLY_REFERRED_TO" },
	{ "weapon_flechette", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_flechette", 100, IT_WEAPON, WP_FLECHETTE, "models/weapons2/golan_arms/golan_arms.md3", { "models/weapons2/golan_arms/golan_arms_w.glm", NULL, NULL }, "", "", "@MENUS_WIDELY_USED_BY_THE_CORPORATE" },
	{ "weapon_rocket_launcher", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_merrsonn", 3, IT_WEAPON, WP_ROCKET_LAUNCHER, "models/weapons2/merr_sonn/merr_sonn.md3", { "models/weapons2/merr_sonn/merr_sonn_w.glm", NULL, NULL }, "", "", "@MENUS_THE_PLX_2M_IS_AN_EXTREMELY" },
	{ "ammo_thermal", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_thermal", 4, IT_AMMO, AMMO_THERMAL, "models/weapons2/thermal/thermal.md3", { "models/weapons2/thermal/thermal_pu.md3", "models/weapons2/thermal/thermal_w.glm", NULL }, "", "", "@MENUS_THE_THERMAL_DETONATOR" },
	{ "ammo_tripmine", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_tripmine", 3, IT_AMMO, AMMO_TRIPMINE, "models/weapons2/laser_trap/laser_trap.md3", { "models/weapons2/laser_trap/laser_trap_pu.md3", "models/weapons2/laser_trap/laser_trap_w.glm", NULL }, "", "", "@MENUS_TRIP_MINES_CONSIST_OF" },
	{ "ammo_detpack", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_detpack", 3, IT_AMMO, AMMO_DETPACK, "models/weapons2/detpack/det_pack.md3", { "models/weapons2/detpack/det_pack_pu.md3", "models/weapons2/detpack/det_pack_proj.glm", "models/weapons2/detpack/det_pack_w.glm" }, "", "", "@MENUS_A_DETONATION_PACK_IS" },
	{ "weapon_thermal", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_thermal", 4, IT_WEAPON, WP_THERMAL, "models/weapons2/thermal/thermal.md3", { "models/weapons2/thermal/thermal_w.glm", "models/weapons2/thermal/thermal_pu.md3", NULL }, "", "", "@MENUS_THE_THERMAL_DETONATOR" },
	{ "weapon_trip_mine", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_tripmine", 3, IT_WEAPON, WP_TRIP_MINE, "models/weapons2/laser_trap/laser_trap.md3", { "models/weapons2/laser_trap/laser_trap_w.glm", "models/weapons2/laser_trap/laser_trap_pu.md3", NULL }, "", "", "@MENUS_TRIP_MINES_CONSIST_OF" },
	{ "weapon_det_pack", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_detpack", 3, IT_WEAPON, WP_DET_PACK, "models/weapons2/detpack/det_pack.md3", { "models/weapons2/detpack/det_pack_proj.glm", "models/weapons2/detpack/det_pack_pu.md3", "models/weapons2/detpack/det_pack_w.glm" }, "", "", "@MENUS_A_DETONATION_PACK_IS" },
	{ "weapon_emplaced", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_blaster", 50, IT_WEAPON, WP_EMPLACED_GUN, "models/weapons2/blaster_r/blaster.md3", { "models/weapons2/blaster_r/blaster_w.glm", NULL, NULL }, "", "", "" },
	{ "weapon_turretwp", "sound/weapons/w_pkup.wav", "gfx/hud/w_icon_blaster", 50, IT_WEAPON, WP_TURRET, "models/weapons2/blaster_r/blaster.md3", { "models/weapons2/blaster_r/blaster_w.glm", NULL, NULL }, "", "", "" },
	{ "ammo_force", "sound/player/pickupenergy.wav", "gfx/hud/w_icon_blaster", 100, IT_AMMO, AMMO_FORCE, NULL, { "models/items/energy_cell.md3", NULL, NULL }, "", "", "" },
	{ "ammo_blaster", "sound/player/pickupenergy.wav", "gfx/hud/i_icon_battery", 100, IT_AMMO, AMMO_BLASTER, NULL, { "models/items/energy_cell.md3", NULL, NULL }, "", "", "" },
	{ "ammo_powercell", "sound/player/pickupenergy.wav", "gfx/mp/ammo_power_cell", 100, IT_AMMO, AMMO_POWERCELL, NULL, { "models/items/power_cell.md3", NULL, NULL }, "", "", "" },
	{ "ammo_metallic_bolts", "sound/player/pickupenergy.wav", "gfx/mp/ammo_metallic_bolts", 100, IT_AMMO, AMMO_METAL_BOLTS, NULL, { "models/items/metallic_bolts.md3", NULL, NULL }, "", "", "" },
	{ "ammo_rockets", "sound/player/pickupenergy.wav", "gfx/mp/ammo_rockets", 3, IT_AMMO, AMMO_ROCKETS, NULL, { "models/items/rockets.md3", NULL, NULL }, "", "", "" },
	{ "ammo_all", "sound/player/pickupenergy.wav", "gfx/mp/ammo_rockets", 0, IT_AMMO, -1, NULL, { "models/items/battery.md3", NULL, NULL }, "", "", "" },
	{ "team_CTF_redflag", NULL, "gfx/hud/mpi_rflag", 0, IT_TEAM, PW_REDFLAG, NULL, { "models/flags/r_flag.md3", "models/flags/r_flag_ysal.md3", NULL }, "", "", "" },
	{ "team_CTF_blueflag", NULL, "gfx/hud/mpi_bflag", 0, IT_TEAM, PW_BLUEFLAG, NULL, { "models/flags/b_flag.md3", "models/flags/b_flag_ysal.md3", NULL }, "", "", "" },
	{ "team_CTF_neutralflag", NULL, "icons/iconf_neutral1", 0, IT_TEAM, PW_NEUTRALFLAG, NULL, { "models/flags/n_flag.md3", NULL, NULL }, "", "", "" },
	{ "item_redcube", "sound/player/pickupenergy.wav", "icons/iconh_rorb", 0, IT_TEAM, 0, NULL, { "models/powerups/orb/r_orb.md3", NULL, NULL }, "", "", "" },
	{ "item_bluecube", "sound/player/pickupenergy.wav", "icons/iconh_borb", 0, IT_TEAM, 0, NULL, { "models/powerups/orb/b_orb.md3", NULL, NULL }, "", "", "" },
	// end of list marker
	{ NULL }
};
const size_t bg_numItems = ARRAY_LEN( bg_itemlist ) - 1;

qboolean BG_HasYsalamiri( int gametype, playerState_t *ps ) {
	if ( gametype == GT_CTY &&
		(ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG]) ) {
		return qtrue;
	}

	if ( ps->powerups[PW_YSALAMIRI] ) {
		return qtrue;
	}

	return qfalse;
}

qboolean BG_CanUseFPNow( int gametype, playerState_t *ps, int time, forcePowers_t power ) {

	if ( BG_HasYsalamiri( gametype, ps ) ) {
		return qfalse;
	}

	if ( ps->forceRestricted || ps->trueNonJedi ) {
		return qfalse;
	}

	if ( ps->weapon == WP_EMPLACED_GUN ) { //can't use any of your powers while on an emplaced weapon
		return qfalse;
	}

	if ( ps->m_iVehicleNum ) { //can't use powers while riding a vehicle (this may change, I don't know)
		return qfalse;
	}
#ifdef PROJECT_GAME
	if ( ps->duelInProgress && !g_entities[ps->clientNum].duelFullForce ) {
#else
	if (ps->duelInProgress) {
#endif
		if ( power != FP_SABER_OFFENSE && power != FP_SABER_DEFENSE && /*power != FP_SABERTHROW &&*/
			power != FP_LEVITATION ) {
			if ( !ps->saberLockFrame || power != FP_PUSH ) {
				return qfalse;
			}
		}
	}

	if ( ps->saberLockFrame || ps->saberLockTime > time ) {
		if ( power != FP_PUSH ) {
			return qfalse;
		}
	}

	if ( ps->fallingToDeath ) {
		return qfalse;
	}

	if ( (ps->brokenLimbs & (1 << BROKENLIMB_RARM)) ||
		(ps->brokenLimbs & (1 << BROKENLIMB_LARM)) ) { //powers we can't use with a broken arm
		switch ( power ) {
		case FP_PUSH:
		case FP_PULL:
		case FP_GRIP:
		case FP_LIGHTNING:
		case FP_DRAIN:
			return qfalse;
		default:
			break;
		}
	}

	return qtrue;
}

const gitem_t *BG_FindItemForPowerup( powerup_t pw ) {
	const gitem_t *it = NULL;

	for ( it = bg_itemlist + 1; it->classname; it++ ) {
		if ( (it->giType == IT_POWERUP || it->giType == IT_TEAM) && it->giTag == (int)pw )
			return it;
	}

	return NULL;
}

const gitem_t *BG_FindItemForHoldable( holdable_t hi ) {
	const gitem_t *it = NULL;

	for ( it = bg_itemlist + 1; it->classname; it++ ) {
		if ( it->giType == IT_HOLDABLE && it->giTag == (int)hi )
			return it;
	}

	Com_Error( ERR_DROP, "HoldableItem not found" );
	return NULL;
}

const gitem_t *BG_FindItemForWeapon( weapon_t wp ) {
	const gitem_t *it = NULL;

	for ( it = bg_itemlist + 1; it->classname; it++ ) {
		if ( it->giType == IT_WEAPON && it->giTag == (int)wp )
			return it;
	}

	Com_Error( ERR_DROP, "BG_FindItemForWeapon: Couldn't find item for weapon %i", wp );
	return NULL;
}

const gitem_t *BG_FindItemForAmmo( ammo_t ammo ) {
	const gitem_t *it = NULL;

	for ( it = bg_itemlist + 1; it->classname; it++ ) {
		if ( it->giType == IT_AMMO && it->giTag == (int)ammo )
			return it;
	}

	Com_Error( ERR_DROP, "Couldn't find item for ammo %i", ammo );
	return NULL;
}

const gitem_t *BG_FindItem( const char *classname ) {
	const gitem_t *it = NULL;

	for ( it = bg_itemlist + 1; it->classname; it++ ) {
		if ( !Q_stricmp( it->classname, classname ) )
			return it;
	}

	return NULL;
}

// Items can be picked up without actually touching their physical bounds to make grabbing them easier
qboolean BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vector3 origin;

	BG_EvaluateTrajectory( &item->pos, atTime, &origin );

	// we are ignoring ducked differences here
	if ( ps->origin.x - origin.x > 44
		|| ps->origin.x - origin.x < -50
		|| ps->origin.y - origin.y > 36
		|| ps->origin.y - origin.y < -36
		|| ps->origin.z - origin.z > 36
		|| ps->origin.z - origin.z < -36 ) {
		return qfalse;
	}

	return qtrue;
}

int BG_ProperForceIndex( int power ) {
	int i;

	for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
		if ( forcePowerSorted[i] == power ) {
			return i;
		}
	}

	return -1;
}

void BG_CycleForce( playerState_t *ps, int direction ) {
	int i, x, presel;
	int foundnext = -1;

	x = i = ps->fd.forcePowerSelected;

	// no valid force powers
	if ( x >= NUM_FORCE_POWERS || x == -1 )
		return;

	presel = x = BG_ProperForceIndex( x );

	// get the next/prev power and handle overflow
	if ( direction == 1 )	x++;
	else					x--;
	if ( x >= NUM_FORCE_POWERS )	x = 0;
	if ( x < 0 )					x = NUM_FORCE_POWERS - 1;

	i = forcePowerSorted[x]; //the "sorted" value of this power

	while ( x != presel ) {
		// loop around to the current force power
		if ( ps->fd.forcePowersKnown & (1 << i) && i != (signed)ps->fd.forcePowerSelected ) {
			// we have this power
			if ( i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE && i != FP_SABERTHROW ) {
				// it's selectable
				foundnext = i;
				break;
			}
		}

		// get the next/prev power and handle overflow
		if ( direction == 1 ) {
			x++;
		}
		else {
			x--;
		}
		if ( x >= NUM_FORCE_POWERS ) {
			x = 0;
		}
		if ( x < 0 ) {
			x = NUM_FORCE_POWERS - 1;
		}

		i = forcePowerSorted[x]; //set to the sorted value again
	}

	// if we found one, select it
	if ( foundnext != -1 )
		ps->fd.forcePowerSelected = (unsigned)foundnext;
}

// Get the itemlist index from the tag and type
int BG_GetItemIndexByTag( int tag, itemType_t type ) {
	size_t i;
	const gitem_t *it = NULL;

	for ( i = 1, it = bg_itemlist + 1; i < bg_numItems; i++, it++ ) {
		if ( it->giTag == tag && it->giType == type )
			return (int)i;
	}

	return 0;
}

//yeah..
qboolean BG_IsItemSelectable( playerState_t *ps, int item ) {
	if ( item == HI_HEALTHDISP || item == HI_AMMODISP ||
		item == HI_JETPACK ) {
		return qfalse;
	}
	return qtrue;
}

void BG_CycleInven( playerState_t *ps, int direction ) {
	int i;
	int dontFreeze = 0;
	int original;

	i = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	original = i;

	if ( direction == 1 ) { //next
		i++;
		if ( i == HI_NUM_HOLDABLE ) {
			i = 1;
		}
	}
	else { //previous
		i--;
		if ( i == 0 ) {
			i = HI_NUM_HOLDABLE - 1;
		}
	}

	while ( i != original ) { //go in a full loop until hitting something, if hit nothing then select nothing
		if ( ps->stats[STAT_HOLDABLE_ITEMS] & (1 << i) ) { //we have it, select it.
			if ( BG_IsItemSelectable( ps, i ) ) {
				ps->stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag( i, IT_HOLDABLE );
				break;
			}
		}

		if ( direction == 1 ) { //next
			i++;
		}
		else { //previous
			i--;
		}

		if ( i <= 0 ) { //wrap around to the last
			i = HI_NUM_HOLDABLE - 1;
		}
		else if ( i >= HI_NUM_HOLDABLE ) { //wrap around to the first
			i = 1;
		}

		dontFreeze++;
		if ( dontFreeze >= 32 ) { //yeah, sure, whatever (it's 2 am and I'm paranoid and can't frickin think)
			break;
		}
	}
}

static qboolean BG_AlwaysPickupWeapons( void ) {
#ifdef PROJECT_GAME
	return !!((jp_cinfo.bits & CINFO_ALWAYSPICKUPWEAP));
#elif defined( PROJECT_CGAME )
	return !!((cgs.japp.jp_cinfo & CINFO_ALWAYSPICKUPWEAP));
#else
	return qfalse;
#endif
}

// Returns false if the item should not be picked up.
// This needs to be the same for client side prediction and server use.
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	const gitem_t	*item;

	if ( ent->modelindex < 1 || ent->modelindex >= (int)bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->modelindex];

	if ( ps ) {
		if ( ps->trueJedi ) {//force powers and saber only
			if ( item->giType != IT_TEAM //not a flag
				&& item->giType != IT_ARMOR//not shields
				&& (item->giType != IT_WEAPON
				|| item->giTag != WP_SABER)//not a saber
				&& (item->giType != IT_HOLDABLE || item->giTag != HI_SEEKER)//not a seeker
				&& (item->giType != IT_POWERUP || item->giTag == PW_YSALAMIRI) )//not a force pick-up
			{
				return qfalse;
			}
		}
		else if ( ps->trueNonJedi ) {//can't pick up force powerups
			if ( (item->giType == IT_POWERUP && item->giTag != PW_YSALAMIRI) //if a powerup, can only can pick up ysalamiri
				|| (item->giType == IT_HOLDABLE && item->giTag == HI_SEEKER)//if holdable, cannot pick up seeker
				|| (item->giType == IT_WEAPON && item->giTag == WP_SABER) )//or if it's a saber
			{
				return qfalse;
			}
		}
		if ( ps->isJediMaster && item && (item->giType == IT_WEAPON || item->giType == IT_AMMO) ) {//jedi master cannot pick up weapons
			return qfalse;
		}
		if ( ps->duelInProgress ) { //no picking stuff up while in a duel, no matter what the type is
			return qfalse;
		}
	}
	else {//safety return since below code assumes a non-null ps
		return qfalse;
	}

	switch ( item->giType ) {
	case IT_WEAPON:
		if ( ent->generic1 == ps->clientNum && ent->powerups ) {
			return qfalse;
		}
		if ( !BG_AlwaysPickupWeapons() ) {
			if ( !(ent->eFlags & EF_DROPPEDWEAPON) && (ps->stats[STAT_WEAPONS] & (1 << item->giTag)) &&
				item->giTag != WP_THERMAL && item->giTag != WP_TRIP_MINE && item->giTag != WP_DET_PACK ) { //weaponstay stuff.. if this isn't dropped, and you already have it, you don't get it.
				return qfalse;
			}
		}
		else if ( BG_AlwaysPickupWeapons() || (item->giTag == WP_THERMAL || item->giTag == WP_TRIP_MINE || item->giTag == WP_DET_PACK) ) { //check to see if full on ammo for this, if so, then..
			int ammoIndex = weaponData[item->giTag].ammoIndex;
			if ( ps->ammo[ammoIndex] >= ammoMax[ammoIndex] && (ps->stats[STAT_WEAPONS] & (1 << item->giTag)) ) { //don't need it
				return qfalse;
			}
		}
		return qtrue;	// weapons are always picked up

	case IT_AMMO:
		if ( item->giTag == -1 ) { //special case for "all ammo" packs
			return qtrue;
		}
		if ( ps->ammo[item->giTag] >= ammoMax[item->giTag] ) {
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH]/* * item->giTag*/ ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
		if ( (ps->fd.forcePowersActive & (1 << FP_RAGE)) ) {
			return qfalse;
		}

		if ( item->quantity == 5 || item->quantity == 100 ) {
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		if ( ps && (ps->powerups[PW_YSALAMIRI]) ) {
			if ( item->giTag != PW_YSALAMIRI ) {
				return qfalse;
			}
		}
		return qtrue;	// powerups are always picked up

	case IT_TEAM: // team items, such as flags
		if ( gametype == GT_CTF || gametype == GT_CTY ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if ( ps->persistant[PERS_TEAM] == TEAM_RED ) {
				if ( item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]) )
					return qtrue;
			}
			else if ( ps->persistant[PERS_TEAM] == TEAM_BLUE ) {
				if ( item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]) )
					return qtrue;
			}
		}

		return qfalse;

	case IT_HOLDABLE:
		if ( ps->stats[STAT_HOLDABLE_ITEMS] & (1 << item->giTag) ) {
			return qfalse;
		}
		return qtrue;

	case IT_BAD:
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
	default:
#ifdef _DEBUG
		Com_Printf( "BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
		break;
	}

	return qfalse;
}

void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vector3 *result ) {
	float deltaTime, phase;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( &tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = (atTime - tr->trTime) * 0.001f;	// milliseconds to seconds
		VectorMA( &tr->trBase, deltaTime, &tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
		phase = sinf( deltaTime * M_PI * 2 );
		VectorMA( &tr->trBase, phase, &tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration )
			atTime = tr->trTime + tr->trDuration;
		deltaTime = (atTime - tr->trTime) * 0.001f;	// milliseconds to seconds
		if ( deltaTime < 0 )
			deltaTime = 0;
		VectorMA( &tr->trBase, deltaTime, &tr->trDelta, result );
		break;
	case TR_NONLINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		//new slow-down at end
		if ( atTime - tr->trTime > tr->trDuration || atTime - tr->trTime <= 0 ) {
			deltaTime = 0;
		}
		else {//FIXME: maybe scale this somehow?  So that it starts out faster and stops faster?
			deltaTime = tr->trDuration*0.001f*((float)cosf( DEG2RAD( 90.0f - (90.0f*((float)(atTime - tr->trTime)) / (float)tr->trDuration) ) ));
		}
		VectorMA( &tr->trBase, deltaTime, &tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001f;	// milliseconds to seconds
		VectorMA( &tr->trBase, deltaTime, &tr->trDelta, result );
		result->raw[2] -= 0.5f * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	}
}

// For determining velocity at a given time
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vector3 *result ) {
	float deltaTime, phase;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( &tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
		phase = cosf( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5f;
		VectorScale( &tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( &tr->trDelta, result );
		break;
	case TR_NONLINEAR_STOP:
		if ( atTime - tr->trTime > tr->trDuration || atTime - tr->trTime <= 0 ) {
			VectorClear( result );
			return;
		}
		deltaTime = tr->trDuration*0.001f*((float)cosf( DEG2RAD( 90.0f - (90.0f*((float)(atTime - tr->trTime)) / (float)tr->trDuration) ) ));
		VectorScale( &tr->trDelta, deltaTime, result );
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001f;	// milliseconds to seconds
		VectorCopy( &tr->trDelta, result );
		result->raw[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	}
}

// given a boltmatrix, return in vec a normalised vector for the axis requested in flags
void BG_GiveMeVectorFromMatrix( mdxaBone_t *boltMatrix, uint32_t flags, vector3 *vec ) {
	switch ( flags ) {
	case ORIGIN:
		vec->x = boltMatrix->matrix[0][3];
		vec->y = boltMatrix->matrix[1][3];
		vec->z = boltMatrix->matrix[2][3];
		break;
	case POSITIVE_Y:
		vec->x = boltMatrix->matrix[0][1];
		vec->y = boltMatrix->matrix[1][1];
		vec->z = boltMatrix->matrix[2][1];
		break;
	case POSITIVE_X:
		vec->x = boltMatrix->matrix[0][0];
		vec->y = boltMatrix->matrix[1][0];
		vec->z = boltMatrix->matrix[2][0];
		break;
	case POSITIVE_Z:
		vec->x = boltMatrix->matrix[0][2];
		vec->y = boltMatrix->matrix[1][2];
		vec->z = boltMatrix->matrix[2][2];
		break;
	case NEGATIVE_Y:
		vec->x = -boltMatrix->matrix[0][1];
		vec->y = -boltMatrix->matrix[1][1];
		vec->z = -boltMatrix->matrix[2][1];
		break;
	case NEGATIVE_X:
		vec->x = -boltMatrix->matrix[0][0];
		vec->y = -boltMatrix->matrix[1][0];
		vec->z = -boltMatrix->matrix[2][0];
		break;
	case NEGATIVE_Z:
		vec->x = -boltMatrix->matrix[0][2];
		vec->y = -boltMatrix->matrix[1][2];
		vec->z = -boltMatrix->matrix[2][2];
		break;
	default:
		break;
	}
}

const char *eventnames[] = {
	"EV_NONE",

	"EV_CLIENTJOIN",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_GHOUL2_MARK",			//create a projectile impact mark on something with a client-side g2 instance.

	"EV_GLOBAL_DUEL",
	"EV_PRIVATE_DUEL",

	"EV_JUMP",
	"EV_ROLL",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_VEH_FIRE",

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_ALT_FIRE",
	"EV_SABER_ATTACK",
	"EV_SABER_HIT",
	"EV_SABER_BLOCK",
	"EV_SABER_CLASHFLARE",
	"EV_SABER_UNHOLSTER",
	"EV_BECOME_JEDIMASTER",
	"EV_DISRUPTOR_MAIN_SHOT",
	"EV_DISRUPTOR_SNIPER_SHOT",
	"EV_DISRUPTOR_SNIPER_MISS",
	"EV_DISRUPTOR_HIT",
	"EV_DISRUPTOR_ZOOMSOUND",

	"EV_PREDEFSOUND",

	"EV_TEAM_POWER",

	"EV_SCREENSHAKE",

	"EV_LOCALTIMER",

	"EV_USE",			// +Use key

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEMUSEFAIL",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex
	"EV_MISSILE_STICK",

	"EV_PLAY_EFFECT",
	"EV_PLAY_EFFECT_ID", //finally gave in and added it..
	"EV_PLAY_PORTAL_EFFECT_ID",

	"EV_PLAYDOORSOUND",
	"EV_PLAYDOORLOOPSOUND",
	"EV_BMODEL_SOUND",

	"EV_MUTE_SOUND",
	"EV_VOICECMD_SOUND",
	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",
	"EV_ENTITY_SOUND",

	"EV_PLAY_ROFF",

	"EV_GLASS_SHATTER",
	"EV_DEBRIS",
	"EV_MISC_MODEL_EXP",

	"EV_CONC_ALT_IMPACT",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	//"EV_POWERUP_REGEN",

	"EV_FORCE_DRAINED",

	"EV_GIB_PLAYER",			// gib a previously living player
	"EV_SCOREPLUM",			// score plum

	"EV_CTFMESSAGE",

	"EV_BODYFADE",

	"EV_SIEGE_ROUNDOVER",
	"EV_SIEGE_OBJECTIVECOMPLETE",

	"EV_DESTROY_GHOUL2_INSTANCE",

	"EV_DESTROY_WEAPON_MODEL",

	"EV_GIVE_NEW_RANK",
	"EV_SET_FREE_SABER",
	"EV_SET_FORCE_DISABLE",

	"EV_WEAPON_CHARGE",
	"EV_WEAPON_CHARGE_ALT",

	"EV_SHIELD_HIT",

	"EV_DEBUG_LINE",
	"EV_TESTLINE",
	"EV_STOPLOOPINGSOUND",
	"EV_STARTLOOPINGSOUND",
	"EV_TAUNT",
	//fixme, added a bunch that aren't here!
};

// Handles the sequence numbers
void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	static vmCvar_t showEvents;
	static qboolean isRegistered = qfalse;

	if ( !isRegistered ) {
		trap->Cvar_Register( &showEvents, "showevents", "0", 0 );
		isRegistered = qtrue;
	}

	if ( showEvents.integer ) {
		Com_Printf( "%cgame event svt %5d -> %5d: num = %20s parm %d\n",
#ifdef PROJECT_GAME
			' ',
#else
			'c',
#endif
			ps->pmove_framecount, ps->eventSequence, eventnames[newEvent], eventParm );
	}
#endif // _DEBUG

	ps->events[ps->eventSequence & (MAX_PS_EVENTS - 1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS - 1)] = eventParm;
	ps->eventSequence++;
}

void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL && ps->pm_type != PM_JETPACK && ps->pm_type != PM_FLOAT )
		return;

	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;

	// give the player the velocity from the jumppad
	VectorCopy( &jumppad->origin2, &ps->velocity );

	ps->fd.forcePowersActive &= ~(1 << FP_LEVITATION);
}

// Shared code for emplaced angle gun constriction
int BG_EmplacedView( vector3 *baseAngles, vector3 *angles, float *newYaw, float constraint ) {
	float dif = AngleSubtract( baseAngles->yaw, angles->yaw );

	if ( dif > constraint ||
		dif < -constraint ) {
		float amt;

		if ( dif > constraint ) {
			amt = (dif - constraint);
			dif = constraint;
		}
		else if ( dif < -constraint ) {
			amt = (dif + constraint);
			dif = -constraint;
		}
		else {
			amt = 0.0f;
		}

		*newYaw = AngleSubtract( angles->yaw, -dif );

		if ( amt > 1.0f || amt < -1.0f ) { //significant, force the view
			return 2;
		}
		else { //just a little out of range
			return 1;
		}
	}

	return 0;
}

//To see if the client is trying to use one of the included skins not meant for MP.
//I don't much care for hardcoded strings, but this seems the best way to go.
qboolean BG_IsValidCharacterModel( const char *modelName, const char *skinName ) {
	//Raz: To address invisible skins, filter these invalid models properly.
#if 0 //switch to old behaviour if needed
	if ( strstr( skinName, "menu" ) )
		return qfalse;

	if ( !Q_stricmp( modelName, "kyle" ) && Q_stricmpn( skinName, "fpls", 4 ) )
		return qfalse;

	return qtrue;
#else
	if ( !Q_stricmp( skinName, "menu" ) ) {
		return qfalse;
	}
	else if ( !Q_stricmp( modelName, "kyle" ) ) {
		if ( !Q_stricmp( skinName, "fpls" ) ) {
			return qfalse;
		}
		else if ( !Q_stricmp( skinName, "fpls2" ) ) {
			return qfalse;
		}
		else if ( !Q_stricmp( skinName, "fpls3" ) ) {
			return qfalse;
		}
	}
	return qtrue;
#endif
}

qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, vector3 *colors ) {
	if ( strlen( modelName ) > 5 && !Q_stricmpn( modelName, "jedi_", 5 ) ) {
		//argh, it's a custom player skin!
		if ( team == TEAM_RED && colors )
			VectorSet( colors, 1.0f, 0.0f, 0.0f );
		else if ( team == TEAM_BLUE && colors )
			VectorSet( colors, 0.0f, 0.0f, 1.0f );
		return qtrue;
	}

	if ( team == TEAM_RED ) {
		if ( Q_stricmp( "red", skinName ) != 0 ) {//not "red"
			if ( Q_stricmp( "blue", skinName ) == 0
				|| Q_stricmp( "default", skinName ) == 0
				|| strchr( skinName, '|' )//a multi-skin playerModel
				|| !BG_IsValidCharacterModel( modelName, skinName ) ) {
				Q_strncpyz( skinName, "red", MAX_QPATH );
				return qfalse;
			}
			else {//need to set it to red
				int len = strlen( skinName );
				if ( len < 3 ) {//too short to be "red"
					Q_strcat( skinName, MAX_QPATH, "_red" );
				}
				else {
					char	*start = &skinName[len - 3];
					if ( Q_strncmp( "red", start, 3 ) != 0 ) {//doesn't already end in "red"
						if ( len + 4 >= MAX_QPATH ) {//too big to append "_red"
							Q_strncpyz( skinName, "red", MAX_QPATH );
							return qfalse;
						}
						else {
							Q_strcat( skinName, MAX_QPATH, "_red" );
						}
					}
				}
				//if file does not exist, set to "red"
				if ( !BG_FileExists( va( "models/players/%s/model_%s.skin", modelName, skinName ) ) ) {
					Q_strncpyz( skinName, "red", MAX_QPATH );
				}
				return qfalse;
			}
		}

	}
	else if ( team == TEAM_BLUE ) {
		if ( Q_stricmp( "blue", skinName ) != 0 ) {
			if ( Q_stricmp( "red", skinName ) == 0
				|| Q_stricmp( "default", skinName ) == 0
				|| strchr( skinName, '|' )//a multi-skin playerModel
				|| !BG_IsValidCharacterModel( modelName, skinName ) ) {
				Q_strncpyz( skinName, "blue", MAX_QPATH );
				return qfalse;
			}
			else {//need to set it to blue
				int len = strlen( skinName );
				if ( len < 4 ) {//too short to be "blue"
					Q_strcat( skinName, MAX_QPATH, "_blue" );
				}
				else {
					char	*start = &skinName[len - 4];
					if ( Q_strncmp( "blue", start, 4 ) != 0 ) {//doesn't already end in "blue"
						if ( len + 5 >= MAX_QPATH ) {//too big to append "_blue"
							Q_strncpyz( skinName, "blue", MAX_QPATH );
							return qfalse;
						}
						else {
							Q_strcat( skinName, MAX_QPATH, "_blue" );
						}
					}
				}
				//if file does not exist, set to "blue"
				if ( !BG_FileExists( va( "models/players/%s/model_%s.skin", modelName, skinName ) ) ) {
					Q_strncpyz( skinName, "blue", MAX_QPATH );
				}
				return qfalse;
			}
		}
	}
	return qtrue;
}

// This is done after each set of usercmd_t on the server, and after local prediction on the client
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	}
	else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	}
	else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( &ps->origin, &s->pos.trBase );
	if ( snap )
		VectorSnap( &s->pos.trBase );

	// set the trDelta for flag direction
	VectorCopy( &ps->velocity, &s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( &ps->viewangles, &s->apos.trBase );
	if ( snap ) {
		VectorSnap( &s->apos.trBase );
	}

	s->trickedEntIndex[0] = ps->fd.forceMindtrickTargetIndex[0];
	s->trickedEntIndex[1] = ps->fd.forceMindtrickTargetIndex[1];
	s->trickedEntIndex[2] = ps->fd.forceMindtrickTargetIndex[2];
	s->trickedEntIndex[3] = ps->fd.forceMindtrickTargetIndex[3];

	s->forceFrame = ps->saberLockFrame;

	s->emplacedOwner = ps->electrifyTime;

	s->speed = ps->speed;

	s->genericenemyindex = ps->genericEnemyIndex;

	s->activeForcePass = ps->activeForcePass;

	s->angles2.yaw = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;

	s->legsFlip = ps->legsFlip;
	s->torsoFlip = ps->torsoFlip;

	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	s->eFlags2 = ps->eFlags2;

	s->saberInFlight = ps->saberInFlight;
	s->saberEntityNum = ps->saberEntityNum;
	s->saberMove = ps->saberMove;
	s->forcePowersActive = ps->fd.forcePowersActive;

	if ( ps->duelInProgress ) {
		s->bolt1 = 1;
	}
	else {
		s->bolt1 = 0;
	}

	s->otherEntityNum2 = ps->emplacedIndex;

	s->saberHolstered = ps->saberHolstered;

	if ( ps->genericEnemyIndex != -1 ) {
		s->eFlags |= EF_SEEKERDRONE;
	}

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	}
	else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}


	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0; i < MAX_POWERUPS; i++ ) {
		if ( ps->powerups[i] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	//NOT INCLUDED IN ENTITYSTATETOPLAYERSTATE:
	s->modelindex2 = ps->weaponstate;
	s->constantLight = ps->weaponChargeTime;

	VectorCopy( &ps->lastHitLoc, &s->origin2 );

	s->isJediMaster = ps->isJediMaster;

	s->time2 = ps->holocronBits;

	s->fireflag = ps->fd.saberAnimLevel;

	s->heldByClient = ps->heldByClient;
	s->ragAttach = ps->ragAttach;

	s->iModelScale = ps->iModelScale;

	s->brokenLimbs = ps->brokenLimbs;

	s->hasLookTarget = ps->hasLookTarget;
	s->lookTarget = ps->lookTarget;

	s->customRGBA[0] = ps->customRGBA[0];
	s->customRGBA[1] = ps->customRGBA[1];
	s->customRGBA[2] = ps->customRGBA[2];
	s->customRGBA[3] = ps->customRGBA[3];

	s->m_iVehicleNum = ps->m_iVehicleNum;
}

// This is done after each set of usercmd_t on the server, and after local prediction on the client
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	}
	else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	}
	else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( &ps->origin, &s->pos.trBase );
	if ( snap ) {
		VectorSnap( &s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( &ps->velocity, &s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( &ps->viewangles, &s->apos.trBase );
	if ( snap ) {
		VectorSnap( &s->apos.trBase );
	}

	s->trickedEntIndex[0] = ps->fd.forceMindtrickTargetIndex[0];
	s->trickedEntIndex[1] = ps->fd.forceMindtrickTargetIndex[1];
	s->trickedEntIndex[2] = ps->fd.forceMindtrickTargetIndex[2];
	s->trickedEntIndex[3] = ps->fd.forceMindtrickTargetIndex[3];

	s->forceFrame = ps->saberLockFrame;

	s->emplacedOwner = ps->electrifyTime;

	s->speed = ps->speed;

	s->genericenemyindex = ps->genericEnemyIndex;

	s->activeForcePass = ps->activeForcePass;

	s->angles2.yaw = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;

	s->legsFlip = ps->legsFlip;
	s->torsoFlip = ps->torsoFlip;

	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	s->eFlags2 = ps->eFlags2;

	s->saberInFlight = ps->saberInFlight;
	s->saberEntityNum = ps->saberEntityNum;
	s->saberMove = ps->saberMove;
	s->forcePowersActive = ps->fd.forcePowersActive;

	if ( ps->duelInProgress ) {
		s->bolt1 = 1;
	}
	else {
		s->bolt1 = 0;
	}

	s->otherEntityNum2 = ps->emplacedIndex;

	s->saberHolstered = ps->saberHolstered;

	if ( ps->genericEnemyIndex != -1 ) {
		s->eFlags |= EF_SEEKERDRONE;
	}

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	}
	else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}
	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0; i < MAX_POWERUPS; i++ ) {
		if ( ps->powerups[i] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	//NOT INCLUDED IN ENTITYSTATETOPLAYERSTATE:
	s->modelindex2 = ps->weaponstate;
	s->constantLight = ps->weaponChargeTime;

	VectorCopy( &ps->lastHitLoc, &s->origin2 );

	s->isJediMaster = ps->isJediMaster;

	s->time2 = ps->holocronBits;

	s->fireflag = ps->fd.saberAnimLevel;

	s->heldByClient = ps->heldByClient;
	s->ragAttach = ps->ragAttach;

	s->iModelScale = ps->iModelScale;

	s->brokenLimbs = ps->brokenLimbs;

	s->hasLookTarget = ps->hasLookTarget;
	s->lookTarget = ps->lookTarget;

	s->customRGBA[0] = ps->customRGBA[0];
	s->customRGBA[1] = ps->customRGBA[1];
	s->customRGBA[2] = ps->customRGBA[2];
	s->customRGBA[3] = ps->customRGBA[3];

	s->m_iVehicleNum = ps->m_iVehicleNum;
}

// perform the appropriate model precache routine
int BG_ModelCache( const char *modelName, const char *skinName ) {
#ifdef PROJECT_GAME
	void *g2 = NULL;

	if ( skinName && skinName[0] ) {
		trap->R_RegisterSkin( skinName );
	}

	//I could hook up a precache ghoul2 function, but oh well, this works
	trap->G2API_InitGhoul2Model( &g2, modelName, 0, 0, 0, 0, 0 );
	if ( g2 ) { //now get rid of it
		trap->G2API_CleanGhoul2Models( &g2 );
	}
	return 0;
#else
	if ( skinName && skinName[0] ) {
		trap->R_RegisterSkin( skinName );
	}
	return trap->R_RegisterModel( modelName );
#endif
}

#ifdef PROJECT_GAME
#define MAX_POOL_SIZE	(12*1024*1024) // 12mB, was 3mB
#define BGALLOCSTR "S"
#elif defined PROJECT_CGAME
#define MAX_POOL_SIZE	(8*1024*1024) // 8mB, was 2mB
#define BGALLOCSTR "CG"
#else
#define MAX_POOL_SIZE	(512*1024) // 512kB
#define BGALLOCSTR "UI"
#endif

//I am using this for all the stuff like NPC client structures on server/client and
//non-humanoid animations as well until/if I can get dynamic memory working properly
//with casted datatypes, which is why it is so large.


static char			bg_pool[MAX_POOL_SIZE];
static unsigned int	bg_poolSize = 0u;
static unsigned int	bg_poolTail = MAX_POOL_SIZE;

void *BG_Alloc( int size ) {
	bg_poolSize = ((bg_poolSize + 0x00000003u) & 0xfffffffcu);

#ifdef _DEBUG
	//	Com_Printf( "BG_Alloc: %i bytes from "BGALLOCSTR"\n", size );
#endif

	if ( bg_poolSize + size > bg_poolTail ) {
		Com_Error( ERR_DROP, "BG_Alloc: buffer exceeded tail (%d > %d)", bg_poolSize + size, bg_poolTail );
		return 0;
	}

	bg_poolSize += size;

	return &bg_pool[bg_poolSize - size];
}

void *BG_AllocUnaligned( int size ) {
	if ( bg_poolSize + size > bg_poolTail ) {
		Com_Error( ERR_DROP, "BG_AllocUnaligned: buffer exceeded tail (%d > %d)", bg_poolSize + size, bg_poolTail );
		return 0;
	}

	bg_poolSize += size;

	return &bg_pool[bg_poolSize - size];
}

void *BG_TempAlloc( int size ) {
	size = ((size + 0x00000003) & 0xfffffffc);

	if ( bg_poolTail - size < bg_poolSize ) {
		Com_Error( ERR_DROP, "BG_TempAlloc: buffer exceeded head (%d > %d)", bg_poolTail - size, bg_poolSize );
		return 0;
	}

	bg_poolTail -= size;

	return &bg_pool[bg_poolTail];
}

void BG_TempFree( int size ) {
	size = ((size + 0x00000003) & 0xfffffffc);

	if ( bg_poolTail + size > MAX_POOL_SIZE ) {
		Com_Error( ERR_DROP, "BG_TempFree: tail greater than size (%d > %d)", bg_poolTail + size, MAX_POOL_SIZE );
	}

	bg_poolTail += size;
}

char *BG_StringAlloc( const char *source ) {
	char *dest;
	int len = strlen( source + 1 );

#ifdef _DEBUG
	Com_Printf( "BG_StringAlloc: %i bytes\n", len );
#endif

	dest = (char *)BG_Alloc( len );
	strcpy( dest, source );
	return dest;
}

qboolean BG_OutOfMemory( void ) {
	return (bg_poolSize >= MAX_POOL_SIZE) ? qtrue : qfalse;
}

const char *gametypeStringShort[GT_MAX_GAME_TYPE] = {
	"FFA",
	"HOLO",
	"JM",
	"1v1",
	"2v1",
	"SP",
	"TDM",
	"SAGA",
	"CTF",
	"CTY"
};

const char *BG_GetGametypeString( int gametype ) {
	switch ( gametype ) {
	case GT_FFA:
		return "Free For All";
	case GT_HOLOCRON:
		return "Holocron";
	case GT_JEDIMASTER:
		return "Jedi Master";
	case GT_DUEL:
		return "Duel";
	case GT_POWERDUEL:
		return "Power Duel";
	case GT_SINGLE_PLAYER:
		return "Cooperative";

	case GT_TEAM:
		return "Team Deathmatch";
	case GT_SIEGE:
		return "Siege";
	case GT_CTF:
		return "Capture The Flag";
	case GT_CTY:
		return "Capture The Ysalimiri";

	default:
		return "Unknown Gametype";
	}
}

int BG_GetGametypeForString( const char *gametype ) {
	if ( !Q_stricmp( gametype, "ffa" )
		|| !Q_stricmp( gametype, "dm" ) )			return GT_FFA;
	else if ( !Q_stricmp( gametype, "holocron" ) )		return GT_HOLOCRON;
	else if ( !Q_stricmp( gametype, "jm" ) )			return GT_JEDIMASTER;
	else if ( !Q_stricmp( gametype, "duel" ) )			return GT_DUEL;
	else if ( !Q_stricmp( gametype, "powerduel" ) )		return GT_POWERDUEL;
	else if ( !Q_stricmp( gametype, "sp" )
		|| !Q_stricmp( gametype, "coop" ) )			return GT_SINGLE_PLAYER;
	else if ( !Q_stricmp( gametype, "tdm" )
		|| !Q_stricmp( gametype, "tffa" )
		|| !Q_stricmp( gametype, "team" ) )			return GT_TEAM;
	else if ( !Q_stricmp( gametype, "siege" ) )			return GT_SIEGE;
	else if ( !Q_stricmp( gametype, "ctf" ) )			return GT_CTF;
	else if ( !Q_stricmp( gametype, "cty" ) )			return GT_CTY;
	else												return -1;
}

qboolean GetCInfo( uint32_t bit ) {
#if defined(PROJECT_GAME)
	uint32_t cinfo = (unsigned)jp_cinfo.integer;
#elif defined(PROJECT_CGAME)
	uint32_t cinfo = cgs.japp.jp_cinfo;
#else
	uint32_t cinfo = 0u;
#endif
	return !!(cinfo & bit);
}

qboolean GetCPD( bgEntity_t *self, uint32_t bit ) {
#if defined(PROJECT_GAME)
	uint32_t cpd = ((gentity_t *)self)->client->pers.CPD;
#elif defined(PROJECT_CGAME)
	uint32_t cpd = (unsigned)cp_pluginDisable.integer;
#else
	uint32_t cpd = 0u;
#endif
	return !!(cpd & bit);
}

team_t BG_GetOpposingTeam( team_t team ) {
	if ( team == TEAM_RED ) {
		return TEAM_BLUE;
	}
	else if ( team == TEAM_BLUE ) {
		return TEAM_RED;
	}
	else {
		return team;
	}
}

#if defined(PROJECT_GAME)
bool BG_HasSetSaberOnly( void )
#elif defined(PROJECT_CGAME) || defined(PROJECT_UI)
bool BG_HasSetSaberOnly( const char *info )
#endif
{
#if defined(PROJECT_GAME)
	const int gametype = level.gametype;
#elif defined(PROJECT_CGAME)
	const int gametype = cgs.gametype;
#elif defined(PROJECT_UI)
	const int gametype = ui_gameType.integer;
#endif

	if ( gametype == GT_JEDIMASTER ) {
		return false;
	}

	const uint32_t weaponDisable = (gametype == GT_DUEL || gametype == GT_POWERDUEL)
#if defined(PROJECT_GAME)
		? g_duelWeaponDisable.integer
		: g_weaponDisable.integer;
#elif defined(PROJECT_CGAME) || defined(PROJECT_UI)
		? atoi( Info_ValueForKey( info, "g_duelWeaponDisable" ) )
		: atoi( Info_ValueForKey( info, "g_weaponDisable" ) );
#endif

	for ( int wp = WP_NONE; wp < WP_NUM_WEAPONS; wp++ ) {
		if ( !(weaponDisable & (1 << wp))
			&& wp != WP_NONE
			&& wp != WP_STUN_BATON
			&& wp != WP_MELEE
			&& wp != WP_SABER
			&& wp != WP_EMPLACED_GUN
			&& wp != WP_TURRET )
		{
			return false;
		}
	}

	return true;
}
