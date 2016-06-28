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
	qboolean isFrozen; // are they frozen?
	telemark_t *telemark; // last marked location
	qboolean empowered; // are they empowered?
	qboolean merc; // are they merced?
	qboolean clanMember; // are they logged in with clanpass?
	int renamedTime; // level.time they were renamed
	int logineffect;

	// saving these for amempower
	int forcePowersKnown;
	int forcePowerBaseLevel[NUM_FORCE_POWERS];
	int forcePowerLevel[NUM_FORCE_POWERS];

	// saving these for ammerc
	int forcePowerMax;
} adminData_t;

// admin users
typedef struct adminUser_s {
	char		user[64];		// user
	char		password[64];	// password
	uint64_t	privileges;		// 64 privs
	char		loginMsg[128];	// login message
	int			rank;			// rank
	int         logineffect;    // login effect

	struct adminUser_s *next;
} adminUser_t;

#define PRIV_WHOIS		(0x0000000000000001u)
#define PRIV_STATUS		(0x0000000000000002u)
#define PRIV_ANNOUNCE	(0x0000000000000004u)
#define PRIV_GHOST		(0x0000000000000008u)
#define PRIV_CLIP		(0x0000000000000010u)
#define PRIV_TELEPORT	(0x0000000000000020u)
#define PRIV_POLL		(0x0000000000000040u)
#define PRIV_KILLVOTE	(0x0000000000000080u)
#define PRIV_FORCETEAM	(0x0000000000000100u)
#define PRIV_PROTECT	(0x0000000000000200u)
#define PRIV_EMPOWER	(0x0000000000000400u)
#define PRIV_SLAP		(0x0000000000000800u)
#define PRIV_SLEEP		(0x0000000000001000u)
#define PRIV_SILENCE	(0x0000000000002000u)
#define PRIV_SLAY		(0x0000000000004000u)
#define PRIV_KICK		(0x0000000000008000u)
#define PRIV_BAN		(0x0000000000010000u)
#define PRIV_REMAP		(0x0000000000020000u)
#define PRIV_WEATHER	(0x0000000000040000u)
#define PRIV_ENTSPAWN	(0x0000000000080000u)
#define PRIV_NPCSPAWN	(0x0000000000100000u)
#define PRIV_LUA		(0x0000000000200000u)
#define PRIV_VSTR		(0x0000000000400000u)
#define PRIV_MERC		(0x0000000000800000u)
#define PRIV_MAP		(0x0000000001000000u)
#define PRIV_RENAME		(0x0000000002000000u)
#define PRIV_LOCKTEAM	(0x0000000004000000u)
#define PRIV_GRANT		(0x0000000008000000u)
#define PRIV_GIVE		(0x0000000010000000u)
#define PRIV_NOTARGET	(0x0000000020000000u)
#define PRIV_MINDTRICK	(0x0000000040000000u)

void		 AM_AddAdmin( const char *user, const char *pass, uint64_t privileges, const int rank, const char *loginMsg, int effect );
void		 AM_DeleteAdmin( const char *user );
void		 AM_ListAdmins( void );
void		 AM_LoadAdmins( void );
void		 AM_LoadStrings(void);
void		 AM_SaveAdmins( void );
void		 AM_LoadTelemarks( void );
void		 AM_SaveTelemarks( void );
adminUser_t	*AM_ChecksumLogin( const char *checksum );
void		 AM_ApplySessionTransition( gentity_t *ent );
qboolean	 AM_HasPrivilege( const gentity_t *ent, uint64_t privilege );
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
void G_SleepClient( gclient_t *cl );
void G_WakeClient( gclient_t *cl );
void Empower_On( gentity_t *ent );
void Empower_Off( gentity_t *ent );
void Merc_On( gentity_t *ent );
void Merc_Off( gentity_t *ent );
void Ghost_On( gentity_t *ent );
void Ghost_Off( gentity_t *ent );
void Slap( gentity_t *targ );
void ClanMemberStatus_ON( gentity_t *targ );
void ClanMemberStatus_OFF( gentity_t *targ );
 