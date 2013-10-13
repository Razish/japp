#pragma once

#include "q_shared.h"

#define MAX_FBO_COLOR_TEXTURES (4)

//Enums
typedef enum internalFormat_e
{
	IF_INVALID=0,
	IF_RGBA8,
	IF_RGBA16F,
	IF_DEPTH_COMPONENT16,
	IF_DEPTH_COMPONENT24,
	IF_STENCIL_INDEX4,
	IF_STENCIL_INDEX8,
	IF_DEPTH24_STENCIL8
} internalFormat_t;

typedef enum textureTarget_e { TT_POT, TT_NPOT } textureTarget_t;

//Structs
typedef struct texture_s
{
	unsigned int	id;
	textureTarget_t target;
	unsigned int	width;
	unsigned int	height;
} texture_t;

typedef struct framebuffer_s
{
	unsigned int	id;
	const texture_t	*colorTextures[MAX_FBO_COLOR_TEXTURES];
	unsigned int	depthTexture;

	// will reconsider adding this back if i ever need it
	unsigned int	stencilTexture;
} framebuffer_t;

// Function prototypes
void			 CheckGLErrors( const char *filename, int line );

qboolean		 R_EXT_FramebufferInit( void );
void			 R_EXT_FramebufferCleanup( void );
framebuffer_t	*R_EXT_CreateFramebuffer( void );
void			 R_EXT_AttachColorTextureToFramebuffer( framebuffer_t *framebuffer, const texture_t *texture, unsigned int slot );
void			 R_EXT_AttachDepthTextureToFramebuffer( framebuffer_t *framebuffer, const texture_t *texture );
void			 R_EXT_AttachDepthStencilTextureToFramebuffer( framebuffer_t *framebuffer, const texture_t *texture );
void			 R_EXT_AttachDepthRenderbufferToFramebuffer( framebuffer_t *framebuffer, unsigned int renderbufferId );
void			 R_EXT_AttachStencilRenderbufferToFramebuffer( framebuffer_t *framebuffer, unsigned int renderbufferId );
void			 R_EXT_CheckFramebuffer( const framebuffer_t *framebuffer );

void			 R_EXT_BindDefaultFramebuffer( void );
void			 R_EXT_BindFramebuffer( const framebuffer_t *framebuffer );
void			 R_EXT_BlitFramebuffer( const framebuffer_t *source, const framebuffer_t *destination, int sourceWidth, int sourceHeight, int destWidth, int destHeight, unsigned int bufferBits );
void			 R_EXT_BlitFramebufferColor( const framebuffer_t *source, const framebuffer_t *destination, int sourceWidth, int sourceHeight, int destWidth, int destHeight );
void			 R_EXT_BlitFramebufferColorAndDepth( const framebuffer_t *source, const framebuffer_t *destination, int sourceWidth, int sourceHeight, int destWidth, int destHeight );

texture_t		*R_EXT_CreateTexture( unsigned int width, unsigned int height, internalFormat_t internalFormat, unsigned int minFilter, unsigned int magFilter );
texture_t		*R_EXT_CreateBlankTexture( unsigned int width, unsigned int height, internalFormat_t internalFormat );
unsigned int	 R_EXT_CreateRenderbuffer( unsigned int width, unsigned int height, internalFormat_t internalFormat );

unsigned int	 R_EXT_GetGLTextureTarget( textureTarget_t textureTarget );
