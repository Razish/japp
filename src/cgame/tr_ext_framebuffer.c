#include "tr_ext_public.h"
#include "tr_ext_framebuffer.h"
#include "cg_local.h"
#include "GLee.h"

#ifdef R_POSTPROCESSING

#define MAX_FRAMEBUFFERS (24)
#define MAX_TEXTURES (5 * MAX_FRAMEBUFFERS)
#define MAX_RENDERBUFFERS (2 * MAX_FRAMEBUFFERS)

static int numFramebuffers = 0;
static framebuffer_t framebuffers[MAX_FRAMEBUFFERS] = { {0} };

static int numTextures = 0;
static texture_t textures[MAX_TEXTURES] = { {0} };

static unsigned int numRenderbuffers = 0;
static unsigned int renderbuffers[MAX_RENDERBUFFERS] = { 0 };

static const framebuffer_t *currentReadFramebuffer	= NULL;
static const framebuffer_t *currentWriteFramebuffer	= NULL;

void CheckGLErrors( const char* filename, int line )
{
	unsigned int error = glGetError();
	if ( error != GL_NO_ERROR )
	{
		switch ( error )
		{
		case GL_INVALID_ENUM:
			CG_Printf (S_COLOR_RED "GL_INVALID_ENUM in file %s:%d.\n", filename, line);
			break;
		case GL_INVALID_VALUE:
			CG_Printf (S_COLOR_RED "GL_INVALID_VALUE in file %s:%d.\n", filename, line);
			break;
		case GL_INVALID_OPERATION:
			CG_Printf (S_COLOR_RED "GL_INVALID_OPERATION in file %s:%d.\n", filename, line);
			break;
		case GL_STACK_OVERFLOW:
			CG_Printf (S_COLOR_RED "GL_STACK_OVERFLOW in file %s:%d.\n", filename, line);
			break;
		case GL_STACK_UNDERFLOW:
			CG_Printf (S_COLOR_RED "GL_STACK_UNDERFLOW in file %s:%d.\n", filename, line);
			break;
		case GL_OUT_OF_MEMORY:
			CG_Printf (S_COLOR_RED "GL_OUT_OF_MEMORY in file %s:%d.\n", filename, line);
			break;
		default:
			CG_Printf (S_COLOR_RED "Error code 0x%X on line %d.\n", error, line);
			break;
		}
		tr.postprocessing.loaded = qfalse;
	//	BREAK;
	}
}

#if 0
static qboolean IsPowerOfTwo( unsigned int value )
{
	return !!(value && !(value & (value - 1)));
}
#endif

static unsigned int R_EXT_GetGLInternalFormat ( internalFormat_t internalFormat )
{
    switch ( internalFormat )
    {
    default:
    case IF_RGBA8:
        return GL_RGBA8;
        
    case IF_RGBA16F:
        return GL_RGBA16F_ARB;
        
    case IF_DEPTH_COMPONENT16:
        return GL_DEPTH_COMPONENT16;
        
    case IF_DEPTH_COMPONENT24:
        return GL_DEPTH_COMPONENT24;
        
    case IF_STENCIL_INDEX4:
        return GL_STENCIL_INDEX4_EXT;
    
    case IF_STENCIL_INDEX8:
        return GL_STENCIL_INDEX8_EXT;
        
    case IF_DEPTH24_STENCIL8:
        return GL_DEPTH24_STENCIL8_EXT;
    }
}



static unsigned int R_EXT_GetGLFormat ( internalFormat_t internalFormat )
{
    switch ( internalFormat )
    {
    default:
    case IF_RGBA8:
    case IF_RGBA16F:
        return GL_RGBA;
        
    case IF_DEPTH_COMPONENT16:
    case IF_DEPTH_COMPONENT24:
        return GL_DEPTH_COMPONENT;
        
    case IF_STENCIL_INDEX4:
    case IF_STENCIL_INDEX8:
        return GL_STENCIL_INDEX;
        
    case IF_DEPTH24_STENCIL8:
        return GL_DEPTH_STENCIL_EXT;
    }
}


unsigned int R_EXT_GetGLTextureTarget( textureTarget_t textureTarget )
{
	switch ( textureTarget )
	{
	case TT_POT:
	case TT_NPOT:
		return GL_TEXTURE_2D;
	}

	return 0;
}

static unsigned int R_EXT_GetDataTypeForFormat ( internalFormat_t internalFormat )
{
    switch ( internalFormat )
    {
    default:
        return GL_UNSIGNED_BYTE;
        
    case IF_RGBA16F:
        return GL_FLOAT;
        
    case IF_DEPTH24_STENCIL8:
        return GL_UNSIGNED_INT_24_8_EXT;
    }
}


qboolean R_EXT_FramebufferInit( void )
{
	if ( !strstr( cgs.glconfig.extensions_string, "GL_EXT_framebuffer_object" ) ||
		 !strstr( cgs.glconfig.extensions_string, "GL_EXT_framebuffer_blit" ) )
	{
		CG_Printf( S_COLOR_RED "Framebuffer extension NOT loaded.\nRequired OpenGL extensions not available.\n" );
		tr.postprocessing.loaded = qfalse;
		return qfalse;
	}

//  memset( framebuffers,	0,	sizeof( framebuffers )	);
//  memset( textures,		0,	sizeof( textures )		);
//  memset( renderbuffers,	0,	sizeof( renderbuffers )	);

	CG_Printf( "Framebuffer extension loaded\n" );

	CheckGLErrors( __FILE__, __LINE__ );

	return qtrue;
}

void R_EXT_FramebufferCleanup ( void )
{
	int				i			= 0;
	framebuffer_t	*fbo		= framebuffers;
	texture_t		*texture	= textures;

	for ( i=0; i<numFramebuffers; i++, fbo++ )
	{
		if ( !fbo->id )
			continue;

		glDeleteFramebuffersEXT( 1, &fbo->id );
		CheckGLErrors( __FILE__, __LINE__ );
	}

	for ( i=0; i<numTextures; i++, texture++ )
	{
		if ( !texture->id )
			continue;

		glDeleteTextures( 1, &texture->id );
		CheckGLErrors( __FILE__, __LINE__ );
	}
	glDeleteRenderbuffersEXT( numRenderbuffers, renderbuffers );

	currentReadFramebuffer = currentWriteFramebuffer = NULL;
	numFramebuffers = numTextures = numRenderbuffers = 0;
	memset( framebuffers, 0, sizeof( framebuffers ) );
	memset( textures, 0, sizeof( textures ) );
	memset( renderbuffers, 0, sizeof( renderbuffers ) );
}

framebuffer_t *R_EXT_CreateFramebuffer( void )
{
	framebuffer_t *fbo = NULL;

	if ( numFramebuffers >= MAX_FRAMEBUFFERS )
	{
		Com_Error( ERR_DROP, "Maximum number of framebuffers exeeded.\n" );
		return NULL;
	}

	fbo = &framebuffers[numFramebuffers];
	glGenFramebuffersEXT (1, &fbo->id);

	if ( !fbo->id )
	{
		Com_Error( ERR_DROP, "Failed to create framebuffer with internal ID %d.\n", numFramebuffers );
		return NULL;
	}

	numFramebuffers++;

	CheckGLErrors( __FILE__, __LINE__ );

	return fbo;
}

void R_EXT_AttachColorTextureToFramebuffer( framebuffer_t *framebuffer, const texture_t *texture, unsigned int slot )
{
	if ( slot >= MAX_FBO_COLOR_TEXTURES )
	{
		CG_Printf( "Invalid slot number given (%d), valid range is 0 - %d.\n", slot, MAX_FBO_COLOR_TEXTURES - 1 );
		return;
	}

	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + slot, R_EXT_GetGLTextureTarget( texture->target ), texture->id, 0 );
	framebuffer->colorTextures[slot] = texture;

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_AttachDepthRenderbufferToFramebuffer( framebuffer_t *framebuffer, unsigned int renderbufferId )
{
	glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbufferId );
	framebuffer->depthTexture = renderbufferId;

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_AttachDepthTextureToFramebuffer( framebuffer_t *framebuffer, const texture_t *texture )
{
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, R_EXT_GetGLTextureTarget( texture->target ), texture->id, 0 );
	framebuffer->depthTexture = texture->id;

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_AttachDepthStencilTextureToFramebuffer( framebuffer_t *framebuffer, const texture_t *texture )
{
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, R_EXT_GetGLTextureTarget( texture->target ), texture->id, 0 );
	framebuffer->depthTexture = texture->id;
	framebuffer->stencilTexture = texture->id;

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_AttachStencilRenderbufferToFramebuffer( framebuffer_t *framebuffer, unsigned int renderbufferId )
{
	glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbufferId );
	framebuffer->stencilTexture = renderbufferId;

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_CheckFramebuffer( const framebuffer_t *framebuffer )
{
	unsigned int status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );

	switch ( status )
	{
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
		CG_Printf( "One or more framebuffer attachment points are not complete.\n" );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		CG_Printf( "One or more attached images have different dimensions.\n" );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
		CG_Printf( "Invalid framebuffer attachment object type used.\n" );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		CG_Printf( "More than one internal format was used in the color attachments.\n" );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
		CG_Printf( "Missing a read buffer.\n" );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
		CG_Printf( "No images were attached to the framebuffer.\n" );
		break;

	case GL_FRAMEBUFFER_COMPLETE_EXT:
		break;
	}

	//Raz: Maybe Com_Error from here to unload cgame before crashing
	if ( status != GL_FRAMEBUFFER_COMPLETE_EXT )
		CG_Printf( "Creation of framebuffer %d could not be completed.\n", framebuffer->id );

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_BindDefaultFramebuffer( void )
{
	if ( currentReadFramebuffer || currentWriteFramebuffer )
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		currentReadFramebuffer	= NULL;
		currentWriteFramebuffer	= NULL;
	}

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_BindFramebuffer( const framebuffer_t *framebuffer )
{
	if ( currentReadFramebuffer != framebuffer || currentWriteFramebuffer != framebuffer )
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, framebuffer->id );
		currentReadFramebuffer	= framebuffer;
		currentWriteFramebuffer	= framebuffer;
	}

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_BlitFramebuffer( const framebuffer_t *source, const framebuffer_t *destination, int sourceWidth, int sourceHeight, int destWidth, int destHeight, unsigned int bufferBits )
{
	if ( currentReadFramebuffer != source )
	{
		glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, source ? source->id : 0 );
		currentReadFramebuffer = source;
	}

	if ( currentWriteFramebuffer != destination )
	{
		glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, destination ? destination->id : 0 );
		currentWriteFramebuffer = destination;
	}

	glBlitFramebufferEXT(
		sourceWidth, sourceHeight, 0, 0,
		destWidth, destHeight, 0, 0,
		bufferBits, GL_NEAREST );

	CheckGLErrors( __FILE__, __LINE__ );
}

void R_EXT_BlitFramebufferColor( const framebuffer_t *source, const framebuffer_t *destination, int sourceWidth, int sourceHeight, int destWidth, int destHeight )
{
    R_EXT_BlitFramebuffer( source, destination, sourceWidth, sourceHeight, destWidth, destHeight, GL_COLOR_BUFFER_BIT);
}

void R_EXT_BlitFramebufferColorAndDepth( const framebuffer_t *source, const framebuffer_t *destination, int sourceWidth, int sourceHeight, int destWidth, int destHeight )
{
    R_EXT_BlitFramebuffer( source, destination, sourceWidth, sourceHeight, destWidth, destHeight, GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}


static GLint filterTable[] =
{
    GL_NEAREST,
    GL_LINEAR,
    GL_LINEAR_MIPMAP_LINEAR
};

texture_t *R_EXT_CreateTexture( unsigned int width, unsigned int height, internalFormat_t internalFormat, unsigned int minFilter, unsigned int magFilter )
{
    unsigned int textureId = 0;
    unsigned int glTexTarget = 0;
    texture_t* texture = NULL;

    if ( numTextures >= MAX_TEXTURES )
    {
        CG_Printf ("Exceeded maximum number of textures.\n");
        return NULL;
    }

    glGenTextures (1, &textureId);
    if ( textureId == 0 )
    {
        CG_Printf ("Failed to create texture with internal ID %d.\n", numTextures);
        return NULL;
    }
    
    texture = &textures[numTextures];
    texture->id = textureId;
    texture->target = TT_POT;
    texture->width = width;
    texture->height = height;
    
    glTexTarget = R_EXT_GetGLTextureTarget (texture->target);
    
    glBindTexture (glTexTarget, textureId);
    
    glTexParameteri (glTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (glTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (glTexTarget, GL_TEXTURE_MIN_FILTER, filterTable[minFilter]);
    glTexParameteri (glTexTarget, GL_TEXTURE_MAG_FILTER, filterTable[magFilter]);
    
    glTexImage2D (glTexTarget, 0, R_EXT_GetGLInternalFormat (internalFormat), width, height, 0, R_EXT_GetGLFormat (internalFormat), R_EXT_GetDataTypeForFormat (internalFormat), NULL);
    
    glBindTexture (glTexTarget, 0);
    
    numTextures++;
    
    CheckGLErrors (__FILE__, __LINE__);
    
    return texture;
}

texture_t *R_EXT_CreateBlankTexture( unsigned int width, unsigned int height, internalFormat_t internalFormat )
{
	unsigned int	textureId	= 0;
	unsigned int	glTexTarget	= 0;
	texture_t		*texture	= NULL;

	if ( numTextures >= MAX_TEXTURES )
	{
		Com_Error( ERR_DROP, "Exceeded maximum number of textures\n" );
		return NULL;
	}

	glGenTextures( 1, &textureId );
	if ( !textureId )
	{
		Com_Error( ERR_DROP, "Failed to create texture with internal ID %d.\n", numTextures );
		return NULL;
	}

	texture			= &textures[numTextures];
	texture->id		= textureId;
	texture->target	= TT_POT; //IsPowerOfTwo (width) && IsPowerOfTwo (height) ? TT_POT : TT_NPOT;
	texture->width	= width;
	texture->height	= height;

	glTexTarget = R_EXT_GetGLTextureTarget( texture->target );

	glBindTexture( glTexTarget, textureId );

	glTexParameteri( glTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( glTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( glTexTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( glTexTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexImage2D( glTexTarget, 0, R_EXT_GetGLInternalFormat( internalFormat ), width, height, 0, R_EXT_GetGLFormat( internalFormat ), R_EXT_GetDataTypeForFormat( internalFormat ), NULL );

	glBindTexture( glTexTarget, 0 );

	numTextures++;

	CheckGLErrors( __FILE__, __LINE__ );

	return texture;
}

unsigned int R_EXT_CreateRenderbuffer( unsigned int width, unsigned int height, internalFormat_t internalFormat )
{
	unsigned int renderbufferId = 0;

	if ( numRenderbuffers >= MAX_RENDERBUFFERS )
	{
		Com_Error( ERR_DROP, "Exceeded maximum number of renderbuffers\n" );
		return 0;
	}

	glGenRenderbuffersEXT (1, &renderbufferId);
	if ( renderbufferId == 0 )
	{
		Com_Error( ERR_DROP, "Failed to create renderbuffer with internal ID %d.\n", numRenderbuffers );
		return 0;
	}

	glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, renderbufferId );
	glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, R_EXT_GetGLInternalFormat( internalFormat ), width, height );

	renderbuffers[numRenderbuffers] = renderbufferId;
	numRenderbuffers++;

	CheckGLErrors( __FILE__, __LINE__ );

	return renderbufferId;
}

#endif
