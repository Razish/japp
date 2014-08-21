// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_local.h -- local definitions for game module

#pragma once

#ifdef _DEBUG
#define DEBUG_SABER_BOX
#endif

#define	FOFS(x) offsetof(gentity_t, x)

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_vehicles.h"
#include "g_public.h"

typedef struct gentity_s gentity_t;

#include "g_admin.h"
#include "b_public.h"
#include "g_unlagged.h"
#include "g_team.h"
#define XCVAR_PROTO
#include "g_xcvar.h"
#undef XCVAR_PROTO

#define	GAMEVERSION					"JA+ Mod v2.6 B1" //"JA++ 0.2 build 2"
#define DEFAULT_NAME				"Padawan"
#define BODY_QUEUE_SIZE				(8)
#define	FRAMETIME					(100) // msec
#define	CARNAGE_REWARD_TIME			(3000)
#define REWARD_SPRITE_TIME			(2000)
#define	INTERMISSION_DELAY_TIME		(1000)
#define	SP_INTERMISSION_DELAY_TIME	(5000)
#define	START_TIME_LINK_ENTS		(FRAMETIME*1) // time-delay after map start at which all ents have been spawned, so can link them
#define	START_TIME_FIND_LINKS		(FRAMETIME*2) // time-delay after map start at which you can find linked entities
#define	START_TIME_MOVERS_SPAWNED	(FRAMETIME*3) // time-delay after map start at which all movers should be spawned
#define	START_TIME_REMOVE_ENTS		(FRAMETIME*4) // time-delay after map start to remove temporary ents
#define	START_TIME_NAV_CALC			(FRAMETIME*5) // time-delay after map start to connect waypoints and calc routes
#define	START_TIME_FIND_WAYPOINT	(FRAMETIME*6) // time-delay after map start after which it's okay to try to find your best waypoint
#define	MAX_G_SHARED_BUFFER_SIZE	(1024*8)
#define	FOLLOW_ACTIVE1				(-1)
#define	FOLLOW_ACTIVE2				(-2)
#define	MAX_VOTE_COUNT				(3)
#define NUM_CLIENT_TRAILS			(10)
#define MAX_INTEREST_POINTS			(64)
#define MAX_COMBAT_POINTS			(512)
#define	MAX_ALERT_EVENTS			(32)
#define MAX_REFNAME					(32)
#define MAX_FILEPATH				(144)

#define PRIVDUEL_ALLOW				(0x0001u)
#define PRIVDUEL_MULTI				(0x0002u)
#define PRIVDUEL_RESPAWN			(0x0004u)
#define PRIVDUEL_NOSEVER			(0x0008u)
#define PRIVDUEL_TEAM				(0x0010u)
#define PRIVDUEL_WEAP				(0x0020u)

#define FINDCL_SUBSTR				(0x0001u)
#define FINDCL_FIRSTMATCH			(0x0002u)
#define FINDCL_CASE					(0x0004u)
#define FINDCL_PRINT				(0x0008u)


#define SPF_BUTTON_USABLE			(0x0001u)
#define SPF_BUTTON_FPUSHABLE		(0x0002u)

#define	RTF_NONE					(0x00000000u)
#define	RTF_NAVGOAL					(0x00000001u)

#define	PSG_VOTED					(0x0001u) // already cast a vote

#define DAMAGE_NORMAL				(0x00000000u) // No flags set.
#define DAMAGE_RADIUS				(0x00000001u) // damage was indirect
#define DAMAGE_NO_ARMOR				(0x00000002u) // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK			(0x00000004u) // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		(0x00000008u) // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION	(0x00000010u) // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_UNUSED00000020		(0x00000020u) //
#define DAMAGE_EXTRA_KNOCKBACK		(0x00000040u) // add extra knockback to this damage
#define DAMAGE_DEATH_KNOCKBACK		(0x00000080u) // only does knockback on death of target
#define DAMAGE_IGNORE_TEAM			(0x00000100u) // damage is always done, regardless of teams
#define DAMAGE_NO_DAMAGE			(0x00000200u) // do no actual damage but react as if damage was taken
#define DAMAGE_HALF_ABSORB			(0x00000400u) // half shields, half health
#define DAMAGE_HALF_ARMOR_REDUCTION	(0x00000800u) // This damage doesn't whittle down armor as efficiently.
#define DAMAGE_HEAVY_WEAP_CLASS		(0x00001000u) // Heavy damage
#define DAMAGE_NO_HIT_LOC			(0x00002000u) // No hit location
#define DAMAGE_NO_SELF_PROTECTION	(0x00004000u) // Dont apply half damage to self attacks
#define DAMAGE_NO_DISMEMBER			(0x00008000u) // Dont do dismemberment
#define DAMAGE_SABER_KNOCKBACK1		(0x00010000u) // Check the attacker's first saber for a knockbackScale
#define DAMAGE_SABER_KNOCKBACK2		(0x00020000u) // Check the attacker's second saber for a knockbackScale
#define DAMAGE_SABER_KNOCKBACK1_B2	(0x00040000u) // Check the attacker's first saber for a knockbackScale2
#define DAMAGE_SABER_KNOCKBACK2_B2	(0x00080000u) // Check the attacker's second saber for a knockbackScale2

#define	FL_GODMODE					(0x00000001u) //
#define	FL_NOTARGET					(0x00000002u) //
#define	FL_TEAMSLAVE				(0x00000004u) // not the first on the team
#define FL_NO_KNOCKBACK				(0x00000008u) //
#define FL_DROPPED_ITEM				(0x00000010u) //
#define FL_NO_BOTS					(0x00000020u) // spawn point not for bot use
#define FL_NO_HUMANS				(0x00000040u) // spawn point just for bots
#define FL_FORCE_GESTURE			(0x00000080u) // force gesture on client
#define FL_INACTIVE					(0x00000100u) // inactive
#define FL_NAVGOAL					(0x00000200u) // for npc nav stuff
#define	FL_DONT_SHOOT				(0x00000400u) //
#define FL_SHIELDED					(0x00000800u) //
#define FL_UNDYING					(0x00001000u) // takes damage down to 1, but never dies
#define	FL_BOUNCE					(0x00002000u) // for missiles
#define	FL_BOUNCE_HALF				(0x00004000u) // for missiles
#define	FL_BOUNCE_SHRAPNEL			(0x00008000u) // special shrapnel flag
#define	FL_VEH_BOARDING				(0x00010000u) //
#define FL_DMG_BY_SABER_ONLY		(0x00020000u) // only take dmg from saber
#define FL_DMG_BY_HEAVY_WEAP_ONLY	(0x00040000u) // only take dmg from explosives
#define FL_BBRUSH					(0x00080000u) // i am a breakable brush

typedef enum moverState_e {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

typedef enum hitLocation_e {
	HL_NONE = 0,
	HL_FOOT_RT,
	HL_FOOT_LT,
	HL_LEG_RT,
	HL_LEG_LT,
	HL_WAIST,
	HL_BACK_RT,
	HL_BACK_LT,
	HL_BACK,
	HL_CHEST_RT,
	HL_CHEST_LT,
	HL_CHEST,
	HL_ARM_RT,
	HL_ARM_LT,
	HL_HAND_RT,
	HL_HAND_LT,
	HL_HEAD,
	HL_GENERIC1,
	HL_GENERIC2,
	HL_GENERIC3,
	HL_GENERIC4,
	HL_GENERIC5,
	HL_GENERIC6,
	HL_MAX
} hitLocation_t;

typedef struct channel_s {
	char identifier[32];
	char shortname[32]; //only legacy clients need this
	struct channel_s *next;
} channel_t;

struct gentity_s {
	entityState_t		s; // communicated by server to clients
	playerState_t		*playerState; // ptr to playerstate if applicable (for bg ents)
	Vehicle_t			*m_pVehicle; // vehicle data
	void				*ghoul2; // g2 instance
	int					localAnimIndex; // index locally (game/cgame) to anim data for this skel
	vector3				modelScale;
	// from here up must be the same as centity_t/bgEntity_t
	entityShared_t		r;
	int					taskID[NUM_TIDS];
	parms_t				*parms;
	char				*behaviorSet[NUM_BSETS];
	const char			*script_targetname;
	int					delayScriptTime;
	const char			*fullName;
	const char			*targetname;
	const char			*classname;
	int					waypoint;
	int					lastWaypoint;
	int					lastValidWaypoint;
	int					noWaypointTime;
	int					combatPoint;
	int					failedWaypoints[MAX_FAILED_NODES];
	int					failedWaypointCheckTime;
	int					next_roff_time;
	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s	*client;			// NULL if not a client
	gNPC_t				*NPC;//Only allocated if the entity becomes an NPC
	int					cantHitEnemyCounter;//HACK - Makes them look for another enemy on the same team if the one they're after can't be hit
	qboolean			noLumbar; //see note in cg_local.h
	qboolean			inuse;
	int					lockCount; //used by NPCs
	uint32_t			spawnflags;			// set in QuakeEd
	int					teamnodmg;			// damage will be ignored if it comes from this team
	char				*roffname;			// set in QuakeEd
	char				*rofftarget;		// set in QuakeEd
	char				*healingclass; //set in quakeed
	char				*healingsound; //set in quakeed
	int					healingrate; //set in quakeed
	int					healingDebounce; //debounce for generic object healing shiz
	char				*ownername;
	int					objective;
	int					side;
	int					passThroughNum;		// set to index to pass through (+1) for missiles
	int					aimDebounceTime;
	int					painDebounceTime;
	int					attackDebounceTime;
	int					alliedTeam;			// only useable by this team, never target this team
	int					roffid;				// if roffname != NULL then set on spawn
	qboolean			neverFree;			// if true, FreeEntity will only unlink
	uint32_t			flags;				// FL_* variables
	const char			*model, *model2;
	int					freetime;			// level.time when the object was freed
	int					eventTime;			// events will be cleared EVENT_VALID_MSEC after set
	qboolean			freeAfterEvent;
	qboolean			unlinkAfterEvent;
	qboolean			physicsObject;		// if true, it can be pushed by movers and fall off edges.
	float				physicsBounce;		// 1.0f = continuous bounce, 0.0f = no bounce
	uint32_t			clipmask;			// brushes with this content value will be collided against when moving.
	const char			*NPC_type;
	char				*NPC_targetname;
	char				*NPC_target;
	moverState_t		moverState;
	int					soundPos1, sound1to2, sound2to1, soundPos2, soundLoop;
	gentity_t			*parent;
	gentity_t			*nextTrain, *prevTrain;
	vector3				pos1, pos2;
	vector3				pos3;
	char				*message;
	int					timestamp;		// body queue sinking, etc
	float				angle;			// set in editor, -1 = up, -2 = down
	char				*target, *target2, *target3, *target4, *target5, *target6;
	char				*team;
	char				*targetShaderName, *targetShaderNewName;
	gentity_t			*target_ent;
	char				*closetarget;
	char				*opentarget;
	char				*paintarget;
	char				*goaltarget;
	char				*idealclass;
	float				radius;
	int					maxHealth; //used as a base for crosshair health display
	float				speed;
	vector3				movedir;
	float				mass;
	int					setTime;
	int					nextthink;
	void( *think )(gentity_t *self);
	void( *reached )(gentity_t *self); // movers call this when hitting endpoint
	void( *blocked )(gentity_t *self, gentity_t *other);
	void( *touch )(gentity_t *self, gentity_t *other, trace_t *trace);
	void( *use )(gentity_t *self, gentity_t *other, gentity_t *activator);
	void( *pain )(gentity_t *self, gentity_t *attacker, int damage);
	void( *die )(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
	int					pain_debounce_time;
	int					fly_sound_debounce_time;	// wind tunnel
	int					last_move_time;
	int					health;
	qboolean			takedamage;
	material_t			material;
	uint32_t			dflags;
	int					damage, splashDamage;
	int					splashRadius;
	int					methodOfDeath, splashMethodOfDeath;
	int					locationDamage[HL_MAX];		// Damage accumulated on different body locations
	int					count;
	int					bounceCount;
	qboolean			alt_fire;
	gentity_t			*chain;
	gentity_t			*enemy;
	gentity_t			*lastEnemy;
	gentity_t			*activator;
	gentity_t			*teamchain;		// next entity in team
	gentity_t			*teammaster;	// master of the team
	int					watertype;
	int					waterlevel;
	int					noise_index;
	float				wait;
	float				random;
	int					delay;
	int					genericValue1, genericValue2, genericValue3, genericValue4, genericValue5, genericValue6, genericValue7,
		genericValue8, genericValue9, genericValue10, genericValue11, genericValue12, genericValue13,
		genericValue14, genericValue15;
	char				*soundSet;
	qboolean			isSaberEntity;
	int					damageRedirect; //if entity takes damage, redirect to..
	int					damageRedirectTo; //this entity number
	vector3				epVelocity;
	float				epGravFactor;
	const gitem_t		*item;			// for bonus items
	int					userinfoChanged;
	int					userinfoSpam;
	vector3				portal_matrix[3];
	qboolean			jpSpawned;
	uint32_t			savedContents;
	struct {
		int					percentStart, percentEnd;
		int					timeStart, timeEnd;
	} modelscale;
};

typedef enum damageRedirect_e {
	DR_NONE = 0,
	DR_HEAD,
	DR_RLEG,
	DR_LLEG,
} damageRedirect_t;

typedef enum clientConnected_e {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum spectatorState_e {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum playerTeamStateState_e {
	TEAM_BEGIN,		// Beginning a team game, spawn at base
	TEAM_ACTIVE		// Now actively playing
} playerTeamStateState_t;

typedef struct playerTeamState_s {
	playerTeamStateState_t	state;
	int						location;
	int						captures;
	int						basedefense;
	int						carrierdefense;
	int						flagrecovery;
	int						fragcarrier;
	int						assists;
	float					lasthurtcarrier;
	float					lastreturnedflag;
	float					flagsince;
	float					lastfraggedcarrier;
} playerTeamState_t;

typedef struct clientSession_s {
	team_t				sessionTeam;
	int					spectatorTime; // for determining next-in-line to play
	spectatorState_t	spectatorState;
	int					spectatorClient; // for chasecam and follow mode
	int					wins, losses; // tournament stats
	unsigned int		selectedFP; // check against this, if doesn't match value in playerstate then update userinfo
	int					saberLevel; // similar to above method, but for current saber attack level
	int					setForce; // set to true once player is given the chance to set force powers
	int					updateUITime; // only update userinfo for FP/SL if < level.time
	char				siegeClass[64];
	int					duelTeam;
	int					siegeDesiredTeam;
	char				IP[NET_ADDRSTRMAXLEN];
} clientSession_t;

typedef struct clientPersistant_s {
	clientConnected_t	connected;
	usercmd_t			cmd; // we would lose angles if not persistant
	qboolean			localClient; // true if "ip" info key is "localhost"
	qboolean			initialSpawn; // the first spawn should be at a cool location
	qboolean			predictItemPickup; // based on cg_predictItems userinfo
	char				netname[MAX_NETNAME], netnameClean[MAX_NETNAME];
	int					netnameTime; // Last time the name was changed
	int					maxHealth; // for handicapping
	int					enterTime; // level.time the client entered the game
	playerTeamState_t	teamState; // status in teamplay games
	qboolean			teamInfo; // send team overlay updates?
	adminUser_t			*adminUser;
	adminData_t			adminData;
	qboolean			ready;
	int					vote; // 0 = none, 1 = yes, 2 = no
	int					connectTime;
	uint32_t			CSF, CPD; // CSF_***/CPD_*** bit-flags
	char				saber1[MAX_QPATH], saber2[MAX_QPATH];
	channel_t			*channels, *activeChannel; // linked list of channels. activeChannel only used for legacy clients that don't support inline chat filters/tabs
	int					duelWeapon; // for weapon-specific duels - pistols etc
	vector3				duelStartPos; // respawn client to this position when duel ends
	qboolean			ignore[MAX_CLIENTS];
	int					speed;
} clientPersistant_t;

typedef struct clientTrail_s {
	vector3		mins, maxs;
	vector3		currentOrigin, currentAngles;
	int			time, leveltime;
} clientTrail_t;

typedef struct renderInfo_s {
	int			headYawRangeLeft, headYawRangeRight, headPitchRangeUp, headPitchRangeDown;
	int			torsoYawRangeLeft, torsoYawRangeRight, torsoPitchRangeUp, torsoPitchRangeDown;
	int			legsFrame, torsoFrame;
	float		legsFpsMod, torsoFpsMod;
	vector3		customRGB;
	int			customAlpha;
	uint32_t	renderFlags;
	vector3		muzzlePoint, muzzlePointOld;
	vector3		muzzleDir, muzzleDirOld;
	int			mPCalcTime; // Last time muzzle point was calced
	float		lockYaw;
	vector3		headPoint;
	vector3		headAngles;
	vector3		handRPoint, handLPoint;
	vector3		crotchPoint;
	vector3		footRPoint, footLPoint;
	vector3		torsoPoint;
	vector3		torsoAngles;
	vector3		eyePoint, eyeAngles;
	int			lookTarget;
	lookMode_t	lookMode;
	int			lookTargetClearTime;//Time to clear the lookTarget
	int			lastVoiceVolume;//Last frame's voice volume
	vector3		lastHeadAngles;//Last headAngles, NOT actual facing of head model
	vector3		headBobAngles;//headAngle offsets
	vector3		targetHeadBobAngles;//head bob angles will try to get to targetHeadBobAngles
	int			lookingDebounceTime;//When we can stop using head looking angle behavior
	float		legsYaw;//yaw angle your legs are actually rendering at
	void		*lastG2; //if it doesn't match ent->ghoul2, the bolts are considered invalid.
	int			headBolt, handRBolt, handLBolt, torsoBolt, crotchBolt, footRBolt, footLBolt, motionBolt;
	int			boltValidityTime;
} renderInfo_t;

#define EMF_NONE	(0x00u)
#define EMF_STATIC	(0x01u) // hold animation on torso + legs, don't allow movement
#define EMF_HOLD	(0x02u) // hold animation on torso
#define EMF_HOLSTER	(0x08u) // forcibly deactivate saber

typedef struct emote_s {
	const char *name;
	animNumber_t animLoop, animLeave;
	uint32_t flags;
} emote_t;

typedef struct gclient_s {
	playerState_t		ps; // communicated by server to clients
	clientPersistant_t	pers;
	clientSession_t		sess;
	saberInfo_t			saber[MAX_SABERS];
	void				*weaponGhoul2[MAX_SABERS];
	int					tossableItemDebounce;
	int					bodyGrabTime;
	int					bodyGrabIndex;
	int					pushEffectTime;
	int					invulnerableTimer;
	int					saberCycleQueue;
	int					legsAnimExecute;
	int					torsoAnimExecute;
	qboolean			legsLastFlip, torsoLastFlip;
	qboolean			readyToExit;		// wishes to leave the intermission
	qboolean			noclip;
	int					lastCmdTime;
	int					buttons, oldbuttons, latched_buttons;
	int					damage_armor;		// damage absorbed by armor
	int					damage_blood;		// damage taken out of health
	vector3				damage_from;		// origin for vector calculation
	qboolean			damage_fromWorld;	// if true, don't use the damage_from vector
	int					accuracy_shots, accuracy_hits; // total number of shots/hits
	int					respawnTime;		// can respawn when time > this, force after g_forcerespwan
	int					inactivityTime;		// kick players when time > this
	qboolean			inactivityWarning;	// qtrue if the five seoond warning has been given
	int					rewardTime;			// clear the EF_AWARD_IMPRESSIVE, etc when time > this
	int					airOutTime;
	int					lastKillTime;		// for multiple kill rewards
	qboolean			fireHeld;			// used for hook
	gentity_t			*hook;				// grapple hook if out
	int					switchTeamTime;		// time the player switched teams
	int					switchDuelTeamTime;	// time the player switched duel teams
	int					switchClassTime;	// class changed debounce timer
	int					timeResidual;
	char				*areabits;
	int					g2LastSurfaceHit; //index of surface hit during the most recent ghoul2 collision performed on this client.
	int					g2LastSurfaceTime; //time when the surface index was set (to make sure it's up to date)
	int					corrTime;
	vector3				lastHeadAngles;
	int					lookTime;
	int					brokenLimbs;
	qboolean			noCorpse; //don't leave a corpse on respawn this time.
	int					jetPackTime;
	qboolean			jetPackOn;
	int					jetPackToggleTime;
	int					jetPackDebRecharge;
	int					jetPackDebReduce;
	int					cloakToggleTime;
	int					cloakDebRecharge;
	int					cloakDebReduce;
	int					saberStoredIndex; //stores saberEntityNum from playerstate for when it's set to 0 (indicating saber was knocked out of the air)
	int					saberKnockedTime; //if saber gets knocked away, can't pull it back until this value is < level.time
	vector3				olderSaberBase; //Set before lastSaberBase_Always, to whatever lastSaberBase_Always was previously
	qboolean			olderIsValid;	//is it valid?
	vector3				lastSaberDir_Always, lastSaberBase_Always;
	int					lastSaberStorageTime; //server time that the above two values were updated (for making sure they aren't out of date)
	qboolean			hasCurrentPosition;	//are lastSaberTip and lastSaberBase valid?
	int					dangerTime;		// level.time when last attack occured
	int					idleTime;		//keep track of when to play an idle anim on the client.
	int					idleHealth;		//stop idling if health decreases
	vector3				idleViewAngles;	//stop idling if viewangles change
	int					forcePowerSoundDebounce; //if > level.time, don't do certain sound events again (drain sound, absorb sound, etc)
	char				modelname[MAX_QPATH];
	qboolean			fjDidJump;
	qboolean			ikStatus;
	float				hiddenDist;//How close ents have to be to pick you up as an enemy
	vector3				hiddenDir;//Normalized direction in which NPCs can't see you (you are hidden)
	renderInfo_t		renderInfo;
	npcteam_t			playerTeam, enemyTeam;
	gentity_t			*leader;
	class_t				NPC_class;
	vector3				pushVec;
	int					pushVecTime;
	int					siegeClass;
	int					holdingObjectiveItem;
	int					timeMedHealed;
	int					timeMedSupplied;
	int					medSupplyDebounce;
	int					isHacking;
	vector3				hackingAngles;
	int					siegeEDataSend;
	int					ewebIndex; //index of e-web gun if spawned
	int					ewebTime; //e-web use debounce
	int					ewebHealth; //health of e-web (to keep track between deployments)
	int					inSpaceIndex; //ent index of space trigger if inside one
	int					inSpaceSuffocation; //suffocation timer
	int					tempSpectate; //time to force spectator mode
	int					jediKickIndex;
	int					jediKickTime;
	int					grappleIndex;
	int					grappleState;
	int					solidHack;
	int					noLightningTime;
	uint32_t			mGameFlags;
	qboolean			iAmALoser;
	int					lastGenCmd;
	int					lastGenCmdTime;
	qboolean			hookHasBeenFired;
	int					trailHead;
	clientTrail_t		trail[NUM_CLIENT_TRAILS];
	clientTrail_t		saved; // used to restore after time shift
	int					lastScoresTime; // level.time the last scoreboard message was went
	qboolean			scoresWaiting;
	struct {
		qboolean			freeze;
		animNumber_t		nextAnim;
	} emote;
	struct {
		int					drain, lightning;
	} forceDebounce;
} gclient_t;

typedef struct interestPoint_s {
	vector3		origin;
	char		*target;
} interestPoint_t;

typedef struct combatPoint_s {
	vector3		origin;
	uint32_t	flags;
	qboolean	occupied;
	int			waypoint;
	int			dangerTime;
} combatPoint_t;

typedef enum alertEventType_e {
	AET_SIGHT,
	AET_SOUND,
} alertEventType_t;

typedef enum alertEventLevel_e {
	AEL_NONE = 0,
	AEL_MINOR,			//Enemy responds to the sound, but only by looking
	AEL_SUSPICIOUS,		//Enemy looks at the sound, and will also investigate it
	AEL_DISCOVERED,		//Enemy knows the player is around, and will actively hunt
	AEL_DANGER,			//Enemy should try to find cover
	AEL_DANGER_GREAT,	//Enemy should run like hell!
} alertEventLevel_t;

typedef struct alertEvent_s {
	vector3				position;	//Where the event is located
	float				radius;		//Consideration radius
	alertEventLevel_t	level;		//Priority level of the event
	alertEventType_t	type;		//Event type (sound,sight)
	gentity_t			*owner;		//Who made the sound
	float				light;		//ambient light level at point
	float				addLight;	//additional light- makes it more noticable, even in darkness
	int					ID;			//unique... if get a ridiculous number, this will repeat, but should not be a problem as it's just comparing it to your lastAlertID
	int					timestamp;	//when it was created
} alertEvent_t;

typedef struct waypointData_s {
	char	targetname[MAX_QPATH];
	char	target[MAX_QPATH];
	char	target2[MAX_QPATH];
	char	target3[MAX_QPATH];
	char	target4[MAX_QPATH];
	int		nodeID;
} waypointData_t;

typedef struct level_locals_s {
	struct gclient_s	*clients;		// [maxclients]
	struct gentity_s	*gentities;
	int					num_entities;		// current number, <= MAX_GENTITIES
	int					warmupTime;			// restart match at this time
	int					maxclients;
	int					framenum;
	int					time;					// in msec
	int					previousTime;			// so movers can back up when blocked
	int					startTime;				// level.time the map was started
	int					teamScores[TEAM_NUM_TEAMS];
	int					lastTeamLocationTime;		// last time of client team location update
	qboolean			newSession;				// don't use any old session data, because we changed gametype
	qboolean			restarted;				// waiting for a map_restart to fire
	int					numConnectedClients;
	int					numNonSpectatorClients;	// includes connecting clients
	int					numPlayingClients;		// connected, non-spectators
	int					sortedClients[MAX_CLIENTS];		// sorted by score
	int					follow1, follow2;		// clientNums for auto-follow spectators
	int					snd_fry;				// sound index for standing in lava
	int					snd_hack;				//hacking loop sound
	int					snd_medHealed;			//being healed by supply class
	int					snd_medSupplied;		//being supplied by supply class
	char				voteString[MAX_STRING_CHARS];
	char				voteStringClean[MAX_STRING_CHARS];
	char				voteDisplayString[MAX_STRING_CHARS];
	int					voteTime;				// level.time vote was called
	int					voteExecuteTime;		// time the vote is executed
	int					voteExecuteDelay;		// set per-vote
	int					voteYes, voteNo;
	qboolean			votePoll;
	char				voteStringPoll[MAX_STRING_CHARS];
	char				voteStringPollCreator[MAX_NETNAME];
	int					numVotingClients;		// set by CalculateRanks
	qboolean			votingGametype;
	int					votingGametypeTo;
	qboolean			spawning;				// the G_Spawn*() functions are valid
	qboolean			manualSpawning;
	int					numSpawnVars;
	char				*spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int					numSpawnVarChars;
	char				spawnVarChars[MAX_SPAWN_VARS_CHARS];
	int					intermissionQueued; // intermission was qualified, but wait INTERMISSION_DELAY_TIME before actually going there so the last frag can be watched. Disable future kills during this delay
	int					intermissiontime; // time the intermission was started
	qboolean			readyToExit; // at least one client wants to exit
	int					exitTime;
	vector3				intermission_origin, intermission_angle;
	qboolean			locationLinked; // target_locations get linked
	gentity_t			*locationHead; // head of the location list
	int					bodyQueIndex; // dead bodies
	gentity_t			*bodyQue[BODY_QUEUE_SIZE];
	alertEvent_t		alertEvents[MAX_ALERT_EVENTS];
	int					numAlertEvents;
	int					curAlertID;
	AIGroupInfo_t		groups[MAX_FRAME_GROUPS];
	interestPoint_t		interestPoints[MAX_INTEREST_POINTS];
	int					numInterestPoints;
	combatPoint_t		combatPoints[MAX_COMBAT_POINTS];
	int					numCombatPoints;
	int					mNumBSPInstances;
	int					mBSPInstanceDepth;
	vector3				mOriginAdjust;
	float				mRotationAdjust;
	char				*mTargetAdjust;
	char				mTeamFilter[MAX_QPATH];
	char				rawmapname[MAX_QPATH];
	int					frameStartTime; //NT - actual time frame started
	int					gametype;
	qboolean			allReady;
	qboolean			lockedTeams[TEAM_NUM_TEAMS];

	struct {
		fileHandle_t		admin, console, security;
	} log;

	struct {
		int					state; //OSP: paused state of the match
		int					time;
	} pause;

	struct {
		int					num;
		char				*infos[MAX_BOTS];
	} bots;

	struct {
		int					num;
		char				*infos[MAX_ARENAS];
	} arenas;
} level_locals_t;

typedef struct reference_tag_s {
	char		name[MAX_REFNAME];
	vector3		origin;
	vector3		angles;
	uint32_t	flags;	//Just in case
	int			radius;	//For nav goals
	qboolean	inuse;
} reference_tag_t;

typedef struct bot_settings_s {
	char personalityfile[MAX_FILEPATH];
	float skill;
	char team[MAX_FILEPATH];
} bot_settings_t;

// japp_saberTweaks
#define SABERTWEAK_INTERPOLATE		(0x0001u)
#define SABERTWEAK_PROLONGDAMAGE	(0x0002u)
#define SABERTWEAK_DEFLECTION		(0x0004u)
#define SABERTWEAK_SPECIALMOVES		(0x0008u)

typedef enum teleportBits_e {
	JAPP_TPBIT_SILENT = 0,
	JAPP_TPBIT_NOSLICK,
	JAPP_TPBIT_KEEPVELOCITY,
	JAPP_TPBIT_KEEPANGLES,
	JAPP_TPBIT_NOTELEFRAG,
} teleportBits_t;

typedef enum userinfoValidationBits_e {
	// validation & (1<<(numUserinfoFields+USERINFO_VALIDATION_BLAH))
	USERINFO_VALIDATION_SIZE = 0,
	USERINFO_VALIDATION_SLASH,
	USERINFO_VALIDATION_EXTASCII,
	USERINFO_VALIDATION_CONTROLCHARS,
	USERINFO_VALIDATION_MAX
} userinfoValidationBits_t;

typedef enum matchPause_e {
	PAUSE_NONE = 0,
	PAUSE_PAUSED,
	PAUSE_UNPAUSING,
} matchPause_t;

void				Add_Ammo( gentity_t *ent, int weapon, int count );
void				AddScore( gentity_t *ent, vector3 *origin, int score );
void				AddSightEvent( gentity_t *owner, vector3 *position, float radius, alertEventLevel_t alertLevel, float addLight );
void				AddSoundEvent( gentity_t *owner, vector3 *position, float radius, alertEventLevel_t alertLevel, qboolean needLOS );
void				B_InitAlloc( void );
void				B_CleanupAlloc( void );
void				BeginIntermission( void );
void				BlowDetpacks( gentity_t *ent );
void				body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
int					BotAILoadMap( int restart );
int					BotAISetup( int restart );
int					BotAIShutdown( int restart );
int					BotAIShutdownClient( int client, qboolean restart );
int					BotAISetupClient( int client, struct bot_settings_s *settings, qboolean restart );
int					BotAIStartFrame( int time );
void				BotInterbreedEndMatch( void );
void				BroadcastTeamChange( gclient_t *client, int oldTeam );
void				CalcMuzzlePoint( const gentity_t *ent, vector3 *forward, vector3 *right, vector3 *up, vector3 *muzzlePoint );
void				CalculateRanks( void );
void				ClearRegisteredItems( void );
void				ClientSpawn( gentity_t *ent );
void				ClientCleanName( const char *in, char *out, int outSize );
qboolean			CheckDuplicateName( int clientNum );
const char *		ClientConnect( int clientNum, qboolean firstTime, qboolean isBot );
qboolean			ClientUserinfoChanged( int clientNum );
void				ClientDisconnect( int clientNum );
void				ClientBegin( int clientNum, qboolean allowTeamReset );
void				ClientCommand( int clientNum );
void				ClientThink( int clientNum, usercmd_t *ucmd );
void				ClientEndFrame( gentity_t *ent );
qboolean			Client_Supports( const gentity_t *ent, uint32_t supportFlag );
void				Cmd_EngageDuel_f( gentity_t *ent );
void				Cmd_FollowCycle_f( gentity_t *ent, int dir );
void				Cmd_SaberAttackCycle_f( gentity_t *ent );
void				Cmd_Score_f( gentity_t *ent );
void				Cmd_ToggleSaber_f( gentity_t *ent );
char *				ConcatArgs( int start );
qboolean			ConsoleCommand( void );
gentity_t *			CreateMissile( vector3 *org, vector3 *dir, float vel, int life, gentity_t *owner, qboolean altFire );
gentity_t *			Drop_Item( gentity_t *ent, const gitem_t *item, float angle );
void				FindIntermissionPoint( void );
void				FinishSpawningItem( gentity_t *ent );
gentity_t *			fire_grapple( gentity_t *self, vector3 *start, vector3 *dir );
void				FireWeapon( gentity_t *ent, qboolean altFire );
void				ForceAbsorb( gentity_t *self );
void				ForceGrip( gentity_t *self );
void				ForceHeal( gentity_t *self );
int					ForcePowerUsableOn( gentity_t *attacker, gentity_t *other, forcePowers_t forcePower );
void				ForceProtect( gentity_t *self );
void				ForceRage( gentity_t *self );
void				ForceSeeing( gentity_t *self );
void				ForceSpeed( gentity_t *self, int forceDuration );
void				ForceTeamForceReplenish( gentity_t *self );
void				ForceTeamHeal( gentity_t *self );
void				ForceTelepathy( gentity_t *self );
void				ForceThrow( gentity_t *self, qboolean pull );
qboolean			G_ActivateBehavior( gentity_t *self, int bset );
void				G_AddBot( const char *name, float skill, const char *team, int delay, char *altname );
void				G_AddEvent( gentity_t *ent, int event, int eventParm );
void				G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
char *				G_AddSpawnVarToken( const char *string );
void *				G_Alloc( int size );
void				G_AvoidBox( gentity_t *ent );
int					G_BoneIndex( const char *name );
qboolean			G_BotConnect( int clientNum, qboolean restart );
void				G_BounceProjectile( vector3 *start, vector3 *impact, vector3 *dir, vector3 *endout );
void				G_BreakArm( gentity_t *ent, int arm );
int					G_BSPIndex( const char *name );
qboolean			G_CallSpawn( gentity_t *ent );
int					G_CheckAlertEvents( gentity_t *self, qboolean checkSight, qboolean checkSound, float maxSeeDist, float maxHearDist, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel ); //ignoreAlert = -1, mustHaveOwner = qfalse, minAlertLevel = AEL_MINOR
void				G_CheckBotSpawn( void );
void				G_CheckClientTimeouts( gentity_t *ent );
qboolean			G_CheckForDanger( gentity_t *self, int alertEvent );
void				G_CheckForDismemberment( gentity_t *ent, gentity_t *enemy, vector3 *point, int damage, int deathAnim, qboolean postDeath );
qboolean			G_CheckInSolid( gentity_t *self, qboolean fix );
void				G_CheckTeamItems( void );
void				G_ClearClientLog( int client );
qboolean			G_ClearLOS( gentity_t *self, const vector3 *start, const vector3 *end );
qboolean			G_ClearLOS2( gentity_t *self, gentity_t *ent, const vector3 *end );
qboolean			G_ClearLOS3( gentity_t *self, const vector3 *start, gentity_t *ent );
qboolean			G_ClearLOS4( gentity_t *self, gentity_t *ent );
qboolean			G_ClearLOS5( gentity_t *self, const vector3 *end );
void				G_ClearVote( gentity_t *ent );
int					G_ClientFromString( const gentity_t *ent, const char *match, uint32_t flags );
void				G_CreateFakeClient( int entNum, gclient_t **cl );
void				G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vector3 *dir, vector3 *point, int damage, uint32_t dflags, int mod );
qboolean			G_DoesMapSupportGametype( const char *mapname, int gametype );
int					G_EffectIndex( const char *name );
void				G_EntitySound( gentity_t *ent, int channel, int soundIndex );
void				G_ExplodeMissile( gentity_t *ent );
gentity_t *			G_Find( gentity_t *from, int fieldofs, const char *match );
void				G_FreeEntity( gentity_t *e );
gentity_t *			G_GetDuelWinner( gclient_t *client );
char *				G_GetBotInfoByName( const char *name );
const char *		G_GetStringEdString( const char *refSection, const char *refName );
int					G_IconIndex( const char *name );
void				G_InitBots( qboolean restart );
void				G_InitGentity( gentity_t *e );
void				G_InitMemory( void );
void				G_InitSessionData( gclient_t *client, char *userinfo, qboolean isBot );
void				G_InitWorldSession( void );
int					G_ItemUsable( playerState_t *ps, int forcedUse );
void				G_KillBox( gentity_t *ent );
void				G_KillG2Queue( int entNum );
void				G_LoadArenas( void );
void				G_LogExit( const char *string );
void				G_LogPrintf( fileHandle_t filehandle, const char *fmt, ... );
void				G_LogWeaponDamage( int client, int mod, int amount );
void				G_LogWeaponDeath( int client, int weaponid );
void				G_LogWeaponFire( int client, int weaponid );
void				G_LogWeaponFrag( int attacker, int deadguy );
void				G_LogWeaponItem( int client, int itemid );
void				G_LogWeaponInit( void );
void				G_LogWeaponKill( int client, int mod );
void				G_LogWeaponOutput( void );
void				G_LogWeaponPickup( int client, int weaponid );
void				G_LogWeaponPowerup( int client, int powerupid );
int					G_ModelIndex( const char *name );
void				G_MuteSound( int entnum, int channel );
char *				G_NewString( const char *string );
gentity_t *			G_PickTarget( char *targetname );
void				G_PlayDoorLoopSound( gentity_t *ent );
void				G_PlayDoorSound( gentity_t *ent, int type );
gentity_t *			G_PlayEffect( int fxID, vector3 *org, vector3 *ang );
gentity_t *			G_PlayEffectID( const int fxID, vector3 *org, vector3 *ang );
void				G_PowerDuelCount( int *loners, int *doubles, qboolean countSpec );
void				G_PrecacheDispensers( void );
gentity_t *			G_PreDefSound( vector3 *org, int pdSound );
const char *		G_PrintClient( int clientNum );
void				G_PrintCommands( gentity_t *ent );
qboolean			G_RadiusDamage( vector3 *origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, gentity_t *missile, int mod );
int					G_RadiusList( vector3 *origin, float radius, gentity_t *ignore, qboolean takeDamage, gentity_t *ent_list[MAX_GENTITIES] );
void				G_ReadSessionData( gclient_t *client );
trace_t *			G_RealTrace( gentity_t *ent, float dist );
void				G_ReflectMissile( gentity_t *ent, gentity_t *missile, vector3 *forward );
const char *		G_RefreshNextMap( int gametype, qboolean forced );
void				G_RemoveQueuedBotBegin( int clientNum );
void				G_RunClient( gentity_t *ent );
void				G_RunExPhys( gentity_t *ent, float gravity, float mass, float bounce, qboolean autoKill, int *g2Bolts, int numG2Bolts );
void				G_RunItem( gentity_t *ent );
void				G_RunMissile( gentity_t *ent );
void				G_RunMover( gentity_t *ent );
void				G_RunObject( gentity_t *ent );
void				G_RunThink( gentity_t *ent );
qboolean			G_SaberModelSetup( gentity_t *ent );
void				G_ScaleNetHealth( gentity_t *self );
gentity_t *			G_ScreenShake( vector3 *org, gentity_t *target, float intensity, int duration, qboolean global );
void				G_SendG2KillQueue( void );
qboolean			G_SetSaber( gentity_t *ent, int saberNum, const char *saberName, qboolean siegeOverride );
void				G_SetAngles( gentity_t *ent, vector3 *angles );
void				G_SetAnim( gentity_t *ent, usercmd_t *ucmd, int setAnimParts, int anim, uint32_t setAnimFlags, int blendTime );
void				G_SetEnemy( gentity_t *self, gentity_t *enemy );
void				G_SetMovedir( vector3 *angles, vector3 *movedir );
void				G_SetOrigin( gentity_t *ent, vector3 *origin );
void				G_SetStats( gentity_t *ent );
void				G_ShowGameMem( void );
void				G_SiegeClientExData( gentity_t *msgTarg );
void				G_Sound( gentity_t *ent, int channel, int soundIndex );
void				G_SoundAtLoc( vector3 *loc, int channel, int soundIndex );
int					G_SoundIndex( const char *name );
int					G_SoundSetIndex( const char *name );
gentity_t *			G_Spawn( void );
qboolean			G_SpawnBoolean( const char *key, const char *defaultString, qboolean *out );
void				G_SpawnEntitiesFromString( qboolean inSubBSP );
qboolean			G_SpawnFloat( const char *key, const char *defaultString, float *out );
void				G_SpawnGEntityFromSpawnVars( qboolean inSubBSP );
qboolean			G_SpawnInt( const char *key, const char *defaultString, int *out );
void				G_SpawnItem( gentity_t *ent, const gitem_t *item );
qboolean			G_SpawnString( const char *key, const char *defaultString, char **out );
qboolean			G_SpawnVector( const char *key, const char *defaultString, vector3 *out );
void				G_TeamCommand( team_t team, const char *cmd );
gentity_t *			G_TempEntity( vector3 *origin, int event );
void				G_TestLine( vector3 *start, vector3 *end, int color, int time );
void				G_Throw( gentity_t *targ, vector3 *newDir, float push );
void				G_TouchSolids( gentity_t *ent );
void				G_TouchTriggers( gentity_t *ent );
void				G_UpdateClientAnims( gentity_t *self, float animSpeedScale );
void				G_UseTargets( gentity_t *ent, gentity_t *activator );
void				G_UseTargets2( gentity_t *ent, gentity_t *activator, const char *string );
void				G_WriteSessionData( void );
void				GetAnglesForDirection( const vector3 *p1, const vector3 *p2, vector3 *out );
void				GlobalUse( gentity_t *self, gentity_t *other, gentity_t *activator );
qboolean			HasSetSaberOnly( void );
void				ItemUse_Binoculars( gentity_t *ent );
void				ItemUse_Jetpack( gentity_t *ent );
void				ItemUse_MedPack( gentity_t *ent );
void				ItemUse_MedPack_Big( gentity_t *ent );
void				ItemUse_Seeker( gentity_t *ent );
void				ItemUse_Sentry( gentity_t *ent );
void				ItemUse_Shield( gentity_t *ent );
void				ItemUse_UseCloak( gentity_t *ent );
void				ItemUse_UseDisp( gentity_t *ent, int type );
void				ItemUse_UseEWeb( gentity_t *ent );
void				InitBodyQue( void );
void				InitSiegeMode( void );
qboolean			InFront( vector3 *spot, vector3 *from, vector3 *fromAngles, float threshHold );
int					InFieldOfVision( vector3 *viewangles, float fov, vector3 *angles );
qboolean			Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc );
void				Jetpack_Off( gentity_t *ent );
void				Jetpack_On( gentity_t *ent );
gentity_t *			LaunchItem_Throw( const gitem_t *item, vector3 *origin, vector3 *velocity );
qboolean			LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void				MaintainBodyQueue( gentity_t *ent );
void				MoveClientToIntermission( gentity_t *client );
float				NPC_GetHFOVPercentage( vector3 *spot, vector3 *from, vector3 *facing, float hFOV );
float				NPC_GetVFOVPercentage( vector3 *spot, vector3 *from, vector3 *facing, float vFOV );
qboolean			OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
int					OrgVisible( vector3 *org1, vector3 *org2, int ignore );
team_t				PickTeam( int ignoreClientNum );
void				player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );
void				PrecacheItem( const gitem_t *it );
void				RegisterItem( const gitem_t *item );
void				RemoveDetpacks( gentity_t *ent );
void				respawn( gentity_t *ent );
void				RespawnItem( gentity_t *ent );
void				SaveRegisteredItems( void );
gentity_t *			SelectSpawnPoint( vector3 *avoidPoint, vector3 *origin, vector3 *angles, team_t team );
void				SendScoreboardMessageToAllClients( void );
void				SetClientViewAngle( gentity_t *ent, vector3 *angle );
qboolean			SetTeam( gentity_t *ent, const char *s, qboolean forced );
void				SnapVectorTowards( vector3 *v, const vector3 *to );
qboolean			SpotWouldTelefrag( gentity_t *spot );
qboolean			SpotWouldTelefrag2( gentity_t *mover, vector3 *dest );
qboolean			SpotWouldTelefrag3( vector3 *spot );
void				StopFollowing( gentity_t *ent );
void				SV_ToggleUserinfoValidation_f( void );
void				SV_ToggleAllowVote_f( void );
void				Svcmd_AddBot_f( void );
void				Svcmd_BotList_f( void );
void				Svcmd_MapList_f( void );
void				TAG_Init( void );
reference_tag_t *	TAG_Find( const char *owner, const char *name );
reference_tag_t *	TAG_Add( const char *name, const char *owner, vector3 *origin, vector3 *angles, int radius, uint32_t flags );
int					TAG_GetOrigin( const char *owner, const char *name, vector3 *origin );
int					TAG_GetOrigin2( const char *owner, const char *name, vector3 *origin );
int					TAG_GetAngles( const char *owner, const char *name, vector3 *angles );
int					TAG_GetRadius( const char *owner, const char *name );
uint32_t			TAG_GetFlags( const char *owner, const char *name );
int					TeamCount( int ignoreClientNum, team_t team );
void				Team_CheckDroppedItem( gentity_t *dropped );
void				TeleportPlayer( gentity_t *player, vector3 *origin, vector3 *angles );
void				TIMER_Clear( void );
void				TIMER_Clear2( gentity_t *ent );
void				TIMER_Set( gentity_t *ent, const char *identifier, int duration );
int					TIMER_Get( gentity_t *ent, const char *identifier );
qboolean			TIMER_Done( gentity_t *ent, const char *identifier );
qboolean			TIMER_Start( gentity_t *self, const char *identifier, int duration );
qboolean			TIMER_Done2( gentity_t *ent, const char *identifier, qboolean remove );
qboolean			TIMER_Exists( gentity_t *ent, const char *identifier );
void				TIMER_Remove( gentity_t *ent, const char *identifier );
void				TossClientItems( gentity_t *self );
void				TossClientWeapon( gentity_t *self, vector3 *direction, float speed );
void				Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );
void				Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace );
void				TryUse( gentity_t *ent );
void				UseHoldableItem( gentity_t *ent );
void				Weapon_GrapplingHook_Fire( gentity_t *ent );
void				Weapon_HookFree( gentity_t *ent );
void				Weapon_HookThink( gentity_t *ent );
void				WP_DeactivateSaber( gentity_t *self, qboolean clearLength );
void				WP_FireBlasterMissile( gentity_t *ent, vector3 *start, vector3 *dir, qboolean altFire );
void				WP_FireGenericBlasterMissile( gentity_t *ent, vector3 *start, vector3 *dir, qboolean altFire, int damage, int velocity, int mod );
void				WP_FireTurretMissile( gentity_t *ent, vector3 *start, vector3 *dir, qboolean altFire, int damage, int velocity, int mod, gentity_t *ignore );
void				WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
void				WP_ForcePowersUpdate( gentity_t *self, usercmd_t *ucmd );
void				WP_InitForcePowers( gentity_t *ent );
int					WP_SaberCanBlock( gentity_t *self, vector3 *point, uint32_t dflags, int mod, qboolean projectile, int attackStr );
void				WP_SaberInitBladeData( gentity_t *ent );
void				WP_SpawnInitForcePowers( gentity_t *ent );
void				WP_SaberPositionUpdate( gentity_t *self, usercmd_t *ucmd );

extern	level_locals_t	level;
extern	gentity_t		g_entities[MAX_GENTITIES];
extern int				gPainMOD;
extern int				gPainHitLoc;
extern vector3			gPainPoint;
extern char				gSharedBuffer[MAX_G_SHARED_BUFFER_SIZE];
extern void				*precachedKyle;
extern void				*g2SaberInstance;
extern qboolean			gEscaping;
extern int				gEscapeTime;
extern int				gGAvoidDismember;
extern int				BMS_START;
extern int				BMS_MID;
extern int				BMS_END;
extern gentity_t		*gJMSaberEnt;
extern qboolean			gDoSlowMoDuel;
extern int				gSlowMoDuelTime;
extern const char		*supportFlagNames[];
extern gameImport_t		*trap;
