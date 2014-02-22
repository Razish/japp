#pragma once

// all of the model, shader, and sound references that are loaded at gamestate time are stored in cgMedia_t
// other media that can be tied to clients, weapons, or items are stored in the clientInfo_t, itemInfo_t, weaponInfo_t
//	and powerupInfo_t

typedef struct cgMedia_s {
	struct {
		struct {
			sfxHandle_t assist;
			sfxHandle_t capture;
			sfxHandle_t defense;
			sfxHandle_t denied;
			sfxHandle_t excellent;
			sfxHandle_t holyShit;
			sfxHandle_t humiliation;
			sfxHandle_t impressive;
		} awards;

		struct {
			sfxHandle_t chunk;
			sfxHandle_t crackle;
			sfxHandle_t crateBreak[2];
			sfxHandle_t fall;
			sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];
			sfxHandle_t glassChunk;
			sfxHandle_t grate;
			sfxHandle_t land;
			sfxHandle_t metalBounce[2];
			sfxHandle_t respawn;
			sfxHandle_t rockBounce[2];
			sfxHandle_t rockBreak;
			sfxHandle_t roll;
			sfxHandle_t teleIn;
			sfxHandle_t teleOut;
			sfxHandle_t waterIn;
			sfxHandle_t waterOut;
			sfxHandle_t waterUnder;
		} environment;

		struct {
			sfxHandle_t drain;
			sfxHandle_t noforce;
			sfxHandle_t teamHeal;
			sfxHandle_t teamRegen;
		} force;

		struct {
			sfxHandle_t deploySeeker;
			sfxHandle_t holocronPickup;
			sfxHandle_t medkit;
			sfxHandle_t select;
			sfxHandle_t talk;
		} interface;

		struct {
			sfxHandle_t blueFlagReturned;
			sfxHandle_t blueLeads;
			sfxHandle_t blueScored;
			sfxHandle_t blueTookFlag;
			sfxHandle_t blueTookYsal;
			sfxHandle_t blueYsalReturned;
			sfxHandle_t redFlagReturned;
			sfxHandle_t redLeads;
			sfxHandle_t redScored;
			sfxHandle_t redTookFlag;
			sfxHandle_t redTookYsal;
			sfxHandle_t redYsalReturned;
			sfxHandle_t teamsTied;
		} team;

		struct {
			sfxHandle_t count1;
			sfxHandle_t count2;
			sfxHandle_t count3;
			sfxHandle_t countFight;
			sfxHandle_t fiveMinute;
			sfxHandle_t oneFrag;
			sfxHandle_t oneMinute;
			sfxHandle_t threeFrag;
			sfxHandle_t twoFrag;
		} warning;

		struct {
			sfxHandle_t grenadeBounce1;
			sfxHandle_t grenadeBounce2;
			sfxHandle_t	noAmmo;
			sfxHandle_t zoomEnd;
			sfxHandle_t zoomLoop;
			sfxHandle_t zoomStart;
		} weapons;

		sfxHandle_t dramaticFailure;
		sfxHandle_t happyMusic;
		sfxHandle_t loser;
		sfxHandle_t winner;
	} sounds;

	struct {
		struct {
			struct {
				qhandle_t frameBottom;
				qhandle_t frameLeft;
				qhandle_t frameRight;
				qhandle_t frameTop;
				qhandle_t playerIcon;
				qhandle_t rocketIcon;
			} automap;

			struct {
				qhandle_t arrow;
				qhandle_t circle;
				qhandle_t mask;
				qhandle_t overlay;
				qhandle_t staticMask;
				qhandle_t tri;
			} binoculars;

			struct {
				qhandle_t charge;
				qhandle_t insert;
				qhandle_t insertTick;
				qhandle_t light;
				qhandle_t mask;
			} disruptor;

			struct {
				qhandle_t assist;
				qhandle_t capture;
				qhandle_t defend;
				qhandle_t excellent;
				qhandle_t gauntlet;
				qhandle_t impressive;
			} medals;

			struct {
				qhandle_t assault;
				qhandle_t blue;
				qhandle_t camp;
				qhandle_t defend;
				qhandle_t escort;
				qhandle_t flags[3];
				qhandle_t follow;
				qhandle_t patrol;
				qhandle_t red;
				qhandle_t retrieve;
			} team;

			qhandle_t backTile;
			qhandle_t balloon;
			qhandle_t charset;
			qhandle_t chunkyNumbers[11];
			qhandle_t connection;
			qhandle_t crosshairs[NUM_CROSSHAIRS];
			qhandle_t cursor;
			qhandle_t cursorSelect;
			qhandle_t cursorSize;
			qhandle_t defer;
			qhandle_t forceCorona;
			qhandle_t forceIconBackground;
			qhandle_t forcePowerIcons[NUM_FORCE_POWERS];
			qhandle_t hackerIcon;
			qhandle_t heart;
			qhandle_t invenIcons[HI_NUM_HOLDABLE];
			qhandle_t inventoryIconBackground;
			qhandle_t lagometer;
			qhandle_t loadBarLED;
			qhandle_t loadBarLEDCap;
			qhandle_t loadBarLEDSurround;
			qhandle_t numbers[11];
			qhandle_t pain;
			qhandle_t painShields;
			qhandle_t painShieldsAndHealth;
			qhandle_t powerduelAlly;
			qhandle_t radar;
			qhandle_t rageRecovery;
			qhandle_t scoreboardLine;
			qhandle_t siegeItem;
			qhandle_t smallNumbers[11];
			qhandle_t teamStatusBar;
			qhandle_t vchat;
			qhandle_t weaponIconBackground;
			qhandle_t weaponIcons[WP_NUM_WEAPONS];
			qhandle_t weaponIconsInactive[WP_NUM_WEAPONS];
		} interface;

		struct {
			struct {
				struct { qhandle_t heavy, light; } left, right;
			} footsteps;

			struct {
				struct { qhandle_t core, glow; } red, orange, yellow, green, blue, purple, rgb;
				struct { qhandle_t core, glow, trail; } black, rgb2, rgb3, rgb4, rgb5;
				struct { qhandle_t blade, blade2, end, end2, trail; } sfx;

				qhandle_t blur;
				qhandle_t swordTrail;
			} saber;

			qhandle_t bdecal_bodyburn1;
			qhandle_t bdecal_burn1;
			qhandle_t bdecal_saberglow;
			qhandle_t bolt;
			qhandle_t boon;
			qhandle_t bryarFrontFlash;
			qhandle_t cloaked;
			qhandle_t demp2Shell;
			qhandle_t disruptor;
			qhandle_t electricBody;
			qhandle_t electricBody2;
			qhandle_t endarkenment;
			qhandle_t enlightenment;
			qhandle_t forceShell;
			qhandle_t forceSightBubble;
			qhandle_t glassShard;
			qhandle_t greenFrontFlash;
			qhandle_t halfShield;
			qhandle_t invulnerability;
			qhandle_t itemRespawningPlaceholder;
			qhandle_t itemRespawningRezOut;
			qhandle_t lightningFlash;
			qhandle_t playerShieldDamage;
			qhandle_t protect;
			qhandle_t refraction;
			qhandle_t rivetMark;
			qhandle_t saberDamageGlow;
			qhandle_t shadowMark;
			qhandle_t sightShell;
			qhandle_t solidWhite;
			qhandle_t surfaceExplosion;
			qhandle_t wakeMark;
			qhandle_t whiteShader;
			qhandle_t yellowDroppedSaber;
			qhandle_t ysalimari, ysalimariRed, ysalimariBlue;
		} world;
	} gfx;

	struct {
		struct {
			fxHandle_t droidImpact;
			fxHandle_t fleshImpact;
			fxHandle_t shot;
			fxHandle_t wallImpact;
		} blaster;

		struct {
			fxHandle_t impact;
			fxHandle_t shot;
		} bowcaster;

		struct {
			fxHandle_t altRing;
			fxHandle_t impact;
			fxHandle_t shot;
		} concussion;

		struct {
			fxHandle_t altDetonate;
			fxHandle_t fleshImpact;
			fxHandle_t projectile;
			fxHandle_t wallImpact;
		} demp2;

		struct {
			fxHandle_t explosion;
		} detpack;

		struct {
			fxHandle_t altHit;
			fxHandle_t altMiss;
			fxHandle_t fleshImpact;
			fxHandle_t projectile;
			fxHandle_t rings;
			fxHandle_t wallImpact;
		} disruptor;

		struct {
			fxHandle_t altBlow;
			fxHandle_t altShot;
			fxHandle_t fleshImpact;
			fxHandle_t shot;
			fxHandle_t wallImpact;
		} flechette;

		struct {
			fxHandle_t gravel;
			fxHandle_t mud;
			fxHandle_t sand;
			fxHandle_t snow;
		} footstep;

		struct {
			fxHandle_t confusionOld;
			fxHandle_t drain, drained, drainWide;
			fxHandle_t lightning, lightningWide;
		} force;

		struct {
			fxHandle_t dirt;
			fxHandle_t gravel;
			fxHandle_t mud;
			fxHandle_t sand;
			fxHandle_t snow;
		} landing;

		struct {
			fxHandle_t droidImpact;
			fxHandle_t fleshImpact;
			fxHandle_t powerupShot;
			fxHandle_t shot;
			fxHandle_t wallImpact;
			fxHandle_t wallImpact2;
			fxHandle_t wallImpact3;
		} pistol;

		struct {
			fxHandle_t blue;
			fxHandle_t orange;
		} portal;

		struct {
			fxHandle_t altProjectile;
			fxHandle_t altWallImpact;
			fxHandle_t fleshImpact;
			fxHandle_t projectile;
			fxHandle_t wallImpact;
		} repeater;

		struct {
			fxHandle_t explosion;
			fxHandle_t shot;
		} rocket;

		struct {
			fxHandle_t acid;
			fxHandle_t lava;
			fxHandle_t water;
		} splash;

		struct {
			fxHandle_t fleshImpact;
		} stunbaton;

		struct {
			fxHandle_t explosion;
			fxHandle_t shockwave;
		} thermal;

		struct {
			fxHandle_t explosion;
			fxHandle_t glow;
			fxHandle_t laser;
		} tripmine;

		fxHandle_t blackSmoke;
		fxHandle_t blasterDeflect;
		fxHandle_t blasterSmoke;
		fxHandle_t bobaJet;
		fxHandle_t disruptorDeathSmoke;
		fxHandle_t emplacedDeadSmoke;
		fxHandle_t emplacedExplode;
		fxHandle_t emplacedMuzzleFlash;
		fxHandle_t flamethrower;
		fxHandle_t hyperspaceStars;
		fxHandle_t itemCone;
		fxHandle_t jediSpawn;
		fxHandle_t saberBlock;
		fxHandle_t saberBloodSparks;
		fxHandle_t saberBloodSparksMid;
		fxHandle_t saberBloodSparksSmall;
		fxHandle_t saberCut;
		fxHandle_t shipDestBurning;
		fxHandle_t shipDestDestroyed;
		fxHandle_t sparkExplosion;
		fxHandle_t sparks;
		fxHandle_t sparksExplodeNoSound;
		fxHandle_t spawn;
		fxHandle_t turretExplode;
		fxHandle_t turretMuzzleFlash;
		fxHandle_t turretShot;
	} efx;

	struct {
		qhandle_t blueFlag;
		qhandle_t chunks[NUM_CHUNK_TYPES][4];
		qhandle_t demp2Shell;
		qhandle_t explosion;
		qhandle_t forceHolocrons[NUM_FORCE_POWERS];
		qhandle_t halfShield;
		qhandle_t itemHolo;
		qhandle_t redFlag;
	} models;
} cgMedia_t;
extern cgMedia_t media;

void CG_PreloadMedia( void );
void CG_LoadMedia( void );
