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
#define PRIV_GIVE       (0x10000000u)

typedef enum admin_strings_e {
	ADMIN_STRING_BAN = 0,
	ADMIN_STRING_EMPOWER,
	ADMIN_STRING_EMPOWER_ANNOUNCE,
	ADMIN_STRING_UNEMPOWER,
	ADMIN_STRING_FREEZE,
	ADMIN_STRING_UNFREEZED,
	ADMIN_STRING_GHOST,
	ADMIN_STRING_GHOST_ANNOUNCE,
	ADMIN_STRING_UNGHOSTED,
	ADMIN_STRING_GIVE,
	ADMIN_STRING_GIVE_ANNOUNCE,
	ADMIN_STRING_KICK,
	ADMIN_STRING_MAP,
	ADMIN_STRING_MERC,
	ADMIN_STRING_MERC_ANNOUNCE,
	ADMIN_STRING_UNMERCED,
	ADMIN_STRING_PROTECT,
	ADMIN_STRING_PROTECT_ANNOUNCE,
	ADMIN_STRING_UNPROTECTED,
	ADMIN_STRING_SILENCE,
	ADMIN_STRING_SILENCE_ANNOUNCE,
	ADMIN_STRING_SILENCE_ALL,
	ADMIN_STRING_UNSILENCED,
	ADMIN_STRING_UNSILENCED_ALL,
	ADMIN_STRING_SLAP,
	ADMIN_STRING_SLAY,
	ADMIN_STRING_SLAY_ANNOUNCE,
	ADMIN_STRING_SLAY_ALL,
	ADMIN_STRING_SLEEP,
	ADMIN_STRING_SLEEP_ANNOUNCE,
	ADMIN_STRING_SLEEP_ALL,
	ADMIN_STRING_TELE,
	ADMIN_STRING_TELE_ANNOUNCE,
	ADMIN_STRING_WAKE,
	ADMIN_STRING_WAKE_ANNOUNCE,
	ADMIN_STRING_WAKE_ALL,
	ADMIN_STRING_WEATHER,
	ADMIN_STRING_RENAME,

	ADMIN_STRING_MAX
} admin_strings_t;

void		 AM_AddAdmin( const char *user, const char *pass, uint32_t privileges, const int rank, const char *loginMsg, int effect );
void		 AM_DeleteAdmin( const char *user );
void		 AM_ListAdmins( void );
void		 AM_LoadAdmins( void );
void		 AM_LoadStrings(void);
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
void G_SleepClient( gclient_t *cl );
void G_WakeClient( gclient_t *cl );
void Empower_On( gentity_t *ent );
void Empower_Off( gentity_t *ent );
void Merc_On( gentity_t *ent );
void Merc_Off( gentity_t *ent );
void Ghost_On( gentity_t *ent );
void Ghost_Off( gentity_t *ent );
void Slap( gentity_t *targ );
