#pragma once

#include "qcommon/q_shared.h"

// named telemark
#define MAX_TELEMARK_NAME_LEN (32)
typedef struct telemark_s {
	char name[MAX_TELEMARK_NAME_LEN];
	vector3 position;
	gentity_t *ent; // tempent

	struct telemark_s *next;
} telemark_t;

// client admin data
typedef struct adminData_s {
	qboolean silenced; // have they been silenced?
	qboolean isGhost; // are they a ghost?
	qboolean isSlept; // are they frozen?
	telemark_t *telemark; // last marked location
	qboolean empowered; // are they empowered?
	qboolean merc; // are they merced?
	int renamedTime; // level.time they were renamed
	int logineffect;

	// saving these for amempower
	int forcePowersKnown;
	int forcePowerBaseLevel[NUM_FORCE_POWERS];
	int forcePowerLevel[NUM_FORCE_POWERS];
} adminData_t;

// admin users
typedef struct adminUser_s {
	char		user[64];		// user
	char		password[64];	// password
	uint32_t	privileges;		// 32 privs
	char		loginMsg[128];	// login message
	int			rank;			// rank
	int         logineffect;    // login effect 

	struct adminUser_s *next;
} adminUser_t;

#define PRIV_WHOIS		(0x00000001u)
#define PRIV_STATUS		(0x00000002u)
#define PRIV_ANNOUNCE	(0x00000004u)
#define PRIV_GHOST		(0x00000008u)
#define PRIV_CLIP		(0x00000010u)
#define PRIV_TELEPORT	(0x00000020u)
#define PRIV_POLL		(0x00000040u)
#define PRIV_KILLVOTE	(0x00000080u)
#define PRIV_FORCETEAM	(0x00000100u)
#define PRIV_PROTECT	(0x00000200u)
#define PRIV_EMPOWER	(0x00000400u)
#define PRIV_SLAP		(0x00000800u)
#define PRIV_SLEEP		(0x00001000u)
#define PRIV_SILENCE	(0x00002000u)
#define PRIV_SLAY		(0x00004000u)
#define PRIV_KICK		(0x00008000u)
#define PRIV_BAN		(0x00010000u)
#define	PRIV_REMAP		(0x00020000u)
#define PRIV_WEATHER	(0x00040000u)
#define PRIV_ENTSPAWN	(0x00080000u)
#define PRIV_NPCSPAWN	(0x00100000u)
#define PRIV_LUA		(0x00200000u)
#define PRIV_VSTR		(0x00400000u)
#define PRIV_MERC		(0x00800000u)
#define PRIV_MAP		(0x01000000u)
#define PRIV_RENAME		(0x02000000u)
#define PRIV_LOCKTEAM	(0x04000000u)
#define PRIV_GRANT      (0x08000000u)

void		 AM_AddAdmin( const char *user, const char *pass, uint32_t privileges, const int rank, const char *loginMsg, int effect );
void		 AM_DeleteAdmin( const char *user );
void		 AM_ListAdmins( void );
void		 AM_LoadAdmins( void );
void		 AM_SaveAdmins( void );
void		 AM_LoadTelemarks( void );
void		 AM_SaveTelemarks( void );
adminUser_t	*AM_ChecksumLogin( const char *checksum );
void		 AM_ApplySessionTransition( gentity_t *ent );
qboolean	 AM_HasPrivilege( const gentity_t *ent, uint32_t privilege );
void		 AM_PrintCommands( gentity_t *ent, printBufferSession_t *pb );
qboolean	 AM_HandleCommands( gentity_t *ent, const char *cmd );
void		 G_BroadcastToAdminsOnly( gentity_t *ent );

// bans
void		 JP_Bans_Clear( void );
void		 JP_Bans_LoadBans( void );
void		 JP_Bans_Init( void );
void		 JP_Bans_SaveBans( void );
void		 JP_Bans_List( void );
qboolean	 JP_Bans_Remove( byte *ip );
int			 JP_Bans_AddBanString( const char *ip, const char *duration, const char *reason, char *failedMsg, size_t msgLen );
const char	*JP_Bans_IsBanned( byte *ip );
byteAlias_t *BuildByteFromIP( const char *ip );

//Utils
/*
void G_SleepClient(gclient_t *cl);
void G_WakeClient(gclient_t *cl);
void Empower_On(gentity_t *ent);
void Empower_Off(gentity_t *ent);
void Merc_On(gentity_t *ent);
void Merc_Off(gentity_t *ent);
void Slap(gentity_t *targ);
*/


