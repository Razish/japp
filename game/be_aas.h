// Copyright (C) 1999-2000 Id Software, Inc.
//

/*****************************************************************************
 * name:		be_aas.h
 *
 * desc:		Area Awareness System, stuff exported to the AI
 *
 * $Archive: /source/code/botlib/be_aas.h $
 * $Author: osman $
 * $Revision: 1.4 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 2003/03/15 23:43:59 $
 *
 *****************************************************************************/

#ifndef MAX_STRINGFIELD
#define MAX_STRINGFIELD				80
#endif

//travel flags
#define TFL_INVALID				(0x00000001u) // traveling temporary not possible
#define TFL_WALK				(0x00000002u) // walking
#define TFL_CROUCH				(0x00000004u) // crouching
#define TFL_BARRIERJUMP			(0x00000008u) // jumping onto a barrier
#define TFL_JUMP				(0x00000010u) // jumping
#define TFL_LADDER				(0x00000020u) // climbing a ladder
#define TFL_UNUSED00000040		(0x00000040u) //
#define TFL_WALKOFFLEDGE		(0x00000080u) // walking of a ledge
#define TFL_SWIM				(0x00000100u) // swimming
#define TFL_WATERJUMP			(0x00000200u) // jumping out of the water
#define TFL_TELEPORT			(0x00000400u) // teleporting
#define TFL_ELEVATOR			(0x00000800u) // elevator
#define TFL_ROCKETJUMP			(0x00001000u) // rocket jumping
#define TFL_BFGJUMP				(0x00002000u) // bfg jumping
#define TFL_GRAPPLEHOOK			(0x00004000u) // grappling hook
#define TFL_DOUBLEJUMP			(0x00008000u) // double jump
#define TFL_RAMPJUMP			(0x00010000u) // ramp jump
#define TFL_STRAFEJUMP			(0x00020000u) // strafe jump
#define TFL_JUMPPAD				(0x00040000u) // jump pad
#define TFL_AIR					(0x00080000u) // travel through air
#define TFL_WATER				(0x00100000u) // travel through water
#define TFL_SLIME				(0x00200000u) // travel through slime
#define TFL_LAVA				(0x00400000u) // travel through lava
#define TFL_DONOTENTER			(0x00800000u) // travel through donotenter area
#define TFL_FUNCBOB				(0x01000000u) // func bobbing
#define TFL_FLIGHT				(0x02000000u) // flight
#define TFL_BRIDGE				(0x04000000u) // move over a bridge
#define TFL_NOTTEAM1			(0x08000000u) // not team 1
#define TFL_NOTTEAM2			(0x10000000u) // not team 2

//default travel flags
#define TFL_DEFAULT	TFL_WALK|TFL_CROUCH|TFL_BARRIERJUMP|\
	TFL_JUMP|TFL_LADDER|\
	TFL_WALKOFFLEDGE|TFL_SWIM|TFL_WATERJUMP|\
	TFL_TELEPORT|TFL_ELEVATOR|\
	TFL_AIR|TFL_WATER|TFL_JUMPPAD|TFL_FUNCBOB

typedef enum
{
	SOLID_NOT,			// no interaction with other objects
	SOLID_TRIGGER,		// only touch when inside, after moving
	SOLID_BBOX,			// touch on edge
	SOLID_BSP			// bsp clip, touch on edge
} solid_t;

//a trace is returned when a box is swept through the AAS world
typedef struct aas_trace_s
{
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vector3		endpos;		// final position
	int			ent;		// entity blocking the trace
	int			lastarea;	// last area the trace was in (zero if none)
	int			area;		// area blocking the trace (zero if none)
	int			planenum;	// number of the plane that was hit
} aas_trace_t;

/* Defined in botlib.h

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	uint32_t flags;
	int value;
} bsp_surface_t;

//a trace is returned when a box is swept through the BSP world
typedef struct bsp_trace_s
{
	qboolean		allsolid;	// if true, plane is not valid
	qboolean		startsolid;	// if true, the initial point was in a solid area
	float			fraction;	// time completed, 1.0 = didn't hit anything
	vector3			endpos;		// final position
	cplane_t		plane;		// surface normal at impact
	float			exp_dist;	// expanded plane distance
	int				sidenum;	// number of the brush side hit
	bsp_surface_t	surface;	// hit surface
	uint32_t		contents;	// contents on other side of surface hit
	int				ent;		// number of entity hit
} bsp_trace_t;
//
*/

//entity info
typedef struct aas_entityinfo_s
{
	int		valid;			// true if updated this frame
	int		type;			// entity type
	uint32_t flags;			// entity flags
	float	ltime;			// local time
	float	update_time;	// time between last and current update
	int		number;			// number of the entity
	vector3	origin;			// origin of the entity
	vector3	angles;			// angles of the model
	vector3	old_origin;		// for lerping
	vector3	lastvisorigin;	// last visible origin
	vector3	mins;			// bounding box minimums
	vector3	maxs;			// bounding box maximums
	int		groundent;		// ground entity
	int		solid;			// solid type
	int		modelindex;		// model used
	int		modelindex2;	// weapons, CTF flags, etc
	int		frame;			// model frame number
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
	int		eventParm;		// even parameter
	uint32_t powerups;		// bit flags
	int		weapon;			// determines weapon and flash model, etc
	int		legsAnim;		// current legs anim
	int		torsoAnim;		// current torso anim
} aas_entityinfo_t;

// area info
typedef struct aas_areainfo_s {
	uint32_t contents;
	uint32_t flags;
	int presencetype;
	int cluster;
	vector3 mins, maxs;
	vector3 center;
} aas_areainfo_t;

// client movement prediction stop events, stop as soon as:
#define SE_NONE					(0x0000u) //
#define SE_HITGROUND			(0x0001u) // the ground is hit
#define SE_LEAVEGROUND			(0x0002u) // there's no ground
#define SE_ENTERWATER			(0x0004u) // water is entered
#define SE_ENTERSLIME			(0x0008u) // slime is entered
#define SE_ENTERLAVA			(0x0010u) // lava is entered
#define SE_HITGROUNDDAMAGE		(0x0020u) // the ground is hit with damage
#define SE_GAP					(0x0040u) // there's a gap
#define SE_TOUCHJUMPPAD			(0x0080u) // touching a jump pad area
#define SE_TOUCHTELEPORTER		(0x0100u) // touching teleporter
#define SE_ENTERAREA			(0x0200u) // the given stoparea is entered
#define SE_HITGROUNDAREA		(0x0400u) // a ground face in the area is hit
#define SE_HITBOUNDINGBOX		(0x0800u) // hit the specified bounding box
#define SE_TOUCHCLUSTERPORTAL	(0x1000u) // touching a cluster portal

typedef struct aas_clientmove_s {
	vector3 endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	vector3 velocity;		//velocity at the end of movement prediction
	aas_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	uint32_t endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
} aas_clientmove_t;

// alternate route goals
#define ALTROUTEGOAL_ALL				(0x0001u)
#define ALTROUTEGOAL_CLUSTERPORTALS		(0x0002u)
#define ALTROUTEGOAL_VIEWPORTALS		(0x0004u)

typedef struct aas_altroutegoal_s
{
	vector3 origin;
	int areanum;
	unsigned short starttraveltime;
	unsigned short goaltraveltime;
	unsigned short extratraveltime;
} aas_altroutegoal_t;

// route prediction stop events
#define RSE_NONE				(0x0000u)
#define RSE_NOROUTE				(0x0001u) // no route to goal
#define RSE_USETRAVELTYPE		(0x0002u) // stop as soon as on of the given travel types is used
#define RSE_ENTERCONTENTS		(0x0004u) // stop when entering the given contents
#define RSE_ENTERAREA			(0x0008u) // stop when entering the given area

typedef struct aas_predictroute_s {
	vector3 endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	int stopevent;			//event that made the prediction stop
	uint32_t endcontents;		//contents at the end of movement prediction
	uint32_t endtravelflags;		//end travel flags
	int numareas;			//number of areas predicted ahead
	int time;				//time predicted ahead (in hundreth of a sec)
} aas_predictroute_t;
