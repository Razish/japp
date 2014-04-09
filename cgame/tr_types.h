#pragma once

#define	MAX_DLIGHTS		32			// can't be increased, because bit flags are used on surfaces
#define	MAX_ENTITIES	2048		// 11 bits, can't be increased without changing drawsurf bit packing (QSORT_ENTITYNUM_SHIFT)
#define	MAX_MINI_ENTITIES	1024

#define	TR_WORLDENT		(MAX_ENTITIES-1)

// renderfx flags
#define	RF_MINLIGHT			(0x00000001u) // allways have some light (viewmodel, some items)
#define	RF_THIRD_PERSON		(0x00000002u) // don't draw through eyes, only mirrors (player bodies, chat sprites)
#define	RF_FIRST_PERSON		(0x00000004u) // only draw through eyes (view weapon, damage blood blob)
#define	RF_DEPTHHACK		(0x00000008u) // for view weapon Z crunching
#define RF_NODEPTH			(0x00000010u) // No depth at all (seeing through walls)
#define RF_VOLUMETRIC		(0x00000020u) // fake volumetric shading
#define	RF_NOSHADOW			(0x00000040u) // don't add stencil shadows
#define RF_LIGHTING_ORIGIN	(0x00000080u) // use refEntity->lightingOrigin instead of refEntity->origin for lighting. This allows entities to sink into the floor with their origin going solid, and allows all parts of a player to get the same lighting
#define	RF_SHADOW_PLANE		(0x00000100u) // use refEntity->shadowPlane
#define	RF_WRAP_FRAMES		(0x00000200u) // mod the model frames by the maxframes to allow continuous animation without needing to know the frame count
#define RF_FORCE_ENT_ALPHA	(0x00000400u) // override shader alpha settings
#define RF_RGB_TINT			(0x00000800u) // override shader rgb settings
#define	RF_SHADOW_ONLY		(0x00001000u) // add surfs for shadowing but don't draw them -rww
#define	RF_DISTORTION		(0x00002000u) // area distortion effect -rww
#define RF_FORKED			(0x00004000u) // override lightning to have forks
#define RF_TAPERED			(0x00008000u) // lightning tapers
#define RF_GROW				(0x00010000u) // lightning grows from start to end during its life
#define RF_DISINTEGRATE1	(0x00020000u) // does a procedural hole-ripping thing.
#define RF_DISINTEGRATE2	(0x00040000u) // does a procedural hole-ripping thing with scaling at the ripping point
#define RF_SETANIMINDEX		(0x00080000u) // use backEnd.currentEntity->e.skinNum for R_BindAnimatedImage
#define RF_ALPHA_DEPTH		(0x00100000u) // depth write on alpha model
#define RF_FORCEPOST		(0x00200000u) // force it to post-render -rww

// refdef flags
#define RDF_NOWORLDMODEL	(0x0001u) // used for player configuration screen
#define RDF_HYPERSPACE		(0x0004u) // teleportation effect
#define RDF_SKYBOXPORTAL	(0x0008u)
#define RDF_DRAWSKYBOX		(0x0010u) // the above marks a scene as being a 'portal sky'.  this flag says to draw it or not
#define RDF_AUTOMAP			(0x0020u) // means this scene is to draw the automap -rww
#define	RDF_NOFOG			(0x0040u) // no global fog in this scene (but still brush fog) -rww

extern int	skyboxportal;
extern int	drawskyboxportal;

typedef byte color4ub_t[4];

typedef struct polyVert_s {
	vector3		xyz;
	float		st[2];
	byte		modulate[4];
} polyVert_t;

typedef struct poly_s {
	qhandle_t			hShader;
	int					numVerts;
	polyVert_t			*verts;
} poly_t;

typedef enum refEntityType_e {
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_ORIENTED_QUAD,
	RT_BEAM,
	RT_SABER_GLOW,
	RT_ELECTRICITY,
	RT_PORTALSURFACE,		// doesn't draw anything, just info for portals
	RT_LINE,
	RT_ORIENTEDLINE,
	RT_CYLINDER,
	RT_ENT_CHAIN,

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef struct miniRefEntity_s {
	refEntityType_t		reType;
	int					renderfx;

	qhandle_t			hModel;				// opaque type outside refresh

	// most recent data
	vector3				axis[3];			// rotation vectors
	qboolean			nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	vector3				origin;				// also used as MODEL_BEAM's "from"

	// previous data for frame interpolation
	vector3				oldorigin;			// also used as MODEL_BEAM's "to"

	// texturing
	qhandle_t			customShader;		// use one image for the entire thing

	// misc
	byte				shaderRGBA[4];		// colors used by rgbgen entity shaders
	vector2				shaderTexCoord;		// texture coordinates used by tcMod entity modifiers

	// extra sprite information
	float				radius;
	float				rotation;			// size 2 for RT_CYLINDER or number of verts in RT_ELECTRICITY

	// misc
	float		shaderTime;			// subtracted from refdef time to control effect start times
	int			frame;				// also used as MODEL_BEAM's diameter

} miniRefEntity_t;

#if defined(_WIN32) && !defined(MINGW32)
#pragma warning (disable : 4201 )
#endif
typedef struct refEntity_s {
	// this stucture must remain identical as the miniRefEntity_t
	//
	//
	refEntityType_t		reType;
	int					renderfx;

	qhandle_t			hModel;				// opaque type outside refresh

	// most recent data
	vector3				axis[3];			// rotation vectors
	qboolean			nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	vector3				origin;				// also used as MODEL_BEAM's "from"

	// previous data for frame interpolation
	vector3				oldorigin;			// also used as MODEL_BEAM's "to"

	// texturing
	qhandle_t			customShader;		// use one image for the entire thing

	// misc
	byte				shaderRGBA[4];		// colors used by rgbgen entity shaders
	vector2				shaderTexCoord;		// texture coordinates used by tcMod entity modifiers

	// extra sprite information
	float				radius;
	float				rotation;

	// misc
	float		shaderTime;			// subtracted from refdef time to control effect start times
	int			frame;				// also used as MODEL_BEAM's diameter
	//
	//
	// end miniRefEntity_t

	//
	//
	// specific full refEntity_t data
	//
	//

	// most recent data
	vector3		lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float		shadowPlane;		// projection shadows go here, stencils go slightly lower

	// previous data for frame interpolation
	int			oldframe;
	float		backlerp;			// 0.0f = current, 1.0f = old

	// texturing
	int			skinNum;			// inline skin index
	qhandle_t	customSkin;			// NULL for default skin

	// texturing
	union {
		struct {
			int		miniStart;
			int		miniCount;
		} uMini;
	} uRefEnt;

	// extra sprite information
	union {
		struct {
			float rotation;
			float radius;
			byte  vertRGBA[4][4];
		} sprite;
		struct {
			float width;
			float width2;
			float stscale;
		} line;
		struct {
			float	width;
			vector3	control1;
			vector3	control2;
		} bezier;
		struct {
			float width;
			float width2;
			float stscale;
			float height;
			float bias;
			qboolean wrap;
		} cylinder;
		struct {
			float width;
			float deviation;
			float stscale;
			qboolean wrap;
			qboolean taper;
		} electricity;
	} data;

	float		endTime;
	float		saberLength;

	/*
	Ghoul2 Insert Start
	*/
	vector3		angles;				// rotation angles - used for Ghoul2

	vector3		modelScale;			// axis scale for models
	//	CGhoul2Info_v	*ghoul2;  		// has to be at the end of the ref-ent in order for it to be created properly
	void		*ghoul2;  		// has to be at the end of the ref-ent in order for it to be created properly
	/*
	Ghoul2 Insert End
	*/
} refEntity_t;


#define	MAX_RENDER_STRINGS			8
#define	MAX_RENDER_STRING_LENGTH	32

typedef struct refdef_s {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vector3		vieworg;
	vector3		viewangles;
	vector3		viewaxis[3];		// transformation matrix
	int			viewContents;		// world contents at vieworg

	// time in milliseconds for shader effects and other time dependent rendering issues
	int			time;

	uint32_t	rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} refdef_t;


typedef enum stereoFrame_e {
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
} stereoFrame_t;


/*
** glconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
typedef enum textureCompression_e {
	TC_NONE,
	TC_S3TC,
	TC_S3TC_DXT
} textureCompression_t;

typedef struct glconfig_s {
	const char				*renderer_string;
	const char				*vendor_string;
	const char				*version_string;
	const char				*extensions_string;

	int						maxTextureSize;			// queried from GL
	int						maxActiveTextures;		// multitexture ability
	float					maxTextureFilterAnisotropy;

	int						colorBits, depthBits, stencilBits;

	qboolean				deviceSupportsGamma;
	textureCompression_t	textureCompression;
	qboolean				textureEnvAddAvailable;
	qboolean				clampToEdgeAvailable;

	int						vidWidth, vidHeight;

	int						displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	qboolean				isFullscreen;
	qboolean				stereoEnabled;
} glconfig_t;


#if !defined _WIN32

#define OPENGL_DRIVER_NAME	"libGL.so"

#else

#define OPENGL_DRIVER_NAME	"opengl32"

#endif	// !defined _WIN32
