// make sure this is the same character as we use in chats in g_cmd.c
#define EC	"\x19"

//match template contexts
#define MTCONTEXT_UNUSED0001			(0x0001u)
#define MTCONTEXT_MISC					(0x0002u)
#define MTCONTEXT_INITIALTEAMCHAT		(0x0004u)
#define MTCONTEXT_TIME					(0x0008u)
#define MTCONTEXT_TEAMMATE				(0x0010u)
#define MTCONTEXT_ADDRESSEE				(0x0020u)
#define MTCONTEXT_PATROLKEYAREA			(0x0040u)
#define MTCONTEXT_REPLYCHAT				(0x0080u)
#define MTCONTEXT_CTF					(0x0100u)

//message types
#define MSG_NEWLEADER					1		//new leader
#define MSG_ENTERGAME					2		//enter game message
#define MSG_HELP						3		//help someone
#define MSG_ACCOMPANY					4		//accompany someone
#define MSG_DEFENDKEYAREA				5		//defend a key area
#define MSG_RUSHBASE					6		//everyone rush to base
#define MSG_GETFLAG						7		//get the enemy flag
#define MSG_STARTTEAMLEADERSHIP			8		//someone wants to become the team leader
#define MSG_STOPTEAMLEADERSHIP			9		//someone wants to stop being the team leader
#define MSG_WHOISTEAMLAEDER				10		//who is the team leader
#define MSG_WAIT						11		//wait for someone
#define MSG_WHATAREYOUDOING				12		//what are you doing?
#define MSG_JOINSUBTEAM					13		//join a sub-team
#define MSG_LEAVESUBTEAM				14		//leave a sub-team
#define MSG_CREATENEWFORMATION			15		//create a new formation
#define MSG_FORMATIONPOSITION			16		//tell someone his/her position in a formation
#define MSG_FORMATIONSPACE				17		//set the formation intervening space
#define MSG_DOFORMATION					18		//form a known formation
#define MSG_DISMISS						19		//dismiss commanded team mates
#define MSG_CAMP						20		//camp somewhere
#define MSG_CHECKPOINT					21		//remember a check point
#define MSG_PATROL						22		//patrol between certain keypoints
#define MSG_LEADTHEWAY					23		//lead the way
#define MSG_GETITEM						24		//get an item
#define MSG_KILL						25		//kill someone
#define MSG_WHEREAREYOU					26		//where is someone
#define MSG_RETURNFLAG					27		//return the flag
#define MSG_WHATISMYCOMMAND				28		//ask the team leader what to do
#define MSG_WHICHTEAM					29		//ask which team a bot is in
#define MSG_TASKPREFERENCE				30		//tell your teamplay task preference
#define MSG_ATTACKENEMYBASE				31		//attack the enemy base
#define MSG_HARVEST						32		//go harvest
#define MSG_SUICIDE						33		//order to suicide
//
#define MSG_ME							100
#define MSG_EVERYONE					101
#define MSG_MULTIPLENAMES				102
#define MSG_NAME						103
#define MSG_PATROLKEYAREA				104
#define MSG_MINUTES						105
#define MSG_SECONDS						106
#define MSG_FOREVER						107
#define MSG_FORALONGTIME				108
#define MSG_FORAWHILE					109
//
#define MSG_CHATALL						200
#define MSG_CHATTEAM					201
#define MSG_CHATTELL					202
//
#define MSG_CTF							300		//ctf message

//command sub types
#define ST_SOMEWHERE		(0x00000000u)
#define ST_NEARITEM			(0x00000001u)
#define ST_ADDRESSED		(0x00000002u)
#define ST_METER			(0x00000004u)
#define ST_FEET				(0x00000008u)
#define ST_TIME				(0x00000010u)
#define ST_HERE				(0x00000020u)
#define ST_THERE			(0x00000040u)
#define ST_I				(0x00000080u)
#define ST_MORE				(0x00000100u)
#define ST_BACK				(0x00000200u)
#define ST_REVERSE			(0x00000400u)
#define ST_SOMEONE			(0x00000800u)
#define ST_GOTFLAG			(0x00001000u)
#define ST_CAPTUREDFLAG		(0x00002000u)
#define ST_RETURNEDFLAG		(0x00004000u)
#define ST_TEAM				(0x00008000u)
#define ST_1FCTFGOTFLAG		(0x00010000u)

//ctf task preferences
#define ST_DEFENDER						1
#define ST_ATTACKER						2
#define ST_ROAMER						4


//word replacement variables
#define THE_ENEMY						7
#define THE_TEAM						7
//team message variables
#define NETNAME							0
#define PLACE							1
#define FLAG							1
#define MESSAGE							2
#define ADDRESSEE						2
#define ITEM							3
#define TEAMMATE						4
#define TEAMNAME						4
#define ENEMY							4
#define KEYAREA							5
#define FORMATION						5
#define POSITION						5
#define NUMBER							5
#define TIME							6
#define NAME							6
#define MORE							6


