#include <vector>

#include "qcommon/q_shared.h"
#include "game/bg_saga.h"

//#define FORCEJUMP_INSTANTMETHOD 1

#define MAX_CHAT_BUFFER_SIZE 8192
#define MAX_CHAT_LINE_SIZE 128

#define TABLE_BRANCH_DISTANCE 32
#define MAX_NODETABLE_SIZE 16384

#define MAX_LOVED_ONES 4
#define MAX_ATTACHMENT_NAME 64

#define MAX_FORCE_INFO_SIZE 2048

#define WPFLAG_UNUSED00000001 (0x00000001u)
#define WPFLAG_UNUSED00000002 (0x00000002u)
#define WPFLAG_UNUSED00000004 (0x00000004u)
#define WPFLAG_UNUSED00000008 (0x00000008u)
#define WPFLAG_JUMP (0x00000010u) // jump when we hit this
#define WPFLAG_DUCK (0x00000020u) // duck while moving around here
#define WPFLAG_UNUSED00000040 (0x00000040u)
#define WPFLAG_UNUSED00000080 (0x00000080u)
#define WPFLAG_UNUSED00000100 (0x00000100u)
#define WPFLAG_UNUSED00000200 (0x00000200u)
#define WPFLAG_NOVIS (0x00000400u)             // go here for a bit even with no visibility
#define WPFLAG_SNIPEORCAMPSTAND (0x00000800u)  // a good position to snipe or camp - stand
#define WPFLAG_WAITFORFUNC (0x00001000u)       // wait for a func brushent under this point before moving here
#define WPFLAG_SNIPEORCAMP (0x00002000u)       // a good position to snipe or camp - crouch
#define WPFLAG_ONEWAY_FWD (0x00004000u)        // can only go forward on the trial from here (e.g. went over a ledge)
#define WPFLAG_ONEWAY_BACK (0x00008000u)       // can only go backward on the trail from here
#define WPFLAG_GOALPOINT (0x00010000u)         // make it a goal to get here.. goal points will be decided by setting "weight" values
#define WPFLAG_RED_FLAG (0x00020000u)          // red flag
#define WPFLAG_BLUE_FLAG (0x00040000u)         // blue flag
#define WPFLAG_SIEGE_REBELOBJ (0x00080000u)    // rebel siege objective
#define WPFLAG_SIEGE_IMPERIALOBJ (0x00100000u) // imperial siege objective
#define WPFLAG_NOMOVEFUNC (0x00200000u)        // don't move over if a func is under
#define WPFLAG_CALCULATED (0x00400000u)        // don't calculate it again
#define WPFLAG_NEVERONEWAY (0x00800000u)       // never flag it as one-way

#define LEVELFLAG_NOPOINTPREDICTION (0x0001u) // don't take waypoint beyond current into account when adjusting path view angles
#define LEVELFLAG_IGNOREINFALLBACK (0x0002u)  // ignore enemies when in a fallback navigation routine
#define LEVELFLAG_IMUSTNTRUNAWAY (0x0004u)    // don't be scared

#define WP_KEEP_FLAG_DIST 128

#define BWEAPONRANGE_MELEE 1
#define BWEAPONRANGE_MID 2
#define BWEAPONRANGE_LONG 3
#define BWEAPONRANGE_SABER 4

#define MELEE_ATTACK_RANGE 256
#define SABER_ATTACK_RANGE 128
#define MAX_CHICKENWUSS_TIME 10000 // wait 10 secs between checking which run-away path to take

#define BOT_RUN_HEALTH 40
#define BOT_WPTOUCH_DISTANCE 32
#define ENEMY_FORGET_MS 10000
// if our enemy isn't visible within 10000ms (aprx 10sec) then "forget" about him and treat him like every other threat, but still look for
// more immediate threats while main enemy is not visible

#define BOT_PLANT_DISTANCE 256      // plant if within this radius from the last spotted enemy position
#define BOT_PLANT_INTERVAL 15000    // only plant once per 15 seconds at max
#define BOT_PLANT_BLOW_DISTANCE 256 // blow det packs if enemy is within this radius and I am further away than the enemy

#define BOT_MAX_WEAPON_GATHER_TIME 1000 // spend a max of 1 second after spawn issuing orders to gather weapons before attacking enemy base
#define BOT_MAX_WEAPON_CHASE_TIME 15000 // time to spend gathering the weapon before persuing the enemy base (in case it takes longer than expected)

#define BOT_MAX_WEAPON_CHASE_CTF 5000 // time to spend gathering the weapon before persuing the enemy base (in case it takes longer than expected) [ctf-only]

#define BOT_MIN_SIEGE_GOAL_SHOOT 1024
#define BOT_MIN_SIEGE_GOAL_TRAVEL 128

#define BASE_GUARD_DISTANCE 256        // guarding the flag
#define BASE_FLAGWAIT_DISTANCE 256     // has the enemy flag and is waiting in his own base for his flag to be returned
#define BASE_GETENEMYFLAG_DISTANCE 256 // waiting around to get the enemy's flag

#define BOT_FLAG_GET_DISTANCE 256

#define BOT_SABER_THROW_RANGE 800

enum bot_ctf_state_e {
    CTFSTATE_NONE,
    CTFSTATE_ATTACKER,
    CTFSTATE_DEFENDER,
    CTFSTATE_RETRIEVAL,
    CTFSTATE_GUARDCARRIER,
    CTFSTATE_GETFLAGHOME,
    CTFSTATE_MAXCTFSTATES
};

enum bot_siege_state_e { SIEGESTATE_NONE, SIEGESTATE_ATTACKER, SIEGESTATE_DEFENDER, SIEGESTATE_MAXSIEGESTATES };

enum bot_teamplay_state_e { TEAMPLAYSTATE_NONE, TEAMPLAYSTATE_FOLLOWING, TEAMPLAYSTATE_ASSISTING, TEAMPLAYSTATE_REGROUP, TEAMPLAYSTATE_MAXTPSTATES };

typedef struct botattachment_s {
    int level;
    char name[MAX_ATTACHMENT_NAME];
} botattachment_t;

typedef struct nodeobject_s {
    vector3 origin;
    //	int index;
    float weight;
    uint32_t flags;
    int neighbornum;
    int inuse;
} nodeobject_t;

typedef struct boteventtracker_s {
    int eventSequence;
    int events[MAX_PS_EVENTS];
    float eventTime;
} boteventtracker_t;

typedef struct botskills_s {
    int reflex;
    float accuracy;
    float turnspeed;
    float turnspeed_combat;
    float maxturn;
    int perfectaim;
} botskills_t;

// bot state
typedef struct bot_state_s {
    int inuse;                // true if this state is used by a bot client
    int botthink_residual;    // residual for the bot thinks
    int client;               // client number of the bot
    int entitynum;            // entity number of the bot
    playerState_t cur_ps;     // current player state
    usercmd_t lastucmd;       // usercmd from last frame
    bot_settings_t settings;  // several bot settings
    float thinktime;          // time the bot thinks this frame
    vector3 origin;           // origin of the bot
    vector3 velocity;         // velocity of the bot
    vector3 eye;              // eye coordinates of the bot
    int setupcount;           // true when the bot has just been setup
    float ltime;              // local bot time
    float entergame_time;     // time the bot entered the game
    int ms;                   // move state of the bot
    int gs;                   // goal state of the bot
    int ws;                   // weapon state of the bot
    vector3 viewangles;       // current view angles
    vector3 ideal_viewangles; // ideal view angles
    vector3 viewanglespeed;

    // rww - new AI values
    gentity_t *currentEnemy;
    gentity_t *revengeEnemy;

    gentity_t *squadLeader;

    gentity_t *lastHurt;
    gentity_t *lastAttacked;

    gentity_t *wantFlag;

    gentity_t *touchGoal;
    gentity_t *shootGoal;

    gentity_t *dangerousObject;

    vector3 staticFlagSpot;

    int revengeHateLevel;
    int isSquadLeader;

    int squadRegroupInterval;
    int squadCannotLead;

    int lastDeadTime;

    wpobject_t *wpCurrent;
    wpobject_t *wpDestination;
    wpobject_t *wpStoreDest;
    vector3 goalAngles;
    vector3 goalMovedir;
    vector3 goalPosition;

    vector3 lastEnemySpotted;
    vector3 hereWhenSpotted;
    int lastVisibleEnemyIndex;
    int hitSpotted;

    int wpDirection;

    float destinationGrabTime;
    float wpSeenTime;
    float wpTravelTime;
    float wpDestSwitchTime;
    float wpSwitchTime;
    float wpDestIgnoreTime;

    float timeToReact;

    float enemySeenTime;

    float chickenWussCalculationTime;

    float beStill;
    float duckTime;
    float jumpTime;
    float jumpHoldTime;
    float jumpPrep;
    float forceJumping;
    float jDelay;

    float aimOffsetTime;
    float aimOffsetAmtYaw;
    float aimOffsetAmtPitch;

    float frame_Waypoint_Len;
    int frame_Waypoint_Vis;
    float frame_Enemy_Len;
    int frame_Enemy_Vis;

    int isCamper;
    float isCamping;
    wpobject_t *wpCamping;
    wpobject_t *wpCampingTo;
    qboolean campStanding;

    int randomNavTime;
    int randomNav;

    int saberSpecialist;

    int canChat;
    int chatFrequency;
    char currentChat[MAX_CHAT_LINE_SIZE];
    float chatTime;
    float chatTime_stored;
    int doChat;
    int chatTeam;
    gentity_t *chatObject;
    gentity_t *chatAltObject;

    float meleeStrafeTime;
    int meleeStrafeDir;
    float meleeStrafeDisable;

    int altChargeTime;

    float escapeDirTime;

    float dontGoBack;

    int doAttack;
    int doAltAttack;

    int forceWeaponSelect;
    int virtualWeapon;

    int plantTime;
    int plantDecided;
    int plantContinue;
    int plantKillEmAll;

    int runningLikeASissy;
    int runningToEscapeThreat;

    // char				chatBuffer[MAX_CHAT_BUFFER_SIZE];
    // Since we're once again not allocating bot structs dynamically,
    // shoving a 64k chat buffer into one is a bad thing.

    botskills_t skills;

    botattachment_t loved[MAX_LOVED_ONES];
    int lovednum;

    int loved_death_thresh;

    int deathActivitiesDone;

    float botWeaponWeights[WP_NUM_WEAPONS];

    int ctfState;

    int siegeState;

    int teamplayState;

    int jmState;

    int state_Forced; // set by player ordering menu

    int saberDefending;
    int saberDefendDecideTime;
    int saberBFTime;
    int saberBTime;
    int saberSTime;
    int saberThrowTime;

    qboolean saberPower;
    int saberPowerTime;

    int botChallengingTime;

    char forceinfo[MAX_FORCE_INFO_SIZE];

#ifndef FORCEJUMP_INSTANTMETHOD
    int forceJumpChargeTime;
#endif

    int doForcePush;

    int noUseTime;
    qboolean doingFallback;

    int iHaveNoIdeaWhereIAmGoing;
    vector3 lastSignificantAreaChange;
    int lastSignificantChangeTime;

    int forceMove_Forward;
    int forceMove_Right;
    int forceMove_Up;
    // end rww
} bot_state_t;

void *B_TempAlloc(int size);
void B_TempFree(int size);

void *B_AllocZ(int size);
void B_Free(void *ptr);

// resets the whole bot state
void BotResetState(bot_state_t *bs);
// returns the number of bots in the game
int NumBots(void);

void BotUtilizePersonality(bot_state_t *bs);
int BotDoChat(bot_state_t *bs, const char *section, int always);
void StandardBotAI(bot_state_t *bs, float thinktime);
void BotWaypointRender(void);
int OrgVisibleBox(vector3 *org1, vector3 *mins, vector3 *maxs, vector3 *org2, int ignore);
int BotIsAChickenWuss(bot_state_t *bs);
int GetNearestVisibleWP(vector3 *org, int ignore);
int GetBestIdleGoal(bot_state_t *bs);

extern vmCvar_t bot_forcepowers;
extern vmCvar_t bot_forgimmick;
extern vmCvar_t bot_honorableduelacceptance;
#ifdef _DEBUG
extern vmCvar_t bot_nogoals;
extern vmCvar_t bot_debugmessages;
#endif

extern vmCvar_t bot_attachments;
extern vmCvar_t bot_camp;

extern vmCvar_t bot_wp_info;
extern vmCvar_t bot_wp_edit;
extern vmCvar_t bot_wp_clearweight;
extern vmCvar_t bot_wp_distconnect;
extern vmCvar_t bot_wp_visconnect;

extern wpobject_t *flagRed;
extern wpobject_t *oFlagRed;
extern wpobject_t *flagBlue;
extern wpobject_t *oFlagBlue;

extern gentity_t *eFlagRed;
extern gentity_t *eFlagBlue;

extern char gBotChatBuffer[MAX_CLIENTS][MAX_CHAT_BUFFER_SIZE];
extern float gWPRenderTime;
extern float gDeactivated;
extern float gBotEdit;
extern int gWPRenderedFrame;

extern std::vector<wpobject_t *> gWPArray;

extern int gLastPrintedIndex;
extern nodeobject_t nodetable[MAX_NODETABLE_SIZE];
extern int nodenum;

extern uint32_t gLevelFlags;

extern float floattime;
#define FloatTime() floattime
