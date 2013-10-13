#include "tr_ext_public.h"
#include "cg_local.h"
#include "GLee.h"
#include "cg_engine.h"
#include "ui/menudef.h"

#ifdef R_POSTPROCESSING

#ifdef _WIN32
	#define GL_BIND_ADDRESS				0x489FA0
	#define GL_SELECTTEXTURE_ADDRESS	0x48A010
	#define GL_STATE_ADDRESS			0x48A290
	EFUNC( void, R_SyncRenderThread,	( void ),	0x4915B0 );
//	EFUNC( void, RB_SetGL2D,			( void ),	0x48B070 );
//	EFUNC( void, RB_EndSurface,			( void ),	0x4AF530 );
#elif MAC_PORT
	#define GL_BIND_ADDRESS				0x0448bc
	#define GL_SELECTTEXTURE_ADDRESS	0x044938
	EFUNC( void, R_SyncRenderThread,	( void ),	0x04ca06 );
#endif

tr_t tr = { 0 };

// ================
//
// FBO/Shader helper functions
//
// ================

static framebuffer_t *R_EXT_CreateFBO( unsigned int width, unsigned int height, internalFormat_t colorbits, internalFormat_t depthbits )
{//Helper function to create an FBO
	framebuffer_t *fbo = R_EXT_CreateFramebuffer();
	R_EXT_BindFramebuffer( fbo );

	//Create color/depth attachments if necessary
	if ( colorbits )
		R_EXT_AttachColorTextureToFramebuffer( fbo, R_EXT_CreateBlankTexture( width, height, colorbits ), 0 );
	if ( depthbits )
		R_EXT_AttachDepthStencilTextureToFramebuffer( fbo, R_EXT_CreateTexture( width, height, depthbits, 1, 1 ) );

	//Check for errors
	R_EXT_CheckFramebuffer( fbo );

	return fbo;
}

static glslProgram_t *R_EXT_CreateShader( const char *source )
{//Helper function to create a GLSL ##FRAGMENT## shader
	glslProgram_t *shader = R_EXT_GLSL_CreateProgram();

	R_EXT_GLSL_AttachShader( shader, R_EXT_GLSL_CreateFragmentShader( source ) );
	R_EXT_GLSL_LinkProgram( shader );
	R_EXT_GLSL_UseProgram( shader );

	return shader;
}

//Uses a static buffer, do NOT nest calls
static const char *ShaderSource( const char *path )
{
	static char buf[1024*64] = {0};
	fileHandle_t f;

	memset( buf, 0, sizeof( buf ) );

	Com_Printf( "Loading GLSL shader %s\n", path );
	trap->FS_Open( path, &f, FS_READ );
	trap->FS_Read( buf, sizeof( buf ), f );
	trap->FS_Close( f );

//	COM_Compress( buf );
	return &buf[0];
}

typedef struct image_s {
	char 	name[64];
	short 	width;
	short 	height;
	int 	texnum;
	int		frameUsed;
	int		imageFormat;
	int		glWrapClampMode;
	char 	mipmap;
	char	allowPicmip;
	short 	levused;
} image_t;

static void GL_Bind( unsigned int texID )
{//ASM bridge to call GL_Bind
	static image_t image;
	image.texnum = texID;
	image.frameUsed = 0;

#if MAC_PORT
	qasm1( push offset image );
	qasm2( mov eax, GL_BIND_ADDRESS );
	qasm1( call eax );
	qasm2( add esp, 4 );
#else
	qasm2( mov esi, offset image );
	qasm2( mov eax, GL_BIND_ADDRESS );
	qasm1( call eax );
#endif
}

static int GL_SelectTexture( int texUnit )
{//ASM bridge to call GL_SelectTexture
#if MAC_PORT
	qasm1( push texUnit );
	qasm2( mov eax, GL_SELECTTEXTURE_ADDRESS );
	qasm1( call eax );	
	qasm2( add esp, 4 );
#else
	qasm2( mov esi, texUnit );
	qasm2( mov eax, GL_SELECTTEXTURE_ADDRESS );
	qasm1( call eax );
#endif
}

static int ENG_GL_State/*<eax>*/( int stateBits/*<ebx>*/ ) {
	int res=0;
	qasm2( mov ebx, stateBits );
	qasm2( mov ecx, GL_STATE_ADDRESS );
	qasm1( call ecx );
	qasm2( mov res, eax );
	return res;
}

#if !MAC_PORT
int (__stdcall *ENG_qglBindTexture)(DWORD textureMode, DWORD textureID) = NULL;
#endif

static void CreateFramebuffers( void )
{
	float	w = cgs.glconfig.vidWidth, h = cgs.glconfig.vidHeight;
	float	size = LUMINANCE_FBO_SIZE;
	int i;
	int steps = Com_Clampi( 1, NUM_BLOOM_DOWNSCALE_FBOS, r_postprocess_bloomSteps.integer );

	tr.postprocessing.bloomSize = r_postprocess_bloomSize.integer ? r_postprocess_bloomSize.value : w;
	tr.postprocessing.bloomSteps = steps;

	//scene
	tr.framebuffers.scene					= R_EXT_CreateFBO( w, h, IF_RGBA8, IF_DEPTH24_STENCIL8 );

	#ifdef R_TONEMAPPING
		//hdr tonemapping
		for ( i=0, size=LUMINANCE_FBO_SIZE; i<NUM_LUMINANCE_FBOS; i++, size /= 2.0f )
			tr.framebuffers.luminance[i]		= R_EXT_CreateFBO( floorf(size), floorf(size), IF_RGBA8, 0 );

		tr.framebuffers.tonemapping				= R_EXT_CreateFBO( w, h, IF_RGBA8, 0 );
	#endif

	tr.framebuffers.brightPass				= R_EXT_CreateFBO( w, h, IF_RGBA8, 0 );

	//bloom
	for ( i=0, size=1.0f; i<tr.postprocessing.bloomSteps; i++, size *= BLOOM_DOWNSCALE_RATE )
		tr.framebuffers.bloomDownscale[i]	= R_EXT_CreateFBO( floorf( tr.postprocessing.bloomSize/size ), floorf( tr.postprocessing.bloomSize/size ), IF_RGBA8, 0 );
	for ( i=0; i<2; i++ )
		tr.framebuffers.bloomBlur[i]		= R_EXT_CreateFBO( floorf( tr.postprocessing.bloomSize/size ), floorf( tr.postprocessing.bloomSize/size ), IF_RGBA8, 0 );
	tr.framebuffers.bloom					= R_EXT_CreateFBO( w, h, IF_RGBA8, 0 );
}

static __inline void LoadShaders( void )
{
	#ifdef R_TONEMAPPING
		// HDR Tonemapping
		tr.glsl.tonemapping = R_EXT_CreateShader( ShaderSource( "shaders/post/tonemapping.glsl" ) );
		R_EXT_GLSL_SetUniform1i( tr.glsl.tonemapping, "SceneTex", 0 );

		// Luminance
		tr.glsl.luminance = R_EXT_CreateShader( ShaderSource( "shaders/post/luminance.glsl" ) );
		R_EXT_GLSL_SetUniform1i( tr.glsl.luminance, "SceneTex", 0 );
		R_EXT_GLSL_SetUniform1f( tr.glsl.luminance, "PixelWidth", (1.0f / LUMINANCE_FBO_SIZE)*0.5f );
		R_EXT_GLSL_SetUniform1f( tr.glsl.luminance, "PixelHeight", (1.0f / LUMINANCE_FBO_SIZE)*0.5f );

		// Downscale luminance
		tr.glsl.downscaleLuminance = R_EXT_CreateShader( ShaderSource( "shaders/post/downscaleLuminance.glsl" ) );
		R_EXT_GLSL_SetUniform1i( tr.glsl.downscaleLuminance, "SceneTex", 0 );
	#endif

	// Bright pass filter
	tr.glsl.brightPass = R_EXT_CreateShader( ShaderSource( "shaders/post/brightpass.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.brightPass, "SceneTex", 0 );

	// Gaussian blurs
	tr.glsl.gaussianBlur3x3[0] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur3x3_horizontal.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur3x3[0], "SceneTex", 0 );
	tr.glsl.gaussianBlur3x3[1] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur3x3_vertical.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur3x3[1], "SceneTex", 0 );

	tr.glsl.gaussianBlur6x6[0] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur6x6_horizontal.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur6x6[0], "SceneTex", 0 );
	tr.glsl.gaussianBlur6x6[1] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur6x6_vertical.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur6x6[1], "SceneTex", 0 );

	// bloom
	tr.glsl.bloom = R_EXT_CreateShader( ShaderSource( "shaders/post/bloom.glsl" ) );
}

void R_EXT_Init( void )
{
	trap->Print( "----------------------------\n" );
	trap->Print( "Loading visual extensions...\n" );

	tr.postprocessing.loaded = qfalse;
	if ( !R_EXT_GLSL_Init() || !R_EXT_FramebufferInit() )
		return;

	tr.postprocessing.loaded = qtrue;
	trap->Print( "----------------------------\n" );

	CreateFramebuffers();
	LoadShaders();

	R_EXT_GLSL_UseProgram( NULL );
	R_EXT_BindDefaultFramebuffer();

#if !MAC_PORT
	ENG_qglBindTexture = (int (__stdcall *)(DWORD textureMode, DWORD textureID)) *(unsigned int*)0x10BCB10;
#endif
	
//	tr.postprocessing.loaded = qtrue;
}

void R_EXT_Cleanup( void )
{
//	glColor4fv( colorTable[CT_WHITE] ); //HACK: Sometimes scopes change the colour.
	ENG_R_SyncRenderThread();

	R_EXT_FramebufferCleanup();
	R_EXT_GLSL_Cleanup();
	tr.postprocessing.loaded = qfalse;
}

static QINLINE void DrawQuad( float x, float y, float width, float height ) {
	glBegin( GL_QUADS );
		glTexCoord2f (0.0f, 0.0f);
		glVertex2f (x, y + height);

		glTexCoord2f (0.0f, 1.0f);
		glVertex2f (x, y);

		glTexCoord2f (1.0f, 1.0f);
		glVertex2f (x + width, y);

		glTexCoord2f (1.0f, 0.0f);
		glVertex2f (x + width, y + height);
	glEnd();
}

#define DrawFullscreenQuad() DrawQuad( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT )
//#define GL_Bind( x ) glBindTexture( GL_TEXTURE_2D, x );
//#define GL_SelectTexture( x ) glActiveTextureARB( GL_TEXTURE##x##_ARB ); glClientActiveTextureARB( GL_TEXTURE##x##_ARB );

//const orientationr_t *tr_viewParms_or = (const orientationr_t *)0xFE37A0;
//const orientationr_t *backEnd_viewParms_world = (const orientationr_t *)0xFE2844;
//const float *projectionMatrix = (const float *)0xFE2908;
const unsigned long *glState_glStateBits = (const unsigned long *)0xFE3144;
const float *tr_viewParms_zFar = (float *)0xFE3988;


#ifdef R_TONEMAPPING
	static float adaptedFrameLuminance	= 0.0f;
	static float adaptedMaxLuminance	= 1.0f;
	static float lastFrameTime			= 0.0f;
	static void CalculateLightAdaptation ( void )
	{
		float	time = trap->Milliseconds() / 1000.0f;
		float	dt;
		vector4	currentFrameLuminance = { 0.0f };
		float tau = 1.0f;
		float rods = 0.4f, cones = 0.8f;

		R_EXT_BindFramebuffer( tr.framebuffers.luminance[NUM_LUMINANCE_FBOS - 1] );
		glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_FLOAT, (GLvoid *)&currentFrameLuminance );

		tau = (currentFrameLuminance.x * rods) + ((1.0-currentFrameLuminance.x) * cones);

		dt = max( time-lastFrameTime, 0.0f );

		//adaptedFrameLuminance = adaptedFrameLuminance + (currentFrameLuminance[0] - adaptedFrameLuminance) * (1.0f - powf (0.98f, 30.0f * dt));
		adaptedFrameLuminance	+= (currentFrameLuminance.x - adaptedFrameLuminance)	* (1.0f - exp(-dt * tau));
		adaptedMaxLuminance		+= (currentFrameLuminance.y - adaptedMaxLuminance)		* (1.0f - exp(-dt * tau));
		lastFrameTime = time;
	}
#endif

static QINLINE void ResizeTarget( float w, float h ) {
	glViewport( 0, 0, w, h );
	glScissor( 0, 0, w, h );
}

EFUNC( void, SetViewportAndScissor, (void), 0x48A610 );
EFUNC( void, Set2D, (void), 0x48B070 );
EFUNC( void, Set3D, (void), 0x48A680 );
EFUNC( void, RB_BeginDrawingView, (void), 0x48A680 );

void CG_Draw2D( void );
static void _R_EXT_PostProcess( void )
{
	int i;
	float w = cgs.glconfig.vidWidth;
	float h = cgs.glconfig.vidHeight;
	framebuffer_t *lastFullscreenFBO = tr.framebuffers.scene;

	if ( !r_postprocess_enable.integer || !tr.postprocessing.loaded )
		return;

	// Render scene
	// The renderer batch renders surfaces. This means that it won't surfaces until the loaded surface is different from the previously
	// loaded surface. By drawing an invisible quad, we ensure that the render pipeline is flushed (i.e. all surfaces have been drawn).
//	ENG_RB_BeginDrawingView();
//	ENG_R_SyncRenderThread();
//	ENG_GL_State( 0x00000100 );
	glColor4fv( (float *)&colorTable[CT_WHITE] ); //HACK: Sometimes scopes change the colour.
	if ( r_postprocess_enable.integer == 2 ) {
		trap->R_DrawStretchPic( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, cgs.media.whiteShader );
		ENG_R_SyncRenderThread();
	}
//	RB_EndSurface();

	// Anything that can be rendered to, be it the screen, or a texture, is known as a framebuffer. The 'default' framebuffer is the
	// backbuffer (which will be displayed on the screen when the driver tell it to). We can copy the data from the default framebuffer
	// to any framebuffer we choose. In this case, I'm copying it to 'sceneFbo'.
	R_EXT_BlitFramebufferColorAndDepth( NULL, tr.framebuffers.scene, w, h, w, h );

	#ifdef R_TONEMAPPING
	if ( r_postprocess_hdr.integer )
	{
		int size = tr.framebuffers.luminance[0]->colorTextures[0]->width;
		float imageKey = 1.0f;
		glPushAttrib( GL_VIEWPORT_BIT|GL_SCISSOR_BIT );
		ResizeTarget( size, size );

		// Downscale and convert to luminance.    
		R_EXT_GLSL_UseProgram( tr.glsl.luminance );
		R_EXT_BindFramebuffer( tr.framebuffers.luminance[0] );
		GL_Bind( lastFullscreenFBO->colorTextures[0]->id );
		DrawFullscreenQuad();

        R_EXT_BlitFramebufferColor (tr.framebuffers.luminance[0], tr.framebuffers.tonemapping, w, h, w, h);

		// Downscale further to 1x1 texture.
		R_EXT_GLSL_UseProgram( tr.glsl.downscaleLuminance );
		for ( i=1; i<NUM_LUMINANCE_FBOS; i++ )
		{
			size = tr.framebuffers.luminance[i]->colorTextures[0]->width;

			R_EXT_GLSL_SetUniform1f( tr.glsl.downscaleLuminance, "PixelWidth", 1.0f / (float)size );
			R_EXT_GLSL_SetUniform1f( tr.glsl.downscaleLuminance, "PixelHeight", 1.0f / (float)size );

			R_EXT_BindFramebuffer( tr.framebuffers.luminance[i] );
			GL_Bind( tr.framebuffers.luminance[i-1]->colorTextures[0]->id );
			ResizeTarget( size, size );

			DrawFullscreenQuad();
		}

		glPopAttrib();

		// Tonemapping.
		CalculateLightAdaptation();

		R_EXT_BindFramebuffer( tr.framebuffers.tonemapping );
		R_EXT_GLSL_UseProgram( tr.glsl.tonemapping );
		R_EXT_GLSL_SetUniform1f( tr.glsl.tonemapping, "Exposure", r_postprocess_hdrExposureBias.value/adaptedFrameLuminance );
		GL_Bind( lastFullscreenFBO->colorTextures[0]->id );
        DrawFullscreenQuad();
		lastFullscreenFBO = tr.framebuffers.tonemapping;
	}
	#endif


	//================================
	//	Bloom
	//================================

	if ( r_postprocess_bloom.integer )
	{
		framebuffer_t *lastFBO = tr.framebuffers.brightPass;
		glslProgram_t *shader = NULL;
		float size = 1.0f;

		//Bright pass
		R_EXT_BindFramebuffer( tr.framebuffers.brightPass );
		R_EXT_GLSL_UseProgram( tr.glsl.brightPass );
		R_EXT_GLSL_SetUniform1f( tr.glsl.brightPass, "Threshold", Com_Clamp( 0.0f, 1.0f, r_postprocess_brightPassThres.value ) );
		GL_Bind( lastFullscreenFBO->colorTextures[0]->id );
		DrawFullscreenQuad();

		R_EXT_GLSL_UseProgram( NULL );
		glPushAttrib( GL_VIEWPORT_BIT|GL_SCISSOR_BIT );

		for ( i=0, size=1.0f; i<tr.postprocessing.bloomSteps; i++, size *= BLOOM_DOWNSCALE_RATE )
		{// downscale
			R_EXT_BindFramebuffer( tr.framebuffers.bloomDownscale[i] );
			GL_Bind( lastFBO->colorTextures[0]->id );
			ResizeTarget( floorf( tr.postprocessing.bloomSize/size ), floorf( tr.postprocessing.bloomSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.bloomDownscale[i];
		}

		for ( i=0; i<2; i++ )
		{// blur
			switch ( r_postprocess_bloomBlurStyle.integer )
			{
			default:
			case 1:
				shader = tr.glsl.gaussianBlur3x3[i];
				break;
			case 2:
				shader = tr.glsl.gaussianBlur6x6[i];
				break;
			case 3:
				shader = (i&1) ? tr.glsl.gaussianBlur3x3[i] : tr.glsl.gaussianBlur6x6[i];
				break;
			case 4:
				shader = (i&1) ? tr.glsl.gaussianBlur6x6[i] : tr.glsl.gaussianBlur3x3[i];
				break;
			}

			R_EXT_BindFramebuffer( tr.framebuffers.bloomBlur[i] );

			R_EXT_GLSL_UseProgram( shader );
			if ( !(i & 1) )	R_EXT_GLSL_SetUniform1f( shader, "PixelWidth", 1.0f/floorf(tr.postprocessing.bloomSize/size) );
			else			R_EXT_GLSL_SetUniform1f( shader, "PixelHeight", 1.0f/floorf(tr.postprocessing.bloomSize/size) );

			GL_Bind( lastFBO->colorTextures[0]->id );
			if ( !(i & 1) ) ResizeTarget( floorf( tr.postprocessing.bloomSize/size ), floorf( tr.postprocessing.bloomSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.bloomBlur[i];
		}

		R_EXT_GLSL_UseProgram( NULL ); //reset the active shader

		for ( i=tr.postprocessing.bloomSteps-1, size/=BLOOM_DOWNSCALE_RATE; i>=0; i--, size /= BLOOM_DOWNSCALE_RATE )
		{// upscale
			R_EXT_BindFramebuffer( tr.framebuffers.bloomDownscale[i] );
			GL_Bind( lastFBO->colorTextures[0]->id );
			ResizeTarget( floorf( tr.postprocessing.bloomSize/size ), floorf( tr.postprocessing.bloomSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.bloomDownscale[i];
		}

		//finally upscale into the brightpass fbo (full screen size)
		R_EXT_BindFramebuffer( tr.framebuffers.brightPass );
		GL_Bind( lastFBO->colorTextures[0]->id );
		ResizeTarget( w, h );
		DrawFullscreenQuad();
		glPopAttrib();

		// add the bloom texture onto the scene texture
		R_EXT_BindFramebuffer( tr.framebuffers.bloom );
		R_EXT_GLSL_UseProgram( tr.glsl.bloom );
		GL_SelectTexture( 0 );	GL_Bind( lastFullscreenFBO->colorTextures[0]->id );				R_EXT_GLSL_SetUniform1i( tr.glsl.bloom, "SceneTex", 0 );
		GL_SelectTexture( 1 );	GL_Bind( tr.framebuffers.brightPass->colorTextures[0]->id );	R_EXT_GLSL_SetUniform1i( tr.glsl.bloom, "BloomTex", 1 );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "BloomMulti", Com_Clamp( 0.0, r_postprocess_bloomMulti.value, r_postprocess_bloomMulti.value ) );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "BloomSat", r_postprocess_bloomSat.value );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "SceneMulti", Com_Clamp( 0.0, r_postprocess_bloomSceneMulti.value, r_postprocess_bloomSceneMulti.value ) );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "SceneSat", r_postprocess_bloomSceneSat.value );
		DrawFullscreenQuad();
		lastFullscreenFBO = tr.framebuffers.bloom;
		GL_SelectTexture( 0 );
	}

	R_EXT_GLSL_UseProgram( NULL );
	R_EXT_BlitFramebufferColor( lastFullscreenFBO, NULL, w, h, w, h );
	R_EXT_BindDefaultFramebuffer();

	#ifdef _DEBUG
		if ( r_postprocess_debugFBOs.integer )
		{
#define ROWS (6.0f)
#define COLUMNS (6.0f)
			int dViews = 0;
			const float weights[36][2] =
			{
				//row 1
				{ (SCREEN_WIDTH/COLUMNS)*0, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 2
				{ (SCREEN_WIDTH/COLUMNS)*1, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 3
				{ (SCREEN_WIDTH/COLUMNS)*2, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 4
				{ (SCREEN_WIDTH/COLUMNS)*3, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 5
				{ (SCREEN_WIDTH/COLUMNS)*4, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 6
				{ (SCREEN_WIDTH/COLUMNS)*5, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*5 },
			};
			framebuffer_t **fbo = (framebuffer_t**)&tr.framebuffers;
			int i=0;

			w = (SCREEN_WIDTH/COLUMNS);
			h = (SCREEN_HEIGHT/ROWS);

			while ( i<sizeof( tr.framebuffers ) / sizeof( framebuffer_t* ) )
			{
				if ( *fbo )
				{
					GL_Bind( (*fbo)->colorTextures[0]->id );
					DrawQuad( weights[dViews][0], weights[dViews][1], w, h );
					dViews++;
				}
				fbo++;
				i++;
			}
		}
		#undef DEBUGVIEW
	#endif

	return;
}

void R_EXT_PostProcess( void ) {
	_R_EXT_PostProcess();
}
#endif
