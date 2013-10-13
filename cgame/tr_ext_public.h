#ifndef TR_EXT_PUBLIC_H
#define TR_EXT_PUBLIC_H

#include "tr_ext_glsl_program.h"
#include "tr_ext_framebuffer.h"

void R_EXT_Init ( void );
void R_EXT_Cleanup ( void );
void R_EXT_PostProcess ( void );

#define NUM_LUMINANCE_FBOS (9)
#define LUMINANCE_FBO_SIZE (256.0f)
#define NUM_BLOOM_DOWNSCALE_FBOS (8)
#define BLOOM_DOWNSCALE_RATE (2.0f)

typedef struct tr_s {
	struct {
		qboolean	loaded;
		float		bloomSize;	//Raz: This will generally be vidWidth, but there is an option to use a custom size.
		int			bloomSteps;
		vector4		grading;	// per-map colour grading
	} postprocessing;

	struct {
		framebuffer_t *scene;
		
		#ifdef R_TONEMAPPING
			framebuffer_t *tonemapping;
			framebuffer_t *luminance[NUM_LUMINANCE_FBOS];
		#endif

		framebuffer_t *brightPass;
		framebuffer_t *bloomDownscale[NUM_BLOOM_DOWNSCALE_FBOS];
		framebuffer_t *bloomBlur[2]; // separable horizontal/vertical blurs
		framebuffer_t *bloom;
	} framebuffers;

	struct {
		glslProgram_t *gaussianBlur3x3[2]; // separable horizontal/vertical blurs

		#ifdef R_TONEMAPPING
			glslProgram_t *downscaleLuminance;
			glslProgram_t *luminance;
			glslProgram_t *tonemapping;
		#endif

		glslProgram_t *brightPass;
		glslProgram_t *gaussianBlur6x6[2]; // separable horizontal/vertical blurs
		glslProgram_t *bloom;
	} glsl;
} tr_t;

extern tr_t tr;
#endif
