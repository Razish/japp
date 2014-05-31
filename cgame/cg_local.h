#pragma once

// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "qcommon/q_shared.h"
#include "tr_types.h"
#include "bg_public.h"
#include "cg_public.h"

//Raz: Added
#define XCVAR_PROTO
#include "cg_xcvar.h"
#undef XCVAR_PROTO

// the entire cgame module is unloaded and reloaded on each level change, so there is NO persistant data between levels
//	on the client side.
// if you absolutely need something stored, it can either be kept by the server in the server stored userinfos, or
//	stashed in a cvar.

#define	POWERUP_BLINKS					(5)
#define	POWERUP_BLINK_TIME				(1000)
#define	FADE_TIME						(180)  //Raz: was 200
#define	PULSE_TIME						(200)
#define	DAMAGE_DEFLECT_TIME				(100)
#define	DAMAGE_RETURN_TIME				(400)
#define DAMAGE_TIME						(500)
#define	LAND_DEFLECT_TIME				(150)
#define	LAND_RETURN_TIME				(300)
#define	STEP_TIME						(200)
#define	DUCK_TIME						(100)
#define	PAIN_TWITCH_TIME				(200)
#define	WEAPON_SELECT_TIME				(1400)
#define	ITEM_SCALEUP_TIME				(1000)
#define	ZOOM_TIME						(150) // not currently used?
#define MAX_ZOOM_FOV					(3.0f)
#define ZOOM_IN_TIME					(1500.0f)
#define ZOOM_OUT_TIME					(100.0f)
#define ZOOM_START_PERCENT				(0.3f)
#define	ITEM_BLOB_TIME					(200)
#define	MUZZLE_FLASH_TIME				(20)
#define	SINK_TIME						(1000) // time for fragments to sink into ground before going away
#define	ATTACKER_HEAD_TIME				(10000)
#define	REWARD_TIME						(3000)
#define	PULSE_SCALE						(1.5f) // amount to scale up the icons when activating
#define	MAX_STEP_CHANGE					(32)
#define	MAX_VERTS_ON_POLY				(10)
#define	MAX_MARK_POLYS					(256)
#define STAT_MINUS						(10) // num frame for '-' stats digit
#define	ICON_SIZE						(48)
#define	CHAR_WIDTH						(32)
#define	CHAR_HEIGHT						(48)
#define	TEXT_ICON_SPACE					(4)
#define	GIANT_WIDTH						(32)
#define	GIANT_HEIGHT					(48)
#define NUM_FONT_BIG					(1)
#define NUM_FONT_SMALL					(2)
#define NUM_FONT_CHUNKY					(3)
#define	NUM_CROSSHAIRS					(9)
#define TEAM_OVERLAY_MAXNAME_WIDTH		(32)
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	(64)
#define	WAVE_AMPLITUDE					(1)
#define	WAVE_FREQUENCY					(0.4f)
#define	DEFAULT_MODEL					"kyle"
#define DEFAULT_REDTEAM_NAME			"Empire"
#define DEFAULT_BLUETEAM_NAME			"Rebellion"
#define	MAX_CUSTOM_COMBAT_SOUNDS		(40)
#define	MAX_CUSTOM_EXTRA_SOUNDS			(40)
#define	MAX_CUSTOM_JEDI_SOUNDS			(40)
// MAX_CUSTOM_SIEGE_SOUNDS defined in bg_public.h
#define MAX_CUSTOM_DUEL_SOUNDS			(40)
#define	MAX_CUSTOM_SOUNDS				(40) // note that for now these must all be the same, because of the way I am
//	cycling through them and comparing for custom sounds.
#define MAX_CG_LOOPSOUNDS				(8)
#define MAX_REWARDSTACK					(10)
#define MAX_SOUNDBUFFER					(20)
#define MAX_PREDICTED_EVENTS			(16)
#define	MAX_CHATBOX_ITEMS				(5)
#define MAX_TICS						(14)
#define NUM_CHUNK_MODELS				(4)
#define MAX_CHATBOX_IDENTIFIER_SIZE		(32)
#define CAMERA_SIZE						(4)

#define NEWFX_DISINT			(0x0001u)
#define NEWFX_RUPTOR			(0x0002u)
#define NEWFX_REPEATER_ALT		(0x0004u)
#define NEWFX_SIMPLEFLAG		(0x0008u)
#define NEWFX_TRANSFLAG			(0x0010u)
#define NEWFX_SFXSABERS			(0X0020u)

#define DRAWTIMER_ENABLE		(0x0001u)
#define DRAWTIMER_COUNTDOWN		(0x0002u)
#define DRAWTIMER_COLOUR		(0x0004u)

#define LEF_PUFF_DONT_SCALE		(0x0001u) // do not scale size over time
#define LEF_TUMBLE				(0x0002u) // tumble over time, used for ejecting shells
#define LEF_FADE_RGB			(0x0004u) // explicitly fade
#define LEF_NO_RANDOM_ROTATE	(0x0008u) // MakeExplosion adds random rotate which could be bad in some cases

#define TURN_ON					(0x0000u)
#define TURN_OFF				(0x0100u)

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

// player entities need to track more information than any other type of entity.
// note that not every player entity is a client entity, because corpses after respawn are outside the normal client
//	numbering range
// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct lerpFrame_s {
	int			oldFrame;
	int			oldFrameTime; // time when ->oldFrame was exactly on
	int			frame;
	int			frameTime; // time when ->frame will be exactly on
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
	int			animationTime; // time when the first frame of the animation will be exact
	float		animationSpeed; // scale the animation speed
	float		animationTorsoSpeed;
	qboolean	torsoYawing;
} lerpFrame_t;

typedef struct playerEntity_s {
	lerpFrame_t	legs, torso, flag;
	int			painTime;
	int			painDirection; // flip from 0 to 1
} playerEntity_t;

// each client has an associated clientInfo_t that contains media references necessary to present the client model and
//	other color coded effects
// this is regenerated each time a client's configstring changes, usually as a result of a userinfo (name, model, etc) change
typedef struct clientInfo_s {
	qboolean	infoValid;
	vector4		colorOverride;
	saberInfo_t	saber[MAX_SABERS];
	void		*ghoul2Weapons[MAX_SABERS];
	char		saberName[64], saber2Name[64];
	char		name[MAX_QPATH];
	team_t		team;
	int			duelTeam;
	int			botSkill; // -1 = not bot, 0-5 = bot
	int			frame;
	vector3		color1, color2;
	int			icolor1, icolor2;
	int			score; // updated by score servercmds
	int			location; // location index for team mode
	int			health, armor, curWeapon; // you only get this info about your teammates
	int			handicap;
	int			wins, losses; // in tourney mode
	int			teamTask; // task in teamplay (offence/defence)
	int			powerups; // so can display quad/flag status
	int			medkitUsageTime;
	int			breathPuffTime;
	char		modelName[MAX_QPATH], skinName[MAX_QPATH];
	char		forcePowers[MAX_QPATH];
	char		teamName[MAX_TEAMNAME];
	int			corrTime;
	vector3		lastHeadAngles;
	int			lookTime;
	int			brokenLimbs;
	qboolean	deferred;
	qboolean	newAnims; // true if using the new mission pack animations
	qboolean	fixedlegs, fixedtorso; // true if legs yaw is always the same as torso yaw, true if torso never changes yaw
	vector3		headOffset; // move head in icon views
	gender_t	gender; // from model
	qhandle_t	legsModel, legsSkin;
	qhandle_t	torsoModel, torsoSkin;
	void		*ghoul2Model;
	qhandle_t	modelIcon;
	qhandle_t	bolt_rhand, bolt_lhand, bolt_head, bolt_motion, bolt_llumbar;
	int			siegeIndex;
	int			siegeDesiredTeam;
	sfxHandle_t	sounds[MAX_CUSTOM_SOUNDS];
	sfxHandle_t	combatSounds[MAX_CUSTOM_COMBAT_SOUNDS];
	sfxHandle_t	extraSounds[MAX_CUSTOM_EXTRA_SOUNDS];
	sfxHandle_t	jediSounds[MAX_CUSTOM_JEDI_SOUNDS];
	sfxHandle_t	siegeSounds[MAX_CUSTOM_SIEGE_SOUNDS];
	sfxHandle_t	duelSounds[MAX_CUSTOM_DUEL_SOUNDS];
	int			legsAnim, torsoAnim;
	float		facial_blink, facial_frown, facial_aux;
	int			superSmoothTime; //do crazy amount of smoothing
	vector3		rgb1, rgb2;
} clientInfo_t;

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
	int				trailTime;		// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;
	vector3			damageAngles;
	int				damageTime;
	int				snapShotTime;	// last time this entity was found in a snapshot
	playerEntity_t	pe;
	vector3			rawAngles;
	vector3			beamEnd;
	vector3			lerpOrigin, lerpAngles; // exact interpolated position of entity on this frame
	vector3			ragLastOrigin;
	int				ragLastOriginTime;
	qboolean		noLumbar; //if true only do anims and things on model_root instead of lower_lumbar, this will be the case for some NPCs.
	qboolean		noFace;
	int				npcLocalSurfOn, npcLocalSurfOff; // for keeping track of the current surface status in relation to the entitystate surface fields.
	int				eventAnimIndex;
	clientInfo_t	*npcClient; //dynamically allocated - always free it, and never stomp over it.
	int				weapon;
	void			*ghoul2weapon; //rww - pointer to ghoul2 instance of the current 3rd person weapon
	float			radius;
	int				boltInfo;
	int				bolt1, bolt2, bolt3, bolt4; // sometimes used as a bolt index, but these values are also used as
	// generic values for clientside entities at times
	float			bodyHeight;
	int				torsoBolt;
	vector3			turAngles;
	vector3			frame_minus1, frame_minus2, frame_minus12, frame_minus22;
	int				frame_minus1_refreshed, frame_minus2_refreshed, frame_minus1_refreshed2, frame_minus2_refreshed2;
	void			*frame_hold; //pointer to a ghoul2 instance
	int				frame_hold_time;
	int				frame_hold_refreshed;
	void			*grip_arm; //pointer to a ghoul2 instance
	int				trickAlpha;
	int				trickAlphaTime;
	int				teamPowerEffectTime;
	int				teamPowerType; //0 regen, 1 heal, 2 drain, 3 absorb
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
	uint32_t		savedSolid;
	int				savedeType;
	vector3			lastOrigin; // for strafe trail
} centity_t;

// local entities are created as a result of events or predicted actions, and live independently from all server
//	transmitted entities
typedef struct markPoly_s {
	struct markPoly_s *prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;

typedef enum leType_e {
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

typedef enum leMarkType_e {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum leBounceSoundType_e {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS,
	LEBS_METAL,
	LEBS_ROCK
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s *prev, *next;
	leType_t			leType;
	uint32_t			leFlags;
	int					startTime, endTime;
	int					fadeInTime;
	float				lifeRate; // 1.0f / (endTime - startTime)
	trajectory_t		pos, angles;
	float				bounceFactor; // 0.0f = no bounce, 1.0f = perfect
	int					bounceSound; // optional sound index to play upon bounce
	float				alpha, dalpha;
	int					forceAlpha;
	float				color[4];
	float				radius;
	float				light;
	vector3				lightColor;
	leMarkType_t		leMarkType; // mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	union {
		struct {
			float radius, dradius;
			vector3 startRGB, dRGB;
		} sprite;
		struct {
			float width, dwidth;
			float length, dlength;
			vector3 startRGB, dRGB;
		} trail;
		struct {
			float width, dwidth;
			// Below are bezier specific.
			vector3 control1, control2; // initial position of control points
			vector3 control1_velocity, control2_velocity; // initial velocity of control points
			vector3 control1_acceleration, control2_acceleration; // constant acceleration of control points
		} line;
		struct {
			float width, dwidth;
			float width2, dwidth2;
			vector3 startRGB, dRGB;
		} line2;
		struct {
			float width, dwidth;
			float width2, dwidth2;
			float height, dheight;
		} cylinder;
		struct {
			float width, dwidth;
		} electricity;
		struct {
			float radius, dradius;
			qboolean( *thinkFn )(struct localEntity_s *le);
			vector3	dir; // magnitude is 1, but this is oldpos-newpos right before the particle is sent to the renderer
		} particle;
		struct {
			qboolean	dontDie;
			vector3		dir;
			float		variance;
			int			delay;
			int			nextthink;
			qboolean( *thinkFn )(struct localEntity_s *le);
			int			data1;
			int			data2;
		} spawner;
		struct {
			float radius;
		} fragment;
	} data;

	refEntity_t		refEntity;
} localEntity_t;

typedef struct score_s {
	int client;
	int score, deaths;
	int ping;
	int time;
	uint32_t scoreFlags;
	int powerUps;
	int accuracy;
	int impressiveCount, excellentCount, gauntletCount, defendCount, assistCount;
	int captures;
	qboolean perfect;
	int team;
} score_t;

// each WP_* weapon enum has an associated weaponInfo_t that contains media references necessary to present the weapon
//	and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	const gitem_t	*item;

	qhandle_t		handsModel, weaponModel, viewModel, barrelModel, flashModel;
	vector3			weaponMidpoint;		// so it will rotate centered instead of by tag
	float			flashDlight;
	vector3			flashDlightColor;
	qhandle_t		weaponIcon, ammoIcon;
	qhandle_t		ammoModel;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose
	sfxHandle_t		firingSound;
	sfxHandle_t		chargeSound;
	fxHandle_t		muzzleEffect;
	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void( *missileTrailFunc )(centity_t *, const struct weaponInfo_s *wi);
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
	void( *altMissileTrailFunc )(centity_t *, const struct weaponInfo_s *wi);
	float			altMissileDlight;
	vector3			altMissileDlightColor;
	int				altMissileRenderfx;
	sfxHandle_t		altMissileHitSound;
	sfxHandle_t		selectSound;
	sfxHandle_t		readySound;
	float			trailRadius;
	float			wiTrailTime;
} weaponInfo_t;

// each IT_* item has an associated itemInfo_t that constains media references necessary to present the item and its
//	effects
typedef struct itemInfo_s {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
	void			*g2Models[MAX_ITEM_MODELS];
	float			radius[MAX_ITEM_MODELS];
} itemInfo_t;

typedef struct powerupInfo_s {
	int itemNum;
} powerupInfo_t;

typedef struct chatBoxItem_s {
	char	string[MAX_SAY_TEXT + 12];
	int		time;
	int		lines;
} chatBoxItem_t;

typedef enum refdefView_e {
	REFDEF_DEFAULT = 0,
	REFDEF_AUTOMAP,
	REFDEF_ALTVIEW,
	REFDEF_SCOREBOARD,
	REFDEF_MAX
} refdefViews_t;

typedef struct cg_s {
	int				clientFrame;		// incremented each frame
	int				clientNum;
	qboolean		demoPlayback;
	qboolean		levelShot;			// taking a level menu screenshot
	int				deferredPlayerLoading;
	qboolean		loading;			// don't defer players at initial startup
	qboolean		intermissionStarted;	// don't play voice rewards, because game will end shortly
	int				latestSnapshotNum;	// the number of snapshots the client system has received
	int				latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet
	snapshot_t		*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t		*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
	float			frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)
	qboolean		mMapChange;
	qboolean		thisFrameTeleport, nextFrameTeleport;
	int				frametime;		// cg.time - cg.oldTime
	int				time;			// this is the time value that the client is rendering at.
	int				oldTime;		// time at last frame, used for missile trails and prediction checking
	int				physicsTime;	// either cg.snap->time or cg.nextSnap->time
	uint32_t		timelimitWarnings, fraglimitWarnings;
	qboolean		mapRestart;			// set on a map restart to set back the weapon
	qboolean		mInRMG; //rwwRMG - added
	qboolean		mRMGWeather; //rwwRMG - added
	qboolean		renderingThirdPerson;		// during deaths, chasecams, etc
	qboolean		hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState, predictedVehicleState;
	qboolean		validPPS;				// clear until the first call to CG_PredictPlayerState
	int				predictedErrorTime;
	vector3			predictedError;
	int				eventSequence;
	int				predictableEvents[MAX_PREDICTED_EVENTS];
	float			stepChange;				// for stair up smoothing
	int				stepTime;
	float			duckChange;				// for duck viewheight smoothing
	int				duckTime;
	float			landChange;				// for landing hard
	int				landTime;
	int				weaponSelect;
	unsigned int	forceSelect;
	int				itemSelect;
	vector3			autoAngles, autoAnglesFast;
	vector3			autoAxis[3], autoAxisFast[3];
	refdef_t		refdef[REFDEF_MAX];
	refdefViews_t	currentRefdef; //Raz: Added
	qboolean		zoomed;
	int				zoomTime;
	float			zoomSensitivity;
	char			infoScreenText[MAX_STRING_CHARS];
	int				scoresRequestTime;
	int				numScores;
	int				selectedScore;
	int				teamScores[2];
	score_t			scores[MAX_CLIENTS];
	qboolean		showScores;
	qboolean		scoreBoardShowing;
	int				scoreFadeTime;
	char			killerName[MAX_NETNAME];
	char			spectatorList[MAX_STRING_CHARS];	// list of names
	int				spectatorLen;						// length of list
	float			spectatorWidth;						// width in device units
	int				spectatorTime;						// next time to offset
	int				spectatorPaintX;					// current paint x
	int				spectatorPaintX2;					// current paint x
	int				spectatorOffset;					// current offset from start
	int				spectatorPaintLen; 					// current offset from start
	int				centerPrintTime;
	int				centerPrintCharWidth;
	int				centerPrintY;
	char			centerPrint[1024];
	int				centerPrintLines;
	int				lowAmmoWarning;		// 1 = low, 2 = empty
	int				lastKillTime;
	int				crosshairClientNum;
	int				crosshairClientTime;
	int				crosshairVehNum;
	int				crosshairVehTime;
	int				powerupActive;
	int				powerupTime;
	int				attackerTime;
	int				voiceTime;
	int				rewardStack;
	int				rewardTime;
	int				rewardCount[MAX_REWARDSTACK];
	qhandle_t		rewardShader[MAX_REWARDSTACK];
	qhandle_t		rewardSound[MAX_REWARDSTACK];
	int				soundBufferIn;
	int				soundBufferOut;
	int				soundTime;
	qhandle_t		soundBuffer[MAX_SOUNDBUFFER];
	int				voiceChatTime;
	int				voiceChatBufferIn;
	int				voiceChatBufferOut;
	int				warmup;
	int				warmupCount;
	int				itemPickup;
	int				itemPickupTime;
	int				itemPickupBlendTime;	// the pulse around the crosshair is timed seperately
	int				weaponSelectTime;
	int				weaponAnimation;
	int				weaponAnimationTime;
	float			damageTime;
	float			damageX, damageY, damageValue;
	float			headYaw;
	float			headEndPitch;
	float			headEndYaw;
	int				headEndTime;
	float			headStartPitch;
	float			headStartYaw;
	int				headStartTime;
	float			v_dmg_time;
	float			v_dmg_pitch;
	float			v_dmg_roll;
	vector3			kick_angles;	// weapon kicks
	int				kick_time;
	vector3			kick_origin;
	float			bobfracsin;
	int				bobcycle;
	float			xyspeed;
	int				nextOrbitTime;
	float			loadFrac;
	int				forceHUDTotalFlashTime;
	int				forceHUDNextFlashTime;
	qboolean		forceHUDActive;				// Flag to show force hud is off/on
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;
	int				VHUDFlashTime;
	qboolean		VHUDTurboFlag;
	float			HUDTickFlashTime;
	qboolean		HUDArmorFlag;
	qboolean		HUDHealthFlag;
	qboolean		iconHUDActive;
	float			iconHUDPercent;
	float			iconSelectTime;
	float			invenSelectTime;
	float			forceSelectTime;
	vector3			lastFPFlashPoint;
	int				testModel;
	snapshot_t		activeSnapshots[2];
	char			sharedBuffer[MAX_CG_SHARED_BUFFER_SIZE];
	short			radarEntityCount;
	short			radarEntities[MAX_CLIENTS + 16];
	short			bracketedEntityCount;
	short			bracketedEntities[MAX_CLIENTS + 16];
	float			distanceCull;
	chatBoxItem_t	chatItems[MAX_CHATBOX_ITEMS];
	int				chatItemActive;

	struct {
		uint32_t		SSF;
		int				fps;		//	FPS for stats HUD
		qboolean		timestamp24Hour;
		qboolean		isGhosted; //amghost

		//Smod: strafe helper
		refEntity_t		velocityVect;
		refEntity_t		leftIdeal, rightIdeal;
		qboolean		isfixedVector;
		vector3			fixedVector;

		qboolean		fakeGun;

		qboolean		trueviewWarning;
	} japp;

	struct {
		fileHandle_t	chat, console, debug, security;
	} log;

	struct {
		char			spectatorList[MAX_STRING_CHARS];
		int				spectatorLen;
		float			spectatorX;
		float			spectatorWidth;
		int				spectatorResetTime;
	} scoreboard;

	struct {
		ivector4		colour;
	} crosshair;

	struct {
		ivector2		pos;
		struct {
			number			scale;
			integer			width;
		} size;
		vector4			background;
	} chatbox;

	struct {
		ivector4		rgba;
		qboolean		forceAlpha;
	} duelColour, shieldColour;

	struct {
		vector3			amount;
		number			speed;
	} gunIdleDrift;

	struct {
		number			pitch, roll, up;
		qboolean		fall;
	} viewBob;

	struct {
		ivector2		position, size;
	} accelerometer;

	ivector2		lagometerPos;
	ivector2		moveKeysPos;
	ivector2		statsPos;
	ivector2		strafeTrailWeights;
	ivector3		allyColour, enemyColour;
	ivector4		bubbleColour;
	ivector4		strafeHelperColour;
	vector3			automapAngle;
	vector3			gunAlign;
	vector3			gunBob;
} cg_t;

typedef struct forceTicPos_s {
	int			x, y;
	int			width, height;
	char		*file;
	qhandle_t	tic;
} forceTicPos_t;

typedef struct cgscreffects_s {
	float		FOV, FOV2;
	float		shake_intensity;
	int			shake_duration;
	int			shake_start;
	float		music_volume_multiplier;
	int			music_volume_time;
	qboolean	music_volume_set;
} cgscreffects_t;

typedef enum chunkModels_e {
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

// The client game static (cgs) structure hold everything loaded or calculated from the gamestate. It will NOT be cleared
//	when a tournament restart is done, allowing all clients to begin playing instantly
typedef struct cgs_s {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale, screenYScale;
	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested
	qboolean		localServer;		// detected on startup by checking sv_running
	int				siegeTeamSwitch;
	int				showDuelHealths;
	gametype_t		gametype;
	int				debugMelee;
	int				stepSlideFix;
	int				noSpecMove;
	uint32_t		dmflags;
	int				fraglimit, duel_fraglimit;
	int				capturelimit;
	int				timelimit;
	int				maxclients;
	qboolean		needpass;
	qboolean		jediVmerc;
	int				wDisable;
	int				fDisable;
	char			mapname[MAX_QPATH], mapnameClean[MAX_QPATH];
	int				voteTime;
	int				voteYes, voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];
	int				teamVoteTime[2];
	int				teamVoteYes[2], teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];
	int				levelStartTime;
	int				scores1, scores2;		// from configstrings
	int				jediMaster;
	int				duelWinner;
	int				duelist1, duelist2, duelist3;
	int				duelist1health, duelist2health, duelist3health;
	int				redflag, blueflag;		// flag status from configstrings
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];
	fxHandle_t		gameEffects[MAX_FX];
	qhandle_t		gameIcons[MAX_ICONS];
	size_t			numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vector3			inlineModelMidpoints[MAX_MODELS];
	clientInfo_t	clientinfo[MAX_CLIENTS];
	int				cursorX, cursorY;
	void			*capturedItem;
	qhandle_t		activeCursor;

	struct {
		uint32_t		jp_cinfo;
		char			serverName[MAX_HOSTNAMELENGTH];
		int				overbounce;
	} japp;
} cgs_t;

typedef struct siegeExtended_s {
	int			health, maxhealth;
	int			ammo;
	int			weapon;
	int			lastUpdated;
} siegeExtended_t;

typedef enum messageMode_e {
	CHAT_ALL = 0,
	CHAT_TEAM,
	CHAT_WHISPER,
} messageMode_t;

extern forceTicPos_t	ammoTicPos[];
extern cg_t				cg;
extern cgs_t			cgs;
extern cgscreffects_t	cgScreenEffects;
extern int				cgSiegeRoundBeganTime;
extern int				cgSiegeRoundState;
extern int				cgSiegeRoundTime;
extern autoMapInput_t	cg_autoMapInput;
extern int				cg_autoMapInputTime;
extern int				cg_beatingSiegeTime;
extern centity_t		cg_entities[MAX_GENTITIES];
extern itemInfo_t		cg_items[MAX_ITEMS];
extern markPoly_t		cg_markPolys[MAX_MARK_POLYS];
extern int				cg_numpermanents;
extern centity_t		*cg_permanents[MAX_GENTITIES];
extern siegeExtended_t	cg_siegeExtendedData[MAX_CLIENTS];
extern int				cg_siegeWinTeam;
extern weaponInfo_t		cg_weapons[MAX_WEAPONS];
extern forceTicPos_t	forceTicPos[];
extern const char		*modNames[];
extern int				numSortedTeamPlayers;
extern int				sortedTeamPlayers[TEAM_MAXOVERLAY];
extern char				systemChat[256];
extern cgameImport_t	*trap;

int				BG_ProperForceIndex( int power );
void			CG_AddBufferedSound( sfxHandle_t sfx );
void			CG_AddGhoul2Mark( int shader, float size, vector3 *start, vector3 *end, int entnum,
	vector3 *entposition, float entangle, void *ghoul2, vector3 *scale,
	int lifeTime );
void			CG_AddLagometerFrameInfo( void );
void			CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void			CG_AddLocalEntities( void );
void			CG_AddMarks( void );
void			CG_AddPacketEntities( qboolean isPortal );
void			CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team,
	vector3 *newAngles, qboolean thirdPerson );
void			CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team );
void			CG_AddViewWeapon( playerState_t *ps );
void			CG_AdjustPositionForMover( const vector3 *in, int moverNum, int fromTime, int toTime, vector3 *out );
void			CG_AdjustEyePos( const char *modelName );
const char *	CG_Argv( int arg );
localEntity_t *	CG_AllocLocalEntity( void );
void			CG_Beam( centity_t *cent );
void			CG_Bleed( vector3 *origin, int entityNum );
void			CG_BubbleTrail( vector3 *start, vector3 *end, float spacing );
void			CG_BuildSolidList( void );
void			CG_BuildSpectatorString( void );
void			CG_CacheG2AnimInfo( char *modelName );
void			CG_CalcEntityLerpPositions( centity_t *cent );
void			CG_CenterPrint( const char *str, int y, int charWidth );
qboolean		CG_ChatboxActive( void );
void			CG_ChatboxAddMessage( const char *message, qboolean multiLine, const char *cbName );
void			CG_ChatboxChar( int key );
void			CG_ChatboxClear( void );
void			CG_ChatboxClose( void );
void			CG_ChatboxDraw( void );
void			CG_ChatboxHistoryDn( void );
void			CG_ChatboxHistoryUp( void );
void			CG_ChatboxInit( void );
void			CG_ChatboxOpen( int mode );
void			CG_ChatboxOutgoing( void );
void			CG_ChatboxScroll( int direction );
void			CG_ChatboxSelect( char *cbName );
void			CG_ChatboxSelectTabNext( void );
void			CG_ChatboxSelectTabNextNoKeys( void );
void			CG_ChatboxSelectTabPrevNoKeys( void );
void			CG_ChatboxTabComplete( void );
void			CG_CheckChangedPredictableEvents( playerState_t *ps );
void			CG_CheckEvents( centity_t *cent );
void			CG_CheckPlayerG2Weapons( playerState_t *ps, centity_t *cent );
void			CG_Chunks( int owner, vector3 *origin, const vector3 *normal, const vector3 *mins,
	const vector3 *maxs, float speed, int numChunks, material_t chunkType,
	int customChunk, float baseScale );
void			CG_CleanJetpackGhoul2( void );
int				CG_CrosshairPlayer( void );
void			CG_ColorForHealth( vector4 *hcolor );
const char *	CG_ConfigString( int index );
qboolean		CG_ConsoleCommand( void );
qboolean		CG_ContainsChannelEscapeChar( char *text );
void			CG_CopyG2WeaponInstance( centity_t *cent, int weaponNum, void *toGhoul2 );
sfxHandle_t		CG_CustomSound( int clientNum, const char *soundName );
float			CG_Cvar_Get( const char *cvar );
const char *	CG_Cvar_VariableString( const char *var_name );
void			CG_CreateBBRefEnts( entityState_t *s1, vector3 *origin );
void			CG_CreateNPCClient( clientInfo_t **ci );
qboolean		CG_DeferMenuScript( char **args );
void			CG_DestroyNPCClient( clientInfo_t **ci );
void			CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, void *ghoul2,
	int g2radius, qhandle_t skin, vector3 *origin, vector3 *angles );
void			CG_DrawActive( void );
void			CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
void			CG_DrawBigString( int x, int y, const char *s, float alpha );
void			CG_DrawBigStringColor( int x, int y, const char *s, const vector4 *color );
void			CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void			CG_DrawHead( float x, float y, float w, float h, int clientNum, vector3 *headAngles );
void			CG_DrawIconBackground( void );
void			CG_DrawInformation( void );
void			CG_DrawMiscEnts( void );
void			CG_DrawNumField( int x, int y, int width, int value, int charWidth, int charHeight,
	int style, qboolean zeroFill );
qboolean		CG_DrawOldScoreboard( void );
void			CG_DrawOldTourneyScoreboard( void );
void			CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void			CG_DrawRect( float x, float y, float width, float height, float size, const vector4 *color );
void			CG_DrawRotatePic( float x, float y, float width, float height, float angle, qhandle_t hShader );
void			CG_DrawRotatePic2( float x, float y, float width, float height, float angle, qhandle_t hShader );
void			CG_DrawSides( float x, float y, float w, float h, float size );
void			CG_DrawSmallString( int x, int y, const char *s, float alpha );
void			CG_DrawSmallStringColor( int x, int y, const char *s, const vector4 *color );
void			CG_DrawString( float x, float y, const char *string, float charWidth, float charHeight,
	const float *modulate );
void			CG_DrawStringExt( int x, int y, const char *string, const vector4 *setColor,
	qboolean forceColor, qboolean shadow, int charWidth, int charHeight,
	int maxChars );
int				CG_DrawStrlen( const char *str );
void			CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team );
void			CG_DrawTopBottom( float x, float y, float w, float h, float size );
void			CG_DrawWeaponSelect( void );
void			CG_EntityEvent( centity_t *cent, vector3 *position );
void			CG_EventHandling( int type );
void			CG_ExecuteNewServerCommands( int latestSequence );
vector4 *		CG_FadeColor( int startMsec, int totalMsec );
qboolean		CG_FadeColor2( vector4 *color, int startMsec, int totalMsec );
void			CG_FillRect( float x, float y, float width, float height, const vector4 *color );
void			CG_FireWeapon( centity_t *cent, qboolean alt_fire );
void			CG_G2Trace( trace_t *result, const vector3 *start, const vector3 *mins,
	const vector3 *maxs, const vector3 *end, int skipNumber, int mask );
qboolean		CG_G2TraceCollide( trace_t *tr, const vector3 *mins, const vector3 *maxs,
	vector3 *lastValidStart, vector3 *lastValidEnd );
void *			CG_G2WeaponInstance( centity_t *cent, int weapon );
uint32_t		CG_GetCameraClip( void );
int				CG_GetClassCount( team_t team, int siegeClass );
void			CG_GetWeaponMuzzleBolt( int clIndex, vector3 *to );
void			CG_GetColorForHealth( int health, int armor, vector4 *hcolor );
const char *	CG_GetGameStatusText( void );
const char *	CG_GetKillerText( void );
const char *	CG_GetLocationString( const char *loc );
refdef_t *		CG_GetRefdef( void );
const char *	CG_GetStringEdString( const char *refSection, const char *refName );
void			CG_GetTeamColor( vector4 *color );
int				CG_GetTeamNonScoreCount( team_t team );
float			CG_GetValue( int ownerDraw );
void			CG_GlassShatter( int entnum, vector3 *dmgPt, vector3 *dmgDir, float dmgRadius, int maxShards );
int				CG_HandleAppendedSkin( char *modelName );
void			CG_ImpactMark( qhandle_t markShader, const vector3 *origin, const vector3 *dir,
	float orientation, float r, float g, float b, float a,
	qboolean alphaFade, float radius, qboolean temporary );
void			CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum,
	qboolean demoPlayback );
void			CG_InitConsoleCommands( void );
void			CG_InitG2Weapons( void );
void			CG_InitGlass( void );
void			CG_InitItems( void );
void			CG_InitJetpackGhoul2( void );
void			CG_InitLocalEntities( void );
void			CG_InitMarkPolys( void );
void			CG_InitSiegeMode( void );
qboolean		CG_IsMindTricked( int trickIndex1, int trickIndex2, int trickIndex3, int trickIndex4,
	int client );
qboolean		CG_IsSpectating( void );
qboolean		CG_IsFollowing( void );
void			CG_KeyEvent( int key, qboolean down );
int				CG_LastAttacker( void );
void			CG_LerpColour( const vector4 *start, const vector4 *end, vector4 *out, float point );
void			CG_ListModelBones_f( void );
void			CG_ListModelSurfaces_f( void );
void			CG_LoadDeferredPlayers( void );
void			CG_LoadHudMenu( void );
void			CG_LoadingString( const char *s );
void			CG_LoadingItem( int itemNum );
void			CG_LoadingClient( int clientNum );
void			CG_LoadMenus( const char *menuFile );
void			CG_LogPrintf( fileHandle_t fileHandle, const char *fmt, ... );
localEntity_t *	CG_MakeExplosion( vector3 *origin, vector3 *dir, qhandle_t hModel, int numframes,
	qhandle_t shader, int msec, qboolean isSprite, float scale,
	uint32_t flags );
void			CG_ManualEntityRender( centity_t *cent );
void			CG_MissileHitPlayer( int weapon, vector3 *origin, vector3 *dir, int entityNum, qboolean alt_fire );
void			CG_MissileHitWall( int weapon, int clientNum, vector3 *origin, vector3 *dir,
	impactSound_t soundType, qboolean alt_fire, int charge );
void			CG_MiscModelExplosion( vector3 *mins, vector3 *maxs, int size, material_t chunkType );
void			CG_MouseEvent( int x, int y );
void			CG_NextForcePower_f( void );
void			CG_NextInventory_f( void );
void			CG_NextWeapon_f( void );
void			CG_NewClientInfo( int clientNum, qboolean entitiesInitialized );
qboolean		CG_OtherTeamDroppedFlag( void );
qboolean		CG_OtherTeamHasFlag( void );
void			CG_OutOfAmmoChange( int oldWeapon ); // should this be in pmove?
void			CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y,
	int ownerDraw, uint32_t ownerDrawFlags, int align, float special,
	float scale, const vector4 *color, qhandle_t shader, int textStyle,
	int font );
qboolean		CG_OwnerDrawVisible( uint32_t flags );
void			CG_PainEvent( centity_t *cent, int health );
void			CG_ParseServerinfo( void );
void			CG_ParseSiegeObjectiveStatus( const char *str );
void			CG_ParseSiegeState( const char *str );
void			CG_ParseWeatherEffect( const char *str );
void			CG_PrecacheNPCSounds( const char *str );
void			CG_PredictPlayerState( void );
void			CG_PrevForcePower_f( void );
void			CG_PrevInventory_f( void );
void			CG_PrevWeapon_f( void );
void			CG_ProcessSnapshots( void );
void			CG_Player( centity_t *cent );
void			CG_PlayerShieldHit( int entitynum, vector3 *angles, int amount );
const char *	CG_PlaceString( int rank );
uint32_t		CG_PointContents( const vector3 *point, int passEntityNum );
void			CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel,
	const char *tagName );
void			CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel,
	const char *tagName );
qboolean		CG_RagDoll( centity_t *cent, vector3 *forcedAngles );
void			CG_RailTrail( clientInfo_t *ci, vector3 *start, vector3 *end );
void			CG_RankRunFrame( void );
void			CG_ReattachLimb( centity_t *source );
void			CG_RegisterItemVisuals( int itemNum );
void			CG_RegisterWeapon( int weaponNum );
char *			CG_RemoveChannelEscapeChar( char *text );
void			CG_ResetPlayerEntity( centity_t *cent );
void			CG_Respawn( void );
void			CG_ROFF_NotetrackCallback( centity_t *cent, const char *notetrack );
void			CG_RunMenuScript( char **args );
void			CG_S_AddLoopingSound( int entityNum, const vector3 *origin, const vector3 *velocity,
	sfxHandle_t sfx );
void			CG_S_AddRealLoopingSound( int entityNum, const vector3 *origin, const vector3 *velocity,
	sfxHandle_t sfx );
void			CG_S_StopLoopingSound( int entityNum, sfxHandle_t sfx );
void			CG_S_UpdateLoopingSounds( int entityNum );
void			CG_ScorePlum( int client, vector3 *org, int score );
void			CG_ScoresDown_f( void );
void			CG_SetConfigValues( void );
void			CG_SetEntitySoundPosition( centity_t *cent );
void			CG_SetGhoul2Info( refEntity_t *ent, centity_t *cent );
void			CG_SetScoreSelection( void *menu );
void			CG_SetSiegeTimerCvar( int msec );
void			CG_ShaderStateChanged( void );
void			CG_ShowResponseHead( void );
void			CG_Shutdown( void );
void			CG_ShutDownG2Weapons( void );
void			CG_SiegeRoundOver( centity_t *ent, int won );
void			CG_SiegeObjectiveCompleted( centity_t *ent, int won, int objectivenum );
localEntity_t *	CG_SmokePuff( const vector3 *p, const vector3 *vel, float radius, float r, float g, float b, float a,
	float duration, int startTime, int fadeInTime, uint32_t leFlags, qhandle_t hShader );
void			CG_Spark( vector3 *origin, vector3 *dir );
void			CG_StartMusic( qboolean bForceStart );
qhandle_t		CG_StatusHandle( int task );
void			CG_SurfaceExplosion( vector3 *origin, vector3 *normal, float radius, float shake_speed, qboolean smoke );
vector4 *		CG_TeamColor( int team );
void			CG_TestModel_f( void );
void			CG_TestGun_f( void );
void			CG_TestG2Model_f( void );
void			CG_TestModelAnimate_f( void );
void			CG_TestModelNextFrame_f( void );
void			CG_TestModelNextSkin_f( void );
void			CG_TestModelPrevFrame_f( void );
void			CG_TestModelPrevSkin_f( void );
void			CG_TestModelSetAnglespost_f( void );
void			CG_TestModelSetAnglespre_f( void );
void			CG_TestModelSurfaceOnOff_f( void );
float			CG_Text_Height( const char *text, float scale, int iMenuFont );
void			CG_Text_Paint( float x, float y, float scale, const vector4 *color, const char *text,
	float adjust, int limit, int style, int iMenuFont );
void			CG_Text_PaintChar( float x, float y, float width, float height, float scale, float s,
	float t, float s2, float t2, qhandle_t hShader );
float			CG_Text_Width( const char *text, float scale, int iMenuFont );
void			CG_TestLine( vector3 *start, vector3 *end, int time, uint32_t color, int radius );
void			CG_TileClear( void );
void			CG_Trace( trace_t *result, const vector3 *start, const vector3 *mins,
	const vector3 *maxs, const vector3 *end, int skipNumber, int mask );
void			CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void			CG_TriggerAnimSounds( centity_t *cent );
void			CG_TrueView( centity_t *cent );
void			CG_TrueViewInit( void );
void			CG_UpdateCvars( void );
void			CG_Weapon_f( void );
void			CG_WeaponClean_f( void );
qboolean		CG_WorldCoordToScreenCoordFloat( const vector3 *point, float *x, float *y );
qboolean		CG_YourTeamDroppedFlag( void );
qboolean		CG_YourTeamHasFlag( void );
void			CG_ZoomDown_f( void );
void			CG_ZoomUp_f( void );
void			CGCam_SetMusicMult( float multiplier, int duration );
void			CGCam_Shake( float intensity, int duration );
char *			ConcatArgs( int start );
void			FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );
void			FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void			FX_BlasterWeaponHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void			FX_BlasterWeaponHitWall( vector3 *origin, vector3 *normal );
void			FX_BryarAltHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void			FX_BryarAltHitWall( vector3 *origin, vector3 *normal, int power );
void			FX_BryarHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void			FX_BryarHitWall( vector3 *origin, vector3 *normal );
void			FX_ConcAltShot( vector3 *start, vector3 *end );
void			FX_ConcussionHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void			FX_ConcussionHitWall( vector3 *origin, vector3 *normal );
void			FX_ConcussionProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void			FX_ForceDrained( vector3 *origin, vector3 *dir );
void			FX_TurretProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void			FX_TurretHitWall( vector3 *origin, vector3 *normal );
void			FX_TurretHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void			HandleTeamBinds( char *buf, int bufsize );
qhandle_t		MenuFontToHandle( int iMenuFont );
void			ScaleModelAxis( refEntity_t *ent );
void			SE_R_AddRefEntityToScene( const refEntity_t *re, int gameEntity );
qboolean		SE_RenderThisEntity( vector3 *testOrigin, int gameEntity );
qboolean		Server_Supports( uint32_t supportFlag );
void			TurretClientRun( centity_t *ent );
void			UI_DrawProportionalString( int x, int y, const char* str, int style, const vector4 *color );
void			UI_DrawScaledProportionalString( int x, int y, const char* str, int style, const vector4 *color,
	float scale );
