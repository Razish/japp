// Copyright (C) 1999-2000 Id Software, Inc.
//

/*****************************************************************************
 * name:		be_ai_move.h
 *
 * desc:		movement AI
 *
 * $Archive: /source/code/botlib/be_ai_move.h $
 * $Author: osman $
 * $Revision: 1.4 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 2003/03/15 23:44:00 $
 *
 *****************************************************************************/

//movement types
#define MOVE_WALK						(0x0001u)
#define MOVE_CROUCH						(0x0002u)
#define MOVE_JUMP						(0x0004u)
#define MOVE_GRAPPLE					(0x0008u)
#define MOVE_ROCKETJUMP					(0x0010u)
#define MOVE_BFGJUMP					(0x0020u)

//move flags
#define MFL_BARRIERJUMP					(0x0001u) // bot is performing a barrier jump
#define MFL_ONGROUND					(0x0002u) // bot is in the ground
#define MFL_SWIMMING					(0x0004u) // bot is swimming
#define MFL_AGAINSTLADDER				(0x0008u) // bot is against a ladder
#define MFL_WATERJUMP					(0x0010u) // bot is waterjumping
#define MFL_TELEPORTED					(0x0020u) // bot is being teleported
#define MFL_GRAPPLEPULL					(0x0040u) // bot is being pulled by the grapple
#define MFL_ACTIVEGRAPPLE				(0x0080u) // bot is using the grapple hook
#define MFL_GRAPPLERESET				(0x0100u) // bot has reset the grapple
#define MFL_WALK						(0x0200u) // bot should walk slowly

// move result flags
#define MOVERESULT_MOVEMENTVIEW			(0x0001u) // bot uses view for movement
#define MOVERESULT_SWIMVIEW				(0x0002u) // bot uses view for swimming
#define MOVERESULT_WAITING				(0x0004u) // bot is waiting for something
#define MOVERESULT_MOVEMENTVIEWSET		(0x0008u) // bot has set the view in movement code
#define MOVERESULT_MOVEMENTWEAPON		(0x0010u) // bot uses weapon for movement
#define MOVERESULT_ONTOPOFOBSTACLE		(0x0020u) // bot is ontop of obstacle
#define MOVERESULT_ONTOPOF_FUNCBOB		(0x0040u) // bot is ontop of a func_bobbing
#define MOVERESULT_ONTOPOF_ELEVATOR		(0x0080u) // bot is ontop of an elevator (func_plat)
#define MOVERESULT_BLOCKEDBYAVOIDSPOT	(0x0100u) // bot is blocked by an avoid spot

//
#define MAX_AVOIDREACH					1
#define MAX_AVOIDSPOTS					32

// avoid spot types
#define AVOID_CLEAR						(0x0000u) // clear all avoid spots
#define AVOID_ALWAYS					(0x0001u) // avoid always
#define AVOID_DONTBLOCK					(0x0002u) // never totally block

// restult types
#define RESULTTYPE_ELEVATORUP			(0x0001u) // elevator is up
#define RESULTTYPE_WAITFORFUNCBOBBING	(0x0002u) // waiting for func bobbing to arrive
#define RESULTTYPE_BADGRAPPLEPATH		(0x0004u) // grapple path is obstructed
#define RESULTTYPE_INSOLIDAREA			(0x0008u) // stuck in solid area, this is bad

//structure used to initialize the movement state
//the or_moveflags MFL_ONGROUND, MFL_TELEPORTED and MFL_WATERJUMP come from the playerstate
typedef struct bot_initmove_s
{
	vector3 origin;				//origin of the bot
	vector3 velocity;			//velocity of the bot
	vector3 viewoffset;			//view offset
	int entitynum;				//entity number of the bot
	int client;					//client number of the bot
	float thinktime;			//time the bot thinks
	int presencetype;			//presencetype of the bot
	vector3 viewangles;			//view angles of the bot
	uint32_t or_moveflags;			//values ored to the movement flags
} bot_initmove_t;

//NOTE: the ideal_viewangles are only valid if MFL_MOVEMENTVIEW is set
typedef struct bot_moveresult_s
{
	int failure;				//true if movement failed all together
	int type;					//failure or blocked type
	int blocked;				//true if blocked by an entity
	int blockentity;			//entity blocking the bot
	int traveltype;				//last executed travel type
	uint32_t flags;					//result flags
	int weapon;					//weapon used for movement
	vector3 movedir;				//movement direction
	vector3 ideal_viewangles;	//ideal viewangles for the movement
} bot_moveresult_t;

// bk001204: from code/botlib/be_ai_move.c
// TTimo 04/12/2001 was moved here to avoid dup defines
typedef struct bot_avoidspot_s
{
	vector3 origin;
	float radius;
	int type;
} bot_avoidspot_t;

//resets the whole move state
void BotResetMoveState(int movestate);
//moves the bot to the given goal
void BotMoveToGoal(bot_moveresult_t *result, int movestate, bot_goal_t *goal, uint32_t travelflags);
//moves the bot in the specified direction using the specified type of movement
int BotMoveInDirection(int movestate, vector3 *dir, float speed, int type);
//reset avoid reachability
void BotResetAvoidReach(int movestate);
//resets the last avoid reachability
void BotResetLastAvoidReach(int movestate);
//returns a reachability area if the origin is in one
int BotReachabilityArea(vector3 *origin, int client);
//view target based on movement
int BotMovementViewTarget(int movestate, bot_goal_t *goal, uint32_t travelflags, float lookahead, vector3 *target);
//predict the position of a player based on movement towards a goal
int BotPredictVisiblePosition(vector3 *origin, int areanum, bot_goal_t *goal, uint32_t travelflags, vector3 *target);
//returns the handle of a newly allocated movestate
int BotAllocMoveState(void);
//frees the movestate with the given handle
void BotFreeMoveState(int handle);
//initialize movement state before performing any movement
void BotInitMoveState(int handle, bot_initmove_t *initmove);
//add a spot to avoid (if type == AVOID_CLEAR all spots are removed)
void BotAddAvoidSpot(int movestate, vector3 *origin, float radius, int type);
//must be called every map change
void BotSetBrushModelTypes(void);
//setup movement AI
int BotSetupMoveAI(void);
//shutdown movement AI
void BotShutdownMoveAI(void);

