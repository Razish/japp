// Copyright (C) 1999-2000 Id Software, Inc.
//
#define MAX_AVOIDGOALS			256
#define MAX_GOALSTACK			8

#define GFL_NONE				(0x0000u)
#define GFL_ITEM				(0x0001u)
#define GFL_ROAM				(0x0002u)
#define GFL_DROPPED				(0x0004u)

//a bot goal
typedef struct bot_goal_s {
	vector3 origin;				//origin of the goal
	int areanum;				//area number of the goal
	vector3 mins, maxs;			//mins and maxs of the goal
	int entitynum;				//number of the goal entity
	int number;					//goal number
	uint32_t flags;					//goal flags
	int iteminfo;				//item information
} bot_goal_t;

//reset the whole goal state, but keep the item weights
void BotResetGoalState( int goalstate );
//reset avoid goals
void BotResetAvoidGoals( int goalstate );
//remove the goal with the given number from the avoid goals
void BotRemoveFromAvoidGoals( int goalstate, int number );
//push a goal onto the goal stack
void BotPushGoal( int goalstate, bot_goal_t *goal );
//pop a goal from the goal stack
void BotPopGoal( int goalstate );
//empty the bot's goal stack
void BotEmptyGoalStack( int goalstate );
//dump the avoid goals
void BotDumpAvoidGoals( int goalstate );
//dump the goal stack
void BotDumpGoalStack( int goalstate );
//get the name name of the goal with the given number
void BotGoalName( int number, char *name, int size );
//get the top goal from the stack
int BotGetTopGoal( int goalstate, bot_goal_t *goal );
//get the second goal on the stack
int BotGetSecondGoal( int goalstate, bot_goal_t *goal );
//choose the best long term goal item for the bot
int BotChooseLTGItem( int goalstate, vector3 *origin, int *inventory, uint32_t travelflags );
//choose the best nearby goal item for the bot
//the item may not be further away from the current bot position than maxtime
//also the travel time from the nearby goal towards the long term goal may not
//be larger than the travel time towards the long term goal from the current bot position
int BotChooseNBGItem( int goalstate, vector3 *origin, int *inventory, uint32_t travelflags,
	bot_goal_t *ltg, float maxtime );
//returns true if the bot touches the goal
int BotTouchingGoal( vector3 *origin, bot_goal_t *goal );
//returns true if the goal should be visible but isn't
int BotItemGoalInVisButNotVisible( int viewer, vector3 *eye, vector3 *viewangles, bot_goal_t *goal );
//search for a goal for the given classname, the index can be used
//as a start point for the search when multiple goals are available with that same classname
int BotGetLevelItemGoal( int index, char *classname, bot_goal_t *goal );
//get the next camp spot in the map
int BotGetNextCampSpotGoal( int num, bot_goal_t *goal );
//get the map location with the given name
int BotGetMapLocationGoal( char *name, bot_goal_t *goal );
//returns the avoid goal time
float BotAvoidGoalTime( int goalstate, int number );
//set the avoid goal time
void BotSetAvoidGoalTime( int goalstate, int number, float avoidtime );
//initializes the items in the level
void BotInitLevelItems( void );
//regularly update dynamic entity items (dropped weapons, flags etc.)
void BotUpdateEntityItems( void );
//interbreed the goal fuzzy logic
void BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child );
//save the goal fuzzy logic to disk
void BotSaveGoalFuzzyLogic( int goalstate, char *filename );
//mutate the goal fuzzy logic
void BotMutateGoalFuzzyLogic( int goalstate, float range );
//loads item weights for the bot
int BotLoadItemWeights( int goalstate, char *filename );
//frees the item weights of the bot
void BotFreeItemWeights( int goalstate );
//returns the handle of a newly allocated goal state
int BotAllocGoalState( int client );
//free the given goal state
void BotFreeGoalState( int handle );
//setup the goal AI
int BotSetupGoalAI( void );
//shut down the goal AI
void BotShutdownGoalAI( void );
