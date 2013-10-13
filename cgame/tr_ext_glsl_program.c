#include "tr_ext_glsl_program.h"
#include "tr_ext_public.h"
#include "cg_local.h"
#include "GLee.h"

#ifdef R_POSTPROCESSING

#define MAX_GLSL_PROGRAMS (10)
#define MAX_GLSL_SHADERS (MAX_GLSL_PROGRAMS * 2)

static unsigned int numPrograms = 0;
static glslProgram_t glslPrograms[MAX_GLSL_PROGRAMS];

static unsigned int numShaders = 0;
static glslShader_t glslShaders[MAX_GLSL_SHADERS];

static const glslProgram_t* lastProgramUsed = NULL;

static void R_EXT_GLSL_OutputInfoLog ( int objectID )
{
    int logLength = 0;
    char* logText = NULL;
    
    glGetObjectParameterivARB (objectID, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLength);
    
    if ( logLength > 0 )
    {
        trap->TrueMalloc ((void**)&logText, logLength);
        glGetInfoLogARB (objectID, logLength, NULL, logText);
        
		if ( strlen( logText ) > 1020 )
			logText[1020] = '\0';

        trap->Print ("%s\n", logText);
        
        trap->TrueFree ((void**)&logText);
    }
}

static glslShader_t* R_EXT_GLSL_CreateShader ( const char* shaderPath, int shaderType )
{
    glslShader_t* shader = NULL;
    char* shaderCode = NULL;
#ifdef FILE_BASED_SHADERS
    int fileHandle = 0;
    int fileLength = 0;
#endif
    int statusCode = 0;

    if ( numShaders >= MAX_GLSL_SHADERS )
    {
        trap->Print ("Maximum number of GLSL shaders exceeded.\n");
		tr.postprocessing.loaded = qfalse;
        return NULL;
    }
    
#ifdef FILE_BASED_SHADERS
    fileLength = trap->FS_Open (shaderPath, &fileHandle, FS_READ);
    if ( !fileHandle )
    {
        trap->Print ("Unable to open shader file %s.\n", shaderPath);
		tr.postProcessing.loaded = qfalse;
        return NULL;
    }
    
    if ( !fileLength )
    {
        trap->Print( "Shader file %s is empty.\n", shaderPath );
		tr.postProcessing.loaded = qfalse;
		trap->FS_Close( fileHandle );
        return NULL;
    }
    
    trap->TrueMalloc( (void**)&shaderCode, fileLength + 1 );
    if ( shaderCode == NULL )
    {
        trap->Print ("Unable to allocate memory for shader code for shader file %s.\n", shaderPath);
        return NULL;
    }
    
    trap->FS_Read (shaderCode, fileLength, fileHandle);
    shaderCode[fileLength] = 0;
    
    trap->FS_Close( fileHandle );
#else
	trap->TrueMalloc( (void **)&shaderCode, 16384 );
    if ( shaderCode == NULL )
    {
        trap->Print ("Unable to allocate memory for shader code for shader file %s.\n", shaderPath);
        return NULL;
    }
	Q_strncpyz( shaderCode, shaderPath, 16384 );
#endif

	shader = &glslShaders[numShaders];
	shader->id = glCreateShaderObjectARB (shaderType);
	Q_strncpyz (shader->name, shaderPath, sizeof (shader->name));
	shader->type = ( shaderType == GL_VERTEX_SHADER_ARB ) ? VERTEX_SHADER : FRAGMENT_SHADER;

	if ( !shader->id )
	{
		trap->Print( "Failed to create vertex shader object for shader %s.\n", shaderPath );

		trap->TrueFree( (void**)&shaderCode );

		return NULL;
	}

	glShaderSourceARB (shader->id, 1, (GLcharARB**)&shaderCode, NULL);
	if ( glGetError() == GL_INVALID_OPERATION )
	{
		trap->Print ("Invalid source code in shader\n");
		R_EXT_GLSL_OutputInfoLog (shader->id);

		glDeleteObjectARB (shader->id);
		trap->TrueFree ((void**)&shaderCode);
		trap->Cvar_Set( "r_postprocess_enable", "0" );

		return NULL;
	}

	trap->TrueFree ((void**)&shaderCode);

	glCompileShaderARB (shader->id);
	glGetObjectParameterivARB (shader->id, GL_OBJECT_COMPILE_STATUS_ARB, &statusCode);

	if ( statusCode == GL_FALSE )
	{
		trap->Print ("Failed to compile shader source\n");
		R_EXT_GLSL_OutputInfoLog (shader->id);

		glDeleteObjectARB (shader->id);
		trap->Cvar_Set( "r_postprocess_enable", "0" );

		return NULL;
	}

	R_EXT_GLSL_OutputInfoLog (shader->id);
    
    numShaders++;
    
    return shader;
}

qboolean R_EXT_GLSL_Init ( void )
{
	if ( !strstr (cgs.glconfig.extensions_string, "GL_ARB_shader_objects") )
	{
		trap->Print (S_COLOR_RED "...GLSL extension NOT loaded. Required OpenGL extensions not available.\n");
		return qfalse;
	}

    memset (glslPrograms, 0, sizeof (glslPrograms));
    memset (glslShaders, 0, sizeof (glslShaders));
    
    trap->Print ("GLSL extension loaded\n");

	return qtrue;
}

static void R_EXT_GLSL_CleanupVariable ( glslProgramVariable_t* variable )
{
    if ( variable && variable->next )
        R_EXT_GLSL_CleanupVariable (variable->next);
    else
        trap->TrueFree ((void**)&variable);
}

static glslProgramVariable_t* R_EXT_GLSL_GetUniformLocation ( glslProgram_t* program, const char* name )
{
	glslProgramVariable_t* variable = program->uniforms, *prev = NULL;
	while ( variable && strcmp (variable->name, name) )
	{
		prev = variable;
		variable = variable->next;
	}

	if ( variable == NULL )
	{
		if ( prev == NULL )
		{
			trap->TrueMalloc ((void**)&program->uniforms, sizeof (glslProgramVariable_t));
			variable = program->uniforms;
		}
		else
		{
			trap->TrueMalloc ((void**)&prev->next, sizeof (glslProgramVariable_t));
			variable = prev->next;
		}

		variable->name = name;
		variable->next = NULL;
		variable->location = glGetUniformLocationARB (program->id, name);
	}

	return variable;
}

#if 0
static glslProgramVariable_t* R_EXT_GLSL_GetAttributeLocation ( glslProgram_t* program, const char* name )
{
    glslProgramVariable_t* variable = program->attributes;
    while ( variable && strcmp (variable->name, name) )
    {
        variable = variable->next;
    }

    if ( variable == NULL )
    {
        trap->TrueMalloc ((void**)&variable, sizeof (glslProgramVariable_t));
        
        variable->name = name;
        variable->next = NULL;
        variable->location = glGetAttribLocationARB (program->id, name);
    }
    
    return variable;
}
#endif

void R_EXT_GLSL_Cleanup ( void )
{
    unsigned int i = 0;
    glslProgram_t* program = NULL;
    glslShader_t* shader = NULL;
    
    for ( ; i < numPrograms; i++ )
    {
        program = &glslPrograms[i];
        if ( program->id == 0 )
        {
            continue;
        }
        
        if ( program->fragmentShader )
        {
            glDetachObjectARB (program->id, program->fragmentShader->id);
        }
        
        if ( program->vertexShader )
        {
            glDetachObjectARB (program->id, program->vertexShader->id);
        }
        
        glDeleteObjectARB (program->id);
        
        R_EXT_GLSL_CleanupVariable (program->uniforms);
        R_EXT_GLSL_CleanupVariable (program->attributes);
    }
    
    for ( i = 0; i < numShaders; i++ )
    {
        shader = &glslShaders[i];

	//	if ( shader->id == 0 )
	//		continue;

		glDeleteObjectARB (shader->id);
    }

	lastProgramUsed = NULL;
	numPrograms = numShaders = 0;
	memset( glslPrograms, 0, sizeof( glslPrograms ) );
	memset( glslShaders, 0, sizeof( glslShaders ) );
}

glslProgram_t* R_EXT_GLSL_CreateProgram ( void )
{
    glslProgram_t* program = NULL;
    if ( numPrograms >= MAX_GLSL_PROGRAMS )
    {
        trap->Print ("Maximum number of GLSL programs exceeded.\n");
		tr.postprocessing.loaded = qfalse;
        return NULL;
    }
    
    program = &glslPrograms[numPrograms];
    
    program->id = glCreateProgramObjectARB();
    if ( program->id == 0 )
    {
        trap->Print ("Failed to create program with internal ID %d\n", numPrograms);
 		tr.postprocessing.loaded = qfalse;
   }
    
    program->fragmentShader = NULL;
    program->vertexShader = NULL;
    
    numPrograms++;
    
    return program;
}

void R_EXT_GLSL_AttachShader ( glslProgram_t* program, glslShader_t* shader )
{
    if ( shader == NULL )
    {
        return;
    }
    
    switch ( shader->type )
    {
    case VERTEX_SHADER:
        if ( program->vertexShader != shader )
        {
            program->vertexShader = shader;
            glAttachObjectARB (program->id, shader->id);
        }
    break;
    
    case FRAGMENT_SHADER:
        if ( program->fragmentShader != shader )
        {
            program->fragmentShader = shader;
            glAttachObjectARB (program->id, shader->id);
        }
    break;
    }
}

glslShader_t* R_EXT_GLSL_CreateVertexShader ( const char* shaderPath )
{
    return R_EXT_GLSL_CreateShader (shaderPath, GL_VERTEX_SHADER_ARB);
}

glslShader_t* R_EXT_GLSL_CreateFragmentShader ( const char* shaderPath )
{
    return R_EXT_GLSL_CreateShader (shaderPath, GL_FRAGMENT_SHADER_ARB);
}

void R_EXT_GLSL_LinkProgram ( const glslProgram_t* program )
{
    int statusCode = 0;

    glLinkProgramARB (program->id);
    glGetObjectParameterivARB (program->id, GL_OBJECT_LINK_STATUS_ARB, &statusCode);
    
    if ( statusCode == GL_FALSE )
    {
        trap->Print (S_COLOR_RED "Failed to link program %d\n", program->id);
        R_EXT_GLSL_OutputInfoLog (program->id);
    }
}

void R_EXT_GLSL_UseProgram ( const glslProgram_t* program )
{
    if ( lastProgramUsed != program )
    {
        int id = ( program != NULL ) ? program->id : 0;
        
        glUseProgramObjectARB (id);
        lastProgramUsed = program;
    }
}

void R_EXT_GLSL_SetUniform1i ( glslProgram_t* program, const char* name, int i )
{
    glslProgramVariable_t* variable = R_EXT_GLSL_GetUniformLocation (program, name);
    glUniform1iARB (variable->location, i);
}

void R_EXT_GLSL_SetUniform1f ( glslProgram_t* program, const char* name, float f )
{
    glslProgramVariable_t* variable = R_EXT_GLSL_GetUniformLocation (program, name);
    glUniform1fARB (variable->location, f);
}

void R_EXT_GLSL_SetUniform2f ( glslProgram_t* program, const char* name, float f1, float f2 )
{
    glslProgramVariable_t* variable = R_EXT_GLSL_GetUniformLocation (program, name);
    glUniform2fARB (variable->location, f1, f2);
}

void R_EXT_GLSL_SetUniform3f ( glslProgram_t* program, const char* name, float f1, float f2, float f3 )
{
    glslProgramVariable_t* variable = R_EXT_GLSL_GetUniformLocation (program, name);
    glUniform3fARB (variable->location, f1, f2, f3);
}

void R_EXT_GLSL_SetUniform4f ( glslProgram_t* program, const char* name, float f1, float f2, float f3, float f4 )
{
    glslProgramVariable_t* variable = R_EXT_GLSL_GetUniformLocation (program, name);
    glUniform4fARB (variable->location, f1, f2, f3, f4);
}

#endif
