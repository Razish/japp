#pragma once

// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "q_shared.h"
#include "tr_types.h"
#include "bg_public.h"
#include "cg_public.h"
//#include "cg_lua.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define NULL_HANDLE			((qhandle_t) 0)
#define NULL_SOUND			((sfxHandle_t) 0)
#define NULL_FX				((fxHandle_t) 0)

#define	POWERUP_BLINKS		5

#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			180	//Raz: was 200
#define	PULSE_TIME			200
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
#define	STEP_TIME			200
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000

// Zoom vars
#define	ZOOM_TIME			150		// not currently used?
#define MAX_ZOOM_FOV		3.0f
#define ZOOM_IN_TIME		1500.0f	
#define ZOOM_OUT_TIME		100.0f
#define ZOOM_START_PERCENT	0.3f	

#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	20
#define	SINK_TIME			1000		// time for fragments to sink into ground before going away
#define	ATTACKER_HEAD_TIME	10000
#define	REWARD_TIME			3000

#define	PULSE_SCALE			1.5			// amount to scale up the icons when activating

#define	MAX_STEP_CHANGE		32

#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define	ICON_SIZE			48
#define	CHAR_WIDTH			32
#define	CHAR_HEIGHT			48
#define	TEXT_ICON_SPACE		4

// very large characters
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

#define NUM_FONT_BIG	1
#define NUM_FONT_SMALL	2
#define NUM_FONT_CHUNKY	3

#define	NUM_CROSSHAIRS		9

#define TEAM_OVERLAY_MAXNAME_WIDTH	32
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	64

#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4f

#define	DEFAULT_MODEL			"kyle"

#define DEFAULT_REDTEAM_NAME		"Empire"
#define DEFAULT_BLUETEAM_NAME		"Rebellion"

typedef enum footstep_e {
	FOOTSTEP_STONEWALK,
	FOOTSTEP_STONERUN,
	FOOTSTEP_METALWALK,
	FOOTSTEP_METALRUN,
	FOOTSTEP_PIPEWALK,
	FOOTSTEP_PIPERUN,
	FOOTSTEP_SPLASH,
	FOOTSTEP_WADE,
	FOOTSTEP_SWIM,
	FOOTSTEP_SNOWWALK,
	FOOTSTEP_SNOWRUN,
	FOOTSTEP_SANDWALK,
	FOOTSTEP_SANDRUN,
	FOOTSTEP_GRASSWALK,
	FOOTSTEP_GRASSRUN,
	FOOTSTEP_DIRTWALK,
	FOOTSTEP_DIRTRUN,
	FOOTSTEP_MUDWALK,
	FOOTSTEP_MUDRUN,
	FOOTSTEP_GRAVELWALK,
	FOOTSTEP_GRAVELRUN,
	FOOTSTEP_RUGWALK,
	FOOTSTEP_RUGRUN,
	FOOTSTEP_WOODWALK,
	FOOTSTEP_WOODRUN,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum impactSound_e {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	qboolean	lastFlip; //if does not match torsoFlip/legsFlip, restart the anim.

	int			lastForcedFrame;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	float		yawSwingDif;

	int			animationNumber;
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact

	float		animationSpeed;		// scale the animation speed
	float		animationTorsoSpeed;

	qboolean	torsoYawing;
} lerpFrame_t;


typedef struct {
	lerpFrame_t		legs, torso, flag;
	int				painTime;
	int				painDirection;	// flip from 0 to 1
	int				lightningFiring;

	// machinegun spinning
	float			barrelAngle;
	int				barrelTime;
	qboolean		barrelSpinning;
} playerEntity_t;

//=================================================

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_COMBAT_SOUNDS	40
#define	MAX_CUSTOM_EXTRA_SOUNDS	40
#define	MAX_CUSTOM_JEDI_SOUNDS	40
// MAX_CUSTOM_SIEGE_SOUNDS defined in bg_public.h
#define MAX_CUSTOM_DUEL_SOUNDS	40

#define	MAX_CUSTOM_SOUNDS	40 //rww - Note that for now these must all be the same, because of the way I am
							   //cycling through them and comparing for custom sounds.

typedef struct clientInfo_s {
	qboolean		infoValid;

	vector4			colorOverride;

	saberInfo_t		saber[MAX_SABERS];
	void			*ghoul2Weapons[MAX_SABERS];

	char			saberName[64];
	char			saber2Name[64];

	char			name[MAX_QPATH];
	team_t			team;

	int				duelTeam;

	int				botSkill;		// 0 = not bot, 1-5 = bot

	int				frame;

	vector3			color1;
	vector3			color2;

	int				icolor1;
	int				icolor2;

	int				score;			// updated by score servercmds
	int				location;		// location index for team mode
	int				health;			// you only get this info about your teammates
	int				armor;
	int				curWeapon;

	int				handicap;
	int				wins, losses;	// in tourney mode

	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader

	int				powerups;		// so can display quad/flag status

	int				medkitUsageTime;

	int				breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char			modelName[MAX_QPATH];
	char			skinName[MAX_QPATH];
//	char			headModelName[MAX_QPATH];
//	char			headSkinName[MAX_QPATH];
	char			forcePowers[MAX_QPATH];
//	char			redTeam[MAX_TEAMNAME];
//	char			blueTeam[MAX_TEAMNAME];

	char			teamName[MAX_TEAMNAME];

	int				corrTime;

	vector3			lastHeadAngles;
	int				lookTime;

	int				brokenLimbs;

	qboolean		deferred;

	qboolean		newAnims;		// true if using the new mission pack animations
	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	vector3			headOffset;		// move head in icon views
	//footstep_t		footsteps;
	gender_t		gender;			// from model

	qhandle_t		legsModel;
	qhandle_t		legsSkin;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;

	//qhandle_t		headModel;
	//qhandle_t		headSkin;

	void			*ghoul2Model;
	
	qhandle_t		modelIcon;

	qhandle_t		bolt_rhand;
	qhandle_t		bolt_lhand;

	qhandle_t		bolt_head;

	qhandle_t		bolt_motion;

	qhandle_t		bolt_llumbar;

	int				siegeIndex;
	int				siegeDesiredTeam;

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];
	sfxHandle_t		combatSounds[MAX_CUSTOM_COMBAT_SOUNDS];
	sfxHandle_t		extraSounds[MAX_CUSTOM_EXTRA_SOUNDS];
	sfxHandle_t		jediSounds[MAX_CUSTOM_JEDI_SOUNDS];
	sfxHandle_t		siegeSounds[MAX_CUSTOM_SIEGE_SOUNDS];
	sfxHandle_t		duelSounds[MAX_CUSTOM_DUEL_SOUNDS];

	int				legsAnim;
	int				torsoAnim;

	float		facial_blink;		// time before next blink. If a minus value, we are in blink mode
	float		facial_frown;		// time before next frown. If a minus value, we are in frown mode
	float		facial_aux;			// time before next aux. If a minus value, we are in aux mode

	int			superSmoothTime; //do crazy amount of smoothing

	//[RGBSabers]
	vector3		rgb1;
	vector3		rgb2;
	//[/RGBSabers]

} clientInfo_t;

//rww - cheap looping sound struct
#define MAX_CG_LOOPSOUNDS 8

typedef struct cgLoopSound_s {
	int entityNum;
	vector3 origin;
	vector3 velocity;
	sfxHandle_t sfx;
} cgLoopSound_t;

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	// This comment below is correct, but now m_pVehicle is the first thing in bg shared entity, so it goes first. - AReis
	//rww - entstate must be first, to correspond with the bg shared entity structure
	entityState_t	currentState;	// from cg.frame
	playerState_t	*playerState;	//ptr to playerstate if applicable (for bg ents)
	Vehicle_t		*m_pVehicle; //vehicle data
	void			*ghoul2; //g2 instance
	int				localAnimIndex; //index locally (game/cgame) to anim data for this skel
	vector3			modelScale; //needed for g2 collision

	//from here up must be unified with bgEntity_t -rww

	entityState_t	nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
//	int				teleportFlag;

	int				trailTime;		// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;

	vector3			damageAngles;
	int				damageTime;

	int				snapShotTime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;

//	int				errorTime;		// decay the error from this time
//	vector3			errorOrigin;
//	vector3			errorAngles;
	
//	qboolean		extrapolated;	// false if origin / angles is an interpolation
//	vector3			rawOrigin;
	vector3			rawAngles;

	vector3			beamEnd;

	// exact interpolated position of entity on this frame
	vector3			lerpOrigin;
	vector3			lerpAngles;

#if 0
	//add up bone offsets until next client frame before adding them in
	qboolean		hasRagOffset;
	vector3			ragOffsets;
	int				ragOffsetTime;
#endif

	vector3			ragLastOrigin;
	int				ragLastOriginTime;

	qboolean		noLumbar; //if true only do anims and things on model_root instead of lower_lumbar, this will be the case for some NPCs.
	qboolean		noFace;

	//For keeping track of the current surface status in relation to the entitystate surface fields.
	int				npcLocalSurfOn;
	int				npcLocalSurfOff;

	int				eventAnimIndex;

	clientInfo_t	*npcClient; //dynamically allocated - always free it, and never stomp over it.

	int				weapon;

	void			*ghoul2weapon; //rww - pointer to ghoul2 instance of the current 3rd person weapon

	float			radius;
	int				boltInfo;

	//sometimes used as a bolt index, but these values are also used as generic values for clientside entities
	//at times
	int				bolt1;
	int				bolt2;
	int				bolt3;
	int				bolt4;

	float			bodyHeight;

	int				torsoBolt;
	
	vector3			turAngles;

	vector3			frame_minus1;
	vector3			frame_minus2;

	int				frame_minus1_refreshed;
	int				frame_minus2_refreshed;

	//Raz: Added
	vector3			frame_minus12;
	vector3			frame_minus22;
	int				frame_minus1_refreshed2;
	int				frame_minus2_refreshed2;

	void			*frame_hold; //pointer to a ghoul2 instance

	int				frame_hold_time;
	int				frame_hold_refreshed;

	void			*grip_arm; //pointer to a ghoul2 instance

	int				trickAlpha;
	int				trickAlphaTime;

	int				teamPowerEffectTime;
	qboolean		teamPowerType; //0 regen, 1 heal, 2 drain, 3 absorb

	qboolean		isRagging;
	qboolean		ownerRagging;
	int				overridingBones;

	int				bodyFadeTime;
	vector3			pushEffectOrigin;

	cgLoopSound_t	loopingSound[MAX_CG_LOOPSOUNDS];
	int				numLoopingSounds;

	int				serverSaberHitIndex;
	int				serverSaberHitTime;
	qboolean		serverSaberFleshImpact; //true if flesh, false if anything else.

	qboolean		ikStatus;

	qboolean		saberWasInFlight;

	float			smoothYaw;

	int				uncloaking;
	qboolean		cloaked;

	int				vChatTime;

	qboolean		shieldHit;
	//Raz: Prediction hacks
	int				savedSolid;
	int				savedeType;

	vector3			lastOrigin; // for strafe trail
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independently from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FADE_SCALE_MODEL, // currently only for Demp2 shock sphere
	LE_FRAGMENT,
	LE_PUFF,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
	LE_OLINE,
	LE_SHOWREFENTITY,
	LE_LINE
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE = 0x0001,			// do not scale size over time
	LEF_TUMBLE			= 0x0002,			// tumble over time, used for ejecting shells
	LEF_FADE_RGB		= 0x0004,			// explicitly fade
	LEF_NO_RANDOM_ROTATE= 0x0008			// MakeExplosion adds random rotate which could be bad in some cases
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS,
	LEBS_METAL,
	LEBS_ROCK
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;
	int				fadeInTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect
	int				bounceSound;		// optional sound index to play upon bounce

	float			alpha;
	float			dalpha;

	int				forceAlpha;

	float			color[4];

	float			radius;

	float			light;
	vector3			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	union {
		struct {
			float radius;
			float dradius;
			vector3 startRGB;
			vector3 dRGB;
		} sprite;
		struct {
			float width;
			float dwidth;
			float length;
			float dlength;
			vector3 startRGB;
			vector3 dRGB;
		} trail;
		struct {
			float width;
			float dwidth;
			// Below are bezier specific.
			vector3			control1;				// initial position of control points
			vector3			control2;
			vector3			control1_velocity;		// initial velocity of control points
			vector3			control2_velocity;
			vector3			control1_acceleration;	// constant acceleration of control points
			vector3			control2_acceleration;
		} line;
		struct {
			float width;
			float dwidth;
			float width2;
			float dwidth2;
			vector3 startRGB;
			vector3 dRGB;
		} line2;
		struct {
			float width;
			float dwidth;
			float width2;
			float dwidth2;
			float height;
			float dheight;
		} cylinder;
		struct {
			float width;
			float dwidth;
		} electricity;
		struct
		{
			// fight the power! open and close brackets in the same column!
			float radius;
			float dradius;
			qboolean (*thinkFn)(struct localEntity_s *le);
			vector3	dir;	// magnitude is 1, but this is oldpos - newpos right before the
							//particle is sent to the renderer
			// may want to add something like particle::localEntity_s *le (for the particle's think fn)
		} particle;
		struct
		{
			qboolean	dontDie;
			vector3		dir;
			float		variance;
			int			delay;
			int			nextthink;
			qboolean	(*thinkFn)(struct localEntity_s *le);
			int			data1;
			int			data2;
		} spawner;
		struct
		{
			float radius;
		} fragment;
	} data;

	refEntity_t		refEntity;		
} localEntity_t;

//======================================================================


typedef struct {
	int				client;
	int				score;
	int				deaths;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerUps;
	int				accuracy;
	int				impressiveCount;
	int				excellentCount;
	int				guantletCount;
	int				defendCount;
	int				assistCount;
	int				captures;
	qboolean	perfect;
	int				team;
} score_t;


// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	const gitem_t	*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;		// this is the pickup model
	qhandle_t		viewModel;			// this is the in-view model used by the player
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	vector3			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vector3			flashDlightColor;

	qhandle_t		weaponIcon;
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;

	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose
	sfxHandle_t		firingSound;
	sfxHandle_t		chargeSound;
	fxHandle_t		muzzleEffect;
	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			missileDlight;
	vector3			missileDlightColor;
	int				missileRenderfx;
	sfxHandle_t		missileHitSound;

	sfxHandle_t		altFlashSound[4];
	sfxHandle_t		altFiringSound;
	sfxHandle_t		altChargeSound;
	fxHandle_t		altMuzzleEffect;
	qhandle_t		altMissileModel;
	sfxHandle_t		altMissileSound;
	void			(*altMissileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			altMissileDlight;
	vector3			altMissileDlightColor;
	int				altMissileRenderfx;
	sfxHandle_t		altMissileHitSound;

	sfxHandle_t		selectSound;

	sfxHandle_t		readySound;
	float			trailRadius;
	float			wiTrailTime;

} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
/*
Ghoul2 Insert Start
*/
	void			*g2Models[MAX_ITEM_MODELS];
	float			radius[MAX_ITEM_MODELS];
/*
Ghoul2 Insert End
*/
} itemInfo_t;


typedef struct {
	int				itemNum;
} powerupInfo_t;


#define MAX_SKULLTRAIL		10

typedef struct {
	vector3 positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;


#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS	16

#if 0 //this feels sooooo good
typedef enum jappMods_e
{
	SMOD_BASE = 0,	//	Base
	SMOD_JAP,		//	JA+
	SMOD_JAPP,		//	JA++
	SMOD_UNKNOWN,	//	Something else, somehow.
} jappMods_t;
#endif

extern const char *modNames[];

#define	MAX_CHATBOX_ITEMS		5
typedef struct chatBoxItem_s
{
	char	string[MAX_SAY_TEXT+12];
	int		time;
	int		lines;
} chatBoxItem_t;

//Raz: Multiple views
typedef enum refdefView_e {
	REFDEF_DEFAULT=0,
	REFDEF_AUTOMAP,
	REFDEF_ALTVIEW,
	REFDEF_MAX
} refdefViews_t;

typedef struct {
	int			clientFrame;		// incremented each frame

	int			clientNum;
	
	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	int			deferredPlayerLoading;
	qboolean	loading;			// don't defer players at initial startup
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
//	snapshot_t	activeSnapshots[2];

	float		frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean	mMapChange;

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime

	int			time;			// this is the time value that the client
								// is rendering at.
	int			oldTime;		// time at last frame, used for missile trails and prediction checking

	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time

	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;

	qboolean	mapRestart;			// set on a map restart to set back the weapon

	qboolean	mInRMG; //rwwRMG - added
	qboolean	mRMGWeather; //rwwRMG - added

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	playerState_t	predictedVehicleState;
	
	//centity_t		predictedPlayerEntity;
	//rww - I removed this and made it use cg_entities[clnum] directly.

	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vector3		predictedError;

	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	int			weaponSelect;

	int			forceSelect;
	int			itemSelect;

	// auto rotating items
	vector3		autoAngles;
	vector3		autoAxis[3];
	vector3		autoAnglesFast;
	vector3		autoAxisFast[3];

	// view rendering
	refdef_t		refdef[REFDEF_MAX];
	refdefViews_t	currentRefdef; //Raz: Added

	// zoom key
	qboolean	zoomed;
	int			zoomTime;
	float		zoomSensitivity;

	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int			scoresRequestTime;
	int			numScores;
	int			selectedScore;
	int			teamScores[2];
	score_t		scores[MAX_CLIENTS];
	qboolean	showScores;
	qboolean	scoreBoardShowing;
	int			scoreFadeTime;
	char		killerName[MAX_NETNAME];
	char		spectatorList[MAX_STRING_CHARS];	// list of names
	int			spectatorLen;						// length of list
	float		spectatorWidth;						// width in device units
	int			spectatorTime;						// next time to offset
	int			spectatorPaintX;					// current paint x
	int			spectatorPaintX2;					// current paint x
	int			spectatorOffset;					// current offset from start
	int			spectatorPaintLen; 					// current offset from start

	// skull trails
	skulltrail_t	skulltrails[MAX_CLIENTS];

	// centerprinting
	int			centerPrintTime;
	int			centerPrintCharWidth;
	int			centerPrintY;
	char		centerPrint[1024];
	int			centerPrintLines;

	// low ammo warning state
	int			lowAmmoWarning;		// 1 = low, 2 = empty

	// kill timers for carnage reward
	int			lastKillTime;

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	int			crosshairVehNum;
	int			crosshairVehTime;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	int			attackerTime;
	int			voiceTime;

	// reward medals
	int			rewardStack;
	int			rewardTime;
	int			rewardCount[MAX_REWARDSTACK];
	qhandle_t	rewardShader[MAX_REWARDSTACK];
	qhandle_t	rewardSound[MAX_REWARDSTACK];

	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];

	// for voice chat buffer
	int			voiceChatTime;
	int			voiceChatBufferIn;
	int			voiceChatBufferOut;

	// warmup countdown
	int			warmup;
	int			warmupCount;

	//==========================

	int			itemPickup;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	// blend blobs
	float		damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	vector3		kick_angles;	// weapon kicks
	int			kick_time;
	vector3		kick_origin;

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
	int     nextOrbitTime;

	//qboolean cameraMode;		// if rendering from a loaded camera
	int			loadLCARSStage;

	int			forceHUDTotalFlashTime;
	int			forceHUDNextFlashTime;
	qboolean	forceHUDActive;				// Flag to show force hud is off/on

	// development tool
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;

	int			VHUDFlashTime;
	qboolean	VHUDTurboFlag;

	// HUD stuff
	float			HUDTickFlashTime;
	qboolean		HUDArmorFlag;
	qboolean		HUDHealthFlag;
	qboolean		iconHUDActive;
	float			iconHUDPercent;
	float			iconSelectTime;
	float			invenSelectTime;
	float			forceSelectTime;

	vector3			lastFPFlashPoint;

/*
Ghoul2 Insert Start
*/
	int				testModel;
	// had to be moved so we wouldn't wipe these out with the memset - these have STL in them and shouldn't be cleared that way
	snapshot_t	activeSnapshots[2];
/*
Ghoul2 Insert End
*/

	char				sharedBuffer[MAX_CG_SHARED_BUFFER_SIZE];

	short				radarEntityCount;
	short				radarEntities[MAX_CLIENTS+16];

	short				bracketedEntityCount;
	short				bracketedEntities[MAX_CLIENTS+16];

	float				distanceCull;

	chatBoxItem_t		chatItems[MAX_CHATBOX_ITEMS];
	int					chatItemActive;

	//Raz: Added
	struct japp
	{
		unsigned int		SSF;

		int					fps;		//	FPS for stats HUD

		qboolean			timestamp24Hour;

		qboolean			isGhosted; //amghost

		//Smod: strafe helper
		refEntity_t			velocityVect;
		refEntity_t			leftIdeal;
		refEntity_t			rightIdeal;
		qboolean			isfixedVector;
		vector3				fixedVector;

		qboolean			fakeGun;
	} japp;

	struct log {
		fileHandle_t	console;
		fileHandle_t	chat;
		fileHandle_t	security;
	} log;

	struct scoreboard {
		char	spectatorList[MAX_STRING_CHARS];
		int		spectatorLen;
		float	spectatorX;
		float	spectatorWidth;
		int		spectatorResetTime;
	} scoreboard;

	struct {
		ivector2	pos;
		struct {
			number		scale;
			integer		width;
		} size;
		vector4		background;
	} chatbox;

	struct {
		ivector4	rgba;
		qboolean	forceAlpha;
	} duelColour;

	struct {
		vector3		amount;
		number		speed;
	} gunIdleDrift;

	struct {
		ivector4	rgba;
		qboolean	forceAlpha;
	} shieldColour;

	struct {
		number		pitch, roll, up;
		qboolean	fall;
	} viewBob;

	struct {
		ivector2 position, size;
	} accelerometer;

	ivector2	lagometerPos;
	ivector2	moveKeysPos;
	ivector2	statsPos;
	ivector2	strafeTrailWeights;
	ivector3	allyColour;
	ivector3	enemyColour;
	ivector4	bubbleColour;
	ivector4	strafeHelperColour;

	vector3		automapAngle;
	vector3		gunAlign;
	vector3		gunBob;
} cg_t;

#define MAX_TICS	14

typedef struct forceTicPos_s
{
	int				x;
	int				y;
	int				width;
	int				height;
	char			*file;
	qhandle_t		tic;
} forceTicPos_t;
extern forceTicPos_t forceTicPos[];
extern forceTicPos_t ammoTicPos[];

typedef struct cgscreffects_s
{
	float		FOV;
	float		FOV2;

	float		shake_intensity;
	int			shake_duration;
	int			shake_start;

	float		music_volume_multiplier;
	int			music_volume_time;
	qboolean	music_volume_set;
} cgscreffects_t;

extern cgscreffects_t cgScreenEffects;

void CGCam_Shake( float intensity, int duration );
void CGCam_SetMusicMult( float multiplier, int duration );

typedef enum 
{
	CHUNK_METAL1 = 0,
	CHUNK_METAL2,
	CHUNK_ROCK1,
	CHUNK_ROCK2,
	CHUNK_ROCK3,
	CHUNK_CRATE1,
	CHUNK_CRATE2,
	CHUNK_WHITE_METAL,
	NUM_CHUNK_TYPES
} chunkModels_t;
#define NUM_CHUNK_MODELS	4

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t	charsetShader;
	qhandle_t	whiteShader;

	qhandle_t	loadBarLED;
	qhandle_t	loadBarLEDCap;
	qhandle_t	loadBarLEDSurround;

	qhandle_t	bryarFrontFlash;
	qhandle_t	greenFrontFlash;
	qhandle_t	lightningFlash;

	qhandle_t	itemHoloModel;
	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;

	qhandle_t	radarShader;
	qhandle_t	siegeItemShader;
	qhandle_t	mAutomapPlayerIcon;
	qhandle_t	mAutomapRocketIcon;

	qhandle_t	wireframeAutomapFrame_left;
	qhandle_t	wireframeAutomapFrame_right;
	qhandle_t	wireframeAutomapFrame_top;
	qhandle_t	wireframeAutomapFrame_bottom;

//Chunks
	qhandle_t	chunkModels[NUM_CHUNK_TYPES][4];
	sfxHandle_t	chunkSound;
	sfxHandle_t	grateSound;
	sfxHandle_t	rockBreakSound;
	sfxHandle_t	rockBounceSound[2];
	sfxHandle_t	metalBounceSound[2];
	sfxHandle_t	glassChunkSound;
	sfxHandle_t	crateBreakSound[2];

	qhandle_t	hackerIconShader;

	// Saber shaders
	//-----------------------------
	qhandle_t	forceCoronaShader;

	qhandle_t	redSaberGlowShader;
	qhandle_t	redSaberCoreShader;
	qhandle_t	orangeSaberGlowShader;
	qhandle_t	orangeSaberCoreShader;
	qhandle_t	yellowSaberGlowShader;
	qhandle_t	yellowSaberCoreShader;
	qhandle_t	greenSaberGlowShader;
	qhandle_t	greenSaberCoreShader;
	qhandle_t	blueSaberGlowShader;
	qhandle_t	blueSaberCoreShader;
	qhandle_t	purpleSaberGlowShader;
	qhandle_t	purpleSaberCoreShader;
	//[RGBSabers]
	qhandle_t	rgbSaberGlowShader;
	qhandle_t	rgbSaberCoreShader;

	qhandle_t	rgbSaberGlow2Shader;
	qhandle_t	rgbSaberCore2Shader;
	qhandle_t	rgbSaberTrail2Shader;

	qhandle_t	rgbSaberGlow3Shader;
	qhandle_t	rgbSaberCore3Shader;
	qhandle_t	rgbSaberTrail3Shader;

	qhandle_t	rgbSaberGlow4Shader;
	qhandle_t	rgbSaberCore4Shader;
	qhandle_t	rgbSaberTrail4Shader;

	qhandle_t	rgbSaberGlow5Shader;
	qhandle_t	rgbSaberCore5Shader;
	qhandle_t	rgbSaberTrail5Shader;

	qhandle_t	blackSaberGlowShader;
	qhandle_t	blackSaberCoreShader;
	qhandle_t	blackBlurShader;
	//[/RGBSabers]
	//[Movie Sabers]
#if 0
	//Original Trilogy Sabers
	qhandle_t otSaberCoreShader;			
	qhandle_t redOTGlowShader;				
	qhandle_t orangeOTGlowShader;			
	qhandle_t yellowOTGlowShader;			
	qhandle_t greenOTGlowShader;			
	qhandle_t blueOTGlowShader;			
	qhandle_t purpleOTGlowShader;			

	//Episode I Sabers
	qhandle_t ep1SaberCoreShader;
	qhandle_t redEp1GlowShader;			
	qhandle_t orangeEp1GlowShader;			
	qhandle_t yellowEp1GlowShader;			
	qhandle_t greenEp1GlowShader;			
	qhandle_t blueEp1GlowShader;			
	qhandle_t purpleEp1GlowShader;

	//Episode II Sabers
	qhandle_t ep2SaberCoreShader;
	qhandle_t whiteIgniteFlare;
	qhandle_t blackIgniteFlare;
	qhandle_t redEp2GlowShader;			
	qhandle_t orangeEp2GlowShader;			
	qhandle_t yellowEp2GlowShader;			
	qhandle_t greenEp2GlowShader;			
	qhandle_t blueEp2GlowShader;			
	qhandle_t purpleEp2GlowShader;

	//Episode III Sabers
	qhandle_t ep3SaberCoreShader;
	qhandle_t whiteIgniteFlare02;
	qhandle_t blackIgniteFlare02;
	qhandle_t redIgniteFlare;
	qhandle_t greenIgniteFlare;
	qhandle_t purpleIgniteFlare;
	qhandle_t blueIgniteFlare;
	qhandle_t orangeIgniteFlare;
	qhandle_t yellowIgniteFlare;
	qhandle_t redEp3GlowShader;			
	qhandle_t orangeEp3GlowShader;			
	qhandle_t yellowEp3GlowShader;			
	qhandle_t greenEp3GlowShader;			
	qhandle_t blueEp3GlowShader;			
	qhandle_t purpleEp3GlowShader;			
#endif
	//[Movie Sabers]
	//[SFXSabers]
	qhandle_t sfxSaberTrailShader;
	qhandle_t sfxSaberBladeShader;
	qhandle_t sfxSaberBlade2Shader;
	qhandle_t sfxSaberEndShader;
	qhandle_t sfxSaberEnd2Shader;
	//[/SFXSabers]

	qhandle_t	saberBlurShader;
	qhandle_t	swordTrailShader;

	qhandle_t	yellowDroppedSaberShader;

	qhandle_t	rivetMarkShader;

	qhandle_t	teamRedShader;
	qhandle_t	teamBlueShader;

	qhandle_t	powerDuelAllyShader;

	qhandle_t	balloonShader;
	qhandle_t	vchatShader;
	qhandle_t	connectionShader;

	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;

	qhandle_t	numberShaders[11];
	qhandle_t	smallnumberShaders[11];
	qhandle_t	chunkyNumberShaders[11];

	qhandle_t	electricBodyShader;
	qhandle_t	electricBody2Shader;

	qhandle_t	fsrMarkShader;
	qhandle_t	fslMarkShader;
	qhandle_t	fshrMarkShader;
	qhandle_t	fshlMarkShader;

	qhandle_t	refractionShader;

	qhandle_t	cloakedShader;

	qhandle_t	boltShader;

	qhandle_t	shadowMarkShader;

	//glass shard shader
	qhandle_t	glassShardShader;

	// wall mark shaders
	qhandle_t	wakeMarkShader;

	// Pain view shader
	qhandle_t	viewPainShader;
	qhandle_t	viewPainShader_Shields;
	qhandle_t	viewPainShader_ShieldsAndHealth;

	qhandle_t	itemRespawningPlaceholder;
	qhandle_t	itemRespawningRezOut;

	qhandle_t	playerShieldDamage;
	qhandle_t	protectShader;
	qhandle_t	forceSightBubble;
	qhandle_t	forceShell;
	qhandle_t	sightShell;

	// Disruptor zoom graphics
	qhandle_t	disruptorMask;
	qhandle_t	disruptorInsert;
	qhandle_t	disruptorLight;
	qhandle_t	disruptorInsertTick;
	qhandle_t	disruptorChargeShader;

	// Binocular graphics
	qhandle_t	binocularCircle;
	qhandle_t	binocularMask;
	qhandle_t	binocularArrow;
	qhandle_t	binocularTri;
	qhandle_t	binocularStatic;
	qhandle_t	binocularOverlay;

	// weapon effect models
	qhandle_t	lightningExplosionModel;

	// explosion assets
	qhandle_t	explosionModel;
	qhandle_t	surfaceExplosionShader;

	qhandle_t	disruptorShader;

	qhandle_t	solidWhite;

	qhandle_t	heartShader;

	// All the player shells
	qhandle_t	ysaliredShader;
	qhandle_t	ysaliblueShader;
	qhandle_t	ysalimariShader;
	qhandle_t	boonShader;
	qhandle_t	endarkenmentShader;
	qhandle_t	enlightenmentShader;
	qhandle_t	invulnerabilityShader;

#ifdef JK2AWARDS
	// medals shown during gameplay
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;
#endif

	// sounds
	sfxHandle_t	selectSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];

	sfxHandle_t	winnerSound;
	sfxHandle_t	loserSound;

	sfxHandle_t crackleSound;

	sfxHandle_t	grenadeBounce1;
	sfxHandle_t	grenadeBounce2;

	sfxHandle_t teamHealSound;
	sfxHandle_t teamRegenSound;

	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

#ifdef JK2AWARDS
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;
	sfxHandle_t defendSound;
	sfxHandle_t holyShitSound;
#endif

	/*
	sfxHandle_t takenLeadSound;
	sfxHandle_t tiedLeadSound;
	sfxHandle_t lostLeadSound;
	*/

	sfxHandle_t rollSound;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t noforceSound;

	sfxHandle_t deploySeeker;
	sfxHandle_t medkitSound;

	// teamplay sounds
#ifdef JK2AWARDS
	sfxHandle_t captureAwardSound;
#endif
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;

	sfxHandle_t redFlagReturnedSound;
	sfxHandle_t blueFlagReturnedSound;
	sfxHandle_t	redTookFlagSound;
	sfxHandle_t blueTookFlagSound;

	sfxHandle_t redYsalReturnedSound;
	sfxHandle_t blueYsalReturnedSound;
	sfxHandle_t	redTookYsalSound;
	sfxHandle_t blueTookYsalSound;

	sfxHandle_t	drainSound;

	//music blips
	sfxHandle_t	happyMusic;
	sfxHandle_t dramaticFailure;

	// tournament sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;

	// new stuff
	qhandle_t patrolShader;
	qhandle_t assaultShader;
	qhandle_t campShader;
	qhandle_t followShader;
	qhandle_t defendShader;
	qhandle_t teamLeaderShader;
	qhandle_t retrieveShader;
	qhandle_t escortShader;
	qhandle_t flagShaders[3];

	qhandle_t halfShieldModel;
	qhandle_t halfShieldShader;

	qhandle_t demp2Shell;
	qhandle_t demp2ShellShader;

	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;

	//weapon icons
	qhandle_t weaponIcons[WP_NUM_WEAPONS];
	qhandle_t weaponIcons_NA[WP_NUM_WEAPONS];

	//holdable inventory item icons
	qhandle_t invenIcons[HI_NUM_HOLDABLE];

	//force power icons
	qhandle_t forcePowerIcons[NUM_FORCE_POWERS];

	qhandle_t rageRecShader;

	//other HUD parts
	int			currentBackground;
	qhandle_t	weaponIconBackground;
	qhandle_t	forceIconBackground;
	qhandle_t	inventoryIconBackground;

	sfxHandle_t	holocronPickup;

	// Zoom
	sfxHandle_t	zoomStart;
	sfxHandle_t	zoomLoop;
	sfxHandle_t	zoomEnd;
	sfxHandle_t	disruptorZoomLoop;

	qhandle_t	bdecal_bodyburn1;
	qhandle_t	bdecal_saberglow;
	qhandle_t	bdecal_burn1;
	qhandle_t	mSaberDamageGlow;

	// For vehicles only now
	sfxHandle_t	noAmmoSound;

	struct {
		qhandle_t	scoreboardLine;
	} japp;
} cgMedia_t;


// Stored FX handles
//--------------------
typedef struct
{
	//concussion
	fxHandle_t	concussionShotEffect;
	fxHandle_t	concussionImpactEffect;

	// BRYAR PISTOL
	fxHandle_t	bryarShotEffect;
	fxHandle_t	bryarPowerupShotEffect;
	fxHandle_t	bryarWallImpactEffect;
	fxHandle_t	bryarWallImpactEffect2;
	fxHandle_t	bryarWallImpactEffect3;
	fxHandle_t	bryarFleshImpactEffect;
	fxHandle_t	bryarDroidImpactEffect;

	// BLASTER
	fxHandle_t  blasterShotEffect;
	fxHandle_t  blasterWallImpactEffect;
	fxHandle_t  blasterFleshImpactEffect;
	fxHandle_t  blasterDroidImpactEffect;

	// DISRUPTOR
	fxHandle_t  disruptorRingsEffect;
	fxHandle_t  disruptorProjectileEffect;
	fxHandle_t  disruptorWallImpactEffect;	
	fxHandle_t  disruptorFleshImpactEffect;	
	fxHandle_t  disruptorAltMissEffect;	
	fxHandle_t  disruptorAltHitEffect;	

	// BOWCASTER
	fxHandle_t	bowcasterShotEffect;
	fxHandle_t	bowcasterImpactEffect;

	// REPEATER
	fxHandle_t  repeaterProjectileEffect;
	fxHandle_t  repeaterAltProjectileEffect;
	fxHandle_t  repeaterWallImpactEffect;	
	fxHandle_t  repeaterFleshImpactEffect;
	fxHandle_t  repeaterAltWallImpactEffect;

	// DEMP2
	fxHandle_t  demp2ProjectileEffect;
	fxHandle_t  demp2WallImpactEffect;
	fxHandle_t  demp2FleshImpactEffect;

	// FLECHETTE
	fxHandle_t	flechetteShotEffect;
	fxHandle_t	flechetteAltShotEffect;
	fxHandle_t	flechetteWallImpactEffect;
	fxHandle_t	flechetteFleshImpactEffect;

	// ROCKET
	fxHandle_t  rocketShotEffect;
	fxHandle_t  rocketExplosionEffect;

	// THERMAL
	fxHandle_t	thermalExplosionEffect;
	fxHandle_t	thermalShockwaveEffect;

	// TRIPMINE
	fxHandle_t	tripmineLaserFX;
	fxHandle_t	tripmineGlowFX;

	//FORCE
	fxHandle_t forceLightning;
	fxHandle_t forceLightningWide;

	fxHandle_t forceDrain;
	fxHandle_t forceDrainWide;
	fxHandle_t forceDrained;

	//TURRET
	fxHandle_t turretShotEffect;

	//Whatever
	fxHandle_t itemCone;

	fxHandle_t	mSparks;
	fxHandle_t	mSaberCut;
	fxHandle_t	mTurretMuzzleFlash;
	fxHandle_t	mSaberBlock;
	fxHandle_t	mSaberBloodSparks;
	fxHandle_t	mSaberBloodSparksSmall;
	fxHandle_t	mSaberBloodSparksMid;
	fxHandle_t	mSpawn;
	fxHandle_t	mJediSpawn;
	fxHandle_t	mBlasterDeflect;
	fxHandle_t	mBlasterSmoke;
	fxHandle_t	mForceConfustionOld;
	fxHandle_t	mDisruptorDeathSmoke;
	fxHandle_t	mSparkExplosion;
	fxHandle_t	mTurretExplode;
	fxHandle_t	mEmplacedExplode;
	fxHandle_t	mEmplacedDeadSmoke;
	fxHandle_t	mTripmineExplosion;
	fxHandle_t	mDetpackExplosion;
	fxHandle_t	mFlechetteAltBlow;
	fxHandle_t	mStunBatonFleshImpact;
	fxHandle_t	mAltDetonate;
	fxHandle_t	mSparksExplodeNoSound;
	fxHandle_t	mTripMineLaster;
	fxHandle_t	mEmplacedMuzzleFlash;
	fxHandle_t	mConcussionAltRing;
	fxHandle_t	mHyperspaceStars;
	fxHandle_t	mBlackSmoke;
	fxHandle_t	mShipDestDestroyed;
	fxHandle_t	mShipDestBurning;
	fxHandle_t	mBobaJet;

	//footstep effects
	fxHandle_t footstepMud;
	fxHandle_t footstepSand;
	fxHandle_t footstepSnow;
	fxHandle_t footstepGravel;
	//landing effects
	fxHandle_t landingMud;
	fxHandle_t landingSand;
	fxHandle_t landingDirt;
	fxHandle_t landingSnow;
	fxHandle_t landingGravel;
	//splashes
	fxHandle_t waterSplash;
	fxHandle_t lavaSplash;
	fxHandle_t acidSplash;

	//Raz: Portals
	fxHandle_t portalBlue;
	fxHandle_t portalOrange;

	fxHandle_t flamethrower;
} cgEffects_t;


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	float			screenXBias;

	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested

	qboolean		localServer;		// detected on startup by checking sv_running

	// parsed from serverinfo
	int				siegeTeamSwitch;
	int				showDuelHealths;
	gametype_t		gametype;
	int				debugMelee;
	int				stepSlideFix;
	int				noSpecMove;
	int				dmflags;
	int				fraglimit;
	int				duel_fraglimit;
	int				capturelimit;
	int				timelimit;
	int				maxclients;
	qboolean		needpass;
	qboolean		jediVmerc;
	int				wDisable;
	int				fDisable;

	char			mapname[MAX_QPATH];
//	char			redTeam[MAX_QPATH];
//	char			blueTeam[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;

	int				scores1, scores2;		// from configstrings
	int				jediMaster;
	int				duelWinner;
	int				duelist1;
	int				duelist2;
	int				duelist3;
// nmckenzie: DUEL_HEALTH.  hmm.
	int				duelist1health;
	int				duelist2health;
	int				duelist3health;

	int				redflag, blueflag;		// flag status from configstrings
	int				flagStatus;

	qboolean  newHud;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];
	fxHandle_t		gameEffects[MAX_FX];
	qhandle_t		gameIcons[MAX_ICONS];

	size_t			numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vector3			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	int cursorX;
	int cursorY;
	void *capturedItem;
	qhandle_t activeCursor;

	// media
	cgMedia_t		media;

	// effects
	cgEffects_t		effects;

	//Raz: serverinfo vars
	struct {
		unsigned int	jp_cinfo;
		char			serverName[MAX_HOSTNAMELENGTH];
		int				overbounce;
	} japp;
} cgs_t;

typedef struct siegeExtended_s
{
	int			health;
	int			maxhealth;
	int			ammo;
	int			weapon;
	int			lastUpdated;
} siegeExtended_t;

//keep an entry available for each client
extern siegeExtended_t cg_siegeExtendedData[MAX_CLIENTS];

//==============================================================================

extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];

extern	centity_t		*cg_permanents[MAX_GENTITIES];
extern	int				cg_numpermanents;

extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	itemInfo_t		cg_items[MAX_ITEMS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];

//Raz: Added
#define XCVAR_PROTO
	#include "cg_xcvar.h"
#undef XCVAR_PROTO

/*
Ghoul2 Insert End
*/

//
// cg_main.c
//
void CG_DrawMiscEnts(void);

const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );
const char *CG_Cvar_VariableString( const char *var_name );

void CG_StartMusic( qboolean bForceStart );

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_LoadMenus(const char *menuFile);
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type);
void CG_RankRunFrame( void );
void CG_SetScoreSelection(void *menu);
void CG_BuildSpectatorString(void);
void CG_NextInventory_f(void);
void CG_PrevInventory_f(void);
void CG_NextForcePower_f(void);
void CG_PrevForcePower_f(void);

//
// cg_view.c
//
void CG_TestModel_f (void);
void CG_TestGun_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);
void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );
void CG_AddBufferedSound( sfxHandle_t sfx);

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
/*
Ghoul2 Insert Start
*/

void CG_TestG2Model_f (void);
void CG_TestModelSurfaceOnOff_f(void);
void CG_ListModelSurfaces_f (void);
void CG_ListModelBones_f (void);
void CG_TestModelSetAnglespre_f(void);
void CG_TestModelSetAnglespost_f(void);
void CG_TestModelAnimate_f(void);
/*
Ghoul2 Insert End
*/

//
// cg_drawtools.c
//
void CG_FillRect( float x, float y, float width, float height, const vector4 *color );
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_DrawRotatePic( float x, float y, float width, float height,float angle, qhandle_t hShader );
void CG_DrawRotatePic2( float x, float y, float width, float height,float angle, qhandle_t hShader );
void CG_DrawString( float x, float y, const char *string, 
				   float charWidth, float charHeight, const float *modulate );

void CG_DrawNumField (int x, int y, int width, int value,int charWidth,int charHeight,int style,qboolean zeroFill);

void CG_DrawStringExt( int x, int y, const char *string, const vector4 *setColor, 
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars );
void CG_DrawBigString( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor( int x, int y, const char *s, vector4 *color );
void CG_DrawSmallString( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringColor( int x, int y, const char *s, vector4 *color );

int CG_DrawStrlen( const char *str );

vector4 *CG_FadeColor( int startMsec, int totalMsec );
qboolean CG_FadeColor2( vector4 *color, int startMsec, int totalMsec );
void CG_LerpColour( const vector4 *start, const vector4 *end, vector4 *out, float point );
vector4 *CG_TeamColor( int team );
void CG_TileClear( void );
void CG_ColorForHealth( vector4 *hcolor );
void CG_GetColorForHealth( int health, int armor, vector4 *hcolor );

void UI_DrawProportionalString( int x, int y, const char* str, int style, vector4 *color );
void UI_DrawScaledProportionalString( int x, int y, const char* str, int style, vector4 *color, float scale);
void CG_DrawRect( float x, float y, float width, float height, float size, const vector4 *color );
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);

//
// cg_draw.c, cg_newDraw.c
//
extern	int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers;
extern  char systemChat[256];

void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vector3 *headAngles );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team );
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vector4 *color, qhandle_t shader, int textStyle,int font);
void CG_Text_Paint(float x, float y, float scale, vector4 *color, const char *text, float adjust, int limit, int style, int iMenuFont);
float CG_Text_Width(const char *text, float scale, int iMenuFont);
float CG_Text_Height(const char *text, float scale, int iMenuFont);
float CG_GetValue(int ownerDraw);
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(char **args);
qboolean CG_DeferMenuScript(char **args);
void CG_ShowResponseHead(void);
void CG_GetTeamColor(vector4 *color);
const char *CG_GetGameStatusText(void);
const char *CG_GetKillerText(void);
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, void *ghoul2, int g2radius, qhandle_t skin, vector3 *origin, vector3 *angles );
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
qboolean CG_YourTeamHasFlag(void);
qboolean CG_OtherTeamHasFlag(void);
qboolean CG_YourTeamDroppedFlag(void);
qboolean CG_OtherTeamDroppedFlag(void);
qhandle_t CG_StatusHandle(int task);
int MenuFontToHandle(int iMenuFont);


//
// cg_player.c
//
qboolean CG_RagDoll(centity_t *cent, vector3 *forcedAngles);
qboolean CG_G2TraceCollide(trace_t *tr, vector3* const mins, vector3* const maxs, vector3 *lastValidStart, vector3 *lastValidEnd);
void CG_AddGhoul2Mark(int shader, float size, vector3 *start, vector3 *end, int entnum,
					  vector3 *entposition, float entangle, void *ghoul2, vector3 *scale, int lifeTime);

void CG_CreateNPCClient(clientInfo_t **ci);
void CG_DestroyNPCClient(clientInfo_t **ci);

void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team );
void CG_NewClientInfo( int clientNum, qboolean entitiesInitialized );
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );
void CG_PlayerShieldHit(int entitynum, vector3 *angles, int amount);
void CG_TriggerAnimSounds( centity_t *cent );


//
// cg_predict.c
//
void CG_BuildSolidList( void );
int	CG_PointContents( const vector3 *point, int passEntityNum );
void CG_Trace( trace_t *result, const vector3 *start, const vector3 *mins, const vector3 *maxs, const vector3 *end, 
					 int skipNumber, int mask );
void CG_G2Trace( trace_t *result, const vector3 *start, const vector3 *mins, const vector3 *maxs, const vector3 *end, 
					 int skipNumber, int mask );
void CG_PredictPlayerState( void );
void CG_LoadDeferredPlayers( void );


//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char	*CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vector3 *position );
void CG_PainEvent( centity_t *cent, int health );
void CG_ReattachLimb(centity_t *source);


//
// cg_ents.c
//

void CG_S_AddLoopingSound(int entityNum, const vector3 *origin, const vector3 *velocity, sfxHandle_t sfx);
void CG_S_AddRealLoopingSound(int entityNum, const vector3 *origin, const vector3 *velocity, sfxHandle_t sfx);
void CG_S_StopLoopingSound(int entityNum, sfxHandle_t sfx);
void CG_S_UpdateLoopingSounds(int entityNum);

void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( qboolean isPortal );
void CG_ManualEntityRender(centity_t *cent);
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover( const vector3 *in, int moverNum, int fromTime, int toTime, vector3 *out );

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName );

/*
Ghoul2 Insert Start
*/
void ScaleModelAxis(refEntity_t	*ent);
/*
Ghoul2 Insert End
*/

//
// cg_turret.c
//
void TurretClientRun(centity_t *ent);

//
// cg_weapons.c
//
void CG_GetClientWeaponMuzzleBoltPoint(int clIndex, vector3 *to);

void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );
void CG_WeaponClean_f( void );

void CG_RegisterWeapon( int weaponNum);
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent, qboolean alt_fire );
void CG_MissileHitWall(int weapon, int clientNum, vector3 *origin, vector3 *dir, impactSound_t soundType, qboolean alt_fire, int charge);
void CG_MissileHitPlayer( int weapon, vector3 *origin, vector3 *dir, int entityNum, qboolean alt_fire);

void CG_AddViewWeapon (playerState_t *ps);
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, vector3 *newAngles, qboolean thirdPerson );
void CG_DrawWeaponSelect( void );
void CG_DrawIconBackground(void);

void CG_OutOfAmmoChange( int oldWeapon );	// should this be in pmove?

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
void	CG_ImpactMark( qhandle_t markShader, 
				    const vector3 *origin, const vector3 *dir, 
					float orientation, 
				    float r, float g, float b, float a, 
					qboolean alphaFade, 
					float radius, qboolean temporary );

//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//
localEntity_t *CG_SmokePuff( const vector3 *p, 
				   const vector3 *vel, 
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader );
void CG_BubbleTrail( vector3 *start, vector3 *end, float spacing );
void CG_GlassShatter(int entnum, vector3 *dmgPt, vector3 *dmgDir, float dmgRadius, int maxShards);
void CG_ScorePlum( int client, vector3 *org, int score );

void CG_Chunks( int owner, vector3 *origin, const vector3 *normal, const vector3 *mins, const vector3 *maxs, 
						float speed, int numChunks, material_t chunkType, int customChunk, float baseScale );
void CG_MiscModelExplosion( vector3 *mins, vector3 *maxs, int size, material_t chunkType );

void CG_Bleed( vector3 *origin, int entityNum );

localEntity_t *CG_MakeExplosion( vector3 *origin, vector3 *dir, 
								qhandle_t hModel, int numframes, qhandle_t shader, int msec,
								qboolean isSprite, float scale, int flags );// Overloaded in single player

void CG_SurfaceExplosion( vector3 *origin, vector3 *normal, float radius, float shake_speed, qboolean smoke );

void CG_TestLine( vector3 *start, vector3 *end, int time, unsigned int color, int radius);

void CG_InitGlass( void );

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );

//
// cg_info.c
//
void CG_LoadingString( const char *s );
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
void CG_DrawInformation( void );

//
// cg_scoreboard.c
//
qboolean CG_DrawOldScoreboard( void );
void CG_DrawOldTourneyScoreboard( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo( void );
void CG_SetConfigValues( void );
void CG_ShaderStateChanged(void);

//
// cg_playerstate.c
//
int CG_IsMindTricked(int trickIndex1, int trickIndex2, int trickIndex3, int trickIndex4, int client);
void CG_Respawn( void );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );


//
// cg_siege.c
//
void CG_InitSiegeMode(void);
void CG_SiegeRoundOver(centity_t *ent, int won);
void CG_SiegeObjectiveCompleted(centity_t *ent, int won, int objectivenum);


void		BG_CycleInven(playerState_t *ps, int direction);
int			BG_ProperForceIndex(int power);
void		BG_CycleForce(playerState_t *ps, int direction);

const char *CG_GetStringEdString( const char *refSection, const char *refName );

void FX_TurretProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_TurretHitWall( vector3 *origin, vector3 *normal );
void FX_TurretHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );

void FX_ConcussionHitWall( vector3 *origin, vector3 *normal );
void FX_ConcussionHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void FX_ConcussionProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_ConcAltShot( vector3 *start, vector3 *end );

//-----------------------------
// Effects related prototypes
//-----------------------------

// Environmental effects 
void CG_Spark( vector3 *origin, vector3 *dir );

// Weapon prototypes
void FX_BryarHitWall( vector3 *origin, vector3 *normal );
void FX_BryarAltHitWall( vector3 *origin, vector3 *normal, int power );
void FX_BryarHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void FX_BryarAltHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );

void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterWeaponHitWall( vector3 *origin, vector3 *normal );
void FX_BlasterWeaponHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );


void FX_ForceDrained(vector3 *origin, vector3 *dir);


//-----------------------------
// Effects related prototypes
//-----------------------------

// Environmental effects 
void CG_Spark( vector3 *origin, vector3 *dir );

// Weapon prototypes
void FX_BryarHitWall( vector3 *origin, vector3 *normal );
void FX_BryarAltHitWall( vector3 *origin, vector3 *normal, int power );
void FX_BryarHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void FX_BryarAltHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );

void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterWeaponHitWall( vector3 *origin, vector3 *normal );
void FX_BlasterWeaponHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );

void		CG_Init_CG(void);
void		CG_Init_CGents(void);


void CG_SetGhoul2Info( refEntity_t *ent, centity_t *cent);
void CG_CreateBBRefEnts(entityState_t *s1, vector3 *origin );

void CG_InitG2Weapons(void);
void CG_ShutDownG2Weapons(void);
void CG_CopyG2WeaponInstance(centity_t *cent, int weaponNum, void *toGhoul2);
void *CG_G2WeaponInstance(centity_t *cent, int weapon);
void CG_CheckPlayerG2Weapons(playerState_t *ps, centity_t *cent);

void CG_SetSiegeTimerCvar( int msec );

/*
Ghoul2 Insert End
*/


void CG_LoadHudMenu( void );

//[SmartEntities]
qboolean SE_RenderThisEntity( vector3 *testOrigin, int gameEntity );
void SE_R_AddRefEntityToScene( const refEntity_t *re, int gameEntity );
//[/SmartEntities]
void QDECL CG_LogPrintf( fileHandle_t fileHandle, const char *fmt, ... );
void QDECL CG_SecurityLogPrintf( const char *fmt, ... );
void CG_ScoresDown_f( void );
//[TrueView]
void CG_TrueViewInit( void );
void CG_AdjustEyePos( const char *modelName );
//[/TrueView]

qboolean Server_Supports( unsigned int supportFlag );
void HandleTeamBinds( char *buf, int bufsize );

// chatbox stuff
#define MAX_CHATBOX_IDENTIFIER_SIZE (32)
qboolean CG_ContainsChannelEscapeChar( char *text );
char *CG_RemoveChannelEscapeChar( char *text );

// chatbox tabs
void CG_ChatboxSelectTabNext( void );
void CG_ChatboxSelectTabNextNoKeys( void );
void CG_ChatboxSelectTabPrevNoKeys( void );
void CG_ChatboxSelect( char *cbName );

// chatbox history
void CG_ChatboxHistoryUp( void );
void CG_ChatboxHistoryDn( void );

// chatbox events
void CG_MessageModeAll_f( void );
void CG_MessageModeTeam_f( void );
void CG_ChatboxTabComplete( void );
void CG_ChatboxScroll( int direction );
void CG_ChatboxChar( int key );
void CG_ChatboxOutgoing( void );
void CG_ChatboxClear( void );

// chatbox management
void CG_ChatboxInit( void );
qboolean CG_ChatboxActive( void );
void CG_ChatboxDraw( void );
void CG_ChatboxEscape( void );
void CG_ChatboxAddMessage( const char *message, qboolean multiLine, char *cbName );


refdef_t *CG_GetRefdef( void );
qboolean CG_WorldCoordToScreenCoordFloat( vector3 *worldCoord, float *x, float *y );
qboolean CG_WorldCoordToScreenCoordFloat_Fixed( vector3 *point, float *x, float *y );

void CG_RailTrail( clientInfo_t *ci, vector3 *start, vector3 *end );
#define NEWFX_DISINT		0x0001
#define NEWFX_RUPTOR		0x0002
#define NEWFX_REPEATER_ALT	0x0004
#define NEWFX_SIMPLEFLAG	0x0008
#define NEWFX_TRANSFLAG		0x0010

#define DRAWTIMER_ENABLE	0x0001
#define DRAWTIMER_COUNTDOWN	0x0002
#define DRAWTIMER_COLOUR	0x0004

extern cgameImport_t *trap;
