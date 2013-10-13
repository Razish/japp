#pragma once

#include "q_shared.h"

// Data type definitions
typedef enum glslShaderType_e
{
    VERTEX_SHADER,
    FRAGMENT_SHADER
} glslShaderType_t;

typedef struct glslShader_s
{
    int id;
    char name[MAX_QPATH];
    glslShaderType_t type;
} glslShader_t;

typedef struct glslProgramVariable_s
{
    const char* name;
    int location;
    
    struct glslProgramVariable_s* next;
} glslProgramVariable_t;

typedef struct glslProgram_s
{
    int id;
    
    glslShader_t* vertexShader;
    glslShader_t* fragmentShader;
    
    glslProgramVariable_t* uniforms;
    glslProgramVariable_t* attributes;
} glslProgram_t;

// Function prototypes
qboolean R_EXT_GLSL_Init ( void );
void R_EXT_GLSL_Cleanup ( void );

glslProgram_t* R_EXT_GLSL_CreateProgram ( void );
void R_EXT_GLSL_LinkProgram ( const glslProgram_t* program );
void R_EXT_GLSL_UseProgram ( const glslProgram_t* program );

void R_EXT_GLSL_SetUniform1i ( glslProgram_t* program, const char* name, int i );
void R_EXT_GLSL_SetUniform1f ( glslProgram_t* program, const char* name, float f );
void R_EXT_GLSL_SetUniform2f ( glslProgram_t* program, const char* name, float f1, float f2 );
void R_EXT_GLSL_SetUniform3f ( glslProgram_t* program, const char* name, float f1, float f2, float f3 );
void R_EXT_GLSL_SetUniform4f ( glslProgram_t* program, const char* name, float f1, float f2, float f3, float f4 );

glslShader_t* R_EXT_GLSL_CreateFragmentShader ( const char* shaderPath );
glslShader_t* R_EXT_GLSL_CreateVertexShader ( const char* shaderPath );
void R_EXT_GLSL_AttachShader ( glslProgram_t* program, glslShader_t* shader );
