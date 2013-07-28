#pragma once

#include "q_shared.h"

//----------------
//	TELEMARKS
//----------------

typedef struct teleMark_s
{//A named telemark
	char		name[32];		//	Name
	vector3		position;		//	Position
	gentity_t	*ent;			//	Tempent
} teleMark_t;


//----------------
//	ADMIN DATA
//----------------

typedef struct serverAdminData_s
{//Server admin data
	teleMark_t		teleMarks[32];	//	Named telemarks for this map
	int				teleMarksIndex;	//	Index of next telemark
	qboolean		teleMarksVisual;//	Are they being viewed by someone?
} serverAdminData_t;

typedef struct adminData_s
{//Client admin data
	qboolean		canTalk;		//	Have they been silenced?
	qboolean		isGhost;		//	Are they a ghost?
	qboolean		isGhost2;		//	Are they a ghost? RAZTEST
	qboolean		isFrozen;		//	Are they frozen?
	vector3			teleMark;		//	Last marked location
	qboolean		empowered;		//	Are they empowered?
	qboolean		merc;			//	Are they merced?

	// saving these for amempower
	int				forcePowersKnown;
	int				forcePowerBaseLevel[NUM_FORCE_POWERS];
	int				forcePowerLevel[NUM_FORCE_POWERS];
} adminData_t;


//----------------
//	ADMIN USERS
//----------------

typedef struct adminUser_s
{
	char			user[64];		// user
	char			password[64];	// password
	unsigned int	privs;			// 32 privs
	char			loginMsg[128];	// login message

	struct adminUser_s *next;
} adminUser_t;


//----------------
//	ADMIN COMMANDS
//----------------

typedef struct adminCommand_s
{//Each command has a privilege mask and function pointer. -1 priv-mask means anyone logged in can use it.
	char			*cmd;
	unsigned int	privs;
	void			(*func)(struct gentity_s *ent);
} adminCommand_t;


//----------------
//	ADMIN PRIVS
//----------------

#define PRIV_BYPASSFLOOD	(0x00000001)
#define PRIV_WHOIS			(0x00000002)
#define PRIV_STATUS			(0x00000004)
#define PRIV_ANNOUNCE		(0x00000008)
#define PRIV_GHOST			(0x00000010)
#define PRIV_CLIP			(0x00000020)
#define PRIV_TELEPORT		(0x00000040)
#define PRIV_POLL			(0x00000080)
#define PRIV_KILLVOTE		(0x00000100)
#define PRIV_FORCETEAM		(0x00000200)
#define PRIV_PROTECT		(0x00000400)
#define PRIV_EMPOWER		(0x00000800)
#define PRIV_SLAP			(0x00001000)
#define PRIV_FREEZE			(0x00002000)
#define PRIV_SILENCE		(0x00004000)
#define PRIV_SLAY			(0x00008000)
#define PRIV_KICK			(0x00010000)
#define PRIV_BAN			(0x00020000)
#define	PRIV_REMAP			(0x00040000)
#define PRIV_WEATHER		(0x00080000)
#define PRIV_ENTSPAWN		(0x00100000)
#define PRIV_NPCSPAWN		(0x00200000)
#define PRIV_LUA			(0x00400000)
#define PRIV_VSTR			(0x00800000)
#define PRIV_MERC			(0x01000000)

// command handling
void		AM_AddAdmin				( const char *user, const char *pass, const int privs, const char *loginMsg );
void		AM_DeleteAdmin			( const char *user );
void		AM_ListAdmins			( void );
void		AM_ParseAdmins			( void );
void		AM_SaveAdmins			( void );
void		AM_TM_ParseTelemarks	( void );
void		AM_PrintCommands		( gentity_t *ent );
qboolean	AM_HandleCommands		( gentity_t *ent, const char *cmd );
void		AM_TM_SaveTelemarks		( void );
// bans
void		JKG_Bans_Clear			( void );
void		JKG_Bans_LoadBans		( void );
void		JKG_Bans_Init			( void );
void		JKG_Bans_SaveBans		( void );
void		JKG_Bans_List			( void );
qboolean	JKG_Bans_Remove			( byte *ip );
int			JKG_Bans_AddBanString	( const char *ip, const char *duration, const char *reason );
const char	*JKG_Bans_IsBanned		( byte *ip );
byte		*BuildByteFromIP		( const char *ip );
