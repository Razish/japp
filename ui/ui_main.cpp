#include "Ghoul2/G2.h"
#include "ui_local.h"
#include "qcommon/qfiles.h"
#include "ui_force.h"
#include "game/bg_saga.h"
#include "game/bg_animTable.h"
#include "ui_shared.h"
#include "JAPP/jp_crash.h"
#include "JAPP/jp_csflags.h"

void UI_SaberAttachToChar( itemDef_t *item );

const char *HolocronIcons[NUM_FORCE_POWERS] = {
	"gfx/mp/f_icon_lt_heal", //FP_HEAL,
	"gfx/mp/f_icon_levitation", //FP_LEVITATION,
	"gfx/mp/f_icon_speed", //FP_SPEED,
	"gfx/mp/f_icon_push", //FP_PUSH,
	"gfx/mp/f_icon_pull", //FP_PULL,
	"gfx/mp/f_icon_lt_telepathy", //FP_TELEPATHY,
	"gfx/mp/f_icon_dk_grip", //FP_GRIP,
	"gfx/mp/f_icon_dk_l1", //FP_LIGHTNING,
	"gfx/mp/f_icon_dk_rage", //FP_RAGE,
	"gfx/mp/f_icon_lt_protect", //FP_PROTECT,
	"gfx/mp/f_icon_lt_absorb", //FP_ABSORB,
	"gfx/mp/f_icon_lt_healother", //FP_TEAM_HEAL,
	"gfx/mp/f_icon_dk_forceother", //FP_TEAM_FORCE,
	"gfx/mp/f_icon_dk_drain", //FP_DRAIN,
	"gfx/mp/f_icon_sight", //FP_SEE,
	"gfx/mp/f_icon_saber_attack", //FP_SABER_OFFENSE,
	"gfx/mp/f_icon_saber_defend", //FP_SABER_DEFENSE,
	"gfx/mp/f_icon_saber_throw" //FP_SABERTHROW
};

const char *forcepowerDesc[NUM_FORCE_POWERS] = {
	"@MENUS_OF_EFFECT_JEDI_ONLY_NEFFECT",
	"@MENUS_DURATION_IMMEDIATE_NAREA",
	"@MENUS_DURATION_5_SECONDS_NAREA",
	"@MENUS_DURATION_INSTANTANEOUS",
	"@MENUS_INSTANTANEOUS_EFFECT_NAREA",
	"@MENUS_DURATION_VARIABLE_20",
	"@MENUS_DURATION_INSTANTANEOUS_NAREA",
	"@MENUS_OF_EFFECT_LIVING_PERSONS",
	"@MENUS_DURATION_VARIABLE_10",
	"@MENUS_DURATION_VARIABLE_NAREA",
	"@MENUS_DURATION_CONTINUOUS_NAREA",
	"@MENUS_OF_EFFECT_JEDI_ALLIES_NEFFECT",
	"@MENUS_EFFECT_JEDI_ALLIES_NEFFECT",
	"@MENUS_VARIABLE_NAREA_OF_EFFECT",
	"@MENUS_EFFECT_NAREA_OF_EFFECT",
	"@SP_INGAME_FORCE_SABER_OFFENSE_DESC",
	"@SP_INGAME_FORCE_SABER_DEFENSE_DESC",
	"@SP_INGAME_FORCE_SABER_THROW_DESC"
};

// Movedata Sounds
typedef enum moveDataSounds_e {
	MDS_NONE = 0,
	MDS_FORCE_JUMP,
	MDS_ROLL,
	MDS_SABER,
	MDS_MOVE_SOUNDS_MAX
} moveDataSounds_t;

// Some hard coded badness
// At some point maybe this should be externalized to a .dat file
const char *datapadMoveTitleData[MD_MOVE_TITLE_MAX] =
{
	"@MENUS_ACROBATICS",
	"@MENUS_SINGLE_FAST",
	"@MENUS_SINGLE_MEDIUM",
	"@MENUS_SINGLE_STRONG",
	"@MENUS_DUAL_SABERS",
	"@MENUS_SABER_STAFF",
};

const char *datapadMoveTitleBaseAnims[MD_MOVE_TITLE_MAX] =
{
	"BOTH_RUN1",
	"BOTH_SABERFAST_STANCE",
	"BOTH_STAND2",
	"BOTH_SABERSLOW_STANCE",
	"BOTH_SABERDUAL_STANCE",
	"BOTH_SABERSTAFF_STANCE",
};

#define MAX_MOVES 16

typedef struct datpadmovedata_s {
	const char	*title;
	const char	*desc;
	const char	*anim;
	short	sound;
} datpadmovedata_t;

static datpadmovedata_t datapadMoveData[MD_MOVE_TITLE_MAX][MAX_MOVES] = {
	{// Acrobatics
		{ "@MENUS_FORCE_JUMP1", "@MENUS_FORCE_JUMP1_DESC", "BOTH_FORCEJUMP1", MDS_FORCE_JUMP },
		{ "@MENUS_FORCE_FLIP", "@MENUS_FORCE_FLIP_DESC", "BOTH_FLIP_F", MDS_FORCE_JUMP },
		{ "@MENUS_ROLL", "@MENUS_ROLL_DESC", "BOTH_ROLL_F", MDS_ROLL },
		{ "@MENUS_BACKFLIP_OFF_WALL", "@MENUS_BACKFLIP_OFF_WALL_DESC", "BOTH_WALL_FLIP_BACK1", MDS_FORCE_JUMP },
		{ "@MENUS_SIDEFLIP_OFF_WALL", "@MENUS_SIDEFLIP_OFF_WALL_DESC", "BOTH_WALL_FLIP_RIGHT", MDS_FORCE_JUMP },
		{ "@MENUS_WALL_RUN", "@MENUS_WALL_RUN_DESC", "BOTH_WALL_RUN_RIGHT", MDS_FORCE_JUMP },
		{ "@MENUS_WALL_GRAB_JUMP", "@MENUS_WALL_GRAB_JUMP_DESC", "BOTH_FORCEWALLREBOUND_FORWARD", MDS_FORCE_JUMP },
		{ "@MENUS_RUN_UP_WALL_BACKFLIP", "@MENUS_RUN_UP_WALL_BACKFLIP_DESC", "BOTH_FORCEWALLRUNFLIP_START", MDS_FORCE_JUMP },
		{ "@MENUS_JUMPUP_FROM_KNOCKDOWN", "@MENUS_JUMPUP_FROM_KNOCKDOWN_DESC", "BOTH_KNOCKDOWN3", MDS_NONE },
		{ "@MENUS_JUMPKICK_FROM_KNOCKDOWN", "@MENUS_JUMPKICK_FROM_KNOCKDOWN_DESC", "BOTH_KNOCKDOWN2", MDS_NONE },
		{ "@MENUS_ROLL_FROM_KNOCKDOWN", "@MENUS_ROLL_FROM_KNOCKDOWN_DESC", "BOTH_KNOCKDOWN1", MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
	},
	{//Single Saber, Fast Style
		{ "@MENUS_STAB_BACK", "@MENUS_STAB_BACK_DESC", "BOTH_A2_STABBACK1", MDS_SABER },
		{ "@MENUS_LUNGE_ATTACK", "@MENUS_LUNGE_ATTACK_DESC", "BOTH_LUNGE2_B__T_", MDS_SABER },
		{ "@MENUS_FAST_ATTACK_KATA", "@MENUS_FAST_ATTACK_KATA_DESC", "BOTH_A1_SPECIAL", MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND", "@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN", MDS_FORCE_JUMP },
		{ "@MENUS_CARTWHEEL", "@MENUS_CARTWHEEL_DESC", "BOTH_ARIAL_RIGHT", MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB", "@MENUS_BOTH_ROLL_STAB2_DESC", "BOTH_ROLL_STAB", MDS_SABER },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
	},
	{//Single Saber, Medium Style
		{ "@MENUS_SLASH_BACK", "@MENUS_SLASH_BACK_DESC", "BOTH_ATTACK_BACK", MDS_SABER },
		{ "@MENUS_FLIP_ATTACK", "@MENUS_FLIP_ATTACK_DESC", "BOTH_JUMPFLIPSLASHDOWN1", MDS_FORCE_JUMP },
		{ "@MENUS_MEDIUM_ATTACK_KATA", "@MENUS_MEDIUM_ATTACK_KATA_DESC", "BOTH_A2_SPECIAL", MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND", "@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN", MDS_FORCE_JUMP },
		{ "@MENUS_CARTWHEEL", "@MENUS_CARTWHEEL_DESC", "BOTH_ARIAL_RIGHT", MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB", "@MENUS_BOTH_ROLL_STAB2_DESC", "BOTH_ROLL_STAB", MDS_SABER },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
	},
	{//Single Saber, Strong Style
		{ "@MENUS_SLASH_BACK", "@MENUS_SLASH_BACK_DESC", "BOTH_ATTACK_BACK", MDS_SABER },
		{ "@MENUS_JUMP_ATTACK", "@MENUS_JUMP_ATTACK_DESC", "BOTH_FORCELEAP2_T__B_", MDS_FORCE_JUMP },
		{ "@MENUS_STRONG_ATTACK_KATA", "@MENUS_STRONG_ATTACK_KATA_DESC", "BOTH_A3_SPECIAL", MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND", "@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN", MDS_FORCE_JUMP },
		{ "@MENUS_CARTWHEEL", "@MENUS_CARTWHEEL_DESC", "BOTH_ARIAL_RIGHT", MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB", "@MENUS_BOTH_ROLL_STAB2_DESC", "BOTH_ROLL_STAB", MDS_SABER },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
	},
	{//Dual Sabers
		{ "@MENUS_SLASH_BACK", "@MENUS_SLASH_BACK_DESC", "BOTH_ATTACK_BACK", MDS_SABER },
		{ "@MENUS_FLIP_FORWARD_ATTACK", "@MENUS_FLIP_FORWARD_ATTACK_DESC", "BOTH_JUMPATTACK6", MDS_FORCE_JUMP },
		{ "@MENUS_DUAL_SABERS_TWIRL", "@MENUS_DUAL_SABERS_TWIRL_DESC", "BOTH_SPINATTACK6", MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND", "@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN_DUAL", MDS_FORCE_JUMP },
		{ "@MENUS_DUAL_SABER_BARRIER", "@MENUS_DUAL_SABER_BARRIER_DESC", "BOTH_A6_SABERPROTECT", MDS_SABER },
		{ "@MENUS_DUAL_STAB_FRONT_BACK", "@MENUS_DUAL_STAB_FRONT_BACK_DESC", "BOTH_A6_FB", MDS_SABER },
		{ "@MENUS_DUAL_STAB_LEFT_RIGHT", "@MENUS_DUAL_STAB_LEFT_RIGHT_DESC", "BOTH_A6_LR", MDS_SABER },
		{ "@MENUS_CARTWHEEL", "@MENUS_CARTWHEEL_DESC", "BOTH_ARIAL_RIGHT", MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB", "@MENUS_BOTH_ROLL_STAB_DESC", "BOTH_ROLL_STAB", MDS_SABER },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
	},
	{// Saber Staff
		{ "@MENUS_STAB_BACK", "@MENUS_STAB_BACK_DESC", "BOTH_A2_STABBACK1", MDS_SABER },
		{ "@MENUS_BACK_FLIP_ATTACK", "@MENUS_BACK_FLIP_ATTACK_DESC", "BOTH_JUMPATTACK7", MDS_FORCE_JUMP },
		{ "@MENUS_SABER_STAFF_TWIRL", "@MENUS_SABER_STAFF_TWIRL_DESC", "BOTH_SPINATTACK7", MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND", "@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN_STAFF", MDS_FORCE_JUMP },
		{ "@MENUS_SPINNING_KATA", "@MENUS_SPINNING_KATA_DESC", "BOTH_A7_SOULCAL", MDS_SABER },
		{ "@MENUS_KICK1", "@MENUS_KICK1_DESC", "BOTH_A7_KICK_F", MDS_FORCE_JUMP },
		{ "@MENUS_JUMP_KICK", "@MENUS_JUMP_KICK_DESC", "BOTH_A7_KICK_F_AIR", MDS_FORCE_JUMP },
		{ "@MENUS_BUTTERFLY_ATTACK", "@MENUS_BUTTERFLY_ATTACK_DESC", "BOTH_BUTTERFLY_FR1", MDS_SABER },
		{ "@MENUS_BOTH_ROLL_STAB", "@MENUS_BOTH_ROLL_STAB2_DESC", "BOTH_ROLL_STAB", MDS_SABER },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE },
		{ NULL, NULL, NULL, MDS_NONE }
	}
};

static siegeClassDesc_t g_UIClassDescriptions[MAX_SIEGE_CLASSES];
static siegeTeam_t *siegeTeam1 = NULL, *siegeTeam2 = NULL;
static int g_UIGloballySelectedSiegeClass = -1;

//Cut down version of the stuff used in the game code
//This is just the bare essentials of what we need to load animations properly for ui ghoul2 models.
//This function doesn't need to be sync'd with the BG_ version in bg_panimate.c unless some sort of fundamental change
//is made. Just make sure the variables/functions accessed in ui_shared.c exist in both modules.
qboolean	UIPAFtextLoaded = qfalse;
animation_t	uiHumanoidAnimations[MAX_TOTALANIMATIONS]; //humanoid animations are the only ones that are statically allocated.

bgLoadedAnim_t bgAllAnims[MAX_ANIM_FILES];
int uiNumAllAnims = 1; //start off at 0, because 0 will always be assigned to humanoid.

animation_t *UI_AnimsetAlloc( void ) {
	assert( uiNumAllAnims < MAX_ANIM_FILES );
	bgAllAnims[uiNumAllAnims].anims = (animation_t *)BG_Alloc( sizeof(animation_t)*MAX_TOTALANIMATIONS );

	return bgAllAnims[uiNumAllAnims].anims;
}

static char UIPAFtext[60000];

// Read a configuration file containing animation counts and rates
// models/players/visor/animation.cfg, etc
int UI_ParseAnimationFile( const char *filename, animation_t *animset, qboolean isHumanoid ) {
	char		*text_p;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			usedIndex = -1;
	int			nextIndex = uiNumAllAnims;

	fileHandle_t	f;
	int				animNum;

	if ( !isHumanoid ) {
		i = 1;
		while ( i < uiNumAllAnims ) { //see if it's been loaded already
			if ( !Q_stricmp( bgAllAnims[i].filename, filename ) ) {
				return i; //alright, we already have it.
			}
			i++;
		}

		//Looks like it has not yet been loaded. Allocate space for the anim set if we need to, and continue along.
		if ( !animset ) {
			if ( strstr( filename, "players/_humanoid/" ) ) { //then use the static humanoid set.
				animset = uiHumanoidAnimations;
				isHumanoid = qtrue;
				nextIndex = 0;
			}
			else {
				animset = UI_AnimsetAlloc();

				if ( !animset ) {
					assert( !"Anim set alloc failed!" );
					return -1;
				}
			}
		}
	}
#ifdef _DEBUG
	else {
		assert( animset );
	}
#endif

	// load the file
	if ( !UIPAFtextLoaded || !isHumanoid ) { //rww - We are always using the same animation config now. So only load it once.
		len = trap->FS_Open( filename, &f, FS_READ );
		if ( (len <= 0) || (len >= (signed)sizeof(UIPAFtext)-1) ) {
			if ( len > 0 ) {
				Com_Error( ERR_DROP, "%s exceeds the allowed ui-side animation buffer!", filename );
			}
			return -1;
		}

		trap->FS_Read( UIPAFtext, len, f );
		UIPAFtext[len] = 0;
		trap->FS_Close( f );
	}
	else {
		return 0; //humanoid index
	}

	// parse the text
	text_p = UIPAFtext;

	//FIXME: have some way of playing anims backwards... negative numFrames?

	//initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for ( i = 0; i < MAX_ANIMATIONS; i++ ) {
		animset[i].firstFrame = 0;
		animset[i].numFrames = 0;
		animset[i].loopFrames = -1;
		animset[i].frameLerp = 100;
		//		animset[i].initialLerp = 100;
	}

	COM_BeginParseSession( "UI_ParseAnimationFile" );

	// read information for each frame
	while ( 1 ) {
		token = COM_Parse( (const char **)(&text_p) );

		if ( !token || !token[0] ) {
			break;
		}

		animNum = GetIDForString( animTable, token );
		if ( animNum == -1 ) {
#ifdef _DEBUG
			//Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, filename);
#endif
			continue;
		}

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) {
			break;
		}
		animset[animNum].firstFrame = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) {
			break;
		}
		animset[animNum].numFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) {
			break;
		}
		animset[animNum].loopFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;//Don't allow divide by zero error
		}
		if ( fps < 0 ) {//backwards
			animset[animNum].frameLerp = floorf( 1000.0f / fps );
		}
		else {
			animset[animNum].frameLerp = ceilf( 1000.0f / fps );
		}

		//animset[animNum].initialLerp = ceilf(1000.0f / fabsf(fps));
	}

#ifdef _DEBUG
	//Check the array, and print the ones that have nothing in them.
	/*
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
	if (animTable[i].name != NULL)		// This animation reference exists.
	{
	if (animset[i].firstFrame <= 0 && animset[i].numFrames <=0)
	{	// This is an empty animation reference.
	Com_Printf("***ANIMTABLE reference #%d (%s) is empty!\n", i, animTable[i].name);
	}
	}
	}
	*/
#endif // _DEBUG

	if ( isHumanoid ) {
		bgAllAnims[0].anims = animset;
		strcpy( bgAllAnims[0].filename, filename );
		UIPAFtextLoaded = qtrue;

		usedIndex = 0;
	}
	else {
		bgAllAnims[nextIndex].anims = animset;
		strcpy( bgAllAnims[nextIndex].filename, filename );

		usedIndex = nextIndex;

		if ( nextIndex ) { //don't bother increasing the number if this ended up as a humanoid load.
			uiNumAllAnims++;
		}
		else {
			UIPAFtextLoaded = qtrue;
			usedIndex = 0;
		}
	}

	return usedIndex;
}

//menuDef_t *Menus_FindByName(const char *p);
void Menu_ShowItemByName( menuDef_t *menu, const char *p, qboolean bShow );


void UpdateForceUsed( void );

char holdSPString[MAX_STRING_CHARS] = { 0 };

uiInfo_t uiInfo;

static void UI_StartServerRefresh( qboolean full );
static void UI_StopServerRefresh( void );
static void UI_DoServerRefresh( void );
static void UI_BuildServerDisplayList( int force );
static void UI_BuildServerStatus( qboolean force );
static void UI_BuildFindPlayerList( qboolean force );
static int UI_ServersQsortCompare( const void *arg1, const void *arg2 );
static int UI_MapCountByGameType( qboolean singlePlayer );
static int UI_HeadCountByColor( void );
static const char *UI_SelectedMap( int index, int *actual );
static int UI_GetIndexFromSelection( int actual );
static void UI_SiegeClassCnt( const int team );

int ProcessNewUI( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 );
int	uiSkinColor = TEAM_FREE;
int	uiHoldSkinColor = TEAM_FREE;	// Stores the skin color so that in non-team games, the player screen remembers the team you chose, in case you're coming back from the force powers screen.

static const char *skillLevels[] = {
	"SKILL1",//"I Can Win",
	"SKILL2",//"Bring It On",
	"SKILL3",//"Hurt Me Plenty",
	"SKILL4",//"Hardcore",
	"SKILL5"//"Nightmare"
};
static const size_t numSkillLevels = ARRAY_LEN( skillLevels );

static const char *netNames[] = {
	"???",
	"UDP",
	NULL
};
static const int numNetNames = ARRAY_LEN( netNames ) - 1;

const char *UI_GetStringEdString( const char *refSection, const char *refName );

const char *UI_TeamName( int team ) {
	if ( team == TEAM_RED )
		return "RED";
	else if ( team == TEAM_BLUE )
		return "BLUE";
	else if ( team == TEAM_SPECTATOR )
		return "SPECTATOR";
	return "FREE";
}

// returns either string or NULL for OOR...
//
static const char *GetCRDelineatedString( const char *psStripFileRef, const char *psStripStringRef, int iIndex ) {
	static char sTemp[256];
	const char *psList = UI_GetStringEdString( psStripFileRef, psStripStringRef );
	char *p;

	while ( iIndex-- ) {
		psList = strchr( psList, '\n' );
		if ( !psList ) {
			return NULL;	// OOR
		}
		psList++;
	}

	strcpy( sTemp, psList );
	p = strchr( sTemp, '\n' );
	if ( p ) {
		*p = '\0';
	}

	return sTemp;
}

static const char *GetMonthAbbrevString( int iMonth ) {
	const char *p = GetCRDelineatedString( "MP_INGAME", "MONTHS", iMonth );

	return p ? p : "Jan";	// sanity
}

#define UIAS_LOCAL		(0)
#define UIAS_GLOBAL1	(1)
#define UIAS_GLOBAL2	(2)
#define UIAS_GLOBAL3	(3)
#define UIAS_GLOBAL4	(4)
#define UIAS_GLOBAL5	(5)
#define UIAS_FAVORITES	(6)
#define UIAS_HISTORY	(7)

#define UI_MAX_MASTER_SERVERS	(5)

// Convert ui's net source to AS_* used by trap calls.
int UI_SourceForLAN( void ) {
	switch ( ui_netSource.integer ) {
	default:
	case UIAS_LOCAL:
		return AS_LOCAL;
	case UIAS_GLOBAL1:
	case UIAS_GLOBAL2:
	case UIAS_GLOBAL3:
	case UIAS_GLOBAL4:
	case UIAS_GLOBAL5:
		return AS_GLOBAL;
	case UIAS_FAVORITES:
		return AS_FAVORITES;
	case UIAS_HISTORY:
		return AS_HISTORY;
	}
}

/*
static const char *netSources[] = {
"Local",
"Internet",
"Favorites"
//	"Mplayer"
};
static const int numNetSources = ARRAY_LEN(netSources);
*/
static const int numNetSources = 7;	// now hard-entered in StringEd file
static const char *GetNetSourceString( int iSource ) {
	static char result[256] = {0};

	Q_strncpyz( result, GetCRDelineatedString( "MP_INGAME", "NET_SOURCES", UI_SourceForLAN() ), sizeof( result ) );
	if ( iSource >= UIAS_GLOBAL1 && iSource <= UIAS_GLOBAL5 )
		Q_strcat( result, sizeof( result ), va( " %d", iSource ) );

	return result;
}

void AssetCache( void ) {
	int n;
	//if (Assets.textFont == NULL) {
	//}
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	uiInfo.uiDC.Assets.gradientBar = trap->R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.fxBasePic = trap->R_RegisterShaderNoMip( ART_FX_BASE );
	uiInfo.uiDC.Assets.fxPic[0] = trap->R_RegisterShaderNoMip( ART_FX_RED );
	uiInfo.uiDC.Assets.fxPic[1] = trap->R_RegisterShaderNoMip( ART_FX_ORANGE );//trap->R_RegisterShaderNoMip( ART_FX_YELLOW );
	uiInfo.uiDC.Assets.fxPic[2] = trap->R_RegisterShaderNoMip( ART_FX_YELLOW );//trap->R_RegisterShaderNoMip( ART_FX_GREEN );
	uiInfo.uiDC.Assets.fxPic[3] = trap->R_RegisterShaderNoMip( ART_FX_GREEN );//trap->R_RegisterShaderNoMip( ART_FX_TEAL );
	uiInfo.uiDC.Assets.fxPic[4] = trap->R_RegisterShaderNoMip( ART_FX_BLUE );
	uiInfo.uiDC.Assets.fxPic[5] = trap->R_RegisterShaderNoMip( ART_FX_PURPLE );//trap->R_RegisterShaderNoMip( ART_FX_CYAN );
	uiInfo.uiDC.Assets.fxPic[6] = trap->R_RegisterShaderNoMip( ART_FX_WHITE );
	uiInfo.uiDC.Assets.scrollBar = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = trap->R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = trap->R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb = trap->R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
	uiInfo.uiDC.Assets.sliderThumbDefault = trap->R_RegisterShaderNoMip( ASSET_SLIDER_THUMB_DEFAULT );

	// Icons for various server settings.
	uiInfo.uiDC.Assets.needPass = trap->R_RegisterShaderNoMip( "gfx/menus/needpass" );
	uiInfo.uiDC.Assets.noForce = trap->R_RegisterShaderNoMip( "gfx/menus/noforce" );
	uiInfo.uiDC.Assets.forceRestrict = trap->R_RegisterShaderNoMip( "gfx/menus/forcerestrict" );
	uiInfo.uiDC.Assets.saberOnly = trap->R_RegisterShaderNoMip( "gfx/menus/saberonly" );
	uiInfo.uiDC.Assets.trueJedi = trap->R_RegisterShaderNoMip( "gfx/menus/truejedi" );

	for ( n = 0; n < NUM_CROSSHAIRS; n++ ) {
		uiInfo.uiDC.Assets.crosshairShader[n] = trap->R_RegisterShaderNoMip( va( "gfx/2d/crosshair%c", 'a' + n ) );
	}
}

void _UI_DrawSides( float x, float y, float w, float h, float size ) {
	size *= uiInfo.uiDC.xscale;
	trap->R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom( float x, float y, float w, float h, float size ) {
	size *= uiInfo.uiDC.yscale;
	trap->R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

// Coordinates are 640*480 virtual values
void _UI_DrawRect( float x, float y, float width, float height, float size, const vector4 *color ) {
	trap->R_SetColor( color );

	_UI_DrawTopBottom( x, y, width, height, size );
	_UI_DrawSides( x, y, width, height, size );

	trap->R_SetColor( NULL );
}

void Text_PaintWithCursor( float x, float y, float scale, const vector4 *color, const char *text, int cursorPos,
	char cursor, int limit, int style, int iMenuFont, bool customFont )
{
	Text_Paint( x, y, scale, color, text, 0, limit, style, iMenuFont, customFont );

	// now print the cursor as well...
	char sTemp[1024];
	int iCopyCount = limit
		? std::min( (signed)strlen( text ), limit )
		: (signed)strlen( text );
	iCopyCount = std::min( iCopyCount, cursorPos );
	iCopyCount = std::min( static_cast<size_t>( iCopyCount ), sizeof(sTemp) );

	// copy text into temp buffer for pixel measure...
	strncpy( sTemp, text, iCopyCount );
	sTemp[iCopyCount] = '\0';

	const char *cursorStr = va( "%c", cursor );
	float iNextXpos = Text_Width( cursorStr, scale, iMenuFont, customFont );

	Text_Paint( x + iNextXpos, y, scale, color, cursorStr, 0, limit, style | ITEM_TEXTSTYLE_BLINK,
		iMenuFont, customFont
	);
}


// maxX param is initially an X limit, but is also used as feedback. 0 = text was clipped to fit within, else maxX = next pos
//
static void Text_Paint_Limit( float *maxX, float x, float y, float scale, const vector4 *color, const char *text,
	float adjust, int limit, int iMenuFont, bool customFont )
{
	float iPixelLen = Text_Width( text, scale, iMenuFont, customFont );
	if ( x + iPixelLen > *maxX ) {
		// whole text won't fit, so we need to print just the amount that does...
		//  Ok, this is slow and tacky, but only called occasionally, and it works...
		char sTemp[4096] = { 0 };	// lazy assumption
		const char *psText = text;
		char *psOut = &sTemp[0];
		char *psOutLastGood = psOut;
		unsigned int uiLetter;

		while ( *psText
			&& (x + Text_Width( sTemp, scale, iMenuFont, customFont ) <= *maxX)
			&& psOut < &sTemp[sizeof(sTemp)-1] )	// sanity
		{
			int iAdvanceCount;
			psOutLastGood = psOut;

			uiLetter = trap->R_AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, NULL );
			psText += iAdvanceCount;

			if ( uiLetter > 255 ) {
				*psOut++ = uiLetter >> 8;
				*psOut++ = uiLetter & 0xFF;
			}
			else {
				*psOut++ = uiLetter & 0xFF;
			}
		}
		*psOutLastGood = '\0';

		*maxX = 0;	// feedback
		Text_Paint( x, y, scale, color, sTemp, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont, customFont );
	}
	else {
		// whole text fits fine, so print it all...
		*maxX = x + iPixelLen;	// feedback the next position, as the caller expects
		Text_Paint( x, y, scale, color, text, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont, customFont );
	}
}

void UI_LoadNonIngame( void ) {
	const char *menuSet = UI_Cvar_VariableString( "ui_menuFilesMP" );
	if ( menuSet == NULL || menuSet[0] == '\0' ) {
		menuSet = "ui/jampmenus.txt";
	}
	UI_LoadMenus( menuSet, qfalse );
	uiInfo.inGameLoad = qfalse;
}

static void UI_BuildPlayerList( void ) {
	uiClientState_t	cs;
	int		n, count, team, team2;
	char	info[MAX_INFO_STRING];

	trap->GetClientState( &cs );
	trap->GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	team = atoi( Info_ValueForKey( info, "t" ) );
	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	for ( n = 0; n < count; n++ ) {
		trap->GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if ( info[0] ) {
			Q_strncpyz( uiInfo.playerNames[uiInfo.playerCount], Info_ValueForKey( info, "n" ), MAX_NETNAME );
			Q_CleanString( uiInfo.playerNames[uiInfo.playerCount], STRIP_COLOUR );
			uiInfo.playerIndexes[uiInfo.playerCount] = n;
			uiInfo.playerCount++;
			team2 = atoi( Info_ValueForKey( info, "t" ) );
			if ( team2 == team && n != uiInfo.playerNumber ) {
				Q_strncpyz( uiInfo.teamNames[uiInfo.myTeamCount], Info_ValueForKey( info, "n" ), MAX_NETNAME );
				Q_CleanString( uiInfo.teamNames[uiInfo.myTeamCount], STRIP_COLOUR );
				uiInfo.teamClientNums[uiInfo.myTeamCount] = n;
				uiInfo.myTeamCount++;
			}
		}
	}

	n = trap->Cvar_VariableValue( "cg_selectedPlayer" );
	if ( n < 0 || n > uiInfo.myTeamCount ) {
		n = 0;
	}


	if ( n < uiInfo.myTeamCount ) {
		trap->Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[n] );
	}
	else {
		trap->Cvar_Set( "cg_selectedPlayerName", "Everyone" );
	}

	if ( !team || team == TEAM_SPECTATOR ) {
		n = uiInfo.myTeamCount;
		trap->Cvar_Set( "cg_selectedPlayer", va( "%d", n ) );
		trap->Cvar_Set( "cg_selectedPlayerName", "N/A" );
	}
}

void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	char buf[256];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if ( Menu_Count() > 0 ) {
		switch ( menu ) {
		case UIMENU_NONE:
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
			return;

		case UIMENU_MAIN:
		{
			trap->Key_SetCatcher( KEYCATCH_UI );
			if ( uiInfo.inGameLoad )
				UI_LoadNonIngame();

			Menus_CloseAll();
			Menus_ActivateByName( "main" );
			trap->Cvar_VariableStringBuffer( "com_errorMessage", buf, sizeof(buf) );

			if ( buf[0] ) {
				if ( !ui_singlePlayerActive.integer )
					Menus_ActivateByName( "error_popmenu" );
				else
					trap->Cvar_Set( "com_errorMessage", "" );
			}
			return;
		}

		case UIMENU_TEAM:
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName( "team" );
			return;

		case UIMENU_POSTGAME:
			trap->Key_SetCatcher( KEYCATCH_UI );
			if ( uiInfo.inGameLoad )
				UI_LoadNonIngame();
			Menus_CloseAll();
			Menus_ActivateByName( "endofgame" );
			return;

		case UIMENU_INGAME:
			trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName( "ingame" );
			return;

		case UIMENU_PLAYERCONFIG:
			trap->Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName( "ingame_player" );
			UpdateForceUsed();
			return;

		case UIMENU_PLAYERFORCE:
			trap->Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName( "ingame_playerforce" );
			UpdateForceUsed();
			return;

		case UIMENU_SIEGEMESSAGE:
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "siege_popmenu" );
			return;

		case UIMENU_SIEGEOBJECTIVES:
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "ingame_siegeobjectives" );
			return;

		case UIMENU_VOICECHAT:
			//	if ( trap->Cvar_VariableValue( "g_gametype" ) < GT_TEAM )
			//		return;

			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "ingame_voicechat" );
			return;

		case UIMENU_CLOSEALL:
			Menus_CloseAll();
			return;

		case UIMENU_CLASSSEL:
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "ingame_siegeclass" );
			return;
		}
	}
}

void UI_DrawCenteredPic( qhandle_t image, int w, int h ) {
	float x, y;
	x = (SCREEN_WIDTH - w) / 2.0f;
	y = (SCREEN_HEIGHT - h) / 2.0f;
	UI_DrawHandlePic( x, y, w, h, image );
}

int frameCount = 0;
int startTime;

char parsedFPMessage[1024];

extern int FPMessageTime;

void Text_PaintCenter( float x, float y, float scale, const vector4 *color, const char *text, float adjust,
	int iMenuFont, bool customFont
);

const char *UI_GetStringEdString( const char *refSection, const char *refName ) {
	static char text[1024] = { 0 };

	trap->SE_GetStringTextString( va( "%s_%s", refSection, refName ), text, sizeof(text) );
	return text;
}

void UI_SetColor( const vector4 *rgba ) {
	trap->R_SetColor( rgba );
}

void UI_CleanupGhoul2( void );
void UI_FreeAllSpecies( void );

void UI_Shutdown( void ) {
	trap->LAN_SaveCachedServers();
	UI_CleanupGhoul2();

	UI_FreeAllSpecies();

#ifdef FAV_SERVERS
	JP_SaveFavServers();
#endif

	DeactivateCrashHandler();
}

char *defaultMenu = NULL;

char *GetMenuBuffer( const char *filename ) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "menu file not found: %s, using default\n", filename );
		return defaultMenu;
	}
	if ( len >= MAX_MENUFILE ) {
		trap->Print( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE );
		trap->FS_Close( f );
		return defaultMenu;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );
	//COM_Compress(buf);
	return buf;
}

qboolean Asset_Parse( int handle ) {
	pc_token_t token;
	const char *s = NULL;

	if ( !trap->PC_ReadToken( handle, &token ) )
		return qfalse;
	if ( Q_stricmp( token.string, "{" ) != 0 ) {
		return qfalse;
	}

	while ( 1 ) {
		memset( &token, 0, sizeof(pc_token_t) );

		if ( !trap->PC_ReadToken( handle, &token ) )
			break;

		if ( Q_stricmp( token.string, "}" ) == 0 ) {
			return qtrue;
		}

#if 0
		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.textFont);
			uiInfo.uiDC.Assets.qhMediumFont = trap->R_RegisterFont(token.string);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmallFont = trap->R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "small2Font") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmall2Font = trap->R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "bigFont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.bigFont);
			uiInfo.uiDC.Assets.qhBigFont = trap->R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "monoFont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.bigFont);
			uiInfo.uiDC.Assets.japp.fontMono = trap->R_RegisterFont(token.string);
			continue;
		}
#endif

		if ( Q_stricmp( token.string, "cursor" ) == 0 ) {
			if ( !PC_String_Parse( handle, &uiInfo.uiDC.Assets.cursorStr ) ) {
				Com_Printf( S_COLOR_YELLOW, "Bad 1st parameter for keyword 'cursor'" );
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = trap->R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr );
			continue;
		}

		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 ) {
			if ( !trap->PC_ReadToken( handle, &token ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = trap->R_RegisterShaderNoMip( token.string );
			continue;
		}

		// enterMenuSound
		if ( Q_stricmp( token.string, "menuEnterSound" ) == 0 ) {
			if ( !trap->PC_ReadToken( handle, &token ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp( token.string, "menuExitSound" ) == 0 ) {
			if ( !trap->PC_ReadToken( handle, &token ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp( token.string, "itemFocusSound" ) == 0 ) {
			if ( !trap->PC_ReadToken( handle, &token ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp( token.string, "menuBuzzSound" ) == 0 ) {
			if ( !trap->PC_ReadToken( handle, &token ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap->S_RegisterSound( token.string );
			continue;
		}

		if ( Q_stricmp( token.string, "fadeClamp" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeClamp ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeCycle" ) == 0 ) {
			if ( !PC_Int_Parse( handle, &uiInfo.uiDC.Assets.fadeCycle ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeAmount" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeAmount ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowX" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowX ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowY" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowY ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowColor" ) == 0 ) {
			if ( !PC_Color_Parse( handle, &uiInfo.uiDC.Assets.shadowColor ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor.a;
			continue;
		}

		if ( Q_stricmp( token.string, "moveRollSound" ) == 0 ) {
			if ( trap->PC_ReadToken( handle, &token ) ) {
				uiInfo.uiDC.Assets.moveRollSound = trap->S_RegisterSound( token.string );
			}
			continue;
		}

		if ( Q_stricmp( token.string, "moveJumpSound" ) == 0 ) {
			if ( trap->PC_ReadToken( handle, &token ) ) {
				uiInfo.uiDC.Assets.moveJumpSound = trap->S_RegisterSound( token.string );
			}

			continue;
		}

		s = "datapadmoveSaberSound";
		if ( !Q_stricmpn( token.string, s, strlen( s ) ) ) {
			int i = '0' - s[strlen( s )];
			if ( i >= 0 && i <= 5 && trap->PC_ReadToken( handle, &token ) )
				uiInfo.uiDC.Assets.datapadmoveSaberSound[i] = trap->S_RegisterSound( token.string );

			continue;
		}

		// precaching various sound files used in the menus
		if ( Q_stricmp( token.string, "precacheSound" ) == 0 ) {
			const char *tempStr;
			if ( PC_Script_Parse( handle, &tempStr ) ) {
				char *soundFile;
				do {
					soundFile = COM_ParseExt( &tempStr, qfalse );
					if ( soundFile[0] != 0 && soundFile[0] != ';' ) {
						trap->S_RegisterSound( soundFile );
					}
				} while ( soundFile[0] );
			}
			continue;
		}
	}
	return qfalse;
}

void UI_Report( void ) {
	String_Report();
	//	Font_Report();
}

void UI_ParseMenu( const char *menuFile ) {
	int handle;
	pc_token_t token;

	//Com_Printf("Parsing menu file: %s\n", menuFile);

	handle = trap->PC_LoadSource( menuFile );
	if ( !handle ) {
		return;
	}

	while ( 1 ) {
		memset( &token, 0, sizeof(pc_token_t) );
		if ( !trap->PC_ReadToken( handle, &token ) ) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 ) {
			if ( Asset_Parse( handle ) ) {
				continue;
			}
			else {
				break;
			}
		}

		if ( Q_stricmp( token.string, "menudef" ) == 0 ) {
			// start a new menu
			Menu_New( handle );
		}
	}
	trap->PC_FreeSource( handle );
}

qboolean Load_Menu( int handle ) {
	pc_token_t token;

	if ( !trap->PC_ReadToken( handle, &token ) )
		return qfalse;
	if ( token.string[0] != '{' ) {
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap->PC_ReadToken( handle, &token ) )
			break;

		if ( token.string[0] == '\0' )
			return qfalse;

		if ( token.string[0] == '}' )
			return qtrue;

		UI_ParseMenu( token.string );
	}
	return qfalse;
}

void UI_LoadMenus( const char *menuFile, qboolean reset ) {
	pc_token_t token;
	int handle;
	//	int start = trap->Milliseconds();

	trap->PC_LoadGlobalDefines( "ui/jamp/menudef.h" );

	handle = trap->PC_LoadSource( menuFile );
	if ( !handle ) {
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		handle = trap->PC_LoadSource( "ui/jampmenus.txt" );
		if ( !handle ) {
			trap->Error( ERR_DROP, S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!\n" );
		}
	}

	if ( reset ) {
		Menu_Reset();
	}

	while ( 1 ) {
		if ( !trap->PC_ReadToken( handle, &token ) )
			break;
		if ( token.string[0] == 0 || token.string[0] == '}' ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token.string, "loadmenu" ) == 0 ) {
			if ( Load_Menu( handle ) ) {
				continue;
			}
			else {
				break;
			}
		}
	}

	//	Com_Printf("UI menu load time = %d milli seconds\n", trap->Milliseconds() - start);

	trap->PC_FreeSource( handle );

	trap->PC_RemoveAllGlobalDefines();
}

void UI_Load( void ) {
	const char *menuSet;
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();

	if ( menu && menu->window.name ) {
		strcpy( lastName, menu->window.name );
	}
	else {
		lastName[0] = 0;
	}

	if ( uiInfo.inGameLoad ) {
		menuSet = "ui/jampingame.txt";
	}
	else {
		menuSet = UI_Cvar_VariableString( "ui_menuFilesMP" );
	}
	if ( menuSet == NULL || menuSet[0] == '\0' ) {
		menuSet = "ui/jampmenus.txt";
	}

	String_Init();

	UI_LoadArenas();
	UI_LoadBots();

	UI_LoadMenus( menuSet, qtrue );
	Menus_CloseAll();
	Menus_ActivateByName( lastName );
}

char	sAll[15] = { 0 };
char	sJediAcademy[30] = { 0 };
const char *UI_FilterDescription( int value ) {
	if ( value <= 0 || value > uiInfo.modCount ) {
		return sAll;
	}

	return uiInfo.modList[value - 1].modDescr;
}

const char *UI_FilterDir( int value ) {
	if ( value <= 0 || value > uiInfo.modCount ) {
		return "";
	}

	return uiInfo.modList[value - 1].modName;
}

static const char *handicapValues[] = { "None", "95", "90", "85", "80", "75", "70", "65", "60", "55", "50", "45", "40", "35", "30", "25", "20", "15", "10", "5", NULL };

static void UI_DrawHandicap( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	int h = Q_clampi( 5, trap->Cvar_VariableValue( "handicap" ), 100 );
	int i = 20 - h / 5;

	Text_Paint( rect->x, rect->y, scale, color, handicapValues[i], 0, 0, textStyle, iMenuFont, customFont );
}

static void UI_SetCapFragLimits( qboolean uiVars ) {
	int cap = 5;
	int frag = 10;

	if ( uiVars ) {
		trap->Cvar_Set( "ui_captureLimit", va( "%d", cap ) );
		trap->Cvar_Set( "ui_fragLimit", va( "%d", frag ) );
	}
	else {
		trap->Cvar_Set( "capturelimit", va( "%d", cap ) );
		trap->Cvar_Set( "fraglimit", va( "%d", frag ) );
	}
}

// ui_gameType assumes gametype 0 is -1 ALL and will not show
static void UI_DrawGameType( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	Text_Paint( rect->x, rect->y, scale, color, BG_GetGametypeString( ui_gameType.integer ), 0, 0, textStyle, iMenuFont,
		customFont
	);
}

static void UI_DrawNetGameType( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	if ( ui_netGameType.integer < 0 || ui_netGameType.integer >= GT_MAX_GAME_TYPE ) {
		trap->Cvar_Set( "ui_netGameType", "0" );
		trap->Cvar_Update( &ui_netGameType );
	}
	Text_Paint( rect->x, rect->y, scale, color, BG_GetGametypeString( ui_netGameType.integer ), 0, 0, textStyle,
		iMenuFont, customFont
	);
}

static void UI_DrawAutoSwitch( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	int switchVal = trap->Cvar_VariableValue( "cg_autoswitch" );
	const char *switchString = "AUTOSWITCH1";
	const char *stripString = NULL;

	switch ( switchVal ) {
	case 2:
		switchString = "AUTOSWITCH2";
		break;
	case 3:
		switchString = "AUTOSWITCH3";
		break;
	case 0:
		switchString = "AUTOSWITCH0";
		break;
	default:
		break;
	}

	stripString = UI_GetStringEdString( "MP_INGAME", (char *)switchString );

	if ( stripString ) {
		Text_Paint( rect->x, rect->y, scale, color, stripString, 0, 0, textStyle, iMenuFont, customFont );
	}
}

static void UI_DrawJoinGameType( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	if ( ui_joinGameType.integer < -1 || ui_joinGameType.integer >= GT_MAX_GAME_TYPE ) {
		trap->Cvar_Set( "ui_joinGameType", "-1" );
		trap->Cvar_Update( &ui_joinGameType );
	}

	if ( ui_joinGameType.integer == -1 ) {
		Text_Paint( rect->x, rect->y, scale, color, "Any", 0, 0, textStyle, iMenuFont, customFont );
	}
	else {
		Text_Paint( rect->x, rect->y, scale, color, BG_GetGametypeString( ui_joinGameType.integer ), 0, 0, textStyle,
			iMenuFont, customFont
		);
	}
}

static int UI_TeamIndexFromName( const char *name ) {
	int i;

	if ( name && *name ) {
		for ( i = 0; i < uiInfo.teamCount; i++ ) {
			if ( Q_stricmp( name, uiInfo.teamList[i].teamName ) == 0 ) {
				return i;
			}
		}
	}

	return 0;
}

static void UI_DrawSkill( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	int i;
	i = trap->Cvar_VariableValue( "g_spSkill" );
	if ( i < 1 || i >( signed )numSkillLevels ) {
		i = 1;
	}
	Text_Paint( rect->x, rect->y, scale, color, (char *)UI_GetStringEdString( "MP_INGAME", (char *)skillLevels[i - 1] ),
		0, 0, textStyle, iMenuFont, customFont
	);
}

static void UI_DrawGenericNum( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int val, int min,
	int max, int type, int iMenuFont, bool customFont )
{
	char s[32];
	Com_sprintf( s, sizeof(s), "%i", Q_clampi( min, val, max ) );
	Text_Paint( rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont, customFont );
}

static void UI_DrawForceMastery( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int val, int min,
	int max, int iMenuFont, bool customFont )
{
	int i;
	char *s;

	i = val;
	if ( i < min ) {
		i = min;
	}
	if ( i > max ) {
		i = max;
	}

	s = (char *)UI_GetStringEdString( "MP_INGAME", forceMasteryLevels[i] );
	Text_Paint( rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont, customFont );
}

static void UI_DrawSkinColor( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int val, int min,
	int max, int iMenuFont, bool customFont )
{
	char s[256];

	switch ( val ) {
	case TEAM_RED:
		trap->SE_GetStringTextString( "MENUS_TEAM_RED", s, sizeof(s) );
		//		Com_sprintf(s, sizeof(s), "Red\0");
		break;
	case TEAM_BLUE:
		trap->SE_GetStringTextString( "MENUS_TEAM_BLUE", s, sizeof(s) );
		//		Com_sprintf(s, sizeof(s), "Blue\0");
		break;
	default:
		trap->SE_GetStringTextString( "MENUS_DEFAULT", s, sizeof(s) );
		//		Com_sprintf(s, sizeof(s), "Default\0");
		break;
	}

	Text_Paint( rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont, customFont );
}

static void UI_DrawForceSide( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int val, int min,
	int max, int iMenuFont, bool customFont )
{
	char s[256];
	menuDef_t *menu;

	char info[MAX_INFO_VALUE];

	info[0] = '\0';
	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

	if ( atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ) ) {
		switch ( (int)(trap->Cvar_VariableValue( "ui_myteam" )) ) {
		case TEAM_RED:
			uiForceSide = FORCESIDE_DARK;
			VectorSet( (vector3 *)color, 0.2f, 0.2f, 0.2f );
			break;
		case TEAM_BLUE:
			uiForceSide = FORCESIDE_LIGHT;
			VectorSet( (vector3 *)color, 0.2f, 0.2f, 0.2f );
			break;
		default:
			break;
		}
	}

	if ( val == FORCESIDE_LIGHT ) {
		trap->SE_GetStringTextString( "MENUS_FORCEDESC_LIGHT", s, sizeof(s) );
		menu = Menus_FindByName( "forcealloc" );
		if ( menu ) {
			Menu_ShowItemByName( menu, "lightpowers", qtrue );
			Menu_ShowItemByName( menu, "darkpowers", qfalse );
			Menu_ShowItemByName( menu, "darkpowers_team", qfalse );

			Menu_ShowItemByName( menu, "lightpowers_team", qtrue );//(ui_gameType.integer >= GT_TEAM));

		}
		menu = Menus_FindByName( "ingame_playerforce" );
		if ( menu ) {
			Menu_ShowItemByName( menu, "lightpowers", qtrue );
			Menu_ShowItemByName( menu, "darkpowers", qfalse );
			Menu_ShowItemByName( menu, "darkpowers_team", qfalse );

			Menu_ShowItemByName( menu, "lightpowers_team", qtrue );//(ui_gameType.integer >= GT_TEAM));
		}
	}
	else {
		trap->SE_GetStringTextString( "MENUS_FORCEDESC_DARK", s, sizeof(s) );
		menu = Menus_FindByName( "forcealloc" );
		if ( menu ) {
			Menu_ShowItemByName( menu, "lightpowers", qfalse );
			Menu_ShowItemByName( menu, "lightpowers_team", qfalse );
			Menu_ShowItemByName( menu, "darkpowers", qtrue );

			Menu_ShowItemByName( menu, "darkpowers_team", qtrue );//(ui_gameType.integer >= GT_TEAM));
		}
		menu = Menus_FindByName( "ingame_playerforce" );
		if ( menu ) {
			Menu_ShowItemByName( menu, "lightpowers", qfalse );
			Menu_ShowItemByName( menu, "lightpowers_team", qfalse );
			Menu_ShowItemByName( menu, "darkpowers", qtrue );

			Menu_ShowItemByName( menu, "darkpowers_team", qtrue );//(ui_gameType.integer >= GT_TEAM));
		}
	}

	Text_Paint( rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont, customFont );
}

static qboolean UI_AllForceDisabled( int force ) {
	int i;

	if ( force ) {
		for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
			if ( !(force & (1 << i)) ) {
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

qboolean UI_TrueJediEnabled( void ) {
	char	info[MAX_INFO_STRING];
	int		gametype = 0, disabledForce = 0, trueJedi = 0;
	qboolean saberOnly = qfalse, allForceDisabled = qfalse;

	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

	//already have serverinfo at this point for stuff below. Don't bother trying to use ui_forcePowerDisable.
	//if (ui_forcePowerDisable.integer)
	//if (atoi(Info_ValueForKey(info, "g_forcePowerDisable")))
	disabledForce = atoi( Info_ValueForKey( info, "g_forcePowerDisable" ) );
	allForceDisabled = UI_AllForceDisabled( disabledForce );
	gametype = atoi( Info_ValueForKey( info, "g_gametype" ) );
	assert( gametype == ui_gameType.integer );
	saberOnly = BG_HasSetSaberOnly( info );

	if ( gametype == GT_HOLOCRON
		|| gametype == GT_JEDIMASTER
		|| saberOnly
		|| allForceDisabled )
	{
		trueJedi = 0;
	}
	else {
		trueJedi = atoi( Info_ValueForKey( info, "g_jediVmerc" ) );
	}
	return (trueJedi != 0);
}

static void UI_DrawJediNonJedi( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int val, int min,
	int max, int iMenuFont, bool customFont )
{
	int i;
	char s[256];
	//menuDef_t *menu;

	char info[MAX_INFO_VALUE];

	i = val;
	if ( i < min || i > max ) {
		i = min;
	}

	info[0] = '\0';
	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

	if ( !UI_TrueJediEnabled() ) {//true jedi mode is not on, do not draw this button type
		return;
	}

	if ( val == FORCE_NONJEDI )
		trap->SE_GetStringTextString( "MENUS_NO", s, sizeof(s) );
	else
		trap->SE_GetStringTextString( "MENUS_YES", s, sizeof(s) );

	Text_Paint( rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont, customFont );
}

static void UI_DrawTeamName( rectDef_t *rect, float scale, const vector4 *color, qboolean blue, int textStyle,
	int iMenuFont, bool customFont )
{
	int i;
	i = UI_TeamIndexFromName( UI_Cvar_VariableString( (blue) ? "ui_blueTeam" : "ui_redTeam" ) );
	if ( i >= 0 && i < uiInfo.teamCount ) {
		Text_Paint( rect->x, rect->y, scale, color,
			va( "%s: %s", (blue) ? "Blue" : "Red", uiInfo.teamList[i].teamName ), 0, 0, textStyle, iMenuFont, customFont
		);
	}
}

static void UI_DrawTeamMember( rectDef_t *rect, float scale, const vector4 *color, qboolean blue, int num,
	int textStyle, int iMenuFont, bool customFont )
{
	// 0 - None
	// 1 - Human
	// 2..NumCharacters - Bot
	int value = trap->Cvar_VariableValue( va( blue ? "ui_blueteam%i" : "ui_redteam%i", num ) );
	const char *text;
	int maxcl = trap->Cvar_VariableValue( "sv_maxClients" );
	vector4 finalColor;
	int numval = num;

	numval *= 2;

	if ( blue ) {
		numval -= 1;
	}

	finalColor.r = color->r;
	finalColor.g = color->g;
	finalColor.b = color->b;
	finalColor.a = color->a;

	if ( numval > maxcl ) {
		finalColor.r *= 0.5f;
		finalColor.g *= 0.5f;
		finalColor.b *= 0.5f;

		value = -1;
	}

	if ( ui_netGameType.integer == GT_SIEGE ) {
		if ( value > 1 ) {
			value = 1;
		}
	}

	if ( value <= 1 ) {
		if ( value == -1 ) {
			//text = "Closed";
			text = UI_GetStringEdString( "MENUS", "CLOSED" );
		}
		else {
			//text = "Human";
			text = UI_GetStringEdString( "MENUS", "HUMAN" );
		}
	}
	else {
		value -= 2;
		if ( value >= UI_GetNumBots() ) {
			value = 1;
		}
		text = UI_GetBotNameByNumber( value );
	}

	Text_Paint( rect->x, rect->y, scale, &finalColor, text, 0, 0, textStyle, iMenuFont, customFont );
}

static void UI_DrawMapPreview( rectDef_t *rect, float scale, const vector4 *color, qboolean net ) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if ( map < 0 || map > uiInfo.mapCount ) {
		if ( net ) {
			trap->Cvar_Set( "ui_currentNetMap", "0" );
			trap->Cvar_Update( &ui_currentNetMap );
		}
		else {
			trap->Cvar_Set( "ui_currentMap", "0" );
			trap->Cvar_Update( &ui_currentMap );
		}
		map = 0;
	}

	if ( uiInfo.mapList[map].levelShot == -1 ) {
		uiInfo.mapList[map].levelShot = trap->R_RegisterShaderNoMip( uiInfo.mapList[map].imageName );
	}

	if ( uiInfo.mapList[map].levelShot > 0 ) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[map].levelShot );
	}
	else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap->R_RegisterShaderNoMip( "menu/art/unknownmap_mp" ) );
	}
}

static void UI_DrawMapCinematic( rectDef_t *rect, float scale, const vector4 *color, qboolean net ) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if ( map < 0 || map > uiInfo.mapCount ) {
		if ( net ) {
			trap->Cvar_Set( "ui_currentNetMap", "0" );
			trap->Cvar_Update( &ui_currentNetMap );
		}
		else {
			trap->Cvar_Set( "ui_currentMap", "0" );
			trap->Cvar_Update( &ui_currentMap );
		}
		map = 0;
	}

	if ( uiInfo.mapList[map].cinematic >= -1 ) {
		if ( uiInfo.mapList[map].cinematic == -1 ) {
			uiInfo.mapList[map].cinematic = trap->CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[map].mapLoadName ), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}
		if ( uiInfo.mapList[map].cinematic >= 0 ) {
			trap->CIN_RunCinematic( uiInfo.mapList[map].cinematic );
			trap->CIN_SetExtents( uiInfo.mapList[map].cinematic, rect->x, rect->y, rect->w, rect->h );
			trap->CIN_DrawCinematic( uiInfo.mapList[map].cinematic );
		}
		else {
			uiInfo.mapList[map].cinematic = -2;
		}
	}
	else {
		UI_DrawMapPreview( rect, scale, color, net );
	}
}

static void UI_SetForceDisabled( int force ) {
	int i = 0;

	if ( force ) {
		while ( i < NUM_FORCE_POWERS ) {
			if ( force & (1 << i) ) {
				uiForcePowersDisabled[i] = qtrue;

				if ( i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE ) {
					uiForcePowersRank[i] = 0;
				}
				else {
					if ( i == FP_LEVITATION ) {
						uiForcePowersRank[i] = 1;
					}
					else {
						uiForcePowersRank[i] = 3;
					}
				}
			}
			else {
				uiForcePowersDisabled[i] = qfalse;
			}
			i++;
		}
	}
	else {
		i = 0;

		while ( i < NUM_FORCE_POWERS ) {
			uiForcePowersDisabled[i] = qfalse;
			i++;
		}
	}
}
// The game type on create server has changed - make the HUMAN/BOTS fields active
void UpdateBotButtons( void ) {
	menuDef_t *menu;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return;
	}

	if ( ui_netGameType.integer == GT_SIEGE ) {
		Menu_ShowItemByName( menu, "humanbotfield", qfalse );
		Menu_ShowItemByName( menu, "humanbotnonfield", qtrue );
	}
	else {
		Menu_ShowItemByName( menu, "humanbotfield", qtrue );
		Menu_ShowItemByName( menu, "humanbotnonfield", qfalse );
	}

}

void UpdateForceStatus( void ) {
	menuDef_t *menu;

	// Currently we don't make a distinction between those that wish to play Jedi of lower than maximum skill.
	/*	if (ui_forcePowerDisable.integer)
		{
		uiForceRank = 0;
		uiForceAvailable = 0;
		uiForceUsed = 0;
		}
		else
		{
		uiForceRank = uiMaxRank;
		uiForceUsed = 0;
		uiForceAvailable = forceMasteryPoints[uiForceRank];
		}
		*/
	menu = Menus_FindByName( "ingame_player" );
	if ( menu ) {
		char	info[MAX_INFO_STRING];
		int		disabledForce = 0;
		qboolean trueJedi = qfalse, allForceDisabled = qfalse;

		trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

		//already have serverinfo at this point for stuff below. Don't bother trying to use ui_forcePowerDisable.
		//if (ui_forcePowerDisable.integer)
		//if (atoi(Info_ValueForKey(info, "g_forcePowerDisable")))
		disabledForce = atoi( Info_ValueForKey( info, "g_forcePowerDisable" ) );
		allForceDisabled = UI_AllForceDisabled( disabledForce );
		trueJedi = UI_TrueJediEnabled();

		if ( !trueJedi || allForceDisabled ) {
			Menu_ShowItemByName( menu, "jedinonjedi", qfalse );
		}
		else {
			Menu_ShowItemByName( menu, "jedinonjedi", qtrue );
		}
		if ( allForceDisabled == qtrue || (trueJedi && uiJediNonJedi == FORCE_NONJEDI) ) {	// No force stuff
			Menu_ShowItemByName( menu, "noforce", qtrue );
			Menu_ShowItemByName( menu, "yesforce", qfalse );
			// We don't want the saber explanation to say "configure saber attack 1" since we can't.
			Menu_ShowItemByName( menu, "sabernoneconfigme", qfalse );
		}
		else {
			UI_SetForceDisabled( disabledForce );
			Menu_ShowItemByName( menu, "noforce", qfalse );
			Menu_ShowItemByName( menu, "yesforce", qtrue );
		}

		//Moved this to happen after it's done with force power disabling stuff
		if ( uiForcePowersRank[FP_SABER_OFFENSE] > 0 || ui_freeSaber.integer ) {	// Show lightsaber stuff.
			Menu_ShowItemByName( menu, "nosaber", qfalse );
			Menu_ShowItemByName( menu, "yessaber", qtrue );
		}
		else {
			Menu_ShowItemByName( menu, "nosaber", qtrue );
			Menu_ShowItemByName( menu, "yessaber", qfalse );
		}

		// The leftmost button should be "apply" unless you are in spectator, where you can join any team.
		if ( (int)(trap->Cvar_VariableValue( "ui_myteam" )) != TEAM_SPECTATOR ) {
			Menu_ShowItemByName( menu, "playerapply", qtrue );
			Menu_ShowItemByName( menu, "playerforcejoin", qfalse );
			Menu_ShowItemByName( menu, "playerforcered", qtrue );
			Menu_ShowItemByName( menu, "playerforceblue", qtrue );
			Menu_ShowItemByName( menu, "playerforcespectate", qtrue );
		}
		else {
			// Set or reset buttons based on choices
			if ( atoi( Info_ValueForKey( info, "g_gametype" ) ) >= GT_TEAM ) {	// This is a team-based game.
				Menu_ShowItemByName( menu, "playerforcespectate", qtrue );

				// This is disabled, always show both sides from spectator.
				if ( atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ) ) {	// Show red or blue based on what side is chosen.
					if ( uiForceSide == FORCESIDE_LIGHT ) {
						Menu_ShowItemByName( menu, "playerforcered", qfalse );
						Menu_ShowItemByName( menu, "playerforceblue", qtrue );
					}
					else if ( uiForceSide == FORCESIDE_DARK ) {
						Menu_ShowItemByName( menu, "playerforcered", qtrue );
						Menu_ShowItemByName( menu, "playerforceblue", qfalse );
					}
					else {
						Menu_ShowItemByName( menu, "playerforcered", qtrue );
						Menu_ShowItemByName( menu, "playerforceblue", qtrue );
					}
				}
				else {
					Menu_ShowItemByName( menu, "playerforcered", qtrue );
					Menu_ShowItemByName( menu, "playerforceblue", qtrue );
				}
			}
			else {
				Menu_ShowItemByName( menu, "playerforcered", qfalse );
				Menu_ShowItemByName( menu, "playerforceblue", qfalse );
			}

			Menu_ShowItemByName( menu, "playerapply", qfalse );
			Menu_ShowItemByName( menu, "playerforcejoin", qtrue );
			Menu_ShowItemByName( menu, "playerforcespectate", qtrue );
		}
	}


	if ( !UI_TrueJediEnabled() ) {// Take the current team and force a skin color based on it.
		char	info[MAX_INFO_STRING];

		switch ( (int)(trap->Cvar_VariableValue( "ui_myteam" )) ) {
		case TEAM_RED:
			uiSkinColor = TEAM_RED;
			break;
		case TEAM_BLUE:
			uiSkinColor = TEAM_BLUE;
			break;
		default:
			trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

			if ( atoi( Info_ValueForKey( info, "g_gametype" ) ) >= GT_TEAM ) {
				uiSkinColor = TEAM_FREE;
			}
			else	// A bit of a hack so non-team games will remember which skin set you chose in the player menu
			{
				uiSkinColor = uiHoldSkinColor;
			}
			break;
		}
	}
}

static void UI_DrawNetSource( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	if ( ui_netSource.integer < 0 || ui_netSource.integer > numNetSources ) {
		trap->Cvar_Set( "ui_netSource", "0" );
		trap->Cvar_Update( &ui_netSource );
	}

	trap->SE_GetStringTextString( "MENUS_SOURCE", holdSPString, sizeof(holdSPString) );
	Text_Paint( rect->x, rect->y, scale, color, va( "%s %s", holdSPString, GetNetSourceString( ui_netSource.integer ) ),
		0, 0, textStyle, iMenuFont, customFont
	);
}

static void UI_DrawNetMapPreview( rectDef_t *rect, float scale, const vector4 *color ) {

	if ( uiInfo.serverStatus.currentServerPreview > 0 ) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview );
	}
	else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap->R_RegisterShaderNoMip( "menu/art/unknownmap_mp" ) );
	}
}

static void UI_DrawNetMapCinematic( rectDef_t *rect, float scale, const vector4 *color ) {
	if ( ui_currentNetMap.integer < 0 || ui_currentNetMap.integer > uiInfo.mapCount ) {
		trap->Cvar_Set( "ui_currentNetMap", "0" );
		trap->Cvar_Update( &ui_currentNetMap );
	}

	if ( uiInfo.serverStatus.currentServerCinematic >= 0 ) {
		trap->CIN_RunCinematic( uiInfo.serverStatus.currentServerCinematic );
		trap->CIN_SetExtents( uiInfo.serverStatus.currentServerCinematic, rect->x, rect->y, rect->w, rect->h );
		trap->CIN_DrawCinematic( uiInfo.serverStatus.currentServerCinematic );
	}
	else {
		UI_DrawNetMapPreview( rect, scale, color );
	}
}

static void UI_DrawNetFilter( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	trap->SE_GetStringTextString( "MENUS_GAME", holdSPString, sizeof(holdSPString) );

	Text_Paint( rect->x, rect->y, scale, color, va( "%s %s", holdSPString,
		UI_FilterDescription( ui_serverFilterType.integer ) ), 0, 0, textStyle, iMenuFont, customFont
	);
}

static const char *UI_AIFromName( const char *name ) {
	int j;
	for ( j = 0; j < uiInfo.aliasCount; j++ ) {
		if ( Q_stricmp( uiInfo.aliasList[j].name, name ) == 0 ) {
			return uiInfo.aliasList[j].ai;
		}
	}
	return "Kyle";
}

/*
static qboolean updateOpponentModel = qtrue;
static void UI_DrawOpponent(rectDef_t *rect) {
static playerInfo_t info2;
char model[MAX_QPATH];
char headmodel[MAX_QPATH];
char team[256];
vector3	viewangles;
vector3	moveangles;

if (updateOpponentModel) {

strcpy(model, UI_Cvar_VariableString("ui_opponentModel"));
strcpy(headmodel, UI_Cvar_VariableString("ui_opponentModel"));
team[0] = '\0';

memset( &info2, 0, sizeof(playerInfo_t) );
viewangles.yaw   = 180 - 10;
viewangles.pitch = 0;
viewangles.roll  = 0;
VectorClear( moveangles );
UI_PlayerInfo_SetModel( &info2, model, headmodel, "");
UI_PlayerInfo_SetInfo( &info2, TORSO_WEAPONREADY3, TORSO_WEAPONREADY3, viewangles, vec3_origin, WP_BRYAR_PISTOL, qfalse );
UI_RegisterClientModelname( &info2, model, headmodel, team);
updateOpponentModel = qfalse;
}

UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info2, uiInfo.uiDC.realTime / 2);

}
*/

static void UI_DrawAllMapsSelection( rectDef_t *rect, float scale, const vector4 *color, int textStyle, qboolean net,
	int iMenuFont, bool customFont )
{
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if ( map >= 0 && map < uiInfo.mapCount ) {
		Text_Paint( rect->x, rect->y, scale, color, uiInfo.mapList[map].mapName, 0, 0, textStyle, iMenuFont, customFont );
	}
}

static void UI_DrawOpponentName( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	Text_Paint( rect->x, rect->y, scale, color, UI_Cvar_VariableString( "ui_opponentName" ), 0, 0, textStyle, iMenuFont,
		customFont
	);
}

static int UI_OwnerDrawWidth( int ownerDraw, float scale ) {
	int i, h, value, findex, iUse = 0;
	const char *text;
	const char *s = NULL;


	switch ( ownerDraw ) {
	case UI_HANDICAP:
		h = Q_clampi( 5, trap->Cvar_VariableValue( "handicap" ), 100 );
		i = 20 - h / 5;
		s = handicapValues[i];
		break;
	case UI_SKIN_COLOR:
		switch ( uiSkinColor ) {
		case TEAM_RED:
			s = UI_GetStringEdString( "MENUS", "TEAM_RED" );
			break;
		case TEAM_BLUE:
			s = UI_GetStringEdString( "MENUS", "TEAM_BLUE" );
			break;
		default:
			s = UI_GetStringEdString( "MENUS", "DEFAULT" );
			break;
		}
		break;
	case UI_FORCE_SIDE:
		i = uiForceSide;
		if ( i < 1 || i > 2 ) {
			i = 1;
		}

		if ( i == FORCESIDE_LIGHT ) {
			s = UI_GetStringEdString( "MENUS", "FORCEDESC_LIGHT" );
		}
		else {
			s = UI_GetStringEdString( "MENUS", "FORCEDESC_DARK" );
		}
		break;
	case UI_JEDI_NONJEDI:
		i = uiJediNonJedi;
		if ( i < 0 || i > 1 ) {
			i = 0;
		}

		if ( i == FORCE_NONJEDI ) {
			s = UI_GetStringEdString( "MENUS", "NO" );
		}
		else {
			s = UI_GetStringEdString( "MENUS", "YES" );
		}
		break;
	case UI_FORCE_RANK:
		i = uiForceRank;
		if ( i < 1 || i > MAX_FORCE_RANK ) {
			i = 1;
		}

		s = UI_GetStringEdString( "MP_INGAME", forceMasteryLevels[i] );
		break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:
		findex = (ownerDraw - UI_FORCE_RANK) - 1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		i = uiForcePowersRank[findex];

		if ( i < 0 || i > NUM_FORCE_POWER_LEVELS - 1 ) {
			i = 0;
		}

		s = va( "%i", uiForcePowersRank[findex] );
		break;
	case UI_GAMETYPE:
		s = BG_GetGametypeString( ui_gameType.integer );
		break;
	case UI_SKILL:
		i = trap->Cvar_VariableValue( "g_spSkill" );
		if ( i < 1 || i > (signed)numSkillLevels ) {
			i = 1;
		}
		s = UI_GetStringEdString( "MP_INGAME", (char *)skillLevels[i - 1] );
		break;
	case UI_BLUETEAMNAME:
		i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_blueTeam" ) );
		if ( i >= 0 && i < uiInfo.teamCount ) {
			s = va( "%s: %s", UI_GetStringEdString( "MENUS", "TEAM_BLUE" ), uiInfo.teamList[i].teamName );
		}
		break;
	case UI_REDTEAMNAME:
		i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_redTeam" ) );
		if ( i >= 0 && i < uiInfo.teamCount ) {
			s = va( "%s: %s", UI_GetStringEdString( "MENUS", "TEAM_RED" ), uiInfo.teamList[i].teamName );
		}
		break;
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
	case UI_BLUETEAM6:
	case UI_BLUETEAM7:
	case UI_BLUETEAM8:
		if ( ownerDraw <= UI_BLUETEAM5 )
			iUse = ownerDraw - UI_BLUETEAM1 + 1;
		else
			iUse = ownerDraw - 274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines

		value = trap->Cvar_VariableValue( va( "ui_blueteam%i", iUse ) );
		if ( value <= 1 )
			text = "Human";
		else {
			value -= 2;
			if ( value >= uiInfo.aliasCount )
				value = 1;
			text = uiInfo.aliasList[value].name;
		}
		s = va( "%i. %s", iUse, text );
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
	case UI_REDTEAM6:
	case UI_REDTEAM7:
	case UI_REDTEAM8:
		if ( ownerDraw <= UI_REDTEAM5 )
			iUse = ownerDraw - UI_REDTEAM1 + 1;
		else
			iUse = ownerDraw - 277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines

		value = trap->Cvar_VariableValue( va( "ui_redteam%i", iUse ) );
		if ( value <= 1 )
			text = "Human";
		else {
			value -= 2;
			if ( value >= uiInfo.aliasCount )
				value = 1;
			text = uiInfo.aliasList[value].name;
		}
		s = va( "%i. %s", iUse, text );
		break;

	case UI_NETSOURCE:
		if ( ui_netSource.integer < 0 || ui_netSource.integer > numNetSources ) {
			trap->Cvar_Set( "ui_netSource", "0" );
			trap->Cvar_Update( &ui_netSource );
		}
		trap->SE_GetStringTextString( "MENUS_SOURCE", holdSPString, sizeof(holdSPString) );
		s = va( "%s %s", holdSPString, GetNetSourceString( ui_netSource.integer ) );
		break;
	case UI_NETFILTER:
		trap->SE_GetStringTextString( "MENUS_GAME", holdSPString, sizeof(holdSPString) );
		s = va( "%s %s", holdSPString, UI_FilterDescription( ui_serverFilterType.integer ) );
		break;
	case UI_ALLMAPS_SELECTION:
		break;
	case UI_OPPONENT_NAME:
		break;
	case UI_KEYBINDSTATUS:
		if ( Display_KeyBindPending() ) {
			s = UI_GetStringEdString( "MP_INGAME", "WAITING_FOR_NEW_KEY" );
		}
		else {
			//	s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
		}
		break;
	case UI_SERVERREFRESHDATE:
		s = UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i", ui_netSource.integer ) );
		break;
	default:
		break;
	}

	if ( s ) {
		return Text_Width( s, scale, 0, false );
	}
	return 0;
}

static void UI_DrawBotName( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	int value = uiInfo.botIndex;
	const char *text = "";
	if ( value >= UI_GetNumBots() ) {
		value = 0;
	}
	text = UI_GetBotNameByNumber( value );
	Text_Paint( rect->x, rect->y, scale, color, text, 0, 0, textStyle, iMenuFont, customFont );
}

static void UI_DrawBotSkill( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	if ( uiInfo.skillIndex < numSkillLevels ) {
		Text_Paint( rect->x, rect->y, scale, color, UI_GetStringEdString( "MP_INGAME", skillLevels[uiInfo.skillIndex] ),
			0, 0, textStyle, iMenuFont, customFont
		);
	}
}

static void UI_DrawRedBlue( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	Text_Paint( rect->x, rect->y, scale, color,
		(uiInfo.redBlue == 0)
			? UI_GetStringEdString( "MP_INGAME", "RED" )
			: UI_GetStringEdString( "MP_INGAME", "BLUE" ),
		0, 0, textStyle, iMenuFont, customFont
	);
}

static void UI_DrawCrosshair( rectDef_t *rect, float scale, const vector4 *color ) {
	float size = 32.0f;

	trap->R_SetColor( color );
	if ( uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS )
		uiInfo.currentCrosshair = 0;

	size = std::min( rect->w, rect->h );

	UI_DrawHandlePic( rect->x, rect->y, size, size, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair] );
	trap->R_SetColor( NULL );
}

static void UI_DrawSelectedPlayer( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh ) {
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}
	Text_Paint( rect->x, rect->y, scale, color, UI_Cvar_VariableString( "cg_selectedPlayerName" ), 0, 0, textStyle,
		iMenuFont, customFont
	);
}

static void UI_DrawServerRefreshDate( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	if ( uiInfo.serverStatus.refreshActive ) {
		vector4 lowLight, newColor;
		lowLight.r = 0.8f * color->r;
		lowLight.g = 0.8f * color->g;
		lowLight.b = 0.8f * color->b;
		lowLight.a = 0.8f * color->a;
		LerpColor( color, &lowLight, &newColor, 0.5f + 0.5f*sinf( (float)(uiInfo.uiDC.realTime / PULSE_DIVISOR) ) );

		trap->SE_GetStringTextString( "MP_INGAME_GETTINGINFOFORSERVERS", holdSPString, sizeof(holdSPString) );
		Text_Paint( rect->x, rect->y, scale, &newColor, va( (char *)holdSPString,
			trap->LAN_GetServerCount( UI_SourceForLAN() ) ), 0, 0, textStyle, iMenuFont, customFont
		);
	}
	else {
		char buff[64];
		Q_strncpyz( buff, UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i", ui_netSource.integer ) ), sizeof( buff ) );
		trap->SE_GetStringTextString( "MP_INGAME_SERVER_REFRESHTIME", holdSPString, sizeof(holdSPString) );

		Text_Paint( rect->x, rect->y, scale, color, va( "%s: %s", holdSPString, buff ), 0, 0, textStyle, iMenuFont,
			customFont
		);
	}
}

static void UI_DrawServerMOTD( rectDef_t *rect, float scale, const vector4 *color, int iMenuFont, bool customFont ) {
	if ( uiInfo.serverStatus.motdLen ) {
		float maxX;

		if ( uiInfo.serverStatus.motdWidth == -1 ) {
			uiInfo.serverStatus.motdWidth = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if ( uiInfo.serverStatus.motdOffset > uiInfo.serverStatus.motdLen ) {
			uiInfo.serverStatus.motdOffset = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if ( uiInfo.uiDC.realTime > uiInfo.serverStatus.motdTime ) {
			uiInfo.serverStatus.motdTime = uiInfo.uiDC.realTime + 10;
			if ( uiInfo.serverStatus.motdPaintX <= rect->x + 2 ) {
				if ( uiInfo.serverStatus.motdOffset < uiInfo.serverStatus.motdLen ) {
					uiInfo.serverStatus.motdPaintX += Text_Width(
						&uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], scale, iMenuFont, customFont
					) - 1;
					uiInfo.serverStatus.motdOffset++;
				}
				else {
					uiInfo.serverStatus.motdOffset = 0;
					if ( uiInfo.serverStatus.motdPaintX2 >= 0 ) {
						uiInfo.serverStatus.motdPaintX = uiInfo.serverStatus.motdPaintX2;
					}
					else {
						uiInfo.serverStatus.motdPaintX = rect->x + rect->w - 2;
					}
					uiInfo.serverStatus.motdPaintX2 = -1;
				}
			}
			else {
				//serverStatus.motdPaintX--;
				uiInfo.serverStatus.motdPaintX -= 2;
				if ( uiInfo.serverStatus.motdPaintX2 >= 0 ) {
					//serverStatus.motdPaintX2--;
					uiInfo.serverStatus.motdPaintX2 -= 2;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		Text_Paint_Limit( &maxX, uiInfo.serverStatus.motdPaintX, rect->y + rect->h - 3, scale, color,
			&uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], 0, 0, iMenuFont, customFont
		);
		if ( uiInfo.serverStatus.motdPaintX2 >= 0 ) {
			float maxX2 = rect->x + rect->w - 2;
			Text_Paint_Limit( &maxX2, uiInfo.serverStatus.motdPaintX2, rect->y + rect->h - 3, scale, color,
				uiInfo.serverStatus.motd, 0, uiInfo.serverStatus.motdOffset, iMenuFont, customFont
			);
		}
		if ( uiInfo.serverStatus.motdOffset && maxX > 0 ) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if ( uiInfo.serverStatus.motdPaintX2 == -1 ) {
				uiInfo.serverStatus.motdPaintX2 = rect->x + rect->w - 2;
			}
		}
		else {
			uiInfo.serverStatus.motdPaintX2 = -1;
		}
	}
}

static void UI_DrawKeyBindStatus( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	if ( Display_KeyBindPending() ) {
		Text_Paint( rect->x, rect->y, scale, color, UI_GetStringEdString( "MP_INGAME", "WAITING_FOR_NEW_KEY" ), 0, 0,
			textStyle, iMenuFont, customFont
		);
	}
}

static void UI_DrawGLInfo( rectDef_t *rect, float scale, const vector4 *color, int textStyle, int iMenuFont,
	bool customFont )
{
	char * eptr;
	char buff[4096];
	const char *lines[128];
	float y = 0.0f;
	float lineHeight = 15.0f;

	Text_Paint( rect->x + 2, rect->y + y, scale, color,
		va( "GL_VENDOR: %s", uiInfo.uiDC.glconfig.vendor_string ), 0, rect->w, textStyle, iMenuFont, customFont
	);
	y += lineHeight;
	Text_Paint( rect->x + 2, rect->y + y, scale, color,
		va( "GL_VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string, uiInfo.uiDC.glconfig.renderer_string ),
		0, rect->w, textStyle, iMenuFont, customFont
	);
	y += lineHeight;
	Text_Paint( rect->x + 2, rect->y + y, scale, color,
		va( "GL_PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)",
			uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits ),
		0, rect->w, textStyle, iMenuFont, customFont
	);

	// build null terminated extension strings
	Q_strncpyz( buff, uiInfo.uiDC.glconfig.extensions_string, 4096 );
	eptr = buff;
	y = rect->y + 45;
	int numLines = 0;
	while ( y < rect->y + rect->h && *eptr ) {
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if ( *eptr && *eptr != ' ' ) {
			lines[numLines++] = eptr;
		}

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	for ( int i = 0; i < numLines; ) {
		Text_Paint( rect->x + 2, y, scale, color, lines[i++], 0, (rect->w / 2), textStyle, iMenuFont, false );
		if ( i < numLines ) {
			Text_Paint( rect->x + rect->w / 2, y, scale, color, lines[i++], 0, (rect->w / 2), textStyle, iMenuFont,
				false
			);
		}
		y += 10.0f;
		if ( y > rect->y + rect->h - 11 ) {
			break;
		}
	}
}

static void UI_Version( rectDef_t *rect, float scale, const vector4 *color, int iMenuFont, bool customFont ) {
	float width = uiInfo.uiDC.textWidth( JAPP_VERSION, scale, iMenuFont, customFont );
	uiInfo.uiDC.drawText( rect->x - width, rect->y, scale, color, JAPP_VERSION, 0, 0, 0, iMenuFont, customFont );
}

//FIXME: table drive
static void UI_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw,
	uint32_t ownerDrawFlags, int align, float special, float scale, const vector4 *color, qhandle_t shader,
	int textStyle, int iMenuFont, bool customFont )
{
	rectDef_t rect;
	int findex;
	int drawRank = 0, iUse = 0;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {
	case UI_HANDICAP:
		UI_DrawHandicap( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_SKIN_COLOR:
		UI_DrawSkinColor( &rect, scale, color, textStyle, uiSkinColor, TEAM_FREE, TEAM_BLUE, iMenuFont, customFont );
		break;
	case UI_FORCE_SIDE:
		UI_DrawForceSide( &rect, scale, color, textStyle, uiForceSide, 1, 2, iMenuFont, customFont );
		break;
	case UI_JEDI_NONJEDI:
		UI_DrawJediNonJedi( &rect, scale, color, textStyle, uiJediNonJedi, 0, 1, iMenuFont, customFont );
		break;
	case UI_FORCE_POINTS:
		UI_DrawGenericNum( &rect, scale, color, textStyle, uiForceAvailable, 1, forceMasteryPoints[MAX_FORCE_RANK],
			ownerDraw, iMenuFont, customFont
		);
		break;
	case UI_FORCE_MASTERY_SET:
		UI_DrawForceMastery( &rect, scale, color, textStyle, uiForceRank, 0, MAX_FORCE_RANK, iMenuFont, customFont );
		break;
	case UI_FORCE_RANK:
		UI_DrawForceMastery( &rect, scale, color, textStyle, uiForceRank, 0, MAX_FORCE_RANK, iMenuFont, customFont );
		break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:

		//		uiForceRank
		findex = (ownerDraw - UI_FORCE_RANK) - 1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		if ( uiForcePowerDarkLight[findex] && uiForceSide != uiForcePowerDarkLight[findex] ) {
			VectorScale( (vector3*)color, 0.5f, (vector3*)color );
		}
		drawRank = uiForcePowersRank[findex];

		UI_DrawForceStars( &rect, scale, color, textStyle, findex, drawRank, 0, NUM_FORCE_POWER_LEVELS - 1 );
		break;
	case UI_PLAYERMODEL:
		//	UI_DrawPlayerModel(&rect);
		break;
	case UI_GAMETYPE:
		UI_DrawGameType( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_NETGAMETYPE:
		UI_DrawNetGameType( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_AUTOSWITCHLIST:
		UI_DrawAutoSwitch( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_JOINGAMETYPE:
		UI_DrawJoinGameType( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_MAPPREVIEW:
		UI_DrawMapPreview( &rect, scale, color, qtrue );
		break;
	case UI_MAP_TIMETOBEAT:
		break;
	case UI_MAPCINEMATIC:
		UI_DrawMapCinematic( &rect, scale, color, qfalse );
		break;
	case UI_STARTMAPCINEMATIC:
		UI_DrawMapCinematic( &rect, scale, color, qtrue );
		break;
	case UI_SKILL:
		UI_DrawSkill( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_TOTALFORCESTARS:
		//      UI_DrawTotalForceStars(&rect, scale, color, textStyle);
		break;
	case UI_BLUETEAMNAME:
		UI_DrawTeamName( &rect, scale, color, qtrue, textStyle, iMenuFont, customFont );
	case UI_REDTEAMNAME:
		UI_DrawTeamName( &rect, scale, color, qfalse, textStyle, iMenuFont, customFont );
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
	case UI_BLUETEAM6:
	case UI_BLUETEAM7:
	case UI_BLUETEAM8:
		if ( ownerDraw <= UI_BLUETEAM5 )
			iUse = ownerDraw - UI_BLUETEAM1 + 1;
		else
			iUse = ownerDraw - 274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
		UI_DrawTeamMember( &rect, scale, color, qtrue, iUse, textStyle, iMenuFont, customFont );
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
	case UI_REDTEAM6:
	case UI_REDTEAM7:
	case UI_REDTEAM8:
		if ( ownerDraw <= UI_REDTEAM5 )
			iUse = ownerDraw - UI_REDTEAM1 + 1;
		else
			iUse = ownerDraw - 277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
		UI_DrawTeamMember( &rect, scale, color, qfalse, iUse, textStyle, iMenuFont, customFont );
		break;
	case UI_NETSOURCE:
		UI_DrawNetSource( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_NETMAPPREVIEW:
		UI_DrawNetMapPreview( &rect, scale, color );
		break;
	case UI_NETMAPCINEMATIC:
		UI_DrawNetMapCinematic( &rect, scale, color );
		break;
	case UI_NETFILTER:
		UI_DrawNetFilter( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_OPPONENTMODEL:
		//UI_DrawOpponent(&rect);
		break;
	case UI_PLAYERLOGO:
	case UI_PLAYERLOGO_METAL:
	case UI_PLAYERLOGO_NAME:
	case UI_OPPONENTLOGO:
	case UI_OPPONENTLOGO_METAL:
	case UI_OPPONENTLOGO_NAME:
		break;
	case UI_ALLMAPS_SELECTION:
		UI_DrawAllMapsSelection( &rect, scale, color, textStyle, qtrue, iMenuFont, customFont );
		break;
	case UI_MAPS_SELECTION:
		UI_DrawAllMapsSelection( &rect, scale, color, textStyle, qfalse, iMenuFont, customFont );
		break;
	case UI_OPPONENT_NAME:
		UI_DrawOpponentName( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_BOTNAME:
		UI_DrawBotName( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_BOTSKILL:
		UI_DrawBotSkill( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_REDBLUE:
		UI_DrawRedBlue( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_CROSSHAIR:
		UI_DrawCrosshair( &rect, scale, color );
		break;
	case UI_SELECTEDPLAYER:
		UI_DrawSelectedPlayer( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_SERVERREFRESHDATE:
		UI_DrawServerRefreshDate( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_SERVERMOTD:
		UI_DrawServerMOTD( &rect, scale, color, iMenuFont, customFont );
		break;
	case UI_GLINFO:
		UI_DrawGLInfo( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_KEYBINDSTATUS:
		UI_DrawKeyBindStatus( &rect, scale, color, textStyle, iMenuFont, customFont );
		break;
	case UI_VERSION:
		UI_Version( &rect, scale, color, iMenuFont, customFont );
		break;
	default:
		break;
	}
}

static qboolean UI_OwnerDrawVisible( uint32_t flags ) {
	qboolean vis = qtrue;

	while ( flags ) {
		if ( flags & UI_SHOW_FFA ) {
			if ( trap->Cvar_VariableValue( "g_gametype" ) != GT_FFA &&
				trap->Cvar_VariableValue( "g_gametype" ) != GT_HOLOCRON &&
				trap->Cvar_VariableValue( "g_gametype" ) != GT_JEDIMASTER ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FFA;
		}
		if ( flags & UI_SHOW_NOTFFA ) {
			if ( trap->Cvar_VariableValue( "g_gametype" ) == GT_FFA ||
				trap->Cvar_VariableValue( "g_gametype" ) == GT_HOLOCRON ||
				trap->Cvar_VariableValue( "g_gametype" ) != GT_JEDIMASTER ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFFA;
		}
		if ( flags & UI_SHOW_FAVORITESERVERS ) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if ( ui_netSource.integer != UIAS_FAVORITES ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		}
		if ( flags & UI_SHOW_NOTFAVORITESERVERS ) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if ( ui_netSource.integer == UIAS_FAVORITES ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		}
		if ( flags & UI_SHOW_ANYTEAMGAME ) {
			if ( ui_gameType.integer <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYTEAMGAME;
		}
		if ( flags & UI_SHOW_ANYNONTEAMGAME ) {
			if ( ui_gameType.integer > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		}
		if ( flags & UI_SHOW_NETANYTEAMGAME ) {
			if ( ui_netGameType.integer <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		}
		if ( flags & UI_SHOW_NETANYNONTEAMGAME ) {
			if ( ui_netGameType.integer > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
		}
		else {
			flags = 0;
		}
	}
	return vis;
}

static qboolean UI_Handicap_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int h = Q_clampi( 5, trap->Cvar_VariableValue( "handicap" ), 100 );
		if ( key == A_MOUSE2 ) {
			h -= 5;
		}
		else {
			h += 5;
		}
		if ( h > 100 ) {
			h = 5;
		}
		else if ( h < 0 ) {
			h = 100;
		}
		trap->Cvar_Set( "handicap", va( "%i", h ) );
		return qtrue;
	}
	return qfalse;
}

void	Item_RunScript( itemDef_t *item, const char *s );		//from ui_shared;

// For hot keys on the chat main menu.
static qboolean UI_Chat_Main_HandleKey( int key ) {
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return (qfalse);
	}

	if ( (key == A_1) || (key == A_PLING) ) {
		item = Menu_FindItemByName( menu, "attack" );
	}
	else if ( (key == A_2) || (key == A_AT) ) {
		item = Menu_FindItemByName( menu, "defend" );
	}
	else if ( (key == A_3) || (key == A_HASH) ) {
		item = Menu_FindItemByName( menu, "request" );
	}
	else if ( (key == A_4) || (key == A_STRING) ) {
		item = Menu_FindItemByName( menu, "reply" );
	}
	else if ( (key == A_5) || (key == A_PERCENT) ) {
		item = Menu_FindItemByName( menu, "spot" );
	}
	else if ( (key == A_6) || (key == A_CARET) ) {
		item = Menu_FindItemByName( menu, "tactics" );
	}
	else {
		return (qfalse);
	}

	if ( item ) {
		Item_RunScript( item, item->action );
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Attack_HandleKey( int key ) {
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return (qfalse);
	}

	if ( (key == A_1) || (key == A_PLING) ) {
		item = Menu_FindItemByName( menu, "att_01" );
	}
	else if ( (key == A_2) || (key == A_AT) ) {
		item = Menu_FindItemByName( menu, "att_02" );
	}
	else if ( (key == A_3) || (key == A_HASH) ) {
		item = Menu_FindItemByName( menu, "att_03" );
	}
	else {
		return (qfalse);
	}

	if ( item ) {
		Item_RunScript( item, item->action );
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Defend_HandleKey( int key ) {
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return (qfalse);
	}

	if ( (key == A_1) || (key == A_PLING) ) {
		item = Menu_FindItemByName( menu, "def_01" );
	}
	else if ( (key == A_2) || (key == A_AT) ) {
		item = Menu_FindItemByName( menu, "def_02" );
	}
	else if ( (key == A_3) || (key == A_HASH) ) {
		item = Menu_FindItemByName( menu, "def_03" );
	}
	else if ( (key == A_4) || (key == A_STRING) ) {
		item = Menu_FindItemByName( menu, "def_04" );
	}
	else {
		return (qfalse);
	}

	if ( item ) {
		Item_RunScript( item, item->action );
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Request_HandleKey( int key ) {
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return (qfalse);
	}

	if ( (key == A_1) || (key == A_PLING) ) {
		item = Menu_FindItemByName( menu, "req_01" );
	}
	else if ( (key == A_2) || (key == A_AT) ) {
		item = Menu_FindItemByName( menu, "req_02" );
	}
	else if ( (key == A_3) || (key == A_HASH) ) {
		item = Menu_FindItemByName( menu, "req_03" );
	}
	else if ( (key == A_4) || (key == A_STRING) ) {
		item = Menu_FindItemByName( menu, "req_04" );
	}
	else if ( (key == A_5) || (key == A_PERCENT) ) {
		item = Menu_FindItemByName( menu, "req_05" );
	}
	else if ( (key == A_6) || (key == A_CARET) ) {
		item = Menu_FindItemByName( menu, "req_06" );
	}
	else {
		return (qfalse);
	}

	if ( item ) {
		Item_RunScript( item, item->action );
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Reply_HandleKey( int key ) {
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return (qfalse);
	}

	if ( (key == A_1) || (key == A_PLING) ) {
		item = Menu_FindItemByName( menu, "rep_01" );
	}
	else if ( (key == A_2) || (key == A_AT) ) {
		item = Menu_FindItemByName( menu, "rep_02" );
	}
	else if ( (key == A_3) || (key == A_HASH) ) {
		item = Menu_FindItemByName( menu, "rep_03" );
	}
	else if ( (key == A_4) || (key == A_STRING) ) {
		item = Menu_FindItemByName( menu, "rep_04" );
	}
	else if ( (key == A_5) || (key == A_PERCENT) ) {
		item = Menu_FindItemByName( menu, "rep_05" );
	}
	else {
		return (qfalse);
	}

	if ( item ) {
		Item_RunScript( item, item->action );
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Spot_HandleKey( int key ) {
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return (qfalse);
	}

	if ( (key == A_1) || (key == A_PLING) ) {
		item = Menu_FindItemByName( menu, "spot_01" );
	}
	else if ( (key == A_2) || (key == A_AT) ) {
		item = Menu_FindItemByName( menu, "spot_02" );
	}
	else if ( (key == A_3) || (key == A_HASH) ) {
		item = Menu_FindItemByName( menu, "spot_03" );
	}
	else if ( (key == A_4) || (key == A_STRING) ) {
		item = Menu_FindItemByName( menu, "spot_04" );
	}
	else {
		return (qfalse);
	}

	if ( item ) {
		Item_RunScript( item, item->action );
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Tactical_HandleKey( int key ) {
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if ( !menu ) {
		return (qfalse);
	}

	if ( (key == A_1) || (key == A_PLING) ) {
		item = Menu_FindItemByName( menu, "tac_01" );
	}
	else if ( (key == A_2) || (key == A_AT) ) {
		item = Menu_FindItemByName( menu, "tac_02" );
	}
	else if ( (key == A_3) || (key == A_HASH) ) {
		item = Menu_FindItemByName( menu, "tac_03" );
	}
	else if ( (key == A_4) || (key == A_STRING) ) {
		item = Menu_FindItemByName( menu, "tac_04" );
	}
	else if ( (key == A_5) || (key == A_PERCENT) ) {
		item = Menu_FindItemByName( menu, "tac_05" );
	}
	else if ( (key == A_6) || (key == A_CARET) ) {
		item = Menu_FindItemByName( menu, "tac_06" );
	}
	else {
		return (qfalse);
	}

	if ( item ) {
		Item_RunScript( item, item->action );
	}

	return (qtrue);
}

static qboolean UI_GameType_HandleKey( uint32_t flags, float *special, int key, qboolean resetMap ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int oldCount = UI_MapCountByGameType( qtrue );
		int value = ui_gameType.integer;

		// hard coded mess here
		if ( key == A_MOUSE2 ) {
			value--;
		}
		else {
			value++;
		}
		if ( value < -1 ) {
			value = GT_MAX_GAME_TYPE - 1;
		}
		else if ( value >= GT_MAX_GAME_TYPE ) {
			value = -1;
		}

		trap->Cvar_Set( "ui_gameType", va( "%d", value ) );
		trap->Cvar_Update( &ui_gameType );
		UI_SetCapFragLimits( qtrue );
		if ( resetMap && oldCount != UI_MapCountByGameType( qtrue ) ) {
			trap->Cvar_Set( "ui_currentMap", "0" );
			trap->Cvar_Update( &ui_currentMap );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, 0, NULL );
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetGameType_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int value = ui_netGameType.integer;

		if ( key == A_MOUSE2 ) {
			value--;
		}
		else {
			value++;
		}

		if ( value < -1 ) {
			value = GT_MAX_GAME_TYPE - 1;
		}
		else if ( value >= GT_MAX_GAME_TYPE ) {
			value = -1;
		}

		trap->Cvar_Set( "ui_netGameType", va( "%d", value ) );
		trap->Cvar_Update( &ui_netGameType );
		trap->Cvar_Set( "ui_currentNetMap", "0" );
		trap->Cvar_Update( &ui_currentNetMap );
		UI_MapCountByGameType( qfalse );
		Menu_SetFeederSelection( NULL, FEEDER_ALLMAPS, 0, NULL );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_AutoSwitch_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int switchVal = trap->Cvar_VariableValue( "cg_autoswitch" );

		if ( key == A_MOUSE2 ) {
			switchVal--;
		}
		else {
			switchVal++;
		}

		if ( switchVal < 0 ) {
			switchVal = 2;
		}
		else if ( switchVal >= 3 ) {
			switchVal = 0;
		}

		trap->Cvar_Set( "cg_autoswitch", va( "%i", switchVal ) );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_JoinGameType_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int value = ui_joinGameType.integer;

		if ( key == A_MOUSE2 ) {
			value--;
		}
		else {
			value++;
		}

		if ( value < -1 ) {
			value = GT_MAX_GAME_TYPE - 1;
		}
		else if ( value >= GT_MAX_GAME_TYPE ) {
			value = -1;
		}

		trap->Cvar_Set( "ui_joinGameType", va( "%d", value ) );
		trap->Cvar_Update( &ui_joinGameType );
		UI_BuildServerDisplayList( qtrue );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Skill_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int i = trap->Cvar_VariableValue( "g_spSkill" );

		if ( key == A_MOUSE2 ) {
			i--;
		}
		else {
			i++;
		}

		if ( i < 1 ) {
			i = numSkillLevels;
		}
		else if ( i >( signed )numSkillLevels ) {
			i = 1;
		}

		trap->Cvar_Set( "g_spSkill", va( "%i", i ) );
		trap->Cvar_Update( &g_spSkill );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_TeamName_HandleKey( uint32_t flags, float *special, int key, qboolean blue ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int i;
		i = UI_TeamIndexFromName( UI_Cvar_VariableString( (blue) ? "ui_blueTeam" : "ui_redTeam" ) );

		if ( key == A_MOUSE2 ) {
			i--;
		}
		else {
			i++;
		}

		if ( i >= uiInfo.teamCount ) {
			i = 0;
		}
		else if ( i < 0 ) {
			i = uiInfo.teamCount - 1;
		}

		trap->Cvar_Set( (blue) ? "ui_blueTeam" : "ui_redTeam", uiInfo.teamList[i].teamName );

		return qtrue;
	}
	return qfalse;
}

static qboolean UI_TeamMember_HandleKey( uint32_t flags, float *special, int key, qboolean blue, int num ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		const char *cvar = va( blue ? "ui_blueteam%i" : "ui_redteam%i", num );
		int value = trap->Cvar_VariableValue( cvar );
		int maxcl = trap->Cvar_VariableValue( "sv_maxClients" );
		int numval = num;

		numval *= 2;

		if ( blue ) {
			numval -= 1;
		}

		if ( numval > maxcl ) {
			return qfalse;
		}

		if ( value < 1 ) {
			value = 1;
		}

		if ( key == A_MOUSE2 ) {
			value--;
		}
		else {
			value++;
		}

		if ( value >= UI_GetNumBots() + 2 ) {
			value = 1;
		}
		else if ( value < 1 ) {
			value = UI_GetNumBots() + 2 - 1;
		}
		//}

		trap->Cvar_Set( cvar, va( "%i", value ) );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetSource_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int value = ui_netSource.integer;

		if ( key == A_MOUSE2 ) {
			value--;
		}
		else {
			value++;
		}

		if ( value >= UIAS_GLOBAL1 && value <= UIAS_GLOBAL5 ) {
			char masterstr[2], cvarname[sizeof( "sv_master1" )];

			while ( value >= UIAS_GLOBAL1 && value <= UIAS_GLOBAL5 ) {
				Com_sprintf( cvarname, sizeof( cvarname ), "sv_master%d", value );
				trap->Cvar_VariableStringBuffer( cvarname, masterstr, sizeof( masterstr ) );
				if ( *masterstr )
					break;

				if ( key == A_MOUSE2 )
					value--;
				else
					value++;
			}
		}

		if ( value >= numNetSources ) {
			value = 0;
		}
		else if ( value < 0 ) {
			value = numNetSources - 1;
		}

		trap->Cvar_Set( "ui_netSource", va( "%d", value ) );
		trap->Cvar_Update( &ui_netSource );

		UI_BuildServerDisplayList( qtrue );
		if ( !(ui_netSource.integer >= UIAS_GLOBAL1 && ui_netSource.integer <= UIAS_GLOBAL5) ) {
			UI_StartServerRefresh( qtrue );
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetFilter_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		int value = ui_serverFilterType.integer;

		if ( key == A_MOUSE2 ) {
			value--;
		}
		else {
			value++;
		}

		if ( value > uiInfo.modCount ) {
			value = 0;
		}
		else if ( value < 0 ) {
			value = uiInfo.modCount;
		}

		trap->Cvar_Set( "ui_serverFilterType", va( "%d", value ) );
		trap->Cvar_Update( &ui_serverFilterType );

		UI_BuildServerDisplayList( qtrue );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotName_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		//		int game = trap->Cvar_VariableValue("g_gametype");
		int value = uiInfo.botIndex;

		if ( key == A_MOUSE2 ) {
			value--;
		}
		else {
			value++;
		}

		/*
		if (game >= GT_TEAM) {
		if (value >= uiInfo.characterCount + 2) {
		value = 0;
		} else if (value < 0) {
		value = uiInfo.characterCount + 2 - 1;
		}
		} else {
		*/
		if ( value >= UI_GetNumBots()/* + 2*/ ) {
			value = 0;
		}
		else if ( value < 0 ) {
			value = UI_GetNumBots()/* + 2*/ - 1;
		}
		//}
		uiInfo.botIndex = value;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotSkill_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		if ( key == A_MOUSE2 )
			uiInfo.skillIndex--;
		else
			uiInfo.skillIndex++;
		// handle overflow
		if ( uiInfo.skillIndex >= numSkillLevels )
			uiInfo.skillIndex = 0;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_RedBlue_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		uiInfo.redBlue ^= 1;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Crosshair_HandleKey( uint32_t flags, float *special, int key ) {
	if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER ) {
		if ( key == A_MOUSE2 )	uiInfo.currentCrosshair--;
		else					uiInfo.currentCrosshair++;

		if ( uiInfo.currentCrosshair >= NUM_CROSSHAIRS )	uiInfo.currentCrosshair = 0;
		else if ( uiInfo.currentCrosshair < 0 )				uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;

		trap->Cvar_Set( "cg_drawCrosshair", va( "%d", uiInfo.currentCrosshair ) );
		return qtrue;
	}
	return qfalse;
}

/*
static qboolean UI_VoiceChat_HandleKey(uint32_t flags, float *special, int key)
{

qboolean ret = qfalse;

switch(key)
{
case A_1:
case A_KP_1:
ret = qtrue;
break;
case A_2:
case A_KP_2:
ret = qtrue;
break;

}

return ret;
}
*/


static qboolean UI_OwnerDrawHandleKey( int ownerDraw, uint32_t flags, float *special, int key ) {
	int findex, iUse = 0;

	switch ( ownerDraw ) {
	case UI_HANDICAP:
		return UI_Handicap_HandleKey( flags, special, key );
	case UI_SKIN_COLOR:
		return UI_SkinColor_HandleKey( flags, special, key, uiSkinColor, TEAM_FREE, TEAM_BLUE, ownerDraw );
	case UI_FORCE_SIDE:
		return UI_ForceSide_HandleKey( flags, special, key, uiForceSide, 1, 2, ownerDraw );
	case UI_JEDI_NONJEDI:
		return UI_JediNonJedi_HandleKey( flags, special, key, uiJediNonJedi, 0, 1, ownerDraw );
	case UI_FORCE_MASTERY_SET:
		return UI_ForceMaxRank_HandleKey( flags, special, key, uiForceRank, 1, MAX_FORCE_RANK, ownerDraw );
	case UI_FORCE_RANK:
		break;
	case UI_CHAT_MAIN:
		return UI_Chat_Main_HandleKey( key );
	case UI_CHAT_ATTACK:
		return UI_Chat_Attack_HandleKey( key );
	case UI_CHAT_DEFEND:
		return UI_Chat_Defend_HandleKey( key );
	case UI_CHAT_REQUEST:
		return UI_Chat_Request_HandleKey( key );
	case UI_CHAT_REPLY:
		return UI_Chat_Reply_HandleKey( key );
	case UI_CHAT_SPOT:
		return UI_Chat_Spot_HandleKey( key );
	case UI_CHAT_TACTICAL:
		return UI_Chat_Tactical_HandleKey( key );
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:
		findex = (ownerDraw - UI_FORCE_RANK) - 1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		return UI_ForcePowerRank_HandleKey( flags, special, key, uiForcePowersRank[findex], 0, NUM_FORCE_POWER_LEVELS - 1, ownerDraw );
	case UI_EFFECTS:
		break;
	case UI_GAMETYPE:
		return UI_GameType_HandleKey( flags, special, key, qtrue );
	case UI_NETGAMETYPE:
		return UI_NetGameType_HandleKey( flags, special, key );
	case UI_AUTOSWITCHLIST:
		return UI_AutoSwitch_HandleKey( flags, special, key );
	case UI_JOINGAMETYPE:
		return UI_JoinGameType_HandleKey( flags, special, key );
	case UI_SKILL:
		return UI_Skill_HandleKey( flags, special, key );
	case UI_BLUETEAMNAME:
		return UI_TeamName_HandleKey( flags, special, key, qtrue );
	case UI_REDTEAMNAME:
		return UI_TeamName_HandleKey( flags, special, key, qfalse );
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
	case UI_BLUETEAM6:
	case UI_BLUETEAM7:
	case UI_BLUETEAM8:
		if ( ownerDraw <= UI_BLUETEAM5 )
			iUse = ownerDraw - UI_BLUETEAM1 + 1;
		else
			iUse = ownerDraw - 274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines

		UI_TeamMember_HandleKey( flags, special, key, qtrue, iUse );
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
	case UI_REDTEAM6:
	case UI_REDTEAM7:
	case UI_REDTEAM8:
		if ( ownerDraw <= UI_REDTEAM5 )
			iUse = ownerDraw - UI_REDTEAM1 + 1;
		else
			iUse = ownerDraw - 277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
		UI_TeamMember_HandleKey( flags, special, key, qfalse, iUse );
		break;
	case UI_NETSOURCE:
		UI_NetSource_HandleKey( flags, special, key );
		break;
	case UI_NETFILTER:
		UI_NetFilter_HandleKey( flags, special, key );
		break;
	case UI_OPPONENT_NAME:
		break;
	case UI_BOTNAME:
		return UI_BotName_HandleKey( flags, special, key );
	case UI_BOTSKILL:
		return UI_BotSkill_HandleKey( flags, special, key );
	case UI_REDBLUE:
		UI_RedBlue_HandleKey( flags, special, key );
		break;
	case UI_CROSSHAIR:
		UI_Crosshair_HandleKey( flags, special, key );
		break;
	case UI_SELECTEDPLAYER:
		break;
	default:
		break;
	}

	return qfalse;
}

static float UI_GetValue( int ownerDraw ) {
	return 0;
}

static int UI_ServersQsortCompare( const void *arg1, const void *arg2 ) {
	return trap->LAN_CompareServers( UI_SourceForLAN(), uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, *(int*)arg1, *(int*)arg2 );
}

void UI_ServersSort( int column, qboolean force ) {
	if ( !force ) {
		if ( uiInfo.serverStatus.sortKey == column ) {
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[0], uiInfo.serverStatus.numDisplayServers, sizeof(int), UI_ServersQsortCompare );
}

#define MODSBUFSIZE (MAX_MODS * MAX_QPATH)

static void UI_LoadMods( void ) {
	int		numdirs;
	char	dirlist[MODSBUFSIZE];
	char	*dirptr;
	char	*descptr;
	int		i;
	int		dirlen;
	char	version[MAX_CVAR_VALUE_STRING] = { 0 };

	trap->SE_GetStringTextString( "MENUS_ALL", sAll, sizeof(sAll) );

	// To still display base game with old engine
	Q_strncpyz( version, UI_Cvar_VariableString( "version" ), sizeof(version) );
	if ( strstr( version, "2003" ) ) {
		trap->SE_GetStringTextString( "MENUS_JEDI_ACADEMY", sJediAcademy, sizeof(sJediAcademy) );
		uiInfo.modList[0].modName = String_Alloc( "" );
		uiInfo.modList[0].modDescr = String_Alloc( sJediAcademy );
		uiInfo.modCount = 1;
	}
	else
		uiInfo.modCount = 0;

	numdirs = trap->FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr = dirlist;
	for ( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[uiInfo.modCount].modName = String_Alloc( dirptr );
		uiInfo.modList[uiInfo.modCount].modDescr = String_Alloc( descptr );
		dirptr += dirlen + strlen( descptr ) + 1;
		uiInfo.modCount++;
		if ( uiInfo.modCount >= MAX_MODS ) {
			break;
		}
	}
}

#if 1

static void UI_LoadDemosInDirectory( const char *directory ) {
	static char demolist[MAX_DEMOLIST], fileList[MAX_DEMOLIST];
	char demoExt[32], *demoname = demolist, *fileName = fileList;
	int i = 0, j = 0, len = 0, numFiles = 0;
	int protocol = trap->Cvar_VariableValue( "com_protocol" ), protocolLegacy = trap->Cvar_VariableValue( "com_legacyprotocol" );

	if ( !protocol )
		protocol = trap->Cvar_VariableValue( "protocol" );
	if ( protocolLegacy == protocol )
		protocolLegacy = 0;

	Com_sprintf( demoExt, sizeof(demoExt), ".%s%d", DEMO_EXTENSION, protocol );

	uiInfo.demoCount += trap->FS_GetFileList( directory, demoExt, demolist, sizeof(demolist) );

	demoname = demolist;

	for ( j = 0; j<2; j++ ) {
		if ( uiInfo.demoCount > MAX_DEMOS )
			uiInfo.demoCount = MAX_DEMOS;

		for ( ; uiInfo.loadedDemos<uiInfo.demoCount; uiInfo.loadedDemos++ ) {
			len = strlen( demoname );
			Com_sprintf( uiInfo.demoList[uiInfo.loadedDemos], sizeof(uiInfo.demoList[0]), "%s/%s", directory + strlen( DEMO_DIRECTORY ), demoname );
			demoname += len + 1;
		}

		if ( !j ) {
			if ( protocolLegacy > 0 && uiInfo.demoCount < MAX_DEMOS ) {
				Com_sprintf( demoExt, sizeof(demoExt), ".%s%d", DEMO_EXTENSION, protocolLegacy );
				uiInfo.demoCount += trap->FS_GetFileList( directory, demoExt, demolist, sizeof(demolist) );
				demoname = demolist;
			}
			else
				break;
		}
	}

	numFiles = trap->FS_GetFileList( directory, "/", fileList, sizeof(fileList) );

	fileName = fileList;
	for ( i = 0; i < numFiles; i++ ) {
		len = strlen( fileName );
		fileName[len] = '\0';
		if ( Q_stricmp( fileName, "." ) && Q_stricmp( fileName, ".." ) )
			UI_LoadDemosInDirectory( va( "%s/%s", directory, fileName ) );
		fileName += len + 1;
	}

}

static void UI_LoadDemos( void ) {
	uiInfo.demoCount = 0;
	uiInfo.loadedDemos = 0;
	memset( uiInfo.demoList, 0, sizeof(uiInfo.demoList) );
	UI_LoadDemosInDirectory( DEMO_DIRECTORY );
}

#else

static void UI_LoadDemos( void )
{
	char	demolist[4096] = {0};
	char	demoExt[8] = {0};
	char	*demoname = NULL;
	int		i, len, extLen;

	Com_sprintf( demoExt, sizeof( demoExt ), "dm_%d", (int)trap->Cvar_VariableValue( "protocol" ) );
	uiInfo.demoCount = Q_clampi( 0, trap->FS_GetFileList( DEMOS_DIRECTORY, demoExt, demolist, sizeof( demolist ) ), MAX_DEMOS );
	Com_sprintf( demoExt, sizeof( demoExt ), ".dm_%d", (int)trap->Cvar_VariableValue( "protocol" ) );
	extLen = strlen( demoExt );

	if ( uiInfo.demoCount )
	{
		demoname = demolist;
		for ( i=0; i<uiInfo.demoCount; i++ )
		{
			len = strlen( demoname );
			if ( !Q_stricmp( demoname + len - extLen, demoExt) )
				demoname[len-extLen] = '\0';
			Q_strupr( demoname );
			uiInfo.demoList[i] = String_Alloc( demoname );
			demoname += len + 1;
		}
	}
}

#endif

static qboolean UI_SetNextMap( int actual, int index ) {
	int i;
	for ( i = actual + 1; i < uiInfo.mapCount; i++ ) {
		if ( uiInfo.mapList[i].active ) {
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, index + 1, "skirmish" );
			return qtrue;
		}
	}
	return qfalse;
}

static void UI_StartSkirmish( qboolean next ) {
	int i, k, delay, temp;
	float skill;
	char buff[MAX_STRING_CHARS] = {};

	if ( next ) {
		int actual, index = trap->Cvar_VariableValue( "ui_mapIndex" );
		UI_MapCountByGameType( qtrue );
		UI_SelectedMap( index, &actual );
		if ( !UI_SetNextMap( actual, index ) ) {
			UI_GameType_HandleKey( 0u, NULL, A_MOUSE1, qfalse );
			UI_MapCountByGameType( qtrue );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, 0, "skirmish" );
		}
	}

	trap->Cvar_SetValue( "g_gametype", ui_gameType.integer );
	trap->Cmd_ExecuteText( EXEC_APPEND, va( "wait 2; map %s\n",
		uiInfo.mapList[ui_currentMap.integer].mapLoadName ) );
	skill = trap->Cvar_VariableValue( "g_spSkill" );
	trap->Cvar_Set( "ui_scoreMap", uiInfo.mapList[ui_currentMap.integer].mapName );

	k = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );

	trap->Cvar_Set( "ui_singlePlayerActive", "1" );

	// set up sp overrides, will be replaced on postgame
	trap->Cvar_Set( "ui_saveCaptureLimit", va( "%i", trap->Cvar_VariableValue( "capturelimit" ) ) );
	trap->Cvar_Set( "ui_saveFragLimit", va( "%i", trap->Cvar_VariableValue( "fraglimit" ) ) );
	trap->Cvar_Set( "ui_saveDuelLimit", va( "%i", trap->Cvar_VariableValue( "duel_fraglimit" ) ) );

	UI_SetCapFragLimits( qfalse );


	trap->Cvar_Set( "ui_drawTimer", va( "%i", trap->Cvar_VariableValue( "cg_drawTimer" ) ) );
	trap->Cvar_Set( "ui_doWarmup", va( "%i", trap->Cvar_VariableValue( "g_doWarmup" ) ) );
	trap->Cvar_Set( "ui_friendlyFire", va( "%i", trap->Cvar_VariableValue( "g_friendlyFire" ) ) );
	trap->Cvar_Set( "ui_maxClients", va( "%i", trap->Cvar_VariableValue( "sv_maxClients" ) ) );
	trap->Cvar_Set( "ui_Warmup", va( "%i", trap->Cvar_VariableValue( "g_warmup" ) ) );
	trap->Cvar_Set( "ui_pure", va( "%i", trap->Cvar_VariableValue( "sv_pure" ) ) );
	trap->Cvar_Set( "cg_cameraOrbit", "0" );
	trap->Cvar_Set( "cg_drawTimer", "1" );
	trap->Cvar_Set( "g_doWarmup", "1" );
	trap->Cvar_Set( "g_warmup", "15" );
	trap->Cvar_Set( "sv_pure", "0" );
	trap->Cvar_Set( "g_friendlyFire", "0" );
	//	trap->Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
	//	trap->Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));

	if ( trap->Cvar_VariableValue( "ui_recordSPDemo" ) ) {
		Com_sprintf( buff, MAX_STRING_CHARS, "%s_%i", uiInfo.mapList[ui_currentMap.integer].mapLoadName, ui_gameType.integer );
		trap->Cvar_Set( "ui_recordSPDemoName", buff );
	}

	delay = 500;

	if ( ui_gameType.integer == GT_DUEL || ui_gameType.integer == GT_POWERDUEL ) {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap->Cvar_Set( "sv_maxClients", va( "%d", temp ) );
		Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %f f, %i \n",
			uiInfo.mapList[ui_currentMap.integer].opponentName, skill, delay );
		trap->Cmd_ExecuteText( EXEC_APPEND, buff );
	}
	else if ( ui_gameType.integer == GT_HOLOCRON || ui_gameType.integer == GT_JEDIMASTER ) {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap->Cvar_Set( "sv_maxClients", va( "%d", temp ) );
		for ( i = 0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++ ) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n",
				UI_AIFromName( uiInfo.teamList[k].teamMembers[i] ), skill, (ui_gameType.integer == GT_HOLOCRON) ? "f" : "b", delay,
				uiInfo.teamList[k].teamMembers[i] );
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
		for ( i = 0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers - 1; i++ ) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n",
				UI_AIFromName( uiInfo.teamList[k].teamMembers[i] ), skill, (ui_gameType.integer == GT_HOLOCRON) ? "f" : "r", delay,
				uiInfo.teamList[k].teamMembers[i] );
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	}
	else {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap->Cvar_Set( "sv_maxClients", va( "%d", temp ) );
		for ( i = 0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++ ) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n",
				UI_AIFromName( uiInfo.teamList[k].teamMembers[i] ), skill, (ui_gameType.integer == GT_FFA) ? "f" : "b", delay,
				uiInfo.teamList[k].teamMembers[i] );
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
		for ( i = 0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers - 1; i++ ) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n",
				UI_AIFromName( uiInfo.teamList[k].teamMembers[i] ), skill, (ui_gameType.integer == GT_FFA) ? "f" : "r", delay,
				uiInfo.teamList[k].teamMembers[i] );
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	}
	if ( ui_gameType.integer >= GT_TEAM ) {
		trap->Cmd_ExecuteText( EXEC_APPEND, "wait 5; team r\n" );
	}
}

static void UI_Update( const char *name ) {
	int	val = trap->Cvar_VariableValue( name );

	if ( !Q_stricmp( name, "s_khz" ) ) {
		trap->Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		return;
	}

	if ( !Q_stricmp( name, "ui_SetName" ) ) {
		char buf[MAX_NETNAME];
		Q_strncpyz( buf, UI_Cvar_VariableString( "ui_Name" ), sizeof(buf) );
		trap->Cvar_Set( "name", buf );
	}

	else if ( !Q_stricmp( name, "ui_setRate" ) ) {
		float rate = trap->Cvar_VariableValue( "rate" );
		if ( rate >= 15000 ) {
			trap->Cvar_Set( "cl_maxpackets", "63" );
			trap->Cvar_Set( "cl_packetdup", "1" );
		}
		else if ( rate >= 5000 ) {
			trap->Cvar_Set( "cl_maxpackets", "30" );
			trap->Cvar_Set( "cl_packetdup", "1" );
		}
		else if ( rate >= 4000 ) {
			// favor less prediction errors when there's packet loss
			trap->Cvar_Set( "cl_maxpackets", "15" );
			trap->Cvar_Set( "cl_packetdup", "2" );
		}
		else {
			// favor lower bandwidth
			trap->Cvar_Set( "cl_maxpackets", "15" );
			trap->Cvar_Set( "cl_packetdup", "1" );
		}
	}

	else if ( !Q_stricmp( name, "ui_GetName" ) ) {
		char buf[MAX_NETNAME];
		Q_strncpyz( buf, UI_Cvar_VariableString( "name" ), sizeof(buf) );
		trap->Cvar_Set( "ui_Name", buf );
	}

	else if ( !Q_stricmp( name, "ui_r_colorbits" ) ) {
		switch ( val ) {
		case 0:
			trap->Cvar_SetValue( "ui_r_depthbits", 0 );
			break;

		case 16:
			trap->Cvar_SetValue( "ui_r_depthbits", 16 );
			break;

		case 32:
		default:
			trap->Cvar_SetValue( "ui_r_depthbits", 24 );
			break;
		}
	}

	else if ( !Q_stricmp( name, "ui_r_glCustom" ) ) {
		switch ( val ) {
		case 0:	// high quality
			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 4 );
			trap->Cvar_SetValue( "ui_r_lodbias", 0 );
			trap->Cvar_SetValue( "ui_r_colorbits", 32 );
			trap->Cvar_SetValue( "ui_r_depthbits", 24 );
			trap->Cvar_SetValue( "ui_r_picmip", 0 );
			trap->Cvar_SetValue( "ui_r_mode", 4 );
			trap->Cvar_SetValue( "ui_r_texturebits", 32 );
			trap->Cvar_SetValue( "ui_r_fastSky", 0 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 1 );
			//	trap->Cvar_SetValue( "ui_cg_shadows", 2 );//stencil
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;

		case 1: // normal
			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 4 );
			trap->Cvar_SetValue( "ui_r_lodbias", 0 );
			trap->Cvar_SetValue( "ui_r_colorbits", 0 );
			trap->Cvar_SetValue( "ui_r_depthbits", 24 );
			trap->Cvar_SetValue( "ui_r_picmip", 1 );
			trap->Cvar_SetValue( "ui_r_mode", 3 );
			trap->Cvar_SetValue( "ui_r_texturebits", 0 );
			trap->Cvar_SetValue( "ui_r_fastSky", 0 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 1 );
			//	trap->Cvar_SetValue( "ui_cg_shadows", 2 );
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;

		case 2: // fast
			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 12 );
			trap->Cvar_SetValue( "ui_r_lodbias", 1 );
			trap->Cvar_SetValue( "ui_r_colorbits", 0 );
			trap->Cvar_SetValue( "ui_r_depthbits", 0 );
			trap->Cvar_SetValue( "ui_r_picmip", 2 );
			trap->Cvar_SetValue( "ui_r_mode", 3 );
			trap->Cvar_SetValue( "ui_r_texturebits", 0 );
			trap->Cvar_SetValue( "ui_r_fastSky", 1 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 0 );
			//	trap->Cvar_SetValue( "ui_cg_shadows", 1 );
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;

		case 3: // fastest
			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 20 );
			trap->Cvar_SetValue( "ui_r_lodbias", 2 );
			trap->Cvar_SetValue( "ui_r_colorbits", 16 );
			trap->Cvar_SetValue( "ui_r_depthbits", 16 );
			trap->Cvar_SetValue( "ui_r_mode", 3 );
			trap->Cvar_SetValue( "ui_r_picmip", 3 );
			trap->Cvar_SetValue( "ui_r_texturebits", 16 );
			trap->Cvar_SetValue( "ui_r_fastSky", 1 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 0 );
			//	trap->Cvar_SetValue( "ui_cg_shadows", 0 );
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;

		case 4: // pro
			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 80 );
			trap->Cvar_SetValue( "ui_r_lodbias", 5 );
			trap->Cvar_SetValue( "ui_r_colorbits", 32 );
			trap->Cvar_SetValue( "ui_r_depthbits", 0 );
			trap->Cvar_SetValue( "ui_r_mode", 3 );
			trap->Cvar_SetValue( "ui_r_picmip", 16 );
			trap->Cvar_SetValue( "ui_r_texturebits", 32 );
			trap->Cvar_SetValue( "ui_r_fastSky", 1 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 0 );
			//	trap->Cvar_SetValue( "ui_cg_shadows", 0 );
			trap->Cvar_Set( "ui_r_texturemode", "GL_NEAREST" );
			break;

		default:
			break;
		}
	}

	else if ( !Q_stricmp( name, "ui_mousePitch" ) ) {
		if ( val == 0 )
			trap->Cvar_SetValue( "m_pitch", 0.022f );
		else
			trap->Cvar_SetValue( "m_pitch", -0.022f );
	}

	else if ( !Q_stricmp( name, "ui_mousePitchVeh" ) ) {
		if ( val == 0 )
			trap->Cvar_SetValue( "m_pitchVeh", 0.022f );
		else
			trap->Cvar_SetValue( "m_pitchVeh", -0.022f );
	}
}

int gUISelectedMap = 0;

// Return true if the menu script should be deferred for later
static qboolean UI_DeferMenuScript( char **args ) {
	const char* name;

	// Whats the reason for being deferred?
	if ( !String_Parse( (char**)args, &name ) ) {
		return qfalse;
	}

	// Handle the custom cases
	if ( !Q_stricmp( name, "VideoSetup" ) ) {
		const char* warningMenuName;
		qboolean	deferred;

		// No warning menu specified
		if ( !String_Parse( (char**)args, &warningMenuName ) ) {
			return qfalse;
		}

		// Defer if the video options were modified
		deferred = trap->Cvar_VariableValue( "ui_r_modified" ) ? qtrue : qfalse;

		if ( deferred ) {
			// Open the warning menu
			Menus_OpenByName( warningMenuName );
		}

		return deferred;
	}
	else if ( !Q_stricmp( name, "RulesBackout" ) ) {
		qboolean deferred;

		deferred = trap->Cvar_VariableValue( "ui_rules_backout" ) ? qtrue : qfalse;

		trap->Cvar_Set( "ui_rules_backout", "0" );

		return deferred;
	}

	return qfalse;
}

// Copies the temporary user interface version of the video cvars into their real counterparts.
//	This is to create a interface which allows you to discard your changes if you did something you didn't want
void UI_UpdateVideoSetup( void ) {
	trap->Cvar_Set( "r_mode", UI_Cvar_VariableString( "ui_r_mode" ) );
	trap->Cvar_Set( "r_fullscreen", UI_Cvar_VariableString( "ui_r_fullscreen" ) );
	trap->Cvar_Set( "r_colorbits", UI_Cvar_VariableString( "ui_r_colorbits" ) );
	//	trap->Cvar_Set( "r_lodbias",					UI_Cvar_VariableString( "ui_r_lodbias" ) );
	trap->Cvar_Set( "r_picmip", UI_Cvar_VariableString( "ui_r_picmip" ) );
	trap->Cvar_Set( "r_intensity", UI_Cvar_VariableString( "ui_r_intensity" ) );
	trap->Cvar_Set( "r_texturebits", UI_Cvar_VariableString( "ui_r_texturebits" ) );
	//	trap->Cvar_Set( "r_texturemode",				UI_Cvar_VariableString( "ui_r_texturemode" ) );
	trap->Cvar_Set( "r_detailtextures", UI_Cvar_VariableString( "ui_r_detailtextures" ) );
	trap->Cvar_Set( "r_ext_compress_textures", UI_Cvar_VariableString( "ui_r_ext_compress_textures" ) );
	trap->Cvar_Set( "r_depthbits", UI_Cvar_VariableString( "ui_r_depthbits" ) );
	trap->Cvar_Set( "r_stencilbits", UI_Cvar_VariableString( "ui_r_stencilbits" ) );
	trap->Cvar_Set( "r_subdivisions", UI_Cvar_VariableString( "ui_r_subdivisions" ) );
	trap->Cvar_Set( "r_fastSky", UI_Cvar_VariableString( "ui_r_fastSky" ) );
	trap->Cvar_Set( "r_inGameVideo", UI_Cvar_VariableString( "ui_r_inGameVideo" ) );
	trap->Cvar_Set( "r_allowExtensions", UI_Cvar_VariableString( "ui_r_allowExtensions" ) );
	trap->Cvar_Set( "cg_shadows", UI_Cvar_VariableString( "ui_cg_shadows" ) );
	trap->Cvar_Set( "ui_r_modified", "0" );

	trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
}

// Retrieves the current actual video settings into the temporary user interface versions of the cvars.
void UI_GetVideoSetup( void ) {
	// Make sure the cvars are registered as read only.
	trap->Cvar_Register( NULL, "ui_r_glCustom", "4", CVAR_ROM | CVAR_INTERNAL | CVAR_ARCHIVE );

	trap->Cvar_Register( NULL, "ui_r_mode", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_fullscreen", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_colorbits", "0", CVAR_ROM | CVAR_INTERNAL );
	//	trap->Cvar_Register( NULL, "ui_r_lodbias",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_picmip", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_intensity", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_texturebits", "0", CVAR_ROM | CVAR_INTERNAL );
	//	trap->Cvar_Register( NULL, "ui_r_texturemode",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_detailtextures", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_ext_compress_textures", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_depthbits", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_stencilbits", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_subdivisions", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_fastSky", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_inGameVideo", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_allowExtensions", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_cg_shadows", "0", CVAR_ROM | CVAR_INTERNAL );
	trap->Cvar_Register( NULL, "ui_r_modified", "0", CVAR_ROM | CVAR_INTERNAL );

	// Copy over the real video cvars into their temporary counterparts
	trap->Cvar_Set( "ui_r_mode", UI_Cvar_VariableString( "r_mode" ) );
	trap->Cvar_Set( "ui_r_colorbits", UI_Cvar_VariableString( "r_colorbits" ) );
	trap->Cvar_Set( "ui_r_fullscreen", UI_Cvar_VariableString( "r_fullscreen" ) );
	//	trap->Cvar_Set( "ui_r_lodbias",					UI_Cvar_VariableString( "r_lodbias" ) );
	trap->Cvar_Set( "ui_r_picmip", UI_Cvar_VariableString( "r_picmip" ) );
	trap->Cvar_Set( "ui_r_intensity", UI_Cvar_VariableString( "r_intensity" ) );
	trap->Cvar_Set( "ui_r_texturebits", UI_Cvar_VariableString( "r_texturebits" ) );
	//	trap->Cvar_Set( "ui_r_texturemode",				UI_Cvar_VariableString( "r_texturemode" ) );
	trap->Cvar_Set( "ui_r_detailtextures", UI_Cvar_VariableString( "r_detailtextures" ) );
	trap->Cvar_Set( "ui_r_ext_compress_textures", UI_Cvar_VariableString( "r_ext_compress_textures" ) );
	trap->Cvar_Set( "ui_r_depthbits", UI_Cvar_VariableString( "r_depthbits" ) );
	trap->Cvar_Set( "ui_r_stencilbits", UI_Cvar_VariableString( "r_stencilbits" ) );
	trap->Cvar_Set( "ui_r_subdivisions", UI_Cvar_VariableString( "r_subdivisions" ) );
	trap->Cvar_Set( "ui_r_fastSky", UI_Cvar_VariableString( "r_fastSky" ) );
	trap->Cvar_Set( "ui_r_inGameVideo", UI_Cvar_VariableString( "r_inGameVideo" ) );
	trap->Cvar_Set( "ui_r_allowExtensions", UI_Cvar_VariableString( "r_allowExtensions" ) );
	trap->Cvar_Set( "ui_cg_shadows", UI_Cvar_VariableString( "cg_shadows" ) );
	trap->Cvar_Set( "ui_r_modified", "0" );
}

// If the game type is siege, hide the addbot button. I would have done a cvar text on that item,
// but it already had one on it.
static void UI_SetBotButton( void ) {
	int gameType = trap->Cvar_VariableValue( "g_gametype" );
	int server;
	menuDef_t *menu;
	itemDef_t *item;
	const char *name = "addBot";

	server = trap->Cvar_VariableValue( "sv_running" );

	// If in siege or a client, don't show add bot button
	if ( (gameType == GT_SIEGE) || (server == 0) )	// If it's not siege, don't worry about it
	{
		menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

		if ( !menu ) {
			return;
		}

		item = Menu_FindItemByName( menu, name );
		if ( item ) {
			Menu_ShowItemByName( menu, name, qfalse );
		}
	}
}

// Update the model cvar and everything is good.
static void UI_UpdateCharacterCvars( void ) {
	char skin[MAX_QPATH];
	char model[MAX_QPATH];
	char head[MAX_QPATH];
	char torso[MAX_QPATH];
	char legs[MAX_QPATH];

	trap->Cvar_VariableStringBuffer( "ui_char_model", model, sizeof(model) );
	trap->Cvar_VariableStringBuffer( "ui_char_skin_head", head, sizeof(head) );
	trap->Cvar_VariableStringBuffer( "ui_char_skin_torso", torso, sizeof(torso) );
	trap->Cvar_VariableStringBuffer( "ui_char_skin_legs", legs, sizeof(legs) );

	Com_sprintf( skin, sizeof(skin), "%s/%s|%s|%s",
		model,
		head,
		torso,
		legs
		);

	trap->Cvar_Set( "model", skin );

	trap->Cvar_Set( "char_color_red", UI_Cvar_VariableString( "ui_char_color_red" ) );
	trap->Cvar_Set( "char_color_green", UI_Cvar_VariableString( "ui_char_color_green" ) );
	trap->Cvar_Set( "char_color_blue", UI_Cvar_VariableString( "ui_char_color_blue" ) );
	trap->Cvar_Set( "ui_selectedModelIndex", "-1" );

}

static void UI_GetCharacterCvars( void ) {
	char *model;
	char *skin;
	int i;

	trap->Cvar_Set( "ui_char_color_red", UI_Cvar_VariableString( "char_color_red" ) );
	trap->Cvar_Set( "ui_char_color_green", UI_Cvar_VariableString( "char_color_green" ) );
	trap->Cvar_Set( "ui_char_color_blue", UI_Cvar_VariableString( "char_color_blue" ) );

	model = UI_Cvar_VariableString( "model" );
	skin = strrchr( model, '/' );
	if ( skin && strchr( model, '|' ) )	//we have a multipart custom jedi
	{
		char skinhead[MAX_QPATH];
		char skintorso[MAX_QPATH];
		char skinlower[MAX_QPATH];
		char *p2;

		*skin = 0;
		skin++;
		//now get the the individual files

		//advance to second
		p2 = strchr( skin, '|' );
		assert( p2 );
		*p2 = 0;
		p2++;
		strcpy( skinhead, skin );


		//advance to third
		skin = strchr( p2, '|' );
		assert( skin );
		*skin = 0;
		skin++;
		strcpy( skintorso, p2 );

		strcpy( skinlower, skin );



		trap->Cvar_Set( "ui_char_model", model );
		trap->Cvar_Set( "ui_char_skin_head", skinhead );
		trap->Cvar_Set( "ui_char_skin_torso", skintorso );
		trap->Cvar_Set( "ui_char_skin_legs", skinlower );

		for ( i = 0; i < uiInfo.playerSpeciesCount; i++ ) {
			if ( !Q_stricmp( model, uiInfo.playerSpecies[i].Name ) ) {
				uiInfo.playerSpeciesIndex = i;
				break;
			}
		}
	}
	else {
		model = UI_Cvar_VariableString( "ui_char_model" );
		for ( i = 0; i < uiInfo.playerSpeciesCount; i++ ) {
			if ( !Q_stricmp( model, uiInfo.playerSpecies[i].Name ) ) {
				uiInfo.playerSpeciesIndex = i;
				return;	//FOUND IT, don't fall through
			}
		}
		//nope, didn't find it.
		uiInfo.playerSpeciesIndex = 0;//jic
		trap->Cvar_Set( "ui_char_model", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name );
		trap->Cvar_Set( "ui_char_skin_head", "head_a1" );
		trap->Cvar_Set( "ui_char_skin_torso", "torso_a1" );
		trap->Cvar_Set( "ui_char_skin_legs", "lower_a1" );
	}
}

void UI_SetSiegeObjectiveGraphicPos( menuDef_t *menu, const char *itemName, const char *cvarName ) {
	itemDef_t	*item;
	char		cvarBuf[1024];
	const char	*holdVal;
	char		*holdBuf;

	item = Menu_FindItemByName( menu, itemName );

	if ( item ) {
		// get cvar data
		trap->Cvar_VariableStringBuffer( cvarName, cvarBuf, sizeof(cvarBuf) );

		holdBuf = cvarBuf;
		if ( String_Parse( &holdBuf, &holdVal ) ) {
			item->window.rectClient.x = atof( holdVal );
			if ( String_Parse( &holdBuf, &holdVal ) ) {
				item->window.rectClient.y = atof( holdVal );
				if ( String_Parse( &holdBuf, &holdVal ) ) {
					item->window.rectClient.w = atof( holdVal );
					if ( String_Parse( &holdBuf, &holdVal ) ) {
						item->window.rectClient.h = atof( holdVal );

						item->window.rect.x = item->window.rectClient.x;
						item->window.rect.y = item->window.rectClient.y;

						item->window.rect.w = item->window.rectClient.w;
						item->window.rect.h = item->window.rectClient.h;
					}
				}
			}
		}
	}
}

void UI_FindCurrentSiegeTeamClass( void ) {
	menuDef_t *menu;
	int myTeam = (int)(trap->Cvar_VariableValue( "ui_myteam" ));
	const char *itemname;
	itemDef_t *item;
	int	baseClass;

	menu = Menu_GetFocused();	// Get current menu

	if ( !menu ) {
		return;
	}

	if ( (myTeam != TEAM_RED) && (myTeam != TEAM_BLUE) ) {
		return;
	}

	// If the player is on a team,
	if ( myTeam == TEAM_RED ) {
		itemDef_t *item;
		item = (itemDef_t *)Menu_FindItemByName( menu, "onteam1" );
		if ( item ) {
			Item_RunScript( item, item->action );
		}
	}
	else if ( myTeam == TEAM_BLUE ) {
		itemDef_t *item;
		item = (itemDef_t *)Menu_FindItemByName( menu, "onteam2" );
		if ( item ) {
			Item_RunScript( item, item->action );
		}
	}


	baseClass = (int)trap->Cvar_VariableValue( "ui_siege_class" );

	// Find correct class button and activate it.
	if ( baseClass == SPC_INFANTRY ) {
		itemname = "class1_button";
	}
	else if ( baseClass == SPC_HEAVY_WEAPONS ) {
		itemname = "class2_button";
	}
	else if ( baseClass == SPC_DEMOLITIONIST ) {
		itemname = "class3_button";
	}
	else if ( baseClass == SPC_VANGUARD ) {
		itemname = "class4_button";
	}
	else if ( baseClass == SPC_SUPPORT ) {
		itemname = "class5_button";
	}
	else if ( baseClass == SPC_SUPPORT ) {
		itemname = "class5_button";
	}
	else if ( baseClass == SPC_JEDI ) {
		itemname = "class6_button";
	}
	else {
		return;
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, itemname );
	if ( item ) {
		Item_RunScript( item, item->action );
	}

}

void UI_UpdateSiegeObjectiveGraphics( void ) {
	menuDef_t *menu;
	int	teamI, objI;

	menu = Menu_GetFocused();	// Get current menu

	if ( !menu ) {
		return;
	}

	// Hiding a bunch of fields because the opening section of the siege menu was getting too long
	Menu_ShowGroup( menu, "class_button", qfalse );
	Menu_ShowGroup( menu, "class_count", qfalse );
	Menu_ShowGroup( menu, "feeders", qfalse );
	Menu_ShowGroup( menu, "classdescription", qfalse );
	Menu_ShowGroup( menu, "minidesc", qfalse );
	Menu_ShowGroup( menu, "obj_longdesc", qfalse );
	Menu_ShowGroup( menu, "objective_pic", qfalse );
	Menu_ShowGroup( menu, "stats", qfalse );
	Menu_ShowGroup( menu, "forcepowerlevel", qfalse );

	// Get objective icons for each team
	for ( teamI = 1; teamI < 3; teamI++ ) {
		for ( objI = 1; objI < 8; objI++ ) {
			Menu_SetItemBackground( menu, va( "tm%i_icon%i", teamI, objI ), va( "*team%i_objective%i_mapicon", teamI, objI ) );
			Menu_SetItemBackground( menu, va( "tm%i_l_icon%i", teamI, objI ), va( "*team%i_objective%i_mapicon", teamI, objI ) );
		}
	}

	// Now get their placement on the map
	for ( teamI = 1; teamI < 3; teamI++ ) {
		for ( objI = 1; objI < 8; objI++ ) {
			UI_SetSiegeObjectiveGraphicPos( menu, va( "tm%i_icon%i", teamI, objI ), va( "team%i_objective%i_mappos", teamI, objI ) );
		}
	}

}

saber_colors_t TranslateSaberColor( const char *name );

static void UI_UpdateSaberCvars( void ) {
	saber_colors_t colorI;

	if ( !Q_stricmpn( UI_Cvar_VariableString( "ui_saber_color" ), "rgb", 3 ) )
		trap->Cvar_Set( "cp_sbRGB1", va( "%i", ui_sab1_r.integer | ((ui_sab1_g.integer | (ui_sab1_b.integer << 8)) << 8) ) );
	else
		trap->Cvar_Set( "cp_sbRGB1", "0" );
	if ( !Q_stricmpn( UI_Cvar_VariableString( "ui_saber2_color" ), "rgb", 3 ) )
		trap->Cvar_Set( "cp_sbRGB2", va( "%i", ui_sab2_r.integer | ((ui_sab2_g.integer | (ui_sab2_b.integer << 8)) << 8) ) );
	else
		trap->Cvar_Set( "cp_sbRGB2", "0" );


	trap->Cvar_Set( "saber1", UI_Cvar_VariableString( "ui_saber" ) );
	trap->Cvar_Set( "saber2", UI_Cvar_VariableString( "ui_saber2" ) );

	colorI = TranslateSaberColor( UI_Cvar_VariableString( "ui_saber_color" ) );
	trap->Cvar_Set( "color1", va( "%d", colorI ) );
	trap->Cvar_Set( "g_saber_color", UI_Cvar_VariableString( "ui_saber_color" ) );

	colorI = TranslateSaberColor( UI_Cvar_VariableString( "ui_saber2_color" ) );
	trap->Cvar_Set( "color2", va( "%d", colorI ) );
	trap->Cvar_Set( "g_saber2_color", UI_Cvar_VariableString( "ui_saber2_color" ) );
}

// More hard coded goodness for the menus.
static void UI_SetSaberBoxesandHilts( void ) {
	menuDef_t *menu;
	itemDef_t *item;
	qboolean	getBig = qfalse;
	char sType[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if ( !menu ) {
		return;
	}

	trap->Cvar_VariableStringBuffer( "ui_saber_type", sType, sizeof(sType) );

	if ( Q_stricmp( "dual", sType ) != 0 ) {
		//		trap->Cvar_Set("ui_saber", "single_1");
		//		trap->Cvar_Set("ui_saber2", "single_1");
		getBig = qtrue;
	}

	else if ( Q_stricmp( "staff", sType ) != 0 ) {
		//		trap->Cvar_Set("ui_saber", "dual_1");
		//		trap->Cvar_Set("ui_saber2", "none");
		getBig = qtrue;
	}

	if ( !getBig ) {
		return;
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, "box2middle" );

	if ( item ) {
		item->window.rect.x = 212;
		item->window.rect.y = 126;
		item->window.rect.w = 219;
		item->window.rect.h = 44;
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, "box2bottom" );

	if ( item ) {
		item->window.rect.x = 212;
		item->window.rect.y = 170;
		item->window.rect.w = 219;
		item->window.rect.h = 60;
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, "box3middle" );

	if ( item ) {
		item->window.rect.x = 418;
		item->window.rect.y = 126;
		item->window.rect.w = 219;
		item->window.rect.h = 44;
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, "box3bottom" );

	if ( item ) {
		item->window.rect.x = 418;
		item->window.rect.y = 170;
		item->window.rect.w = 219;
		item->window.rect.h = 60;
	}
}

qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin );
qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name, int *runTimeLength );
qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName );

static void UI_UpdateSaberType( void ) {
	char sType[MAX_QPATH];
	trap->Cvar_VariableStringBuffer( "ui_saber_type", sType, sizeof(sType) );

	if ( Q_stricmp( "single", sType ) == 0 ||
		Q_stricmp( "staff", sType ) == 0 ) {
		trap->Cvar_Set( "ui_saber2", "" );
	}
}

static void UI_UpdateSaberHilt( qboolean secondSaber ) {
	menuDef_t *menu;
	itemDef_t *item;
	char model[MAX_QPATH], modelPath[MAX_QPATH], skinPath[MAX_QPATH];
	const char *itemName, *saberCvarName;
	int	animRunLength;

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if ( !menu ) {
		return;
	}

	if ( secondSaber ) {
		itemName = "saber2";
		saberCvarName = "ui_saber2";
	}
	else {
		itemName = "saber";
		saberCvarName = "ui_saber";
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, itemName );

	if ( !item ) {
		Com_Error( ERR_FATAL, "UI_UpdateSaberHilt: Could not find item (%s) in menu (%s)", itemName, menu->window.name );
		return;
	}

	trap->Cvar_VariableStringBuffer( saberCvarName, model, sizeof(model) );

	item->text = model;
	//read this from the sabers.cfg
	if ( UI_SaberModelForSaber( model, modelPath ) ) {//successfully found a model
		ItemParse_asset_model_go( item, modelPath, &animRunLength );//set the model
		//get the customSkin, if any
		//COM_StripExtension( modelPath, skinPath );
		//COM_DefaultExtension( skinPath, sizeof( skinPath ), ".skin" );
		if ( UI_SaberSkinForSaber( model, skinPath ) ) {
			ItemParse_model_g2skin_go( item, skinPath );//apply the skin
		}
		else {
			ItemParse_model_g2skin_go( item, NULL );//apply the skin
		}
	}
}

static void UI_UpdateSaberColor( qboolean secondSaber ) {
	//	Com_Printf( va( "ui_saber_color: %s\n", UI_Cvar_VariableString( "ui_saber_color" ) ) );

	//Raz: Reverse engineered JA+ code. Kill me.
	if ( !Q_stricmpn( UI_Cvar_VariableString( "ui_saber_color" ), "rgb", 3 ) )
		trap->Cvar_Set( "cp_sbRGB1", va( "%i", ui_sab1_r.integer | ((ui_sab1_g.integer | (ui_sab1_b.integer << 8)) << 8) ) );
	else
		trap->Cvar_Set( "cp_sbRGB1", "0" );
	if ( !Q_stricmpn( UI_Cvar_VariableString( "ui_saber2_color" ), "rgb", 3 ) )
		trap->Cvar_Set( "cp_sbRGB2", va( "%i", ui_sab2_r.integer | ((ui_sab2_g.integer | (ui_sab2_b.integer << 8)) << 8) ) );
	else
		trap->Cvar_Set( "cp_sbRGB2", "0" );
}

const char *SaberColorToString( saber_colors_t color );

void ParseRGBSaber( char * str, vector3 *c );
static void UI_GetSaberCvars( void ) {
	trap->Cvar_Set( "ui_saber", UI_Cvar_VariableString( "saber1" ) );
	trap->Cvar_Set( "ui_saber2", UI_Cvar_VariableString( "saber2" ) );

	int32_t tmp = trap->Cvar_VariableValue( "color1" );
	trap->Cvar_Set( "g_saber_color", SaberColorToString( (saber_colors_t)tmp ) );
	tmp = trap->Cvar_VariableValue( "color2" );
	trap->Cvar_Set( "g_saber2_color", SaberColorToString( (saber_colors_t)tmp ) );

	trap->Cvar_Set( "ui_saber_color", UI_Cvar_VariableString( "g_saber_color" ) );
	trap->Cvar_Set( "ui_saber2_color", UI_Cvar_VariableString( "g_saber2_color" ) );

	return;
}

qboolean ItemParse_model_g2anim_go( itemDef_t *item, const char *animName );
void UI_UpdateCharacterSkin( void ) {
	menuDef_t *menu;
	itemDef_t *item;
	char skin[MAX_QPATH];
	char model[MAX_QPATH];
	char head[MAX_QPATH];
	char torso[MAX_QPATH];
	char legs[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu

	if ( !menu ) {
		return;
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, "character" );

	if ( !item ) {
		Com_Error( ERR_FATAL, "UI_UpdateCharacterSkin: Could not find item (character) in menu (%s)", menu->window.name );
	}

	trap->Cvar_VariableStringBuffer( "ui_char_model", model, sizeof(model) );
	trap->Cvar_VariableStringBuffer( "ui_char_skin_head", head, sizeof(head) );
	trap->Cvar_VariableStringBuffer( "ui_char_skin_torso", torso, sizeof(torso) );
	trap->Cvar_VariableStringBuffer( "ui_char_skin_legs", legs, sizeof(legs) );

	Com_sprintf( skin, sizeof(skin), "models/players/%s/|%s|%s|%s",
		model,
		head,
		torso,
		legs
		);

	ItemParse_model_g2skin_go( item, skin );
}

static void UI_ResetCharacterListBoxes( void ) {

	itemDef_t *item;
	menuDef_t *menu;
	listBoxDef_t *listPtr;

	menu = Menu_GetFocused();

	if ( menu ) {
		item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "headlistbox" );
		if ( item ) {
			listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
			if ( listPtr ) {
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "torsolistbox" );
		if ( item ) {
			listPtr = (listBoxDef_t*)item->typeData;
			if ( listPtr ) {
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "lowerlistbox" );
		if ( item ) {
			listPtr = (listBoxDef_t*)item->typeData;
			if ( listPtr ) {
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "colorbox" );
		if ( item ) {
			listPtr = (listBoxDef_t*)item->typeData;
			if ( listPtr ) {
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}
	}
}

const char *saberSingleHiltInfo[MAX_SABER_HILTS];
const char *saberStaffHiltInfo[MAX_SABER_HILTS];

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName, int destsize );
void WP_SaberGetHiltInfo( const char *singleHilts[MAX_SABER_HILTS], const char *staffHilts[MAX_SABER_HILTS] );

static void UI_UpdateCharacter( qboolean changedModel ) {
	menuDef_t *menu;
	itemDef_t *item;
	char modelPath[MAX_QPATH];
	int	animRunLength;

	menu = Menu_GetFocused();	// Get current menu

	if ( !menu ) {
		return;
	}

	item = (itemDef_t *)Menu_FindItemByName( menu, "character" );

	if ( !item ) {
		Com_Error( ERR_FATAL, "UI_UpdateCharacter: Could not find item (character) in menu (%s)", menu->window.name );
	}

	ItemParse_model_g2anim_go( item, ui_char_anim.string );

	Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/model.glm", UI_Cvar_VariableString( "ui_char_model" ) );
	ItemParse_asset_model_go( item, modelPath, &animRunLength );

	if ( changedModel ) {//set all skins to first skin since we don't know you always have all skins
		//FIXME: could try to keep the same spot in each list as you swtich models
		UI_FeederSelection( FEEDER_PLAYER_SKIN_HEAD, 0, item );	//fixme, this is not really the right item!!
		UI_FeederSelection( FEEDER_PLAYER_SKIN_TORSO, 0, item );
		UI_FeederSelection( FEEDER_PLAYER_SKIN_LEGS, 0, item );
		UI_FeederSelection( FEEDER_COLORCHOICES, 0, item );
	}
	UI_UpdateCharacterSkin();
}

static void UI_CheckServerName( void ) {
	qboolean	changed = qfalse;

	char hostname[MAX_HOSTNAMELENGTH] = { 0 };
	char *c = hostname;

	trap->Cvar_VariableStringBuffer( "sv_hostname", hostname, sizeof(hostname) );

	while ( *c ) {
		if ( (*c == '\\') || (*c == ';') || (*c == '"') ) {
			*c = '.';
			changed = qtrue;
		}
		c++;
	}
	if ( changed ) {
		trap->Cvar_Set( "sv_hostname", hostname );
	}

}

static qboolean UI_CheckPassword( void ) {
	static char info[MAX_STRING_CHARS];

	int index = uiInfo.serverStatus.currentServer;
	if ( (index < 0) || (index >= uiInfo.serverStatus.numDisplayServers) ) {	// warning?
		return qfalse;
	}

	trap->LAN_GetServerInfo( UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS );

	if ( atoi( Info_ValueForKey( info, "needpass" ) ) ) {

		Menus_OpenByName( "password_request" );
		return qfalse;

	}

	// This isn't going to make it (too late in dev), like James said I should check to see when we receive
	// a packet *if* we do indeed get a 0 ping just make it 1 so then a 0 ping is guaranteed to be bad
	/*
	// also check ping!
	ping = atoi(Info_ValueForKey(info, "ping"));
	// NOTE : PING -- it's very questionable as to whether a ping of < 0 or <= 0 indicates a bad server
	// what I do know, is that getting "ping" from the ServerInfo on a bad server returns 0.
	// So I'm left with no choice but to not allow you to enter a server with a ping of 0
	if( ping <= 0 )
	{
	Menus_OpenByName("bad_server");
	return qfalse;
	}
	*/

	return qtrue;
}

static void UI_JoinServer( void ) {
	char buff[1024] = { 0 };

	//	trap->Cvar_Set("cg_thirdPerson", "0");
	trap->Cvar_Set( "cg_cameraOrbit", "0" );
	trap->Cvar_Set( "ui_singlePlayerActive", "0" );
	if ( uiInfo.serverStatus.currentServer >= 0 && uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers ) {
		trap->LAN_GetServerAddressString( UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, sizeof(buff) );
		trap->Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buff ) );
	}
}

int UI_SiegeClassNum( siegeClass_t *scl ) {
	int i = 0;
	for ( i = 0; i < bgNumSiegeClasses; i++ ) {
		if ( &bgSiegeClasses[i] == scl )
			return i;
	}

	return 0;
}

//called every time a class is selected from a feeder, sets info for shaders to be displayed in the menu about the class -rww
void UI_SiegeSetCvarsForClass( siegeClass_t *scl ) {
	int i = 0;
	int count = 0;
	char shader[MAX_QPATH];

	//let's clear the things out first
	while ( i < WP_NUM_WEAPONS ) {
		trap->Cvar_Set( va( "ui_class_weapon%i", i ), "gfx/2d/select" );
		i++;
	}
	//now for inventory items
	i = 0;
	while ( i < HI_NUM_HOLDABLE ) {
		trap->Cvar_Set( va( "ui_class_item%i", i ), "gfx/2d/select" );
		i++;
	}
	//now for force powers
	i = 0;
	while ( i < NUM_FORCE_POWERS ) {
		trap->Cvar_Set( va( "ui_class_power%i", i ), "gfx/2d/select" );
		i++;
	}

	//now health and armor
	trap->Cvar_Set( "ui_class_health", "0" );
	trap->Cvar_Set( "ui_class_armor", "0" );

	trap->Cvar_Set( "ui_class_icon", "" );

	if ( !scl ) { //no select?
		return;
	}

	//set cvars for which weaps we have
	i = 0;
	trap->Cvar_Set( va( "ui_class_weapondesc%i", count ), " " );	// Blank it out to start with
	while ( i < WP_NUM_WEAPONS ) {

		if ( scl->weapons & (1 << i) ) {
			if ( i == WP_SABER ) { //we want to see what kind of saber they have, and set the cvar based on that
				char saberType[1024];

				if ( scl->saber1[0] &&
					scl->saber2[0] ) {
					strcpy( saberType, "gfx/hud/w_icon_duallightsaber" );
				} //fixme: need saber data access on ui to determine if staff, "gfx/hud/w_icon_saberstaff"
				else {
					char buf[1024];
					if ( scl->saber1[0] && UI_SaberTypeForSaber( scl->saber1, buf ) ) {
						if ( !Q_stricmp( buf, "SABER_STAFF" ) ) {
							strcpy( saberType, "gfx/hud/w_icon_saberstaff" );
						}
						else {
							strcpy( saberType, "gfx/hud/w_icon_lightsaber" );
						}
					}
					else {
						strcpy( saberType, "gfx/hud/w_icon_lightsaber" );
					}
				}

				trap->Cvar_Set( va( "ui_class_weapon%i", count ), saberType );
				trap->Cvar_Set( va( "ui_class_weapondesc%i", count ), "@MENUS_AN_ELEGANT_WEAPON_FOR" );
				count++;
				trap->Cvar_Set( va( "ui_class_weapondesc%i", count ), " " );	// Blank it out to start with
			}
			else {
				const gitem_t *item = BG_FindItemForWeapon( (weapon_t)i );
				trap->Cvar_Set( va( "ui_class_weapon%i", count ), item->icon );
				trap->Cvar_Set( va( "ui_class_weapondesc%i", count ), item->description );
				count++;
				trap->Cvar_Set( va( "ui_class_weapondesc%i", count ), " " );	// Blank it out to start with
			}
		}

		i++;
	}

	//now for inventory items
	i = 0;
	count = 0;

	while ( i < HI_NUM_HOLDABLE ) {
		if ( scl->invenItems & (1 << i) ) {
			const gitem_t *item = BG_FindItemForHoldable( (holdable_t)i );
			trap->Cvar_Set( va( "ui_class_item%i", count ), item->icon );
			trap->Cvar_Set( va( "ui_class_itemdesc%i", count ), item->description );
			count++;
		}
		else {
			trap->Cvar_Set( va( "ui_class_itemdesc%i", count ), " " );
		}
		i++;
	}

	//now for force powers
	i = 0;
	count = 0;

	while ( i < NUM_FORCE_POWERS ) {
		trap->Cvar_Set( va( "ui_class_powerlevel%i", i ), "0" );	// Zero this out to start.
		if ( i < 9 ) {
			trap->Cvar_Set( va( "ui_class_powerlevelslot%i", i ), "0" );	// Zero this out to start.
		}

		if ( scl->forcePowerLevels[i] ) {
			trap->Cvar_Set( va( "ui_class_powerlevel%i", count ), va( "%i", scl->forcePowerLevels[i] ) );
			trap->Cvar_Set( va( "ui_class_power%i", count ), HolocronIcons[i] );
			count++;
		}

		i++;
	}

	//now health and armor
	trap->Cvar_Set( "ui_class_health", va( "%i", scl->maxhealth ) );
	trap->Cvar_Set( "ui_class_armor", va( "%i", scl->maxarmor ) );
	trap->Cvar_Set( "ui_class_speed", va( "%3.2f", scl->speed ) );

	//now get the icon path based on the shader index
	if ( scl->classShader ) {
		trap->R_ShaderNameFromIndex( shader, scl->classShader );
	}
	else { //no shader
		shader[0] = 0;
	}
	trap->Cvar_Set( "ui_class_icon", shader );
}

static int g_siegedFeederForcedSet = 0;
void UI_UpdateCvarsForClass( const int team, const int baseClass, const int index ) {
	siegeClass_t *holdClass = 0;
	char *holdBuf;

	// Is it a valid team
	if ( (team == SIEGETEAM_TEAM1) ||
		(team == SIEGETEAM_TEAM2) ) {

		// Is it a valid base class?
		if ( (baseClass >= SPC_INFANTRY) && (baseClass < SPC_MAX) ) {
			// A valid index?
			if ( (index >= 0) && (index < BG_SiegeCountBaseClass( team, baseClass )) ) {
				if ( !g_siegedFeederForcedSet ) {
					holdClass = BG_GetClassOnBaseClass( team, baseClass, index );
					if ( holdClass )	//clicked a valid item
					{
						g_UIGloballySelectedSiegeClass = UI_SiegeClassNum( holdClass );
						trap->Cvar_Set( "ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc );
						g_siegedFeederForcedSet = 1;
						Menu_SetFeederSelection( NULL, FEEDER_SIEGE_BASE_CLASS, -1, NULL );
						UI_SiegeSetCvarsForClass( holdClass );

						holdBuf = BG_GetUIPortraitFile( team, baseClass, index );
						if ( holdBuf ) {
							trap->Cvar_Set( "ui_classPortrait", holdBuf );
						}
					}
				}
				g_siegedFeederForcedSet = 0;
			}
			else {
				trap->Cvar_Set( "ui_classDesc", " " );
			}
		}
	}

}

void UI_ClampMaxPlayers( void ) {
	// duel requires 2 players
	if ( ui_netGameType.integer == GT_DUEL ) {
		if ( (int)trap->Cvar_VariableValue( "sv_maxClients" ) < 2 )
			trap->Cvar_Set( "sv_maxClients", "2" );
	}

	// power duel requires 3 players
	else if ( ui_netGameType.integer == GT_POWERDUEL ) {
		if ( (int)trap->Cvar_VariableValue( "sv_maxClients" ) < 3 )
			trap->Cvar_Set( "sv_maxClients", "3" );
	}

	// can never exceed MAX_CLIENTS
	if ( (int)trap->Cvar_VariableValue( "sv_maxClients" ) > MAX_CLIENTS ) {
		trap->Cvar_Set( "sv_maxClients", XSTRING( MAX_CLIENTS ) );
	}
}

void UI_UpdateSiegeStatusIcons( void ) {
	menuDef_t *menu = NULL;

	if ( (menu = Menu_GetFocused()) ) {
		int i = 0;
		for ( i = 0; i < 7; i++ ) {
			Menu_SetItemBackground( menu, va( "wpnicon0%d", i ), va( "*ui_class_weapon%d", i ) );
		}
		for ( i = 0; i < 7; i++ ) {
			Menu_SetItemBackground( menu, va( "itemicon0%d", i ), va( "*ui_class_item%d", i ) );
		}
		for ( i = 0; i < 15; i++ ) {
			Menu_SetItemBackground( menu, va( "forceicon%02d", i ), va( "*ui_class_power%d", i ) );
		}
	}
}

static void UI_RunMenuScript( char **args ) {
	const char *name, *name2;
	char buff[1024];

	if ( String_Parse( args, &name ) ) {
		if ( Q_stricmp( name, "StartServer" ) == 0 ) {
			int i, added = 0;
			float skill;
			int warmupTime = 0;
			int doWarmup = 0;

			//	trap->Cvar_Set("cg_thirdPerson", "0");
			trap->Cvar_Set( "cg_cameraOrbit", "0" );
			// for Solo games I set this to 1 in the menu and don't want it stomped here,
			// this cvar seems to be reset to 0 in all the proper places so... -dmv
			//	trap->Cvar_Set("ui_singlePlayerActive", "0");

			// if a solo game is started, automatically turn dedicated off here (don't want to do it in the menu, might get annoying)
			if ( trap->Cvar_VariableValue( "ui_singlePlayerActive" ) ) {
				trap->Cvar_Set( "dedicated", "0" );
			}
			else {
				trap->Cvar_SetValue( "dedicated", Q_clamp( 0, ui_dedicated.integer, 2 ) );
			}

			trap->Cvar_SetValue( "g_gametype", Q_clamp( 0, ui_netGameType.integer, GT_MAX_GAME_TYPE ) );

			//trap->Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
			//trap->Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));
			trap->Cmd_ExecuteText( EXEC_APPEND, va( "wait 2; map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ) );
			skill = trap->Cvar_VariableValue( "g_spSkill" );

			//Cap the warmup values in case the user tries a dumb setting.
			warmupTime = trap->Cvar_VariableValue( "g_warmup" );
			doWarmup = trap->Cvar_VariableValue( "g_doWarmup" );

			if ( doWarmup && warmupTime < 1 ) {
				trap->Cvar_Set( "g_doWarmup", "0" );
			}
			if ( warmupTime < 5 ) {
				trap->Cvar_Set( "g_warmup", "5" );
			}
			if ( warmupTime > 120 ) {
				trap->Cvar_Set( "g_warmup", "120" );
			}

			if ( trap->Cvar_VariableValue( "g_gametype" ) == GT_DUEL ||
				trap->Cvar_VariableValue( "g_gametype" ) == GT_POWERDUEL ) { //always set fraglimit 1 when starting a duel game
				trap->Cvar_Set( "fraglimit", "1" );
				trap->Cvar_Set( "timelimit", "0" );
			}

			for ( i = 0; i < PLAYERS_PER_TEAM; i++ ) {
				int bot = trap->Cvar_VariableValue( va( "ui_blueteam%i", i + 1 ) );
				int maxcl = trap->Cvar_VariableValue( "sv_maxClients" );

				if ( bot > 1 ) {
					int numval = i + 1;

					numval *= 2;

					numval -= 1;

					if ( numval <= maxcl ) {
						if ( ui_netGameType.integer >= GT_TEAM ) {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber( bot - 2 ), skill, "Blue" );
						}
						else {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber( bot - 2 ), skill );
						}
						trap->Cmd_ExecuteText( EXEC_APPEND, buff );
						added++;
					}
				}
				bot = trap->Cvar_VariableValue( va( "ui_redteam%i", i + 1 ) );
				if ( bot > 1 ) {
					int numval = i + 1;

					numval *= 2;

					if ( numval <= maxcl ) {
						if ( ui_netGameType.integer >= GT_TEAM ) {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber( bot - 2 ), skill, "Red" );
						}
						else {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber( bot - 2 ), skill );
						}
						trap->Cmd_ExecuteText( EXEC_APPEND, buff );
						added++;
					}
				}
				if ( added >= maxcl ) { //this means the client filled up all their slots in the UI with bots. So stretch out an extra slot for them, and then stop adding bots.
					trap->Cvar_Set( "sv_maxClients", va( "%i", added + 1 ) );
					break;
				}
			}
		}
		else if ( Q_stricmp( name, "updateSPMenu" ) == 0 ) {
			UI_SetCapFragLimits( qtrue );
			UI_MapCountByGameType( qtrue );
			trap->Cvar_SetValue( "ui_mapIndex", UI_GetIndexFromSelection( ui_currentMap.integer ) );
			trap->Cvar_Update( &ui_mapIndex );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, ui_mapIndex.integer, "skirmish" );
			UI_GameType_HandleKey( 0, 0, A_MOUSE1, qfalse );
			UI_GameType_HandleKey( 0, 0, A_MOUSE2, qfalse );
		}
		else if ( Q_stricmp( name, "resetDefaults" ) == 0 ) {
			trap->Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n" );
			trap->Cmd_ExecuteText( EXEC_APPEND, "exec mpdefault.cfg\n" );
			trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );

		}
		else if ( Q_stricmp( name, "loadArenas" ) == 0 ) {
			UI_LoadArenas();
			UI_MapCountByGameType( qfalse );
			Menu_SetFeederSelection( NULL, FEEDER_ALLMAPS, gUISelectedMap, "createserver" );
			uiForceRank = trap->Cvar_VariableValue( "g_maxForceRank" );
		}
		else if ( Q_stricmp( name, "saveControls" ) == 0 ) {
			Controls_SetConfig( qtrue );
		}
		else if ( Q_stricmp( name, "loadControls" ) == 0 ) {
			Controls_GetConfig();
		}
		else if ( Q_stricmp( name, "clearError" ) == 0 ) {
			trap->Cvar_Set( "com_errorMessage", "" );
		}
		else if ( Q_stricmp( name, "loadGameInfo" ) == 0 ) {
			// ...
		}
		else if ( Q_stricmp( name, "RefreshServers" ) == 0 ) {
			UI_StartServerRefresh( qtrue );
			UI_BuildServerDisplayList( qtrue );
		}
		else if ( Q_stricmp( name, "RefreshFilter" ) == 0 ) {
			UI_StartServerRefresh( qfalse );
			UI_BuildServerDisplayList( qtrue );
		}
		else if ( Q_stricmp( name, "LoadDemos" ) == 0 ) {
			UI_LoadDemos();
		}
		else if ( Q_stricmp( name, "LoadMods" ) == 0 ) {
			UI_LoadMods();
		}
		else if ( Q_stricmp( name, "RunMod" ) == 0 ) {
			trap->Cvar_Set( "fs_game", uiInfo.modList[uiInfo.modIndex].modName );
			trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		}
		else if ( Q_stricmp( name, "RunDemo" ) == 0 ) {
			trap->Cmd_ExecuteText( EXEC_APPEND, va( "demo \"%s\"\n", uiInfo.demoList[uiInfo.demoIndex] ) );
		}
		else if ( Q_stricmp( name, "Quake3" ) == 0 ) {
			trap->Cvar_Set( "fs_game", "" );
			trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		}
		else if ( Q_stricmp( name, "closeJoin" ) == 0 ) {
			if ( uiInfo.serverStatus.refreshActive ) {
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList( qtrue );
			}
			else {
				Menus_CloseByName( "joinserver" );
				Menus_OpenByName( "main" );
			}
		}
		else if ( Q_stricmp( name, "StopRefresh" ) == 0 ) {
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		}
		else if ( Q_stricmp( name, "UpdateFilter" ) == 0 ) {
			trap->Cvar_Update( &ui_netSource );
			if ( ui_netSource.integer == UIAS_LOCAL || !uiInfo.serverStatus.numDisplayServers ) {
				UI_StartServerRefresh( qtrue );
			}
			UI_BuildServerDisplayList( qtrue );
			UI_FeederSelection( FEEDER_SERVERS, 0, NULL );

			UI_LoadMods();
		}
		else if ( Q_stricmp( name, "ServerStatus" ) == 0 ) {
			trap->LAN_GetServerAddressString( UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], uiInfo.serverStatusAddress, sizeof(uiInfo.serverStatusAddress) );
			UI_BuildServerStatus( qtrue );
		}
		else if ( Q_stricmp( name, "FoundPlayerServerStatus" ) == 0 ) {
			Q_strncpyz( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress) );
			UI_BuildServerStatus( qtrue );
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp( name, "FindPlayer" ) == 0 ) {
			UI_BuildFindPlayerList( qtrue );
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp( name, "checkservername" ) == 0 ) {
			UI_CheckServerName();
		}
		else if ( Q_stricmp( name, "checkpassword" ) == 0 ) {
			if ( UI_CheckPassword() ) {
				UI_JoinServer();
			}
		}
		else if ( Q_stricmp( name, "JoinServer" ) == 0 ) {
			UI_JoinServer();
		}
		else if ( Q_stricmp( name, "FoundPlayerJoinServer" ) == 0 ) {
			trap->Cvar_Set( "ui_singlePlayerActive", "0" );
			if ( uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers ) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer] ) );
			}
		}
		else if ( Q_stricmp( name, "Quit" ) == 0 ) {
			trap->Cvar_Set( "ui_singlePlayerActive", "0" );
			trap->Cmd_ExecuteText( EXEC_NOW, "quit" );
		}
		else if ( Q_stricmp( name, "Controls" ) == 0 ) {
			trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "setup_menu2" );
		}
		else if ( Q_stricmp( name, "Leave" ) == 0 ) {
			trap->Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "main" );
		}
		else if ( Q_stricmp( name, "getvideosetup" ) == 0 ) {
			UI_GetVideoSetup();
		}
		else if ( Q_stricmp( name, "getsaberhiltinfo" ) == 0 ) {
			WP_SaberGetHiltInfo( saberSingleHiltInfo, saberStaffHiltInfo );
		}
		// On the solo game creation screen, we can't see siege maps
		else if ( Q_stricmp( name, "checkforsiege" ) == 0 ) {
			if ( ui_netGameType.integer == GT_SIEGE ) {
				// fake out the handler to advance to the next game type
				UI_NetGameType_HandleKey( 0, NULL, A_MOUSE1 );
			}
		}
		else if ( Q_stricmp( name, "updatevideosetup" ) == 0 ) {
			UI_UpdateVideoSetup();
		}
		else if ( Q_stricmp( name, "ServerSort" ) == 0 ) {
			int sortColumn;
			if ( Int_Parse( args, &sortColumn ) ) {
				// if same column we're already sorting on then flip the direction
				if ( sortColumn == uiInfo.serverStatus.sortKey ) {
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}
				// make sure we sort again
				UI_ServersSort( sortColumn, qtrue );
			}
		}
		else if ( Q_stricmp( name, "nextSkirmish" ) == 0 ) {
			UI_StartSkirmish( qtrue );
		}
		else if ( Q_stricmp( name, "SkirmishStart" ) == 0 ) {
			UI_StartSkirmish( qfalse );
		}
		else if ( Q_stricmp( name, "closeingame" ) == 0 ) {
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
		}
		else if ( Q_stricmp( name, "voteMap" ) == 0 ) {
			if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount ) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "callvote map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ) );
			}
		}
		else if ( Q_stricmp( name, "voteKick" ) == 0 ) {
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount ) {
				//trap->Cmd_ExecuteText( EXEC_APPEND, va("callvote kick \"%s\"\n",uiInfo.playerNames[uiInfo.playerIndex]) );
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "callvote clientkick \"%i\"\n", uiInfo.playerIndexes[uiInfo.playerIndex] ) );
			}
		}
		else if ( Q_stricmp( name, "voteGame" ) == 0 ) {
			if ( ui_netGameType.integer >= 0 && ui_netGameType.integer < GT_MAX_GAME_TYPE ) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "callvote g_gametype %i\n", ui_netGameType.integer ) );
			}
		}
		else if ( Q_stricmp( name, "addBot" ) == 0 ) {
			if ( trap->Cvar_VariableValue( "g_gametype" ) >= GT_TEAM ) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "addbot \"%s\" %i %s\n", UI_GetBotNameByNumber( uiInfo.botIndex ), uiInfo.skillIndex + 1, (uiInfo.redBlue == 0) ? "Red" : "Blue" ) );
			}
			else {
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "addbot \"%s\" %i %s\n", UI_GetBotNameByNumber( uiInfo.botIndex ), uiInfo.skillIndex + 1, (uiInfo.redBlue == 0) ? "Red" : "Blue" ) );
			}
		}
		else if ( Q_stricmp( name, "addFavorite" ) == 0 ) {
			if ( ui_netSource.integer != UIAS_FAVORITES ) {
				char name[MAX_HOSTNAMELENGTH];
				char addr[MAX_ADDRESSLENGTH];
				int res;

				trap->LAN_GetServerInfo( UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS );
				name[0] = addr[0] = '\0';
				Q_strncpyz( name, Info_ValueForKey( buff, "hostname" ), sizeof(name) );
				Q_strncpyz( addr, Info_ValueForKey( buff, "addr" ), sizeof(addr) );
				if ( strlen( name ) > 0 && strlen( addr ) > 0 ) {
					res = trap->LAN_AddServer( AS_FAVORITES, name, addr );
					if ( res == 0 ) {
						// server already in the list
						Com_Printf( "Favorite already in list\n" );
					}
					else if ( res == -1 ) {
						// list full
						Com_Printf( "Favorite list full\n" );
					}
					else {
						// successfully added
						Com_Printf( "Added favorite server %s\n", addr );

						/*
						trap->SE_GetStringTextString(
							va( "%s_GETTINGINFOFORSERVERS", uiInfo.uiDC.Assets.stringedFile ),
							holdSPString, sizeof(holdSPString)
						);
						Text_Paint( rect->x, rect->y, scale, newColor,
							va( holdSPString, trap->LAN_GetServerCount( ui_netSource.integer ) ), 0, 0, textStyle,
							false
						);
						*/
					}
				}
			}
		}
		else if ( Q_stricmp( name, "deleteFavorite" ) == 0 ) {
			if ( ui_netSource.integer == UIAS_FAVORITES ) {
				char addr[MAX_ADDRESSLENGTH];
				trap->LAN_GetServerInfo( AS_FAVORITES, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS );
				addr[0] = '\0';
				Q_strncpyz( addr, Info_ValueForKey( buff, "addr" ), sizeof(addr) );
				if ( strlen( addr ) > 0 ) {
					trap->LAN_RemoveServer( AS_FAVORITES, addr );
				}
			}
		}
		else if ( Q_stricmp( name, "createFavorite" ) == 0 ) {
			//	if (ui_netSource.integer == UIAS_FAVORITES)
			//rww - don't know why this check was here.. why would you want to only add new favorites when the filter was favorites?
			{
				char name[MAX_HOSTNAMELENGTH];
				char addr[MAX_ADDRESSLENGTH];
				int res;

				name[0] = addr[0] = '\0';
				Q_strncpyz( name, UI_Cvar_VariableString( "ui_favoriteName" ), sizeof(name) );
				Q_strncpyz( addr, UI_Cvar_VariableString( "ui_favoriteAddress" ), sizeof(addr) );
				if (/*strlen(name) > 0 &&*/ strlen( addr ) > 0 ) {
					res = trap->LAN_AddServer( AS_FAVORITES, name, addr );
					if ( res == 0 ) {
						// server already in the list
						Com_Printf( "Favorite already in list\n" );
					}
					else if ( res == -1 ) {
						// list full
						Com_Printf( "Favorite list full\n" );
					}
					else {
						// successfully added
						Com_Printf( "Added favorite server %s\n", addr );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "setForce" ) == 0 ) {
			const char *teamArg;

			if ( String_Parse( args, &teamArg ) ) {
				if ( Q_stricmp( "none", teamArg ) == 0 ) {
					UI_UpdateClientForcePowers( NULL );
				}
				else if ( Q_stricmp( "same", teamArg ) == 0 ) {//stay on current team
					int myTeam = (int)(trap->Cvar_VariableValue( "ui_myteam" ));
					if ( myTeam != TEAM_SPECTATOR ) {
						UI_UpdateClientForcePowers( UI_TeamName( myTeam ) );//will cause him to respawn, if it's been 5 seconds since last one
					}
					else {
						UI_UpdateClientForcePowers( NULL );//just update powers
					}
				}
				else {
					UI_UpdateClientForcePowers( teamArg );
				}
			}
			else {
				UI_UpdateClientForcePowers( NULL );
			}
		}
		else if ( Q_stricmp( name, "setsiegeclassandteam" ) == 0 ) {
			int team = (int)trap->Cvar_VariableValue( "ui_holdteam" );
			int oldteam = (int)trap->Cvar_VariableValue( "ui_startsiegeteam" );
			qboolean	goTeam = qtrue;
			char	newclassString[512];
			char	startclassString[512];

			trap->Cvar_VariableStringBuffer( "ui_mySiegeClass", newclassString, sizeof(newclassString) );
			trap->Cvar_VariableStringBuffer( "ui_startsiegeclass", startclassString, sizeof(startclassString) );

			// Was just a spectator - is still just a spectator
			if ( (oldteam == team) && (oldteam == 3) ) {
				goTeam = qfalse;
			}
			// If new team and class match old team and class, just return to the game.
			else if ( oldteam == team ) {	// Classes match?
				if ( g_UIGloballySelectedSiegeClass != -1 ) {
					if ( !strcmp( startclassString, bgSiegeClasses[g_UIGloballySelectedSiegeClass].name ) ) {
						goTeam = qfalse;
					}
				}
			}

			if ( goTeam ) {
				if ( team == 1 )	// Team red
				{
					trap->Cvar_Set( "ui_team", va( "%d", team ) );
				}
				else if ( team == 2 )	// Team blue
				{
					trap->Cvar_Set( "ui_team", va( "%d", team ) );
				}
				else if ( team == 3 )	// Team spectator
				{
					trap->Cvar_Set( "ui_team", va( "%d", team ) );
				}

				if ( g_UIGloballySelectedSiegeClass != -1 ) {
					trap->Cmd_ExecuteText( EXEC_APPEND, va( "siegeclass \"%s\"\n", bgSiegeClasses[g_UIGloballySelectedSiegeClass].name ) );
				}
			}
		}
		else if ( Q_stricmp( name, "setBotButton" ) == 0 ) {
			UI_SetBotButton();
		}
		else if ( Q_stricmp( name, "saveTemplate" ) == 0 ) {
			UI_SaveForceTemplate();
		}
		else if ( Q_stricmp( name, "refreshForce" ) == 0 ) {
			UI_UpdateForcePowers();
		}
		else if ( Q_stricmp( name, "glCustom" ) == 0 ) {
			trap->Cvar_Set( "ui_r_glCustom", "4" );
		}
		else if ( Q_stricmp( name, "setMovesListDefault" ) == 0 ) {
			uiInfo.movesTitleIndex = 2;
		}
		else if ( Q_stricmp( name, "resetMovesList" ) == 0 ) {
			menuDef_t *menu;
			menu = Menus_FindByName( "rulesMenu_moves" );
			//update saber models
			if ( menu ) {
				itemDef_t *item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "character" );
				if ( item ) {
					//Raz: From JA+
					if ( uiInfo.movesTitleIndex < 6 )
						UI_SaberAttachToChar( item );
				}
			}

			trap->Cvar_Set( "ui_move_desc", " " );
		}
		else if ( Q_stricmp( name, "resetcharacterlistboxes" ) == 0 ) {
			UI_ResetCharacterListBoxes();
		}
		else if ( Q_stricmp( name, "setMoveCharacter" ) == 0 ) {
			itemDef_t *item;
			menuDef_t *menu;
			modelDef_t *modelPtr;
			int	animRunLength;

			UI_GetCharacterCvars();

			uiInfo.movesTitleIndex = 0;

			menu = Menus_FindByName( "rulesMenu_moves" );

			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "character" );
				if ( item ) {
					modelPtr = (modelDef_t*)item->typeData;
					if ( modelPtr ) {
						char modelPath[MAX_QPATH];

						uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
						ItemParse_model_g2anim_go( item, uiInfo.movesBaseAnim );
						uiInfo.moveAnimTime = 0;

						Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/model.glm", UI_Cvar_VariableString( "ui_char_model" ) );
						ItemParse_asset_model_go( item, modelPath, &animRunLength );

						UI_UpdateCharacterSkin();
						UI_SaberAttachToChar( item );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "character" ) == 0 ) {
			UI_UpdateCharacter( qfalse );
		}
		else if ( Q_stricmp( name, "characterchanged" ) == 0 ) {
			UI_UpdateCharacter( qtrue );
		}
		else if ( Q_stricmp( name, "updatecharcvars" ) == 0
			|| (Q_stricmp( name, "updatecharmodel" ) == 0) ) {
			UI_UpdateCharacterCvars();
		}
		else if ( Q_stricmp( name, "getcharcvars" ) == 0 ) {
			UI_GetCharacterCvars();
		}
		else if ( Q_stricmp( name, "char_skin" ) == 0 ) {
			UI_UpdateCharacterSkin();
		}
		// If this is siege, change all the bots to humans, because we faked it earlier
		//  swapping humans for bots on the menu
		else if ( Q_stricmp( name, "setSiegeNoBots" ) == 0 ) {
			int blueValue, redValue, i;

			if ( ui_netGameType.integer == GT_SIEGE ) {
				//hmm, I guess I'll set bot_minplayers to 0 here too. -rww
				trap->Cvar_Set( "bot_minplayers", "0" );

				for ( i = 1; i<9; i++ ) {
					blueValue = trap->Cvar_VariableValue( va( "ui_blueteam%i", i ) );
					if ( blueValue>1 ) {
						trap->Cvar_Set( va( "ui_blueteam%i", i ), "1" );
					}

					redValue = trap->Cvar_VariableValue( va( "ui_redteam%i", i ) );
					if ( redValue > 1 ) {
						trap->Cvar_Set( va( "ui_redteam%i", i ), "1" );
					}

				}
			}
		}
		else if ( Q_stricmp( name, "clearmouseover" ) == 0 ) {
			itemDef_t *item;
			menuDef_t *menu = Menu_GetFocused();

			if ( menu ) {
				int count, j;
				const char *itemName;
				String_Parse( args, &itemName );

				count = Menu_ItemsMatchingGroup( menu, itemName );

				for ( j = 0; j < count; j++ ) {
					item = Menu_GetMatchingItemByNumber( menu, j, itemName );
					if ( item != NULL ) {
						item->window.flags &= ~WINDOW_MOUSEOVER;
					}
				}
			}
		}
		else if ( Q_stricmp( name, "updateForceStatus" ) == 0 ) {
			UpdateForceStatus();
		}
		else if ( Q_stricmp( name, "update" ) == 0 ) {
			if ( String_Parse( args, &name2 ) ) {
				UI_Update( name2 );
			}
		}
		else if ( Q_stricmp( name, "setBotButtons" ) == 0 ) {
			UpdateBotButtons();
		}
		else if ( Q_stricmp( name, "getsabercvars" ) == 0 ) {
			UI_GetSaberCvars();
		}
		else if ( Q_stricmp( name, "setsaberboxesandhilts" ) == 0 ) {
			UI_SetSaberBoxesandHilts();
		}
		else if ( Q_stricmp( name, "saber_type" ) == 0 ) {
			UI_UpdateSaberType();
		}
		else if ( Q_stricmp( name, "saber_hilt" ) == 0 ) {
			UI_UpdateSaberHilt( qfalse );
		}
		else if ( Q_stricmp( name, "saber_color" ) == 0 ) {
			UI_UpdateSaberColor( qfalse );
		}
		else if ( Q_stricmp( name, "setscreensaberhilt" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "hiltbut" );
				if ( item ) {
					if ( saberSingleHiltInfo[item->cursorPos] ) {
						trap->Cvar_Set( "ui_saber", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "setscreensaberhilt1" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "hiltbut1" );
				if ( item ) {
					if ( saberSingleHiltInfo[item->cursorPos] ) {
						trap->Cvar_Set( "ui_saber", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "setscreensaberhilt2" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "hiltbut2" );
				if ( item ) {
					if ( saberSingleHiltInfo[item->cursorPos] ) {
						trap->Cvar_Set( "ui_saber2", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "setscreensaberstaff" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "hiltbut_staves" );
				if ( item ) {
					if ( saberSingleHiltInfo[item->cursorPos] ) {
						trap->Cvar_Set( "ui_saber", saberStaffHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "saber2_hilt" ) == 0 ) {
			UI_UpdateSaberHilt( qtrue );
		}
		else if ( Q_stricmp( name, "saber2_color" ) == 0 ) {
			UI_UpdateSaberColor( qtrue );
		}
		else if ( Q_stricmp( name, "updatesabercvars" ) == 0 ) {
			UI_UpdateSaberCvars();
		}
		else if ( Q_stricmp( name, "updatesiegeobjgraphics" ) == 0 ) {
			int team = (int)trap->Cvar_VariableValue( "ui_team" );
			trap->Cvar_Set( "ui_holdteam", va( "%d", team ) );

			UI_UpdateSiegeObjectiveGraphics();
		}
		else if ( Q_stricmp( name, "setsiegeobjbuttons" ) == 0 ) {
			const char *itemArg;
			const char *cvarLitArg;
			const char *cvarNormalArg;
			char	string[512];
			char	string2[512];
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				// Set the new item to the background
				if ( String_Parse( args, &itemArg ) ) {

					// Set the old button to it's original background
					trap->Cvar_VariableStringBuffer( "currentObjMapIconItem", string, sizeof(string) );
					item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, string );
					if ( item ) {
						// A cvar holding the name of a cvar - how crazy is that?
						trap->Cvar_VariableStringBuffer( "currentObjMapIconBackground", string, sizeof(string) );
						trap->Cvar_VariableStringBuffer( string, string2, sizeof(string2) );
						Menu_SetItemBackground( menu, item->window.name, string2 );

						// Re-enable this button
						Menu_ItemDisable( menu, (char *)item->window.name, qfalse );
					}

					// Set the new item to the given background
					item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, itemArg );
					if ( item ) {	// store item name
						trap->Cvar_Set( "currentObjMapIconItem", item->window.name );
						if ( String_Parse( args, &cvarNormalArg ) ) {	// Store normal background
							trap->Cvar_Set( "currentObjMapIconBackground", cvarNormalArg );
							// Get higlight background
							if ( String_Parse( args, &cvarLitArg ) ) {	// set hightlight background
								trap->Cvar_VariableStringBuffer( cvarLitArg, string, sizeof(string) );
								Menu_SetItemBackground( menu, item->window.name, string );
								// Disable button
								Menu_ItemDisable( menu, (char *)item->window.name, qtrue );
							}
						}
					}
				}
			}
		}
		else if ( Q_stricmp( name, "updatesiegeclasscnt" ) == 0 ) {
			const char *teamArg;

			if ( String_Parse( args, &teamArg ) ) {
				UI_SiegeClassCnt( atoi( teamArg ) );
			}
		}
		else if ( Q_stricmp( name, "updatesiegecvars" ) == 0 ) {
			int team, baseClass;

			team = (int)trap->Cvar_VariableValue( "ui_holdteam" );
			baseClass = (int)trap->Cvar_VariableValue( "ui_siege_class" );

			UI_UpdateCvarsForClass( team, baseClass, 0 );
		}
		// Save current team and class
		else if ( Q_stricmp( name, "setteamclassicons" ) == 0 ) {
			int team = (int)trap->Cvar_VariableValue( "ui_holdteam" );
			char	classString[512];

			trap->Cvar_VariableStringBuffer( "ui_mySiegeClass", classString, sizeof(classString) );

			trap->Cvar_Set( "ui_startsiegeteam", va( "%d", team ) );
			trap->Cvar_Set( "ui_startsiegeclass", classString );

			// If player is already on a team, set up icons to show it.
			UI_FindCurrentSiegeTeamClass();
		}
		else if ( Q_stricmp( name, "updatesiegeweapondesc" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "base_class_weapons_feed" );
				if ( item ) {
					char	info[MAX_INFO_VALUE];
					trap->Cvar_VariableStringBuffer( va( "ui_class_weapondesc%i", item->cursorPos ), info, sizeof(info) );
					trap->Cvar_Set( "ui_itemforceinvdesc", info );
				}
			}
		}
		else if ( Q_stricmp( name, "updatesiegeinventorydesc" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "base_class_inventory_feed" );
				if ( item ) {
					char info[MAX_INFO_VALUE];
					trap->Cvar_VariableStringBuffer( va( "ui_class_itemdesc%i", item->cursorPos ), info, sizeof(info) );
					trap->Cvar_Set( "ui_itemforceinvdesc", info );
				}
			}
		}
		else if ( Q_stricmp( name, "updatesiegeforcedesc" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "base_class_force_feed" );
				if ( item ) {
					int i;
					char info[MAX_STRING_CHARS];

					trap->Cvar_VariableStringBuffer( va( "ui_class_power%i", item->cursorPos ), info, sizeof(info) );

					//count them up
					for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
						if ( !strcmp( HolocronIcons[i], info ) ) {
							trap->Cvar_Set( "ui_itemforceinvdesc", forcepowerDesc[i] );
						}
					}
				}
			}
		}
		else if ( Q_stricmp( name, "resetitemdescription" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "itemdescription" );
				if ( item ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if ( listPtr ) {
						listPtr->startPos = 0;
						listPtr->cursorPos = 0;
					}
					item->cursorPos = 0;
				}
			}
		}
		else if ( Q_stricmp( name, "resetsiegelistboxes" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "description" );
				if ( item ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if ( listPtr ) {
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}
			}

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "base_class_weapons_feed" );
				if ( item ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if ( listPtr ) {
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}

				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "base_class_inventory_feed" );
				if ( item ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if ( listPtr ) {
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}

				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "base_class_force_feed" );
				if ( item ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if ( listPtr ) {
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}
			}
		}
		else if ( Q_stricmp( name, "updatesiegestatusicons" ) == 0 ) {
			UI_UpdateSiegeStatusIcons();
		}
		else if ( Q_stricmp( name, "setcurrentNetMap" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "maplist" );
				if ( item ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if ( listPtr ) {
						trap->Cvar_Set( "ui_currentNetMap", va( "%d", listPtr->cursorPos ) );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "resetmaplist" ) == 0 ) {
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if ( menu ) {
				item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "maplist" );
				if ( item ) {
					uiInfo.uiDC.feederSelection( item->special.i, item->cursorPos, item );
				}
			}
		}
		else if ( Q_stricmp( name, "getmousepitch" ) == 0 ) {
			trap->Cvar_Set( "ui_mousePitch", (trap->Cvar_VariableValue( "m_pitch" ) >= 0) ? "0" : "1" );
		}
		else if ( Q_stricmp( name, "clampmaxplayers" ) == 0 ) {
			UI_ClampMaxPlayers();
		}
		else if ( !Q_stricmp( name, "sendModSayMsg" ) ) {
			switch ( ui_modSay.integer ) {
				case 0: {
					// clansay
					trap->Cmd_ExecuteText( EXEC_APPEND,
						va( "clansay %s\n", ui_modSayText.string )
					);
				} break;
				case 1: {
					// tell
					trap->Cmd_ExecuteText( EXEC_APPEND,
						va( "tell %i %s\n", uiInfo.playerIndex, ui_modSayText.string )
					);
				} break;
				case 2: {
					// amsay
					trap->Cmd_ExecuteText( EXEC_APPEND,
						va( "amsay %s\n", ui_modSayText.string )
					);
				} break;
				case 3: {
					// ampsay
					trap->Cmd_ExecuteText( EXEC_APPEND,
						va( "ampsay %i %s\n",
							ui_modSayAllPlayer.integer ? -1 : uiInfo.playerIndex,
							ui_modSayText.string
						)
					);
				} break;
			}
		}
		else {
			Com_Printf( "unknown UI script %s\n", name );
		}
	}
}

static void UI_GetTeamColor( vector4 *color ) {
}

void UI_SetSiegeTeams( void ) {
	char			info[MAX_INFO_VALUE];
	const char		*mapname = NULL;
	char			levelname[MAX_QPATH];
	char			btime[1024];
	char			teams[2048];
	char			teamInfo[MAX_SIEGE_INFO_SIZE];
	char			team1[1024];
	char			team2[1024];
	int				len = 0;
	int				gametype;
	fileHandle_t	f;

	//Get the map name from the server info
	if ( trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) ) ) {
		mapname = Info_ValueForKey( info, "mapname" );
	}

	if ( !mapname || !mapname[0] ) {
		return;
	}

	gametype = atoi( Info_ValueForKey( info, "g_gametype" ) );

	//If the server we are connected to is not siege we cannot choose a class anyway
	if ( gametype != GT_SIEGE ) {
		return;
	}

	Com_sprintf( levelname, sizeof(levelname), "maps/%s.siege", mapname );

	if ( !levelname[0] ) {
		return;
	}

	len = trap->FS_Open( levelname, &f, FS_READ );

	if ( !f || len >= MAX_SIEGE_INFO_SIZE ) {
		return;
	}

	trap->FS_Read( siege_info, len, f );
	siege_info[len] = 0;	//ensure null terminated

	trap->FS_Close( f );

	//Found the .siege file

	if ( BG_SiegeGetValueGroup( siege_info, "Teams", teams ) ) {
		char buf[1024];

		trap->Cvar_VariableStringBuffer( "cg_siegeTeam1", buf, 1024 );
		if ( buf[0] && Q_stricmp( buf, "none" ) ) {
			strcpy( team1, buf );
		}
		else {
			BG_SiegeGetPairedValue( teams, "team1", team1 );
		}

		trap->Cvar_VariableStringBuffer( "cg_siegeTeam2", buf, 1024 );
		if ( buf[0] && Q_stricmp( buf, "none" ) ) {
			strcpy( team2, buf );
		}
		else {
			BG_SiegeGetPairedValue( teams, "team2", team2 );
		}
	}
	else {
		return;
	}

	//Set the team themes so we know what classes to make available for selection
	if ( BG_SiegeGetValueGroup( siege_info, team1, teamInfo ) ) {
		if ( BG_SiegeGetPairedValue( teamInfo, "UseTeam", btime ) ) {
			BG_SiegeSetTeamTheme( SIEGETEAM_TEAM1, btime );
		}
	}
	if ( BG_SiegeGetValueGroup( siege_info, team2, teamInfo ) ) {
		if ( BG_SiegeGetPairedValue( teamInfo, "UseTeam", btime ) ) {
			BG_SiegeSetTeamTheme( SIEGETEAM_TEAM2, btime );
		}
	}

	siegeTeam1 = BG_SiegeFindThemeForTeam( SIEGETEAM_TEAM1 );
	siegeTeam2 = BG_SiegeFindThemeForTeam( SIEGETEAM_TEAM2 );

	//set the default description for the default selection
	if ( !siegeTeam1 || !siegeTeam1->classes[0] ) {
		Com_Error( ERR_DROP, "Error loading teams in UI" );
	}

	Menu_SetFeederSelection( NULL, FEEDER_SIEGE_TEAM1, 0, NULL );
	Menu_SetFeederSelection( NULL, FEEDER_SIEGE_TEAM2, -1, NULL );
}

static void UI_SiegeClassCnt( const int team ) {
	UI_SetSiegeTeams();

	trap->Cvar_Set( "ui_infantry_cnt", va( "%d", BG_SiegeCountBaseClass( team, 0 ) ) );
	trap->Cvar_Set( "ui_vanguard_cnt", va( "%d", BG_SiegeCountBaseClass( team, 1 ) ) );
	trap->Cvar_Set( "ui_support_cnt", va( "%d", BG_SiegeCountBaseClass( team, 2 ) ) );
	trap->Cvar_Set( "ui_jedi_cnt", va( "%d", BG_SiegeCountBaseClass( team, 3 ) ) );
	trap->Cvar_Set( "ui_demo_cnt", va( "%d", BG_SiegeCountBaseClass( team, 4 ) ) );
	trap->Cvar_Set( "ui_heavy_cnt", va( "%d", BG_SiegeCountBaseClass( team, 5 ) ) );
}

static int UI_MapCountByGameType( qboolean singlePlayer ) {
	int i, c = 0, game = singlePlayer ? ui_gameType.integer : ui_netGameType.integer;

	if ( game == GT_TEAM ) {
		game = GT_FFA;
	}

	// since GT_CTY uses the same entities as CTF, use the same map sets
	if ( game == GT_CTY ) {
		game = GT_CTF;
	}

	for ( i = 0; i < uiInfo.mapCount; i++ ) {
		uiInfo.mapList[i].active = qfalse;
		if ( uiInfo.mapList[i].typeBits & (1 << game) ) {
			c++;
			uiInfo.mapList[i].active = qtrue;
		}
	}
	return c;
}

qboolean UI_hasSkinForBase( const char *base, const char *team ) {
	char test[MAX_QPATH];
	fileHandle_t f;

	Com_sprintf( test, sizeof(test), "models/players/%s/%s/lower_default.skin", base, team );
	trap->FS_Open( test, &f, FS_READ );
	if ( f != 0 ) {
		trap->FS_Close( f );
		return qtrue;
	}
	Com_sprintf( test, sizeof(test), "models/players/characters/%s/%s/lower_default.skin", base, team );
	trap->FS_Open( test, &f, FS_READ );
	if ( f != 0 ) {
		trap->FS_Close( f );
		return qtrue;
	}
	return qfalse;
}

static int UI_HeadCountByColor( void ) {
	int i, c;
	const char *teamname;
	char saved[MAX_QPATH];

	c = 0;

	switch ( uiSkinColor ) {
	case TEAM_BLUE:
		//	teamname = "/blue";
		teamname = "eulb/";
		break;
	case TEAM_RED:
		//	teamname = "/red";
		teamname = "der/";
		break;
	default:
		//	teamname = "/default";
		teamname = "tluafed/";
	}

	// Count each head with this color
	for ( i = 0; i < uiInfo.q3HeadCount; i++ ) {
		Q_strncpyz( saved, uiInfo.q3HeadNames[i], sizeof(saved) );
		Q_strrev( uiInfo.q3HeadNames[i] );
		if ( uiInfo.q3HeadNames[i] && (!Q_stricmpn( uiInfo.q3HeadNames[i], teamname, strlen( teamname ) ) || (uiSkinColor == TEAM_FREE && Q_stricmpn( uiInfo.q3HeadNames[i], "der", 3 ) && Q_stricmpn( uiInfo.q3HeadNames[i], "eulb", 4 ))) ) {
			Q_strrev( uiInfo.q3HeadNames[i] );
			c++;
			continue;
		}
		Q_strrev( uiInfo.q3HeadNames[i] );
	}
	return c;
}

static void UI_InsertServerIntoDisplayList( int num, int position ) {
	int i;

	if ( position < 0 || position > uiInfo.serverStatus.numDisplayServers ) {
		return;
	}
	//
	uiInfo.serverStatus.numDisplayServers++;
	for ( i = uiInfo.serverStatus.numDisplayServers; i > position; i-- ) {
		uiInfo.serverStatus.displayServers[i] = uiInfo.serverStatus.displayServers[i - 1];
	}
	uiInfo.serverStatus.displayServers[position] = num;
}

static void UI_RemoveServerFromDisplayList( int num ) {
	int i, j;

	for ( i = 0; i < uiInfo.serverStatus.numDisplayServers; i++ ) {
		if ( uiInfo.serverStatus.displayServers[i] == num ) {
			uiInfo.serverStatus.numDisplayServers--;
			for ( j = i; j < uiInfo.serverStatus.numDisplayServers; j++ ) {
				uiInfo.serverStatus.displayServers[j] = uiInfo.serverStatus.displayServers[j + 1];
			}
			return;
		}
	}
}

static void UI_BinaryServerInsertion( int num ) {
	int mid, offset, res, len;

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;
	while ( mid > 0 ) {
		mid = len >> 1;
		//
		res = trap->LAN_CompareServers( UI_SourceForLAN(), uiInfo.serverStatus.sortKey,
			uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[offset + mid] );
		// if equal
		if ( res == 0 ) {
			UI_InsertServerIntoDisplayList( num, offset + mid );
			return;
		}
		// if larger
		else if ( res == 1 ) {
			offset += mid;
			len -= mid;
		}
		// if smaller
		else {
			len -= mid;
		}
	}
	if ( res == 1 ) {
		offset++;
	}
	UI_InsertServerIntoDisplayList( num, offset );
}

static void UI_BuildServerDisplayList( int force ) {
	int i, count, clients, maxClients, ping, game, len, passw/*, visible*/;
	char info[MAX_STRING_CHARS];
	//	qboolean startRefresh = qtrue; TTimo: unused
	static int numinvisible;
	int	lanSource;

	if ( !(force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh) ) {
		return;
	}
	// if we shouldn't reset
	if ( force == 2 ) {
		force = 0;
	}

	// do motd updates here too
	trap->Cvar_VariableStringBuffer( "cl_motdString", uiInfo.serverStatus.motd, sizeof(uiInfo.serverStatus.motd) );
	len = strlen( uiInfo.serverStatus.motd );
	if ( len == 0 ) {
		strcpy( uiInfo.serverStatus.motd, "Welcome to Jedi Academy MP!" );
		len = strlen( uiInfo.serverStatus.motd );
	}
	if ( len != uiInfo.serverStatus.motdLen ) {
		uiInfo.serverStatus.motdLen = len;
		uiInfo.serverStatus.motdWidth = -1;
	}

	lanSource = UI_SourceForLAN();

	if ( force ) {
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		Menu_SetFeederSelection( NULL, FEEDER_SERVERS, 0, NULL );
		// mark all servers as visible so we store ping updates for them
		trap->LAN_MarkServerVisible( lanSource, -1, qtrue );
	}

	// get the server count (comes from the master)
	count = trap->LAN_GetServerCount( lanSource );
	if ( count == -1 || (ui_netSource.integer == UIAS_LOCAL && count == 0) ) {
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
		return;
	}

	trap->Cvar_Update( &ui_browserShowEmpty );
	trap->Cvar_Update( &ui_browserShowFull );
	trap->Cvar_Update( &ui_browserShowPasswordProtected );
	trap->Cvar_Update( &ui_serverFilterType );
	trap->Cvar_Update( &ui_joinGameType );

	//	visible = qfalse;
	for ( i = 0; i < count; i++ ) {
		// if we already got info for this server
		if ( !trap->LAN_ServerIsVisible( lanSource, i ) ) {
			continue;
		}
		//		visible = qtrue;
		// get the ping for this server
		ping = trap->LAN_GetServerPing( lanSource, i );
		if ( ping > 0 || ui_netSource.integer == UIAS_FAVORITES || ui_netSource.integer == UIAS_HISTORY ) {

			trap->LAN_GetServerInfo( lanSource, i, info, MAX_STRING_CHARS );

			clients = atoi( Info_ValueForKey( info, "clients" ) );
			uiInfo.serverStatus.numPlayersOnServers += clients;

			if ( ui_browserShowEmpty.integer == 0 ) {
				if ( clients == 0 ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}

			if ( ui_browserShowFull.integer == 0 ) {
				maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
				if ( clients == maxClients ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}

			if ( ui_browserShowPasswordProtected.integer == 0 ) {
				passw = atoi( Info_ValueForKey( info, "needpass" ) );
				if ( passw && !ui_browserShowPasswordProtected.integer ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}

			if ( ui_joinGameType.integer >= 0 && ui_joinGameType.integer < GT_MAX_GAME_TYPE ) {
				game = atoi( Info_ValueForKey( info, "gametype" ) );
				if ( game != ui_joinGameType.integer ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}

			if ( ui_serverFilterType.integer > 0 && ui_serverFilterType.integer <= uiInfo.modCount ) {
				if ( Q_stricmp( Info_ValueForKey( info, "game" ), UI_FilterDir( ui_serverFilterType.integer ) ) != 0 ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}
			// make sure we never add a favorite server twice
			if ( ui_netSource.integer == UIAS_FAVORITES ) {
				UI_RemoveServerFromDisplayList( i );
			}
			// insert the server into the list
			UI_BinaryServerInsertion( i );
			// done with this server
			if ( ping > 0 ) {
				trap->LAN_MarkServerVisible( lanSource, i, qfalse );
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;

	// if there were no servers visible for ping updates
	//	if (!visible) {
	//		UI_StopServerRefresh();
	//		uiInfo.serverStatus.nextDisplayRefresh = 0;
	//	}
}

struct serverStatusCvar_s { const char *name, *altName; } serverStatusCvars[] = {
	{ "sv_hostname", "Name" },
	{ "Address", "" },
	{ "gamename", "Game name" },
	{ "g_gametype", "Game type" },
	{ "mapname", "Map" },
	{ "version", "" },
	{ "protocol", "" },
	{ "timelimit", "" },
	{ "fraglimit", "" },
	{ NULL, NULL }
};

static void UI_SortServerStatusInfo( serverStatusInfo_t *info ) {
	int i, j, index, numLines;
	const char *tmp1, *tmp2;

	// FIXME: if "gamename" == "base" or "missionpack" then
	// replace the gametype number by FFA, CTF etc.
	//
	index = 0;
	numLines = Q_clampi( 0, info->numLines, MAX_SERVERSTATUS_LINES );
	for ( i = 0; serverStatusCvars[i].name; i++ ) {
		for ( j = 0; j < numLines; j++ ) {
			if ( !info->lines[j][1] || info->lines[j][1][0] ) {
				continue;
			}
			if ( !Q_stricmp( serverStatusCvars[i].name, info->lines[j][0] ) ) {
				// swap lines
				tmp1 = info->lines[index][0];
				tmp2 = info->lines[index][3];
				info->lines[index][0] = info->lines[j][0];
				info->lines[index][3] = info->lines[j][3];
				info->lines[j][0] = tmp1;
				info->lines[j][3] = tmp2;
				//
				if ( strlen( serverStatusCvars[i].altName ) ) {
					info->lines[index][0] = serverStatusCvars[i].altName;
				}
				index++;
			}
		}
	}
}

static int UI_GetServerStatusInfo( const char *serverAddress, serverStatusInfo_t *info ) {
	char *p, *score, *ping, *name;
	int i, len;

	if ( !info ) {
		trap->LAN_ServerStatus( serverAddress, NULL, 0 );
		return qfalse;
	}
	memset( info, 0, sizeof(*info) );
	if ( trap->LAN_ServerStatus( serverAddress, info->text, sizeof(info->text) ) ) {
		Q_strncpyz( info->address, serverAddress, sizeof(info->address) );
		p = info->text;
		info->numLines = 0;
		info->lines[info->numLines][0] = "Address";
		info->lines[info->numLines][1] = "";
		info->lines[info->numLines][2] = "";
		info->lines[info->numLines][3] = info->address;
		info->numLines++;
		// get the cvars
		while ( p && *p ) {
			p = strchr( p, '\\' );
			if ( !p ) break;
			*p++ = '\0';
			if ( *p == '\\' )
				break;
			info->lines[info->numLines][0] = p;
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			p = strchr( p, '\\' );
			if ( !p ) break;
			*p++ = '\0';
			info->lines[info->numLines][3] = p;

			info->numLines++;
			if ( info->numLines >= MAX_SERVERSTATUS_LINES )
				break;
		}
		// get the player list
		if ( info->numLines < MAX_SERVERSTATUS_LINES - 3 ) {
			// empty line
			info->lines[info->numLines][0] = "";
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";
			info->numLines++;
			// header
			info->lines[info->numLines][0] = "num";
			info->lines[info->numLines][1] = "score";
			info->lines[info->numLines][2] = "ping";
			info->lines[info->numLines][3] = "name";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;
			while ( p && *p ) {
				if ( *p == '\\' )
					*p++ = '\0';
				if ( !p )
					break;
				score = p;
				p = strchr( p, ' ' );
				if ( !p )
					break;
				*p++ = '\0';
				ping = p;
				p = strchr( p, ' ' );
				if ( !p )
					break;
				*p++ = '\0';
				name = p;
				Com_sprintf( &info->pings[len], sizeof(info->pings) - len, "%d", i );
				info->lines[info->numLines][0] = &info->pings[len];
				len += strlen( &info->pings[len] ) + 1;
				info->lines[info->numLines][1] = score;
				info->lines[info->numLines][2] = ping;
				info->lines[info->numLines][3] = name;
				info->numLines++;
				if ( info->numLines >= MAX_SERVERSTATUS_LINES )
					break;
				p = strchr( p, '\\' );
				if ( !p )
					break;
				*p++ = '\0';
				//
				i++;
			}
		}
		UI_SortServerStatusInfo( info );
		return qtrue;
	}
	return qfalse;
}

static void UI_BuildFindPlayerList( qboolean force ) {
	static int numFound, numTimeOuts;
	int i, j, resend;
	serverStatusInfo_t info;
	char name[MAX_NAME_LENGTH + 2];
	char infoString[MAX_STRING_CHARS];
	int lanSource;

	if ( !force ) {
		if ( !uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime ) {
			return;
		}
	}
	else {
		memset( &uiInfo.pendingServerStatus, 0, sizeof(uiInfo.pendingServerStatus) );
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap->Cvar_VariableStringBuffer( "ui_findPlayer", uiInfo.findPlayerName, sizeof(uiInfo.findPlayerName) );
		Q_CleanString( uiInfo.findPlayerName, STRIP_COLOUR );
		// should have a string of some length
		if ( !strlen( uiInfo.findPlayerName ) ) {
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}
		// set resend time
		resend = ui_serverStatusTimeOut.integer / 2 - 10;
		if ( resend < 50 ) {
			resend = 50;
		}
		trap->Cvar_Set( "cl_serverStatusResendTime", va( "%d", resend ) );
		// reset all server status requests
		trap->LAN_ServerStatus( NULL, NULL, 0 );
		//
		uiInfo.numFoundPlayerServers = 1;

		trap->SE_GetStringTextString( "MENUS_SEARCHING", holdSPString, sizeof(holdSPString) );
		trap->Cvar_Set( "ui_playerServersFound", va( holdSPString, uiInfo.pendingServerStatus.num, numFound ) );
		//	Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
		//					sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
		//						"searching %d...", uiInfo.pendingServerStatus.num);
		numFound = 0;
		numTimeOuts++;
	}
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		// if this pending server is valid
		if ( uiInfo.pendingServerStatus.server[i].valid ) {
			// try to get the server status for this server
			if ( UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, &info ) ) {
				//
				numFound++;
				// parse through the server status lines
				for ( j = 0; j < info.numLines; j++ ) {
					// should have ping info
					if ( !info.lines[j][2] || !info.lines[j][2][0] ) {
						continue;
					}
					// clean string first
					Q_strncpyz( name, info.lines[j][3], sizeof(name) );
					Q_CleanString( name, STRIP_COLOUR );
					// if the player name is a substring
					if ( Q_stristr( name, uiInfo.findPlayerName ) ) {
						// add to found server list if we have space (always leave space for a line with the number found)
						if ( uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS - 1 ) {
							//
							Q_strncpyz( uiInfo.foundPlayerServerAddresses[uiInfo.numFoundPlayerServers - 1],
								uiInfo.pendingServerStatus.server[i].adrstr,
								sizeof(uiInfo.foundPlayerServerAddresses[0]) );
							Q_strncpyz( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
								uiInfo.pendingServerStatus.server[i].name,
								sizeof(uiInfo.foundPlayerServerNames[0]) );
							uiInfo.numFoundPlayerServers++;
						}
						else {
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}

				trap->SE_GetStringTextString( "MENUS_SEARCHING", holdSPString, sizeof(holdSPString) );
				trap->Cvar_Set( "ui_playerServersFound", va( holdSPString, uiInfo.pendingServerStatus.num, numFound ) );
				//	Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
				//					sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
				//						"searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
				// retrieved the server status so reuse this spot
				uiInfo.pendingServerStatus.server[i].valid = qfalse;
			}
		}
		// if empty pending slot or timed out
		if ( !uiInfo.pendingServerStatus.server[i].valid ||
			uiInfo.pendingServerStatus.server[i].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut.integer ) {
			if ( uiInfo.pendingServerStatus.server[i].valid ) {
				numTimeOuts++;
			}
			// reset server status request for this address
			UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, NULL );
			// reuse pending slot
			uiInfo.pendingServerStatus.server[i].valid = qfalse;
			// if we didn't try to get the status of all servers in the main browser yet
			if ( uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers ) {
				uiInfo.pendingServerStatus.server[i].startTime = uiInfo.uiDC.realTime;
				lanSource = UI_SourceForLAN();
				trap->LAN_GetServerAddressString( lanSource, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num],
					uiInfo.pendingServerStatus.server[i].adrstr, sizeof(uiInfo.pendingServerStatus.server[i].adrstr) );
				trap->LAN_GetServerInfo( lanSource, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num], infoString, sizeof(infoString) );
				Q_strncpyz( uiInfo.pendingServerStatus.server[i].name, Info_ValueForKey( infoString, "hostname" ), sizeof(uiInfo.pendingServerStatus.server[0].name) );
				uiInfo.pendingServerStatus.server[i].valid = qtrue;
				uiInfo.pendingServerStatus.num++;

				trap->SE_GetStringTextString( "MENUS_SEARCHING", holdSPString, sizeof(holdSPString) );
				trap->Cvar_Set( "ui_playerServersFound", va( holdSPString, uiInfo.pendingServerStatus.num, numFound ) );

				//	Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
				//					sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
				//						"searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
			}
		}
	}
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( uiInfo.pendingServerStatus.server[i].valid ) {
			break;
		}
	}
	// if still trying to retrieve server status info
	if ( i < MAX_SERVERSTATUSREQUESTS ) {
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	}
	else {
		// add a line that shows the number of servers found
		if ( !uiInfo.numFoundPlayerServers ) {
			Com_sprintf( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1], sizeof(uiInfo.foundPlayerServerAddresses[0]), "no servers found" );
		}
		else {
			trap->SE_GetStringTextString( "MENUS_SERVERS_FOUNDWITH", holdSPString, sizeof(holdSPString) );
			trap->Cvar_Set( "ui_playerServersFound", va( holdSPString,
				uiInfo.numFoundPlayerServers - 1,
				uiInfo.numFoundPlayerServers == 2 ? "" : "s",
				uiInfo.findPlayerName ) );
		}
		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection( FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer, NULL );
	}
}

static void UI_BuildServerStatus( qboolean force ) {

	if ( uiInfo.nextFindPlayerRefresh ) {
		return;
	}
	if ( !force ) {
		if ( !uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime ) {
			return;
		}
	}
	else {
		Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
		uiInfo.serverStatusInfo.numLines = 0;
		// reset all server status requests
		trap->LAN_ServerStatus( NULL, NULL, 0 );
	}
	if ( uiInfo.serverStatus.currentServer < 0 || uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers || uiInfo.serverStatus.numDisplayServers == 0 ) {
		return;
	}
	if ( UI_GetServerStatusInfo( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) ) {
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo( uiInfo.serverStatusAddress, NULL );
	}
	else {
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
	}
}

static int UI_FeederCount( int feederID ) {
	int team, baseClass, count = 0, i;
	static char info[MAX_STRING_CHARS];

	switch ( feederID ) {
	case FEEDER_SABER_SINGLE_INFO:
		for ( i = 0; i < MAX_SABER_HILTS; i++ ) {
			if ( saberSingleHiltInfo[i] )
				count++;
			else
				break;
		}
		return count;

	case FEEDER_SABER_STAFF_INFO:
		for ( i = 0; i < MAX_SABER_HILTS; i++ ) {
			if ( saberStaffHiltInfo[i] )
				count++;
			else
				break;
		}
		return count;

	case FEEDER_Q3HEADS:
		return UI_HeadCountByColor();

	case FEEDER_SIEGE_TEAM1:
		if ( !siegeTeam1 ) {
			UI_SetSiegeTeams();
			if ( !siegeTeam1 )
				return 0;
		}
		return siegeTeam1->numClasses;

	case FEEDER_SIEGE_TEAM2:
		if ( !siegeTeam2 ) {
			UI_SetSiegeTeams();
			if ( !siegeTeam2 )
				return 0;
		}
		return siegeTeam2->numClasses;

	case FEEDER_FORCECFG:
		if ( uiForceSide == FORCESIDE_LIGHT )
			return uiInfo.forceConfigCount - uiInfo.forceConfigLightIndexBegin;
		else
			return uiInfo.forceConfigLightIndexBegin + 1;

	case FEEDER_CINEMATICS:
		return 0;

	case FEEDER_MAPS:
	case FEEDER_ALLMAPS:
		return UI_MapCountByGameType( feederID == FEEDER_MAPS ? qtrue : qfalse );

	case FEEDER_SERVERS:
		return uiInfo.serverStatus.numDisplayServers;

	case FEEDER_SERVERSTATUS:
		return Q_clampi( 0, uiInfo.serverStatusInfo.numLines, MAX_SERVERSTATUS_LINES );

	case FEEDER_FINDPLAYER:
		return uiInfo.numFoundPlayerServers;

	case FEEDER_PLAYER_LIST:
		if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh ) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.playerCount;

	case FEEDER_MODS:
		return uiInfo.modCount;

	case FEEDER_DEMOS:
		return uiInfo.demoCount;

	case FEEDER_MOVES:
		for ( i = 0; i < MAX_MOVES; i++ ) {
			if ( datapadMoveData[uiInfo.movesTitleIndex][i].title )
				count++;
		}
		return count;

	case FEEDER_MOVES_TITLES:
		return MD_MOVE_TITLE_MAX;

	case FEEDER_PLAYER_SPECIES:
		return uiInfo.playerSpeciesCount;

	case FEEDER_PLAYER_SKIN_HEAD:
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount;

	case FEEDER_PLAYER_SKIN_TORSO:
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount;

	case FEEDER_PLAYER_SKIN_LEGS:
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount;

	case FEEDER_COLORCHOICES:
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount;

	case FEEDER_SIEGE_BASE_CLASS:
		team = (int)trap->Cvar_VariableValue( "ui_team" );
		baseClass = (int)trap->Cvar_VariableValue( "ui_siege_class" );

		if ( team == SIEGETEAM_TEAM1 || team == SIEGETEAM_TEAM2 ) {
			// Is it a valid base class?
			if ( baseClass >= SPC_INFANTRY && baseClass < SPC_MAX )
				return BG_SiegeCountBaseClass( team, baseClass );
		}
		return 0;

		// Get the count of weapons
	case FEEDER_SIEGE_CLASS_WEAPONS:
		//count them up
		for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
			trap->Cvar_VariableStringBuffer( va( "ui_class_weapon%i", i ), info, sizeof(info) );
			if ( Q_stricmp( info, "gfx/2d/select" ) )
				count++;
		}
		return count;

		// Get the count of inventory
	case FEEDER_SIEGE_CLASS_INVENTORY:
		//count them up
		for ( i = 0; i < HI_NUM_HOLDABLE; i++ ) {
			trap->Cvar_VariableStringBuffer( va( "ui_class_item%i", i ), info, sizeof(info) );
			// A hack so health and ammo dispenser icons don't show up.
			if ( Q_stricmp( info, "gfx/2d/select" ) && Q_stricmp( info, "gfx/hud/i_icon_healthdisp" )
				&& Q_stricmp( info, "gfx/hud/i_icon_ammodisp" ) ) {
				count++;
			}
		}
		return count;

		// Get the count of force powers
	case FEEDER_SIEGE_CLASS_FORCE:
		//count them up
		for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
			trap->Cvar_VariableStringBuffer( va( "ui_class_power%i", i ), info, sizeof(info) );
			if ( Q_stricmp( info, "gfx/2d/select" ) )
				count++;
		}
		return count;

	default:
		break;
	}

	return 0;
}

static const char *UI_SelectedMap( int index, int *actual ) {
	int i, c;
	c = 0;
	*actual = 0;

	for ( i = 0; i < uiInfo.mapCount; i++ ) {
		if ( uiInfo.mapList[i].active ) {
			if ( c == index ) {
				*actual = i;
				return uiInfo.mapList[i].mapName;
			}
			else {
				c++;
			}
		}
	}
	return "";
}

static const char *UI_SelectedTeamHead( int index, int *actual ) {
	const char *teamname;
	int i, c = 0;
	char saved[MAX_QPATH] = { 0 };

	switch ( uiSkinColor ) {
	case TEAM_BLUE:
		//	teamname = "/blue";
		teamname = "eulb/";
		break;
	case TEAM_RED:
		//	teamname = "/red";
		teamname = "der/";
		break;
	default:
		//	teamname = "/default";
		teamname = "tluafed/";
	}

	// Count each head with this color

	for ( i = 0; i < uiInfo.q3HeadCount; i++ ) {
		Q_strncpyz( saved, uiInfo.q3HeadNames[i], sizeof(saved) );
		Q_strrev( uiInfo.q3HeadNames[i] );
		if ( uiInfo.q3HeadNames[i] && (!Q_stricmpn( uiInfo.q3HeadNames[i], teamname, strlen( teamname ) ) || (uiSkinColor == TEAM_FREE && Q_stricmpn( uiInfo.q3HeadNames[i], "der", 3 ) && Q_stricmpn( uiInfo.q3HeadNames[i], "eulb", 4 ))) ) {
			Q_strrev( uiInfo.q3HeadNames[i] );
			if ( c == index ) {
				*actual = i;
				return uiInfo.q3HeadNames[i];
			}
			else {
				c++;
			}
			continue;
		}
		Q_strrev( uiInfo.q3HeadNames[i] );
	}
	return "";
}


static int UI_GetIndexFromSelection( int actual ) {
	int i, c;
	c = 0;
	for ( i = 0; i < uiInfo.mapCount; i++ ) {
		if ( uiInfo.mapList[i].active ) {
			if ( i == actual ) {
				return c;
			}
			c++;
		}
	}
	return 0;
}

static void UI_UpdatePendingPings( void ) {
	trap->LAN_ResetPings( UI_SourceForLAN() );
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
}

static const char *UI_FeederItemText( int feederID, int index, int column, qhandle_t *handle1, qhandle_t *handle2,
	qhandle_t *handle3 ) {
	static char info[MAX_STRING_CHARS], hostname[MAX_HOSTNAMELENGTH], clientBuff[32];
	static int lastColumn = -1;
	static int lastTime = 0;

	*handle1 = *handle2 = *handle3 = -1;

	switch ( feederID ) {
	case FEEDER_SABER_SINGLE_INFO:
		UI_SaberProperNameForSaber( saberSingleHiltInfo[index], info, sizeof(info) );
		return info;

	case FEEDER_SABER_STAFF_INFO:
		UI_SaberProperNameForSaber( saberStaffHiltInfo[index], info, sizeof(info) );
		return info;

	case FEEDER_Q3HEADS:
	{
		int actual;
		return UI_SelectedTeamHead( index, &actual );
	}

	case FEEDER_SIEGE_TEAM1:
	case FEEDER_SIEGE_TEAM2:
		return "";

	case FEEDER_FORCECFG:
		if ( index >= 0 && index < uiInfo.forceConfigCount ) {
			if ( index == 0 )
				return uiInfo.forceConfigNames[index];
			else {
				if ( uiForceSide == FORCESIDE_LIGHT ) {
					index += uiInfo.forceConfigLightIndexBegin;
					if ( index < 0 )
						return NULL;
					if ( index >= uiInfo.forceConfigCount )
						return NULL;
					return uiInfo.forceConfigNames[index];
				}
				else if ( uiForceSide == FORCESIDE_DARK ) {
					index += uiInfo.forceConfigDarkIndexBegin;
					if ( index < 0 )
						return NULL;
					if ( index > uiInfo.forceConfigLightIndexBegin )
						return NULL;
					if ( index >= uiInfo.forceConfigCount )
						return NULL;
					return uiInfo.forceConfigNames[index];
				}
				else
					return NULL;
			}
		}
		break;

	case FEEDER_MAPS:
	case FEEDER_ALLMAPS:
	{
		int actual;
		return UI_SelectedMap( index, &actual );
	}

	case FEEDER_SERVERS:
		if ( index >= 0 && index < uiInfo.serverStatus.numDisplayServers ) {
			int ping, game;
			if ( lastColumn != column || lastTime > uiInfo.uiDC.realTime + 5000 ) {
				trap->LAN_GetServerInfo( UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, sizeof(info) );
				lastColumn = column;
				lastTime = uiInfo.uiDC.realTime;
			}
			ping = atoi( Info_ValueForKey( info, "ping" ) );
			// if we ever see a ping that is out of date, do a server refresh
			//	if ( ping == -1 )
			//		UI_UpdatePendingPings();

			switch ( column ) {
			case SORT_HOST:
				if ( ping <= 0 )
					return Info_ValueForKey( info, "addr" );

				else {
					int gametype = atoi( Info_ValueForKey( info, "gametype" ) );
					//check for password
					if ( atoi( Info_ValueForKey( info, "needpass" ) ) )
						*handle3 = uiInfo.uiDC.Assets.needPass;
					//check for saberonly and restricted force powers
					if ( gametype != GT_JEDIMASTER ) {
						qboolean saberOnly = qtrue, restrictedForce = qfalse, allForceDisabled = qfalse;
						int wDisable, i;

						//check force
						restrictedForce = atoi( Info_ValueForKey( info, "fdisable" ) );
						if ( UI_AllForceDisabled( restrictedForce ) ) {
							// all force powers are disabled
							allForceDisabled = qtrue;
							*handle2 = uiInfo.uiDC.Assets.noForce;
						}
						else if ( restrictedForce )
							*handle2 = uiInfo.uiDC.Assets.forceRestrict;

						//check weaps
						wDisable = atoi( Info_ValueForKey( info, "wdisable" ) );

						for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
							if ( !(wDisable & (1 << i)) && i != WP_SABER && i != WP_NONE )
								saberOnly = qfalse;
						}
						if ( saberOnly )
							*handle1 = uiInfo.uiDC.Assets.saberOnly;
						else if ( atoi( Info_ValueForKey( info, "truejedi" ) ) ) {
							// truejedi is on and allowed in this mode
							if ( gametype != GT_HOLOCRON && gametype != GT_JEDIMASTER && !saberOnly && !allForceDisabled )
								*handle1 = uiInfo.uiDC.Assets.trueJedi;
						}
					}
					if ( ui_netSource.integer == UIAS_LOCAL ) {
						int nettype = atoi( Info_ValueForKey( info, "nettype" ) );

						if ( nettype < 0 || nettype >= numNetNames )
							nettype = 0;

						Com_sprintf( hostname, sizeof(hostname), "%s [%s]", Info_ValueForKey( info, "hostname" ),
							netNames[nettype] );
						return hostname;
					}
					else {
						if ( atoi( Info_ValueForKey( info, "sv_allowAnonymous" ) ) )
							Com_sprintf( hostname, sizeof(hostname), "(A) %s", Info_ValueForKey( info, "hostname" ) );
						else
							Q_strncpyz( hostname, Info_ValueForKey( info, "hostname" ), sizeof(hostname) );
						return hostname;
					}
				}
			case SORT_MAP:
				return Info_ValueForKey( info, "mapname" );

			case SORT_CLIENTS:
				Com_sprintf( clientBuff, sizeof(clientBuff), "%s (%s)", Info_ValueForKey( info, "clients" ),
					Info_ValueForKey( info, "sv_maxclients" ) );
				return clientBuff;

			case SORT_GAME:
				game = atoi( Info_ValueForKey( info, "gametype" ) );
				if ( game >= 0 && game < GT_MAX_GAME_TYPE )
					return gametypeStringShort[game];
				else {
					if ( ping <= 0 )
						return "Inactive";
					return "Unknown";
				}

			case SORT_PING:
				if ( ping <= 0 )
					return "...";
				else
					return Info_ValueForKey( info, "ping" );

			default:
				break;
			}
		}
		break;

	case FEEDER_SERVERSTATUS:
		if ( index >= 0 && index < uiInfo.serverStatusInfo.numLines ) {
			if ( column >= 0 && column < 4 )
				return uiInfo.serverStatusInfo.lines[index][column];
		}
		break;

	case FEEDER_FINDPLAYER:
		if ( index >= 0 && index < uiInfo.numFoundPlayerServers )
			return uiInfo.foundPlayerServerNames[index];

	case FEEDER_PLAYER_LIST:
		if ( index >= 0 && index < uiInfo.playerCount )
			return uiInfo.playerNames[index];

	case FEEDER_TEAM_LIST:
		if ( index >= 0 && index < uiInfo.myTeamCount )
			return uiInfo.teamNames[index];

	case FEEDER_MODS:
		if ( index >= 0 && index < uiInfo.modCount ) {
			if ( uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr )
				return uiInfo.modList[index].modDescr;
			else
				return uiInfo.modList[index].modName;
		}
		break;

	case FEEDER_CINEMATICS:
		return 0;

	case FEEDER_DEMOS:
		if ( index >= 0 && index < uiInfo.demoCount )
			return uiInfo.demoList[index];

	case FEEDER_MOVES:
		return datapadMoveData[uiInfo.movesTitleIndex][index].title;

	case FEEDER_MOVES_TITLES:
		return datapadMoveTitleData[index];

	case FEEDER_PLAYER_SPECIES:
		if ( index >= 0 && index < uiInfo.playerSpeciesCount ) {
			return uiInfo.playerSpecies[index].Name;
		}
		break;

	case FEEDER_LANGUAGES:
		return 0;

	case FEEDER_COLORCHOICES:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount ) {
			*handle1 = trap->R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader );
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader;
		}
		break;

	case FEEDER_PLAYER_SKIN_HEAD:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount ) {
			*handle1 = trap->R_RegisterShaderNoMip( va( "models/players/%s/icon_%s",
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name,
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name ) );
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name;
		}
		break;

	case FEEDER_PLAYER_SKIN_TORSO:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount ) {
			*handle1 = trap->R_RegisterShaderNoMip( va( "models/players/%s/icon_%s",
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name,
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name ) );
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name;
		}
		break;

	case FEEDER_PLAYER_SKIN_LEGS:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount ) {
			*handle1 = trap->R_RegisterShaderNoMip( va( "models/players/%s/icon_%s",
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name,
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name ) );
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name;
		}
		break;

	case FEEDER_SIEGE_BASE_CLASS:
	case FEEDER_SIEGE_CLASS_WEAPONS:
		return "";

	default:
		return "";
	}

	return "";
}

static qhandle_t UI_FeederItemImage( int feederID, int index ) {
	int	validCnt, i;
	static char info[MAX_STRING_CHARS];

	switch ( feederID ) {
	case FEEDER_SABER_SINGLE_INFO:
		return 0;

	case FEEDER_SABER_STAFF_INFO:
		return 0;

	case FEEDER_Q3HEADS:
	{
		int actual = -1;
		UI_SelectedTeamHead( index, &actual );
		index = actual;

		if ( index >= 0 && index < uiInfo.q3HeadCount ) {
			// we want it to load them as it draws them, like the TA feeder
			int selModel = trap->Cvar_VariableValue( "ui_selectedModelIndex" );

			if ( selModel != -1 ) {
				if ( uiInfo.q3SelectedHead != selModel ) {
					uiInfo.q3SelectedHead = selModel;
					Menu_SetFeederSelection( NULL, FEEDER_Q3HEADS, selModel, NULL );
				}
			}

			return uiInfo.q3HeadIcons[index];
		}
		break;
	}

	case FEEDER_SIEGE_TEAM1:
		if ( !siegeTeam1 ) {
			UI_SetSiegeTeams();
			if ( !siegeTeam1 )
				return 0;
		}

		if ( siegeTeam1->classes[index] )
			return siegeTeam1->classes[index]->uiPortraitShader;
		return 0;

	case FEEDER_SIEGE_TEAM2:
		if ( !siegeTeam2 ) {
			UI_SetSiegeTeams();
			if ( !siegeTeam2 )
				return 0;
		}

		if ( siegeTeam2->classes[index] )
			return siegeTeam2->classes[index]->uiPortraitShader;
		return 0;

	case FEEDER_ALLMAPS:
	case FEEDER_MAPS:
	{
		int actual;
		UI_SelectedMap( index, &actual );
		index = actual;
		if ( index >= 0 && index < uiInfo.mapCount ) {
			if ( uiInfo.mapList[index].levelShot == -1 )
				uiInfo.mapList[index].levelShot = trap->R_RegisterShaderNoMip( uiInfo.mapList[index].imageName );
			return uiInfo.mapList[index].levelShot;
		}
		break;
	}

	case FEEDER_PLAYER_SKIN_HEAD:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount ) {
			return trap->R_RegisterShaderNoMip( va( "models/players/%s/icon_%s",
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name,
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name ) );
		}
		break;

	case FEEDER_PLAYER_SKIN_TORSO:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount ) {
			return trap->R_RegisterShaderNoMip( va( "models/players/%s/icon_%s",
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name,
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name ) );
		}
		break;

	case FEEDER_PLAYER_SKIN_LEGS:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount ) {
			return trap->R_RegisterShaderNoMip( va( "models/players/%s/icon_%s",
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name,
				uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name ) );
		}
		break;

	case FEEDER_COLORCHOICES:
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount ) {
			return trap->R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader );
		}
		break;

	case FEEDER_SIEGE_BASE_CLASS:
	{
		int team, baseClass;

		team = (int)trap->Cvar_VariableValue( "ui_team" );
		baseClass = (int)trap->Cvar_VariableValue( "ui_siege_class" );

		if ( team == SIEGETEAM_TEAM1 || team == SIEGETEAM_TEAM2 ) {
			// Is it a valid base class?
			if ( baseClass >= SPC_INFANTRY && baseClass < SPC_MAX ) {
				if ( index >= 0 )
					return BG_GetUIPortrait( team, baseClass, index );
			}
		}
		break;
	}

	case FEEDER_SIEGE_CLASS_WEAPONS:
		validCnt = 0;
		//count them up
		for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
			trap->Cvar_VariableStringBuffer( va( "ui_class_weapon%i", i ), info, sizeof(info) );
			if ( Q_stricmp( info, "gfx/2d/select" ) ) {
				if ( validCnt == index )
					return trap->R_RegisterShaderNoMip( info );
				validCnt++;
			}
		}
		break;

	case FEEDER_SIEGE_CLASS_INVENTORY:
		validCnt = 0;
		//count them up
		for ( i = 0; i < HI_NUM_HOLDABLE; i++ ) {
			trap->Cvar_VariableStringBuffer( va( "ui_class_item%i", i ), info, sizeof(info) );
			// A hack so health and ammo dispenser icons don't show up.
			if ( Q_stricmp( info, "gfx/2d/select" ) && Q_stricmp( info, "gfx/hud/i_icon_healthdisp" )
				&& Q_stricmp( info, "gfx/hud/i_icon_ammodisp" ) ) {
				if ( validCnt == index )
					return trap->R_RegisterShaderNoMip( info );
				validCnt++;
			}
		}
		break;

	case FEEDER_SIEGE_CLASS_FORCE:
	{
		int slotI = 0;
		static char info2[MAX_STRING_CHARS];
		menuDef_t *menu;
		itemDef_t *item;

		validCnt = 0;

		if ( (menu = Menu_GetFocused()) ) {
			if ( (item = Menu_FindItemByName( menu, "base_class_force_feed" )) ) {
				listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
				if ( listPtr )
					slotI = listPtr->startPos;
			}
		}

		//count them up
		for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
			trap->Cvar_VariableStringBuffer( va( "ui_class_power%i", i ), info, sizeof(info) );
			if ( Q_stricmp( info, "gfx/2d/select" ) ) {
				if ( validCnt == index ) {
					trap->Cvar_VariableStringBuffer( va( "ui_class_powerlevel%i", validCnt ), info2, sizeof(info2) );

					trap->Cvar_Set( va( "ui_class_powerlevelslot%i", index - slotI ), info2 );
					return trap->R_RegisterShaderNoMip( info );
				}
				validCnt++;
			}
		}
		break;
	}

	default:
		break;

	}

	return 0;
}

int UI_UpdateNormalMenuCharacter( void ) {
	menuDef_t *menu = NULL;
	itemDef_t *item = NULL;
	char modelPath[MAX_QPATH] = { 0 };
	char skinPath[MAX_QPATH] = { 0 };
	char *modelName = NULL;
	char *skinPtr = NULL;
	int animRunLength = 0;
	int result = 0;

	menu = Menu_GetFocused();
	if ( !menu )
		return 1;

	item = Menu_FindItemByName( menu, "character" );
	if ( !item ) {
		if ( (int)trap->Cvar_VariableValue( "g_gametype" ) != GT_SIEGE ) {//Let's just ignore siege..?!
			//	Com_Error( ERR_FATAL, "UI_UpdateNormalMenuCharacter: Could not find item (character) in menu (%s)", menu->window.name );
		}
		return 1;
	}

	ItemParse_model_g2anim_go( item, ui_char_anim.string );

	//Attempt the grab the model's skin variant, if possible
	modelName = UI_Cvar_VariableString( "model" );
	if ( !modelName || !modelName[0] )
		return 1;
	skinPtr = strstr( modelName, "/" );

	if ( skinPtr && skinPtr[0] == '/' ) {//Got a skin
		skinPtr[0] = '\0';
		Com_sprintf( skinPath, sizeof(skinPath), "models/players/%s/model_%s.skin", modelName, skinPtr + 1 );
	}
	else {//Regular/default model
		Com_sprintf( skinPath, sizeof(skinPath), "models/players/%s/model.skin", modelName );
	}
	Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/model.glm", modelName );

	result = ItemParse_asset_model_go( item, modelPath, &animRunLength );
	if ( result ) {
		ItemParse_model_g2skin_go( item, skinPath );
		return 1;
	}
	return result;
}

qboolean UI_FeederSelection( int feederID, int index, itemDef_t *item ) {
	static char info[MAX_STRING_CHARS];

	if ( feederID == FEEDER_Q3HEADS ) {
		int actual = -1;
		UI_SelectedTeamHead( index, &actual );
		uiInfo.q3SelectedHead = index;
		trap->Cvar_Set( "ui_selectedModelIndex", va( "%i", index ) );
		index = actual;
		if ( index >= 0 && index < uiInfo.q3HeadCount ) {
			trap->Cvar_Set( "model", uiInfo.q3HeadNames[index] );	//standard model
			trap->Cvar_Set( "char_color_red", "255" );			//standard colors
			trap->Cvar_Set( "char_color_green", "255" );
			trap->Cvar_Set( "char_color_blue", "255" );

			//Raz: Model selection preview (From JA+ code)
			if ( !UI_UpdateNormalMenuCharacter() ) {
				uiInfo.q3HeadNames[actual][0] = '\0';
				UI_FeederSelection( feederID, uiInfo.q3SelectedHead, item );
			}
		}
	}
	else if ( feederID == FEEDER_MOVES ) {
		itemDef_t *item;
		menuDef_t *menu;
		modelDef_t *modelPtr;

		menu = Menus_FindByName( "rulesMenu_moves" );

		if ( menu ) {
			item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "character" );
			if ( item ) {
				modelPtr = (modelDef_t*)item->typeData;
				if ( modelPtr ) {
					char modelPath[MAX_QPATH];
					int animRunLength;

					ItemParse_model_g2anim_go( item, datapadMoveData[uiInfo.movesTitleIndex][index].anim );

					Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/model.glm", UI_Cvar_VariableString( "ui_char_model" ) );
					ItemParse_asset_model_go( item, modelPath, &animRunLength );
					UI_UpdateCharacterSkin();

					uiInfo.moveAnimTime = uiInfo.uiDC.realTime + animRunLength;

					if ( datapadMoveData[uiInfo.movesTitleIndex][index].anim ) {

						// Play sound for anim
						if ( datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_FORCE_JUMP ) {
							trap->S_StartLocalSound( uiInfo.uiDC.Assets.moveJumpSound, CHAN_LOCAL );
						}
						else if ( datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_ROLL ) {
							trap->S_StartLocalSound( uiInfo.uiDC.Assets.moveRollSound, CHAN_LOCAL );
						}
						else if ( datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_SABER ) {
							// Randomly choose one sound
							trap->S_StartLocalSound( uiInfo.uiDC.Assets.datapadmoveSaberSound[Q_irand( 0, 5 )], CHAN_LOCAL );
						}

						if ( datapadMoveData[uiInfo.movesTitleIndex][index].desc ) {
							trap->Cvar_Set( "ui_move_desc", datapadMoveData[uiInfo.movesTitleIndex][index].desc );
						}
					}
					UI_SaberAttachToChar( item );
				}
			}
		}
	}
	else if ( feederID == FEEDER_MOVES_TITLES ) {
		itemDef_t *item;
		menuDef_t *menu;
		modelDef_t *modelPtr;

		uiInfo.movesTitleIndex = index;
		uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
		menu = Menus_FindByName( "rulesMenu_moves" );

		if ( menu ) {
			item = (itemDef_t *)Menu_FindItemByName( (menuDef_t *)menu, "character" );
			if ( item ) {
				modelPtr = (modelDef_t*)item->typeData;
				if ( modelPtr ) {
					char modelPath[MAX_QPATH];
					int	animRunLength;

					uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
					ItemParse_model_g2anim_go( item, uiInfo.movesBaseAnim );

					Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/model.glm", UI_Cvar_VariableString( "ui_char_model" ) );
					ItemParse_asset_model_go( item, modelPath, &animRunLength );

					UI_UpdateCharacterSkin();

				}
			}
		}
	}
	else if ( feederID == FEEDER_SIEGE_TEAM1 ) {
		if ( !g_siegedFeederForcedSet ) {
			g_UIGloballySelectedSiegeClass = UI_SiegeClassNum( siegeTeam1->classes[index] );
			trap->Cvar_Set( "ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc );

			//g_siegedFeederForcedSet = 1;
			//Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);

			UI_SiegeSetCvarsForClass( siegeTeam1->classes[index] );
		}
		g_siegedFeederForcedSet = 0;
	}
	else if ( feederID == FEEDER_SIEGE_TEAM2 ) {
		if ( !g_siegedFeederForcedSet ) {
			g_UIGloballySelectedSiegeClass = UI_SiegeClassNum( siegeTeam2->classes[index] );
			trap->Cvar_Set( "ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc );

			//g_siegedFeederForcedSet = 1;
			//Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);

			UI_SiegeSetCvarsForClass( siegeTeam2->classes[index] );
		}
		g_siegedFeederForcedSet = 0;
	}
	else if ( feederID == FEEDER_FORCECFG ) {
		int newindex = index;

		if ( uiForceSide == FORCESIDE_LIGHT ) {
			newindex += uiInfo.forceConfigLightIndexBegin;
			if ( newindex >= uiInfo.forceConfigCount ) {
				return qfalse;
			}
		}
		else { //else dark
			newindex += uiInfo.forceConfigDarkIndexBegin;
			if ( newindex >= uiInfo.forceConfigCount || newindex > uiInfo.forceConfigLightIndexBegin ) { //dark gets read in before light
				return qfalse;
			}
		}

		if ( index >= 0 && index < uiInfo.forceConfigCount ) {
			UI_ForceConfigHandle( uiInfo.forceConfigSelected, index );
			uiInfo.forceConfigSelected = index;
		}
	}
	else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS ) {
		int actual, map;
		const char *checkValid = NULL;

		map = (feederID == FEEDER_ALLMAPS) ? ui_currentNetMap.integer : ui_currentMap.integer;
		if ( uiInfo.mapList[map].cinematic >= 0 ) {
			trap->CIN_StopCinematic( uiInfo.mapList[map].cinematic );
			uiInfo.mapList[map].cinematic = -1;
		}
		checkValid = UI_SelectedMap( index, &actual );

		if ( !checkValid || !checkValid[0] ) { //this isn't a valid map to select, so reselect the current
			index = ui_mapIndex.integer;
			UI_SelectedMap( index, &actual );
		}

		trap->Cvar_Set( "ui_mapIndex", va( "%d", index ) );
		gUISelectedMap = index;
		ui_mapIndex.integer = index;

		if ( feederID == FEEDER_MAPS ) {
			trap->Cvar_Set( "ui_currentMap", va( "%d", actual ) );
			trap->Cvar_Update( &ui_currentMap );
			uiInfo.mapList[ui_currentMap.integer].cinematic = trap->CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[ui_currentMap.integer].mapLoadName ), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
			//trap->Cvar_Set("ui_opponentModel", uiInfo.mapList[ui_currentMap.integer].opponentName);
			//updateOpponentModel = qtrue;
		}
		else {
			trap->Cvar_Set( "ui_currentNetMap", va( "%d", actual ) );
			trap->Cvar_Update( &ui_currentNetMap );
			uiInfo.mapList[ui_currentNetMap.integer].cinematic = trap->CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}

	}
	else if ( feederID == FEEDER_SERVERS ) {
		const char *mapName = NULL;
		uiInfo.serverStatus.currentServer = index;
		trap->LAN_GetServerInfo( UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS );
		uiInfo.serverStatus.currentServerPreview = trap->R_RegisterShaderNoMip( va( "levelshots/%s", Info_ValueForKey( info, "mapname" ) ) );
		if ( uiInfo.serverStatus.currentServerCinematic >= 0 ) {
			trap->CIN_StopCinematic( uiInfo.serverStatus.currentServerCinematic );
			uiInfo.serverStatus.currentServerCinematic = -1;
		}
		mapName = Info_ValueForKey( info, "mapname" );
		if ( mapName && *mapName ) {
			uiInfo.serverStatus.currentServerCinematic = trap->CIN_PlayCinematic( va( "%s.roq", mapName ), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}
	}
	else if ( feederID == FEEDER_SERVERSTATUS ) {
		//
	}
	else if ( feederID == FEEDER_FINDPLAYER ) {
		uiInfo.currentFoundPlayerServer = index;
		//
		if ( index < uiInfo.numFoundPlayerServers - 1 ) {
			// build a new server status for this server
			Q_strncpyz( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress) );
			Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
			UI_BuildServerStatus( qtrue );
		}
	}
	else if ( feederID == FEEDER_PLAYER_LIST ) {
		uiInfo.playerIndex = index;
	}
	else if ( feederID == FEEDER_TEAM_LIST ) {
		uiInfo.teamIndex = index;
	}
	else if ( feederID == FEEDER_MODS ) {
		uiInfo.modIndex = index;
	}
	else if ( feederID == FEEDER_DEMOS ) {
		uiInfo.demoIndex = index;
	}
	else if ( feederID == FEEDER_COLORCHOICES ) {
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount ) {
			Item_RunScript( item, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].actionText );
		}
	}
	else if ( feederID == FEEDER_PLAYER_SKIN_HEAD ) {
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount ) {
			trap->Cvar_Set( "ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name );
		}
	}
	else if ( feederID == FEEDER_PLAYER_SKIN_TORSO ) {
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount ) {
			trap->Cvar_Set( "ui_char_skin_torso", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name );
		}
	}
	else if ( feederID == FEEDER_PLAYER_SKIN_LEGS ) {
		if ( index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount ) {
			trap->Cvar_Set( "ui_char_skin_legs", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name );
		}
	}
	else if ( feederID == FEEDER_PLAYER_SPECIES ) {
		if ( index >= 0 && index < uiInfo.playerSpeciesCount ) {
			uiInfo.playerSpeciesIndex = index;
		}
	}
	else if ( feederID == FEEDER_LANGUAGES ) {
		uiInfo.languageCountIndex = index;
	}
	else if ( feederID == FEEDER_SIEGE_BASE_CLASS ) {
		int team, baseClass;

		team = (int)trap->Cvar_VariableValue( "ui_team" );
		baseClass = (int)trap->Cvar_VariableValue( "ui_siege_class" );

		UI_UpdateCvarsForClass( team, baseClass, index );
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_WEAPONS ) {
		//		trap->Cvar_VariableStringBuffer( va("ui_class_weapondesc%i", index), info, sizeof(info) );
		//		trap->Cvar_Set( "ui_itemforceinvdesc", info );
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_INVENTORY ) {
		//		trap->Cvar_VariableStringBuffer( va("ui_class_itemdesc%i", index), info, sizeof(info) );
		//		trap->Cvar_Set( "ui_itemforceinvdesc", info );
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_FORCE ) {
		int i;
		trap->Cvar_VariableStringBuffer( va( "ui_class_power%i", index ), info, sizeof(info) );

		//count them up
		for ( i = 0; i < NUM_FORCE_POWERS; i++ ) {
			if ( !strcmp( HolocronIcons[i], info ) ) {
				trap->Cvar_Set( "ui_itemforceinvdesc", forcepowerDesc[i] );
				break;
			}
		}
	}
	return qtrue;
}

static void UI_Pause( qboolean b ) {
	if ( b ) {
		// pause the game and set the ui keycatcher
		trap->Cvar_Set( "cl_paused", "1" );
		trap->Key_SetCatcher( KEYCATCH_UI );
	}
	else {
		// unpause the game and clear the ui keycatcher
		trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
		trap->Key_ClearStates();
		trap->Cvar_Set( "cl_paused", "0" );
	}
}

static int UI_PlayCinematic( const char *name, float x, float y, float w, float h ) {
	return trap->CIN_PlayCinematic( name, x, y, w, h, (CIN_loop | CIN_silent) );
}

static void UI_StopCinematic( int handle ) {
	if ( handle >= 0 ) {
		trap->CIN_StopCinematic( handle );
	}
	else {
		handle = abs( handle );
		if ( handle == UI_MAPCINEMATIC ) {
			if ( uiInfo.mapList[ui_currentMap.integer].cinematic >= 0 ) {
				trap->CIN_StopCinematic( uiInfo.mapList[ui_currentMap.integer].cinematic );
				uiInfo.mapList[ui_currentMap.integer].cinematic = -1;
			}
		}
		else if ( handle == UI_NETMAPCINEMATIC ) {
			if ( uiInfo.serverStatus.currentServerCinematic >= 0 ) {
				trap->CIN_StopCinematic( uiInfo.serverStatus.currentServerCinematic );
				uiInfo.serverStatus.currentServerCinematic = -1;
			}
		}
	}
}

static void UI_DrawCinematic( int handle, float x, float y, float w, float h ) {
	trap->CIN_SetExtents( handle, x, y, w, h );
	trap->CIN_DrawCinematic( handle );
}

static void UI_RunCinematicFrame( int handle ) {
	trap->CIN_RunCinematic( handle );
}


// Looks in the directory for force config files (.fcf) and loads the name in
void UI_LoadForceConfig_List( void ) {
	int			numfiles = 0;
	char		filelist[2048];
	char		configname[128];
	char		*fileptr = NULL;
	int			j = 0;
	int			filelen = 0;
	qboolean	lightSearch = qfalse;

	uiInfo.forceConfigCount = 0;
	Com_sprintf( uiInfo.forceConfigNames[uiInfo.forceConfigCount], sizeof(uiInfo.forceConfigNames[uiInfo.forceConfigCount]), "Custom" );
	uiInfo.forceConfigCount++;
	//Always reserve index 0 as the "custom" config

nextSearch:
	if ( lightSearch ) { //search light side folder
		numfiles = trap->FS_GetFileList( "forcecfg/light", "fcf", filelist, 2048 );
		uiInfo.forceConfigLightIndexBegin = uiInfo.forceConfigCount - 1;
	}
	else { //search dark side folder
		numfiles = trap->FS_GetFileList( "forcecfg/dark", "fcf", filelist, 2048 );
		uiInfo.forceConfigDarkIndexBegin = uiInfo.forceConfigCount - 1;
	}

	fileptr = filelist;

	for ( j = 0; j < numfiles && uiInfo.forceConfigCount < MAX_FORCE_CONFIGS; j++, fileptr += filelen + 1 ) {
		filelen = strlen( fileptr );
		COM_StripExtension( fileptr, configname, sizeof(configname) );

		if ( lightSearch ) {
			uiInfo.forceConfigSide[uiInfo.forceConfigCount] = qtrue; //light side config
		}
		else {
			uiInfo.forceConfigSide[uiInfo.forceConfigCount] = qfalse; //dark side config
		}

		Com_sprintf( uiInfo.forceConfigNames[uiInfo.forceConfigCount], sizeof(uiInfo.forceConfigNames[uiInfo.forceConfigCount]), configname );
		uiInfo.forceConfigCount++;
	}

	if ( !lightSearch ) {
		lightSearch = qtrue;
		goto nextSearch;
	}
}

// builds path and scans for valid image extentions
static qboolean bIsImageFile( const char* dirptr, const char* skinname ) {
	char fpath[MAX_QPATH];
	int f;

	Com_sprintf( fpath, MAX_QPATH, "models/players/%s/icon_%s.jpg", dirptr, skinname );
	trap->FS_Open( fpath, &f, FS_READ );
	if ( !f ) { //not there, try png
		Com_sprintf( fpath, MAX_QPATH, "models/players/%s/icon_%s.png", dirptr, skinname );
		trap->FS_Open( fpath, &f, FS_READ );
	}
	if ( !f ) { //not there, try tga
		Com_sprintf( fpath, MAX_QPATH, "models/players/%s/icon_%s.tga", dirptr, skinname );
		trap->FS_Open( fpath, &f, FS_READ );
	}
	if ( f ) {
		trap->FS_Close( f );
		return qtrue;
	}

	return qfalse;
}

//Raz: This actually checks for an existing .skin
static qboolean bIsSkinFile( const char* dirptr, const char* skinname ) {
	char fpath[MAX_QPATH];
	int f;

	Com_sprintf( fpath, MAX_QPATH, "models/players/%s/model_%s.skin", dirptr, skinname );
	trap->FS_Open( fpath, &f, FS_READ );
	if ( f ) {
		trap->FS_Close( f );
		return qtrue;
	}

	return qfalse;
}

static qboolean bIsAnimFile( const char* dirptr ) {
	char fpath[MAX_QPATH];
	int f;

	Com_sprintf( fpath, MAX_QPATH, "models/players/%s/animation.cfg", dirptr );
	trap->FS_Open( fpath, &f, FS_READ );
	if ( f ) {
		trap->FS_Close( f );
		return qtrue;
	}

	return qfalse;
}

static void UI_BuildQ3Model_List( void ) {
	int		numdirs = 0;
	int		numfiles = 0;
	char	dirlist[4096/*2048*/] = { 0 };
	char	filelist[4096/*2048*/] = { 0 };
	char	skinname[64] = { 0 };
	char*	dirptr = NULL;
	char*	fileptr = NULL;
	char*	check = NULL;
	int		i = 0;
	int		j, k, p, s;
	int		dirlen;
	int		filelen;

	uiInfo.q3HeadCount = 0;

	// iterate directory of all player models
	numdirs = trap->FS_GetFileList( "models/players", "/", dirlist, sizeof(dirlist) );
	dirptr = dirlist;
	for ( i = 0; i < numdirs && uiInfo.q3HeadCount < MAX_Q3PLAYERMODELS; i++, dirptr += dirlen + 1 ) {
		dirlen = strlen( dirptr );

		if ( dirlen && dirptr[dirlen - 1] == '/' )
			dirptr[dirlen - 1] = '\0';

		if ( !strcmp( dirptr, "." ) || !strcmp( dirptr, ".." ) )
			continue;


		numfiles = trap->FS_GetFileList( va( "models/players/%s", dirptr ), "skin", filelist, sizeof(filelist) );
		fileptr = filelist;
		for ( j = 0;
			j < numfiles && uiInfo.q3HeadCount < MAX_Q3PLAYERMODELS;
			j++, fileptr += filelen + 1 ) {
			int skinLen = 0;

			filelen = strlen( fileptr );

			COM_StripExtension( fileptr, skinname, sizeof(skinname) );

			skinLen = strlen( skinname );
			//Raz: BROKEN!!
			/*
			k = 0;
			while (k < skinLen && skinname[k] && skinname[k] != '_')
			{
			k++;
			}
			*/
			k = strchr( skinname, '/' ) ? (strchr( skinname, '/' ) - skinname) : 0;
			while ( k < skinLen && skinname[k] && skinname[k] != '_' )
				k++;

			if ( skinname[k] == '_' ) {
				p = 0;

				while ( skinname[k] ) {
					skinname[p] = skinname[k];
					k++;
					p++;
				}
				skinname[p] = '\0';
			}
			//	Com_Printf( "Loading %s\n", dirptr );

			/*
			Com_sprintf(fpath, 2048, "models/players/%s/icon%s.jpg", dirptr, skinname);

			trap->FS_Open(fpath, &f, FS_READ);

			if (f)
			*/
			check = &skinname[1];
			if ( (ui_showMissingSkins.integer && bIsSkinFile( dirptr, check ) && !bIsAnimFile( dirptr ) && Q_stricmp( check, "menu" ))
				|| bIsImageFile( dirptr, check ) )
				//	if ( *dirptr )
			{ //if it exists
				qboolean iconExists = qfalse;

				//trap->FS_Close(f);

				if ( skinname[0] == '_' ) { //change character to append properly
					skinname[0] = '/';
				}

				s = 0;

				//Raz: Slider does a hack here to remove siege models and tavion/possessed

				while ( s < uiInfo.q3HeadCount ) { //check for dupes
					if ( !Q_stricmp( va( "%s%s", dirptr, skinname ), uiInfo.q3HeadNames[s] ) ) {
						iconExists = qtrue;
						break;
					}
					s++;
				}

				if ( iconExists ) {
					continue;
				}

				Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof(uiInfo.q3HeadNames[uiInfo.q3HeadCount]), va( "%s%s", dirptr, skinname ) );

				uiInfo.q3HeadIcons[uiInfo.q3HeadCount] = 0;//trap->R_RegisterShaderNoMip(fpath);
				{
					char iconNameFromSkinName[256];
					int i = 0;
					int skinPlace;

					i = strlen( uiInfo.q3HeadNames[uiInfo.q3HeadCount] );

					while ( uiInfo.q3HeadNames[uiInfo.q3HeadCount][i] != '/' )
						i--;

					i++;
					skinPlace = i; //remember that this is where the skin name begins

					//now, build a full path out of what's in q3HeadNames, into iconNameFromSkinName
					Com_sprintf( iconNameFromSkinName, sizeof(iconNameFromSkinName), "models/players/%s", uiInfo.q3HeadNames[uiInfo.q3HeadCount] );

					i = strlen( iconNameFromSkinName );

					while ( iconNameFromSkinName[i] != '/' )
						i--;

					i++;
					iconNameFromSkinName[i] = 0; //terminate, and append..
					Q_strcat( iconNameFromSkinName, 256, "icon_" );

					//and now, for the final step, append the skin name from q3HeadNames onto the end of iconNameFromSkinName
					i = strlen( iconNameFromSkinName );

					while ( uiInfo.q3HeadNames[uiInfo.q3HeadCount][skinPlace] ) {
						iconNameFromSkinName[i] = uiInfo.q3HeadNames[uiInfo.q3HeadCount][skinPlace];
						i++;
						skinPlace++;
					}
					iconNameFromSkinName[i] = 0;

					//and now we are ready to register (thankfully this will only happen once)
					uiInfo.q3HeadIcons[uiInfo.q3HeadCount] = trap->R_RegisterShaderNoMip( iconNameFromSkinName );
					if ( !uiInfo.q3HeadIcons[uiInfo.q3HeadCount] )
						uiInfo.q3HeadIcons[uiInfo.q3HeadCount] = trap->R_RegisterShaderNoMip( "gfx/2d/defer.tga" );
				}
				uiInfo.q3HeadCount++;
				//rww - we are now registering them as they are drawn like the TA feeder, so as to decrease UI load time.
			}

			if ( uiInfo.q3HeadCount >= MAX_Q3PLAYERMODELS ) {
				return;
			}
		}
	}

}

void UI_SiegeInit( void ) {
	//Load the player class types
	BG_SiegeLoadClasses( g_UIClassDescriptions );

	if ( !bgNumSiegeClasses ) { //We didn't find any?!
		Com_Error( ERR_DROP, "Couldn't find any player classes for Siege" );
	}

	//Now load the teams since we have class data.
	BG_SiegeLoadTeams();

	if ( !bgNumSiegeTeams ) { //React same as with classes.
		Com_Error( ERR_DROP, "Couldn't find any player teams for Siege" );
	}
}

static qboolean UI_ParseColorData( char* buf, playerSpeciesInfo_t *species, char*	file ) {
	const char	*token;
	const char	*p;

	p = buf;
	COM_BeginParseSession( file );
	species->ColorCount = 0;
	species->ColorMax = 16;
	species->Color = (playerColor_t *)malloc( species->ColorMax * sizeof(playerColor_t) );

	while ( p ) {
		token = COM_ParseExt( &p, qtrue );	//looking for the shader
		if ( token[0] == 0 ) {
			return species->ColorCount;
		}
		if ( species->ColorCount >= species->ColorMax ) {
			species->ColorMax *= 2;
			species->Color = (playerColor_t *)realloc( species->Color, species->ColorMax * sizeof(playerColor_t) );
		}

		memset( &species->Color[species->ColorCount], 0, sizeof(playerColor_t) );

		Q_strncpyz( species->Color[species->ColorCount].shader, token, MAX_QPATH );

		token = COM_ParseExt( &p, qtrue );	//looking for action block {
		if ( token[0] != '{' ) {
			return qfalse;
		}

		token = COM_ParseExt( &p, qtrue );	//looking for action commands
		while ( token[0] != '}' ) {
			if ( token[0] == 0 ) {	//EOF
				return qfalse;
			}
			Q_strcat( species->Color[species->ColorCount].actionText, ACTION_BUFFER_SIZE, token );
			Q_strcat( species->Color[species->ColorCount].actionText, ACTION_BUFFER_SIZE, " " );
			token = COM_ParseExt( &p, qtrue );	//looking for action commands or final }
		}
		species->ColorCount++;	//next color please
	}
	return qtrue;//never get here
}

static void UI_FreeSpecies( playerSpeciesInfo_t *species ) {
	free( species->SkinHead );
	free( species->SkinTorso );
	free( species->SkinLeg );
	free( species->Color );
	memset( species, 0, sizeof(playerSpeciesInfo_t) );
}

void UI_FreeAllSpecies( void ) {
	for ( int i = 0; i < uiInfo.playerSpeciesCount; i++ ) {
		UI_FreeSpecies( &uiInfo.playerSpecies[i] );
	}
	free( uiInfo.playerSpecies );
}

static void UI_BuildPlayerModel_List( qboolean inGameLoad ) {
	uiInfo.playerSpeciesCount = 0;
	uiInfo.playerSpeciesIndex = 0;
	uiInfo.playerSpeciesMax = 8;
	uiInfo.playerSpecies = (playerSpeciesInfo_t *)malloc( uiInfo.playerSpeciesMax * sizeof(playerSpeciesInfo_t) );

	// iterate directory of all player models
	int dirLen = 0;

	char dirList[2048];
	char *pDir = dirList;
	int numDirs = trap->FS_GetFileList("models/players", "/", dirList, sizeof(dirList) );
	for ( int i = 0; i < numDirs; i++, pDir += dirLen + 1 ) {
		dirLen = strlen( pDir );

		if ( dirLen ) {
			if ( pDir[dirLen-1] == '/' ) {
				pDir[dirLen-1] = '\0';
			}
		}
		else {
			continue;
		}

		if ( !strcmp( pDir, "." ) || !strcmp( pDir, ".." ) ) {
			continue;
		}

		char fPath[2048];
		Com_sprintf( fPath, sizeof(fPath), "models/players/%s/PlayerChoice.txt", pDir );
		fileHandle_t f = NULL_FILE;
		int fileLen = trap->FS_Open( fPath, &f, FS_READ );

		if ( f ) {
			playerSpeciesInfo_t *species;
			char skinName[64];
			uint32_t iSkinParts = 0u;

			char *buffer = (char *)malloc( fileLen + 1 );
			if ( !buffer ) {
				Com_Error( ERR_FATAL, "Could not allocate buffer to read %s", fPath );
			}

			trap->FS_Read( buffer, fileLen, f );
			trap->FS_Close( f );

			buffer[fileLen] = 0;

			//record this species
			if ( uiInfo.playerSpeciesCount >= uiInfo.playerSpeciesMax ) {
				uiInfo.playerSpeciesMax *= 2;
				uiInfo.playerSpecies = (playerSpeciesInfo_t *)realloc(
					uiInfo.playerSpecies, uiInfo.playerSpeciesMax * sizeof(playerSpeciesInfo_t)
				);
			}
			species = &uiInfo.playerSpecies[uiInfo.playerSpeciesCount];
			memset( species, 0, sizeof(playerSpeciesInfo_t) );
			Q_strncpyz( species->Name, pDir, MAX_QPATH );

			if ( !UI_ParseColorData( buffer, species, fPath ) ) {
				Com_Printf( S_COLOR_RED "UI_BuildPlayerModel_List: Errors parsing '%s'\n", fPath );
			}

			species->SkinHeadMax = 8;
			species->SkinTorsoMax = 8;
			species->SkinLegMax = 8;

			species->SkinHead = (skinName_t *)malloc( species->SkinHeadMax * sizeof(skinName_t) );
			species->SkinTorso = (skinName_t *)malloc( species->SkinTorsoMax * sizeof(skinName_t) );
			species->SkinLeg = (skinName_t *)malloc( species->SkinLegMax * sizeof(skinName_t) );

			free( buffer );

			char fileList[2048];
			int numFiles = trap->FS_GetFileList( va( "models/players/%s", pDir ), ".skin", fileList, 2048 );
			char *pFile = fileList;
			for ( int j = 0; j < numFiles; j++, pFile += fileLen + 1 ) {
				if ( trap->Cvar_VariableValue( "fs_copyfiles" ) > 0 ) {
					trap->FS_Open( va( "models/players/%s/%s", pDir, pFile ), &f, FS_READ );
					if ( f ) {
						trap->FS_Close( f );
					}
				}

				fileLen = strlen( pFile );
				COM_StripExtension( pFile, skinName, sizeof(skinName) );

				if ( bIsImageFile( pDir, skinName ) ) {
					// if it exists
					if ( !Q_stricmpn( skinName, "head_", 5 ) ) {
						if ( species->SkinHeadCount >= species->SkinHeadMax ) {
							species->SkinHeadMax *= 2;
							species->SkinHead = (skinName_t *)realloc(
								species->SkinHead, species->SkinHeadMax*sizeof(skinName_t)
							);
						}
						Q_strncpyz( species->SkinHead[species->SkinHeadCount++].name, skinName, SKIN_LENGTH );
						iSkinParts |= 1 << 0;
					}
					else if ( !Q_stricmpn( skinName, "torso_", 6 ) ) {
						if ( species->SkinTorsoCount >= species->SkinTorsoMax ) {
							species->SkinTorsoMax *= 2;
							species->SkinTorso = (skinName_t *)realloc(
								species->SkinTorso, species->SkinTorsoMax*sizeof(skinName_t)
							);
						}
						Q_strncpyz( species->SkinTorso[species->SkinTorsoCount++].name, skinName, SKIN_LENGTH );
						iSkinParts |= 1 << 1;
					}
					else if ( !Q_stricmpn( skinName, "lower_", 6 ) ) {
						if ( species->SkinLegCount >= species->SkinLegMax ) {
							species->SkinLegMax *= 2;
							species->SkinLeg = (skinName_t *)realloc(
								species->SkinLeg, species->SkinLegMax*sizeof(skinName_t)
							);
						}
						Q_strncpyz( species->SkinLeg[species->SkinLegCount++].name, skinName, SKIN_LENGTH );
						iSkinParts |= 1 << 2;
					}
				}
			}
			if ( iSkinParts != 7 ) {
				// didn't get a skin for each, then skip this model.
				UI_FreeSpecies( species );
				continue;
			}
			uiInfo.playerSpeciesCount++;
			if ( !inGameLoad && ui_PrecacheModels.integer ) {
				Com_sprintf( fPath, sizeof( fPath ), "models/players/%s/model.glm", pDir );
				void *ghoul2 = nullptr;
				int g2Model = trap->G2API_InitGhoul2Model( &ghoul2, fPath, 0, 0, 0, 0, 0 );
				if ( g2Model >= 0 ) {
//					trap->G2API_RemoveGhoul2Model( &ghoul2, 0 );
					trap->G2API_CleanGhoul2Models( &ghoul2 );
				}
			}
		}
	}
}

static qhandle_t UI_RegisterShaderNoMip( const char *name ) {
	if ( *name == '*' ) {
		char buf[MAX_CVAR_VALUE_STRING];

		trap->Cvar_VariableStringBuffer( name + 1, buf, sizeof(buf) );

		if ( buf[0] )
			return trap->R_RegisterShaderNoMip( buf );
	}

	return trap->R_RegisterShaderNoMip( name );
}

void UI_Init( qboolean inGameLoad ) {
	const char *menuSet;
	qhandle_t value;

	// Get the list of possible languages
	uiInfo.languageCount = trap->SE_GetNumLanguages();	// this does a dir scan, so use carefully

	uiInfo.inGameLoad = inGameLoad;

	//initialize all these cvars to "0"
	UI_SiegeSetCvarsForClass( NULL );

	UI_SiegeInit();

	UI_UpdateForcePowers();

	UI_RegisterCvars();
	UI_InitMemory();

	if ( japp_crashHandler.integer ) {
		ActivateCrashHandler();
	}

	// Register identifier and support flags this way to ensure validity
	trap->Cvar_Register( NULL, "csf", va( "%X", JAPP_CLIENT_FLAGS ), CVAR_USERINFO | CVAR_ROM );

#ifdef FAV_SERVERS
	JP_ParseFavServers();
#endif

	// cache redundant calulations
	trap->GetGlconfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0f / (float)SCREEN_HEIGHT);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0f / (float)SCREEN_WIDTH);
	extern void UI_Set2DRatio(void);
	UI_Set2DRatio();
	// wide screen
	if ( uiInfo.uiDC.glconfig.vidWidth * SCREEN_HEIGHT > uiInfo.uiDC.glconfig.vidHeight * SCREEN_WIDTH ) {
		uiInfo.uiDC.bias = 0.5f * (uiInfo.uiDC.glconfig.vidWidth - (uiInfo.uiDC.glconfig.vidHeight * ((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT)));
	}
	else {
		uiInfo.uiDC.bias = 0;
	}

	//	UI_Load();
	uiInfo.uiDC.registerShaderNoMip = &UI_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = trap->R_DrawStretchPic;
	uiInfo.uiDC.drawText = &Text_Paint;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.registerModel = trap->R_RegisterModel;
	uiInfo.uiDC.modelBounds = trap->R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &_UI_DrawRect;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = trap->R_ClearScene;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.addRefEntityToScene = &SE_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = trap->R_RenderScene;
	uiInfo.uiDC.RegisterFont = trap->R_RegisterFont;
	uiInfo.uiDC.Font_StrLenPixels = trap->R_Font_StrLenPixels;
	uiInfo.uiDC.Font_StrLenChars = trap->R_Font_StrLenChars;
	uiInfo.uiDC.Font_HeightPixels = trap->R_Font_HeightPixels;
	uiInfo.uiDC.Font_DrawString = trap->R_Font_DrawString;
	uiInfo.uiDC.Language_IsAsian = trap->R_Language_IsAsian;
	uiInfo.uiDC.Language_UsesSpaces = trap->R_Language_UsesSpaces;
	uiInfo.uiDC.AnyLanguage_ReadCharFromString = trap->R_AnyLanguage_ReadCharFromString;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.getValue = &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.deferScript = &UI_DeferMenuScript;
	uiInfo.uiDC.getTeamColor = &UI_GetTeamColor;
	uiInfo.uiDC.setCVar = trap->Cvar_Set;
	uiInfo.uiDC.getCVarString = trap->Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap->Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode = trap->Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = trap->Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = trap->S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.setBinding = trap->Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = trap->Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = trap->Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = trap->Cmd_ExecuteText;
	uiInfo.uiDC.Error = trap->Error;
	uiInfo.uiDC.Print = trap->Print;
	uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound = trap->S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = trap->S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = trap->S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic = &UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic = &UI_StopCinematic;
	uiInfo.uiDC.drawCinematic = &UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame = &UI_RunCinematicFrame;

	uiInfo.uiDC.ext.Font_StrLenPixels = trap->ext.R_Font_StrLenPixels;

	//TODO: communicate between cgame<->ui a better way
	value = trap->R_RegisterFont( "ocr_a" );
	trap->Cvar_Register( NULL, "font_small", va( "%i", value ), CVAR_ROM | CVAR_INTERNAL );
	uiInfo.uiDC.Assets.qhSmallFont = value;

	value = trap->R_RegisterFont( "arialnb" );
	trap->Cvar_Register( NULL, "font_small2", va( "%i", value ), CVAR_ROM | CVAR_INTERNAL );
	uiInfo.uiDC.Assets.qhSmall2Font = value;

	value = trap->R_RegisterFont( "ergoec" );
	trap->Cvar_Register( NULL, "font_medium", va( "%i", value ), CVAR_ROM | CVAR_INTERNAL );
	uiInfo.uiDC.Assets.qhMediumFont = value;

	//Raz: fonts
	value = trap->R_RegisterFont( "japplarge" );
	trap->Cvar_Register( NULL, "font_japplarge", va( "%i", value ), CVAR_ROM | CVAR_INTERNAL );
	uiInfo.uiDC.Assets.japp.fontLarge = value;

	value = trap->R_RegisterFont( "jappsmall" );
	trap->Cvar_Register( NULL, "font_jappsmall", va( "%i", value ), CVAR_ROM | CVAR_INTERNAL );
	uiInfo.uiDC.Assets.japp.fontSmall = value;

	value = trap->R_RegisterFont( "jappmono" );
	trap->Cvar_Register( NULL, "font_jappmono", va( "%i", value ), CVAR_ROM | CVAR_INTERNAL );
	uiInfo.uiDC.Assets.japp.fontMono = value;

	value = uiInfo.uiDC.Assets.qhMediumFont;
	trap->Cvar_Register( NULL, "font_big", va( "%i", value ), CVAR_ROM | CVAR_INTERNAL );
	uiInfo.uiDC.Assets.qhBigFont = value;


	Init_Display( &uiInfo.uiDC );

	UI_BuildPlayerModel_List( inGameLoad );

	String_Init();

	uiInfo.uiDC.cursor = trap->R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	uiInfo.uiDC.whiteShader = trap->R_RegisterShaderNoMip( "white" );

	AssetCache();

	uiInfo.teamCount = 0;
	uiInfo.characterCount = 0;
	uiInfo.aliasCount = 0;

	menuSet = UI_Cvar_VariableString( "ui_menuFilesMP" );
	if ( menuSet == NULL || menuSet[0] == '\0' )
		menuSet = "ui/jampmenus.txt";

#if 1
	if ( inGameLoad )
		UI_LoadMenus( "ui/jampingame.txt", qtrue );
	else if ( !ui_bypassMainMenuLoad.integer )
		UI_LoadMenus( menuSet, qtrue );
#else //this was adding quite a giant amount of time to the load time
	UI_LoadMenus( menuSet, qtrue );
	UI_LoadMenus( "ui/jampingame.txt", qtrue );
#endif

	{
		//Raz: Truncate the name, try and avoid overflow glitches for long names and menu feeders
		//	trap->Cvar_Register( NULL, "ui_name", UI_Cvar_VariableString( "name" ), CVAR_INTERNAL );	//get this now, jic the menus change again trying to setName before getName
		char buf[MAX_NETNAME] = { 0 };
		Q_strncpyz( buf, UI_Cvar_VariableString( "name" ), sizeof(buf) );
		trap->Cvar_Register( NULL, "ui_Name", buf, CVAR_INTERNAL );
	}

	Menus_CloseAll();

	trap->LAN_LoadCachedServers();

	UI_BuildQ3Model_List();
	UI_LoadBots();

	UI_LoadForceConfig_List();

	UI_InitForceShaders();

	// sets defaults for ui temp cvars
	uiInfo.currentCrosshair = (int)trap->Cvar_VariableValue( "cg_drawCrosshair" );
	trap->Cvar_Set( "ui_mousePitch", (trap->Cvar_VariableValue( "m_pitch" ) >= 0) ? "0" : "1" );
	trap->Cvar_Set( "ui_mousePitchVeh", (trap->Cvar_VariableValue( "m_pitchVeh" ) >= 0) ? "0" : "1" );

	uiInfo.serverStatus.currentServerCinematic = -1;

	trap->Cvar_Register( NULL, "debug_protocol", "", 0 );
}

#define	UI_FPS_FRAMES	4
void UI_Refresh( int realtime ) {
	static int index, previousTimes[UI_FPS_FRAMES];

	trap->G2API_SetTime( realtime, 0 );
	trap->G2API_SetTime( realtime, 1 );
	//ghoul2 timer must be explicitly updated during ui rendering.

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0; i<UI_FPS_FRAMES; i++ )
			total += previousTimes[i];
		if ( !total )
			total = 1;
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}

	UI_UpdateCvars();

	if ( Menu_Count() > 0 ) {
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus( qfalse );
		// refresh find player list
		UI_BuildFindPlayerList( qfalse );
	}
	// draw cursor
	UI_SetColor( NULL );
	if ( Menu_Count() > 0 && (trap->Key_GetCatcher() & KEYCATCH_UI) ) {
		UI_DrawHandlePic(uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 40.0f * uiInfo.uiDC.widthRatioCoef, 40.0f, uiInfo.uiDC.Assets.cursor);
	}

	if ( ui_rankChange.integer ) {
		FPMessageTime = realtime + 3000;

		if ( !parsedFPMessage[0] ) {
			const char *printMessage = UI_GetStringEdString( "MP_INGAME", "SET_NEW_RANK" );

			int i = 0, p = 0, linecount = 0;

			while ( printMessage[i] && p < 1024 ) {
				parsedFPMessage[p] = printMessage[i];
				p++;
				i++;
				linecount++;

				if ( linecount > 64 && printMessage[i] == ' ' ) {
					parsedFPMessage[p] = '\n';
					p++;
					linecount = 0;
				}
			}
			parsedFPMessage[p] = '\0';
		}

		uiMaxRank = ui_rankChange.integer;
		uiForceRank = uiMaxRank;

		//Use BG_LegalizedForcePowers and transfer the result into the UI force settings
		UI_ReadLegalForce();

		if ( ui_freeSaber.integer && uiForcePowersRank[FP_SABER_OFFENSE] < 1 )
			uiForcePowersRank[FP_SABER_OFFENSE] = 1;
		if ( ui_freeSaber.integer && uiForcePowersRank[FP_SABER_DEFENSE] < 1 )
			uiForcePowersRank[FP_SABER_DEFENSE] = 1;
		trap->Cvar_Set( "ui_rankChange", "0" );

		// remember to update the force power count after changing the max rank
		UpdateForceUsed();
	}

	if ( ui_freeSaber.integer ) {
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 0;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 0;
	}
	else {
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 1;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 1;
	}
}

void UI_KeyEvent( int key, qboolean down ) {
	if ( Menu_Count() > 0 ) {
		menuDef_t *menu = Menu_GetFocused();
		if ( menu ) {
			if ( key == A_ESCAPE && down && !Menus_AnyFullScreenVisible() ) {
				Menus_CloseAll();
			}
			else {
				Menu_HandleKey( menu, key, down );
			}
		}
		else {
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
		}
	}

	//if ((s > 0) && (s != menu_null_sound)) {
	//  trap->S_StartLocalSound( s, CHAN_LOCAL_SOUND );
	//}
}

void UI_MouseEvent( int dx, int dy ) {
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if ( uiInfo.uiDC.cursorx < 0 )
		uiInfo.uiDC.cursorx = 0;
	else if ( uiInfo.uiDC.cursorx > SCREEN_WIDTH )
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;

	uiInfo.uiDC.cursory += dy;
	if ( uiInfo.uiDC.cursory < 0 )
		uiInfo.uiDC.cursory = 0;
	else if ( uiInfo.uiDC.cursory > SCREEN_HEIGHT )
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;

	if ( Menu_Count() > 0 ) {
		//menuDef_t *menu = Menu_GetFocused();
		//Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove( NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
	}
}

static void UI_ReadableSize( char *buf, int bufsize, int value ) {
	if ( value > 1024 * 1024 * 1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / (1024 * 1024 * 1024) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d GB",
			(value % (1024 * 1024 * 1024)) * 100 / (1024 * 1024 * 1024) );
	}
	else if ( value > 1024 * 1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / (1024 * 1024) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d MB",
			(value % (1024 * 1024)) * 100 / (1024 * 1024) );
	}
	else if ( value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	}
	else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime( char *buf, int bufsize, int time ) {
	time /= 1000;  // change to seconds

	if ( time > 3600 ) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %2d min", time / 3600, (time % 3600) / 60 );
	}
	else if ( time > 60 ) { // mins
		Com_sprintf( buf, bufsize, "%2d min %2d sec", time / 60, time % 60 );
	}
	else { // secs
		Com_sprintf( buf, bufsize, "%2d sec", time );
	}
}

void Text_PaintCenter( float x, float y, float scale, const vector4 *color, const char *text, float adjust,
	int iMenuFont, bool customFont )
{
	int len = Text_Width( text, scale, iMenuFont, customFont );
	Text_Paint( x - len / 2, y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, iMenuFont, customFont );
}

static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, float scale,
	int iMenuFont, bool customFont )
{
	char sDownLoading[256];
	char sEstimatedTimeLeft[256];
	char sTransferRate[256];
	char sOf[20];
	char sCopied[256];
	char sSec[20];
	//
	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int leftWidth;
	const char *s;

	vector4 colorLtGreyAlpha = { 0, 0, 0, .5f };

	UI_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, &colorLtGreyAlpha );

	s = GetCRDelineatedString( "MENUS", "DOWNLOAD_STUFF", 0 );	// "Downloading:"
	strcpy( sDownLoading, s ? s : "" );
	s = GetCRDelineatedString( "MENUS", "DOWNLOAD_STUFF", 1 );	// "Estimated time left:"
	strcpy( sEstimatedTimeLeft, s ? s : "" );
	s = GetCRDelineatedString( "MENUS", "DOWNLOAD_STUFF", 2 );	// "Transfer rate:"
	strcpy( sTransferRate, s ? s : "" );
	s = GetCRDelineatedString( "MENUS", "DOWNLOAD_STUFF", 3 );	// "of"
	strcpy( sOf, s ? s : "" );
	s = GetCRDelineatedString( "MENUS", "DOWNLOAD_STUFF", 4 );	// "copied"
	strcpy( sCopied, s ? s : "" );
	s = GetCRDelineatedString( "MENUS", "DOWNLOAD_STUFF", 5 );	// "sec."
	strcpy( sSec, s ? s : "" );

	downloadSize = trap->Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap->Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap->Cvar_VariableValue( "cl_downloadTime" );

	leftWidth = (SCREEN_WIDTH / 2);

	UI_SetColor( &colorWhite );

	Text_PaintCenter( centerPoint, yStart + 112, scale, &colorWhite, sDownLoading, 0, iMenuFont, customFont );
	Text_PaintCenter( centerPoint, yStart + 192, scale, &colorWhite, sEstimatedTimeLeft, 0, iMenuFont, customFont );
	Text_PaintCenter( centerPoint, yStart + 248, scale, &colorWhite, sTransferRate, 0, iMenuFont, customFont );

	if ( downloadSize > 0 ) {
		s = va( "%s (%d%%)", downloadName, (int)((float)downloadCount * 100.0f / downloadSize) );
	}
	else {
		s = downloadName;
	}

	Text_PaintCenter( centerPoint, yStart + 136, scale, &colorWhite, s, 0, iMenuFont, customFont );

	UI_ReadableSize( dlSizeBuf, sizeof dlSizeBuf, downloadCount );
	UI_ReadableSize( totalSizeBuf, sizeof totalSizeBuf, downloadSize );

	if ( downloadCount < 4096 || !downloadTime ) {
		Text_PaintCenter( leftWidth, yStart + 216, scale, &colorWhite, "estimating", 0, iMenuFont, customFont );
		Text_PaintCenter( leftWidth, yStart + 160, scale, &colorWhite, va( "(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied ), 0, iMenuFont, customFont );
	}
	else {
		if ( (uiInfo.uiDC.realTime - downloadTime) / 1000 ) {
			xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
		}
		else {
			xferRate = 0;
		}
		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if ( downloadSize && xferRate ) {
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime( dlTimeBuf, sizeof dlTimeBuf,
				(n - (((downloadCount / 1024) * n) / (downloadSize / 1024))) * 1000 );

			Text_PaintCenter( leftWidth, yStart + 216, scale, &colorWhite, dlTimeBuf, 0, iMenuFont, customFont );
			Text_PaintCenter( leftWidth, yStart + 160, scale, &colorWhite, va( "(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied ), 0, iMenuFont, customFont );
		}
		else {
			Text_PaintCenter( leftWidth, yStart + 216, scale, &colorWhite, "estimating", 0, iMenuFont, customFont );
			if ( downloadSize ) {
				Text_PaintCenter( leftWidth, yStart + 160, scale, &colorWhite, va( "(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied ), 0, iMenuFont, customFont );
			}
			else {
				Text_PaintCenter( leftWidth, yStart + 160, scale, &colorWhite, va( "(%s %s)", dlSizeBuf, sCopied ), 0, iMenuFont, customFont );
			}
		}

		if ( xferRate ) {
			Text_PaintCenter( leftWidth, yStart + 272, scale, &colorWhite, va( "%s/%s", xferRateBuf, sSec ), 0, iMenuFont, customFont );
		}
	}
}

// This will also be overlaid on the cgame info screen during loading to prevent it from blinking away too rapidly on
//	local or lan games.
void UI_DrawConnectScreen( qboolean overlay ) {
	const char *s;
	uiClientState_t	cstate;
	char			info[MAX_INFO_VALUE];
	char text[256];
	float centerPoint, yStart, scale;

	char sStringEdTemp[256];

	menuDef_t *menu = Menus_FindByName( "Connect" );


	if ( !overlay && menu ) {
		Menu_Paint( menu, qtrue );
	}

	if ( !overlay ) {
		centerPoint = (SCREEN_WIDTH / 2);
		yStart = 130;
		scale = 1.0f;	// -ste
	}
	else {
		centerPoint = (SCREEN_WIDTH / 2);
		yStart = 32;
		scale = 1.0f;	// -ste
		//RAZTODO: see results without returning
		return;
	}

	// see what information we should display
	trap->GetClientState( &cstate );


	info[0] = '\0';
	if ( trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) ) ) {
		trap->SE_GetStringTextString( "MENUS_LOADING_MAPNAME", sStringEdTemp, sizeof(sStringEdTemp) );
		Text_PaintCenter( centerPoint, yStart, scale, &colorWhite, va( /*"Loading %s"*/sStringEdTemp, Info_ValueForKey( info, "mapname" ) ), 0, FONT_MEDIUM, false );
	}

	if ( !Q_stricmp( cstate.servername, "localhost" ) ) {
		trap->SE_GetStringTextString( "MENUS_STARTING_UP", sStringEdTemp, sizeof(sStringEdTemp) );
		Text_PaintCenter( centerPoint, yStart + 48, scale, &colorWhite, sStringEdTemp, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM, false );
	}
	else {
		trap->SE_GetStringTextString( "MENUS_CONNECTING_TO", sStringEdTemp, sizeof(sStringEdTemp) );
		strcpy( text, va(/*"Connecting to %s"*/sStringEdTemp, cstate.servername ) );
		Text_PaintCenter( centerPoint, yStart + 48, scale, &colorWhite, text, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM, false );
	}

	// display global MOTD at bottom
	Text_PaintCenter( centerPoint, 425, scale, &colorWhite, Info_ValueForKey( cstate.updateInfoString, "motd" ), 0, FONT_MEDIUM, false );
	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED ) {
		Text_PaintCenter( centerPoint, yStart + 176, scale, &colorWhite, cstate.messageString, 0, FONT_MEDIUM, false );
	}

	switch ( cstate.connState ) {
	case CA_CONNECTING:
	{
		trap->SE_GetStringTextString( "MENUS_AWAITING_CONNECTION", sStringEdTemp, sizeof(sStringEdTemp) );
		s = va(/*"Awaiting connection...%i"*/sStringEdTemp, cstate.connectPacketCount );
	}
		break;
	case CA_CHALLENGING:
	{
		trap->SE_GetStringTextString( "MENUS_AWAITING_CHALLENGE", sStringEdTemp, sizeof(sStringEdTemp) );
		s = va(/*"Awaiting challenge...%i"*/sStringEdTemp, cstate.connectPacketCount );
	}
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

		trap->Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );
		if ( *downloadName ) {
			UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, scale, FONT_MEDIUM, false );
			return;
		}
	}
		trap->SE_GetStringTextString( "MENUS_AWAITING_GAMESTATE", sStringEdTemp, sizeof(sStringEdTemp) );
		s = /*"Awaiting gamestate..."*/sStringEdTemp;
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}

	if ( Q_stricmp( cstate.servername, "localhost" ) ) {
		Text_PaintCenter( centerPoint, yStart + 80, scale, &colorWhite, s, 0, FONT_MEDIUM, false );
	}
	// password required / connection rejected information goes here
}

static void UI_StopServerRefresh( void ) {
	int count;

	if ( !uiInfo.serverStatus.refreshActive ) {
		// not currently refreshing
		return;
	}
	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf( "%d servers listed in browser with %d players.\n",
		uiInfo.serverStatus.numDisplayServers,
		uiInfo.serverStatus.numPlayersOnServers );
	count = trap->LAN_GetServerCount( UI_SourceForLAN() );
	if ( count - uiInfo.serverStatus.numDisplayServers > 0 ) {
		Com_Printf( "%d servers not listed due to filters, packet loss, or pings higher than %d\n",
			count - uiInfo.serverStatus.numDisplayServers,
			(int)trap->Cvar_VariableValue( "cl_maxPing" ) );
	}
}

static void UI_DoServerRefresh( void ) {
	qboolean wait = qfalse;

	if ( !uiInfo.serverStatus.refreshActive ) {
		return;
	}
	if ( ui_netSource.integer != UIAS_FAVORITES && ui_netSource.integer != UIAS_HISTORY ) {
		if ( ui_netSource.integer == UIAS_LOCAL ) {
			if ( !trap->LAN_GetServerCount( AS_LOCAL ) ) {
				wait = qtrue;
			}
		}
		else {
			if ( trap->LAN_GetServerCount( AS_GLOBAL ) < 0 ) {
				wait = qtrue;
			}
		}
	}

	if ( uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime ) {
		if ( wait ) {
			return;
		}
	}

	// if still trying to retrieve pings
	if ( trap->LAN_UpdateVisiblePings( UI_SourceForLAN() ) ) {
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	}
	else if ( !wait ) {
		// get the last servers in the list
		UI_BuildServerDisplayList( 2 );
		// stop the refresh
		UI_StopServerRefresh();
	}
	//
	UI_BuildServerDisplayList( qfalse );
}

static void UI_StartServerRefresh( qboolean full ) {
	char	*ptr;
	int lanSource;

	qtime_t q;
	trap->RealTime( &q );
	trap->Cvar_Set( va( "ui_lastServerRefresh_%i", ui_netSource.integer ), va( "%s-%i, %i @ %i:%02i", GetMonthAbbrevString( q.tm_mon ), q.tm_mday, 1900 + q.tm_year, q.tm_hour, q.tm_min ) );

	if ( !full ) {
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;
	lanSource = UI_SourceForLAN();
	// mark all servers as visible so we store ping updates for them
	trap->LAN_MarkServerVisible( lanSource, -1, qtrue );
	// reset all the pings
	trap->LAN_ResetPings( lanSource );
	//
	if ( ui_netSource.integer == UIAS_LOCAL ) {
		trap->Cmd_ExecuteText( EXEC_NOW, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
	if ( ui_netSource.integer >= UIAS_GLOBAL1 && ui_netSource.integer <= UIAS_GLOBAL5 ) {
		ptr = UI_Cvar_VariableString( "debug_protocol" );
		if ( strlen( ptr ) ) {
			trap->Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %s full empty\n", ui_netSource.integer-1, ptr ) );
		}
		else {
			trap->Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %d full empty\n", ui_netSource.integer-1, (int)trap->Cvar_VariableValue( "protocol" ) ) );
		}
	}
}

uiLocal_t uiLocal;

#ifdef FAV_SERVERS
static qboolean JP_ProcessFavServer( const char *token )
{
	const char	*tmp;
	favServer_t	*current = &uiLocal.servers[uiLocal.serversCount];

	//If we're still at the end of the previous server, there's no more :o
	if ( !Q_stricmp( token, "}" ) )
		return qfalse;

	//Name
	if ( TP_ParseString( &tmp ) )
		Com_Printf( "Unexpected EOL line %i (Expected 'name')\n", TP_CurrentLine() );
	else
		Q_strncpyz( current->name, tmp, sizeof( current->name ) );

	//IP
	if ( TP_ParseString( &tmp ) )
		Com_Printf( "Unexpected EOL line %i (Expected 'IP')\n", TP_CurrentLine() );
	else
		Q_strncpyz( current->ip, tmp, sizeof( current->ip ) );

	//Admin password
	if ( TP_ParseString( &tmp ) )
		Com_Printf( "Unexpected EOL line %i (Expected 'adminPassword')\n", TP_CurrentLine() );
	else
		Q_strncpyz( current->adminPassword, tmp, sizeof( current->adminPassword ) );

	//Successful write, fix index
	uiLocal.serversCount++;

	token = TP_ParseToken();
	return qtrue;
}

void JP_ParseFavServers( void )
{
	char			*buf;
	char			loadPath[MAX_QPATH];
	unsigned int	len;
	const char		*token;
	fileHandle_t	f;

	//Clear the favourite server table
	memset( &uiLocal.servers[0], 0, sizeof( favServer_t ) * MAX_SERVERS );
	uiLocal.serversCount = 0;

	Com_Printf( "Loading favourite servers..." );
	Com_sprintf( loadPath, sizeof( loadPath ), "favourites.dat" );
	len = trap->FS_Open( loadPath, &f, FS_READ );

	if ( !f )
	{//no file
		Com_Printf( S_COLOR_RED"failed! "S_COLOR_WHITE"(Can't find %s)\n", loadPath );
		return;
	}

	if ( !len || len == -1 )
	{//empty file
		Com_Printf( S_COLOR_RED"failed! "S_COLOR_WHITE"(%s is empty)\n", loadPath );
		trap->FS_Close( f );
		return;
	}

	if ( (buf = (char*)malloc(len+1)) == 0 )
	{
		Com_Printf( S_COLOR_RED"failed! "S_COLOR_WHITE"(Failed to allocate buffer)\n" );
		return;
	}

	trap->FS_Read( buf, len, f );
	trap->FS_Close( f );
	buf[len] = 0;
	TP_NewParseSession( buf );

	while ( 1 )
	{
		token = TP_ParseToken();
		if ( !token[0] )
			break;

		if ( uiLocal.serversCount < MAX_SERVERS )
		{
			if ( !JP_ProcessFavServer( token ) )
				break;
		}
		else
			Com_Printf( S_COLOR_RED"failed! "S_COLOR_WHITE"MAX_SERVERS reached!" );
	}
	free( buf );

	Com_Printf( "done\n" );
	return;
}

void JP_SaveFavServers( void )
{
	static char		buf[1024*4] = { 0 };// 4k file size (static now - don't want a stack overflow)
	char			loadPath[MAX_QPATH];
	long			len;
	fileHandle_t	f;
	unsigned int	i;

	Com_Printf( S_COLOR_CYAN"JA++ Saving favourite servers...\n" );
	Com_sprintf( loadPath, sizeof( loadPath ), "favourites.dat" );
	len = trap->FS_Open( loadPath, &f, FS_WRITE );

	for ( i=0; i<uiLocal.serversCount; i++ )
	{
		favServer_t *current = &uiLocal.servers[i];
		Q_strcat( buf, sizeof( buf ), va( "{ \"%s\" \"%s\" \"%s\" }\n", current->name, current->ip, current->adminPassword ) );
	}

	trap->FS_Write( buf, strlen( buf ), f );
	trap->FS_Close( f );

	return;
}
#endif

uiImport_t *trap = NULL;

Q_CABI {
Q_EXPORT uiExport_t *GetModuleAPI( int apiVersion, uiImport_t *import ) {
	static uiExport_t uie = { 0 };

	assert( import );
	trap = import;
	Com_Printf = trap->Print;
	Com_Error = trap->Error;

	memset( &uie, 0, sizeof(uie) );

	if ( apiVersion != UI_API_VERSION ) {
		trap->Print( "Mismatched UI_API_VERSION: expected %i from engine, got %i\n", UI_API_VERSION, apiVersion );
		return NULL;
	}

	uie.Init = UI_Init;
	uie.Shutdown = UI_Shutdown;
	uie.KeyEvent = UI_KeyEvent;
	uie.MouseEvent = UI_MouseEvent;
	uie.Refresh = UI_Refresh;
	uie.IsFullscreen = Menus_AnyFullScreenVisible;
	uie.SetActiveMenu = UI_SetActiveMenu;
	uie.ConsoleCommand = UI_ConsoleCommand;
	uie.DrawConnectScreen = UI_DrawConnectScreen;
	uie.MenuReset = Menu_Reset;

	return &uie;
}

Q_EXPORT intptr_t vmMain( int command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 ) {
	switch ( command ) {
	case UI_GETAPIVERSION:
		return UI_LEGACY_API_VERSION;

	case UI_INIT:
		UI_Init( arg0 );
		return 0;

	case UI_SHUTDOWN:
		UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		UI_KeyEvent( arg0, arg1 );
		return 0;

	case UI_MOUSE_EVENT:
		UI_MouseEvent( arg0, arg1 );
		return 0;

	case UI_REFRESH:
		UI_Refresh( arg0 );
		return 0;

	case UI_IS_FULLSCREEN:
		return Menus_AnyFullScreenVisible();

	case UI_SET_ACTIVE_MENU:
		UI_SetActiveMenu( (uiMenuCommand_t)arg0 );
		return 0;

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand( arg0 );

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen( arg0 );
		return 0;

	case UI_MENU_RESET:
		Menu_Reset();
		return 0;

	default:
		break;
	}

	return -1;
}
}
