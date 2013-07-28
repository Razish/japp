#pragma once

#include "qcommon/q_engine.h"

#define MAX_DOWNLOAD_WINDOW			(8)		// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE		(2048)	// 2048 byte block chunks
#define	PACKET_BACKUP				(32)	// number of old messages that must be kept on client and server for delta comrpession and ping estimation
#define	PACKET_MASK					(PACKET_BACKUP-1)
#define	MAX_PACKET_USERCMDS			(32)	// max number of usercmd_t in a packet
#define	PORT_ANY					(-1)
#define	MAX_RELIABLE_COMMANDS		(128)	// max string commands buffered for restransmit
#define	MAX_MSGLEN					(49152)	// max length of a message, which may be fragmented into multiple packets

typedef enum {
	NA_BOT,
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
} netadrtype_t;

typedef enum netsrc_e {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

typedef struct netchan_s {
	netsrc_t	sock;

	int			dropped;			// between last packet and previous

	netadr_t	remoteAddress;
	int			qport;				// qport value to write when transmitting

	// sequencing variables
	int			incomingSequence;
	int			outgoingSequence;

	// incoming fragment assembly buffer
	int			fragmentSequence;
	int			fragmentLength;	
	byte		fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	qboolean	unsentFragments;
	int			unsentFragmentStart;
	int			unsentLength;
	byte		unsentBuffer[MAX_MSGLEN];
} netchan_t;

typedef struct clientSnapshot_s {
	int				areabytes;
	byte			areabits[MAX_MAP_AREA_BYTES];		// portalarea visibility bits
	playerState_t	ps;
	playerState_t	vps; //vehicle I'm riding's playerstate (if applicable) -rww

	int				num_entities;
	int				first_entity;		// into the circular sv_packet_entities[]
	// the entities MUST be in increasing state number
	// order, otherwise the delta compression will fail
	int				messageSent;		// time the message was transmitted
	int				messageAcked;		// time the message was acked
	int				messageSize;		// used to rate drop packets
} clientSnapshot_t;

typedef enum clientState_e {
	CS_FREE,		// can be reused for a new connection
	CS_ZOMBIE,		// client has been disconnected, but don't reuse
	// connection for a couple seconds
	CS_CONNECTED,	// has been assigned to a client_t, but no gamestate yet
	CS_PRIMED,		// gamestate has been sent, but client hasn't sent a usercmd
	CS_ACTIVE		// client is fully in game
} clientState_t;

typedef struct client_s {
	clientState_t	state;
	char			userinfo[MAX_INFO_STRING];	// name, etc

	qboolean		sentGamedir;			//see if he has been sent an svc_setgame

	char			reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	int				reliableSequence;		// last added reliable message, not necesarily sent or acknowledged yet
	int				reliableAcknowledge;	// last acknowledged reliable message
	int				reliableSent;			// last sent reliable message, not necesarily acknowledged yet
	int				messageAcknowledge;

	int				gamestateMessageNum;	// netchan->outgoingSequence of gamestate
	int				challenge;

	usercmd_t		lastUsercmd;
	int				lastMessageNum;			// for delta compression
	int				lastClientCommand;		// reliable client message sequence
	char			lastClientCommandString[MAX_STRING_CHARS];
	sharedEntity_t	*gentity;				// SV_GentityNum(clientnum)
	char			name[MAX_NAME_LENGTH];	// extracted from userinfo, high bits masked

	// downloading
	char			downloadName[MAX_QPATH]; // if not empty string, we are downloading
	fileHandle_t	download;			// file being downloaded
	int				downloadSize;		// total bytes (can't use EOF because of paks)
	int				downloadCount;		// bytes sent
	int				downloadClientBlock;	// last block we sent to the client, awaiting ack
	int				downloadCurrentBlock;	// current block number
	int				downloadXmitBlock;	// last block we xmited
	unsigned char	*downloadBlocks[MAX_DOWNLOAD_WINDOW];	// the buffers for the download blocks
	int				downloadBlockSize[MAX_DOWNLOAD_WINDOW];
	qboolean		downloadEOF;		// We have sent the EOF block
	int				downloadSendTime;	// time we last got an ack from the client

	int				deltaMessage;		// frame last client usercmd message
	int				nextReliableTime;	// svs.time when another reliable command will be allowed
	int				lastPacketTime;		// svs.time when packet was last received
	int				lastConnectTime;	// svs.time when connection started
	int				nextSnapshotTime;	// send another snapshot when svs.time >= nextSnapshotTime
	qboolean		rateDelayed;		// true if nextSnapshotTime was set based on rate instead of snapshotMsec
	int				timeoutCount;		// must timeout a few frames in a row so debugging doesn't break
	clientSnapshot_t	frames[PACKET_BACKUP];	// updates can be delta'd from here
	int				ping;
	int				rate;				// bytes / second
	int				snapshotMsec;		// requests a snapshot every snapshotMsec unless rate choked
	int				pureAuthentic;
	netchan_t		netchan;

	int				lastUserInfoChange; //if > svs.time && count > x, deny change -rww
	int				lastUserInfoCount; //allow a certain number of changes within a certain time period -rww
} client_t;

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

typedef struct challenge_s {
	netadr_t	adr;
	int			challenge;
	int			time;				// time the last packet was sent to the autherize server
	int			pingTime;			// time the challenge response was sent to client
	int			firstTime;			// time the adr was first used, for authorize timeout checks
	qboolean	connected;
} challenge_t;

// this structure will be cleared only when the game dll changes
typedef struct serverStatic_s {
	qboolean		initialized;				// sv_init has completed

	int				time;						// will be strictly increasing across level changes

	int				snapFlagServerBit;			// ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer()

	client_t		*clients;					// [sv_maxclients->integer];
	int				numSnapshotEntities;		// sv_maxclients->integer*PACKET_BACKUP*MAX_PACKET_ENTITIES
	int				nextSnapshotEntities;		// next snapshotEntities to use
	entityState_t	*snapshotEntities;			// [numSnapshotEntities]
	int				nextHeartbeatTime;
	challenge_t		challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting
	netadr_t		redirectAddress;			// for rcon return messages

	netadr_t		authorizeAddress;			// for rcon return messages
} serverStatic_t;

#ifndef OPENJK

extern serverStatic_t *svs;
int		(*ENG_FS_FOpenFileByMode)	(const char *qpath, fileHandle_t *f, fsMode_t mode);
void	(*ENG_FS_FCloseFile)		(fileHandle_t f);
int		(*ENG_FS_Write)				(const void *buffer, int len, fileHandle_t f);
char *	(*ENG_FS_BuildOSPath)		(const char *base, const char *game, const char *qpath);
char **	(*ENG_Sys_ListFiles)		(const char *directory, const char *extension, char *filter, int *numfiles, int wantsubs);
void	(*ENG_Sys_FreeFileList)		(char **list);
char *	(*ENG_Sys_Cwd)				(void);
cvar_t *(*ENG_Cvar_Get)				(const char *var_name, const char *var_value, int flags);

#endif // !OPENJK
