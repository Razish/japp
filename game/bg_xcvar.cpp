#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#elif defined(PROJECT_UI)
#include "ui_local.h"
#endif

#include "bg_lua.h"

#include "bg_xcvar.h"

static xcvar* xCvars = nullptr;

xcvar::xcvar( const char* newName, const char* newDefaultStr, uint32_t newCvarFlags, xcvarModificationCallback_t newModCallback, qboolean newTrackChange ) :
	name( newName ), defaultString( newDefaultStr ), string( newDefaultStr ), flags( newCvarFlags ), modificationCallback( newModCallback ), trackChange(newTrackChange), internalCvar()
{
	value = atof( newDefaultStr );
	integer = atoi( newDefaultStr );

	next = xCvars;
	xCvars = this;
}
xcvar::~xcvar()
{
}

void xcvar::onCvarModified( const char* newText )
{
	string = newText;
	value = atof( newText );
	integer = atoi( newText );
	if ( modificationCallback )
	{
		modificationCallback( this );
	}
}
void xcvar::registerInternal()
{
	vmCvar_t *vmCvar = &internalCvar;
	if ( vmCvar )
	{
		trap->Cvar_Register( vmCvar, name.c_str(), defaultString.c_str(), flags );
		onCvarModified( vmCvar->string );
#ifdef JPLUA
		JPLua::Cvar_Update( name.c_str() );
#endif
	}
}
void xcvar::update()
{
	vmCvar_t *vmCvar = &internalCvar;
	if ( vmCvar )
	{
		int modCount = vmCvar->modificationCount;
		trap->Cvar_Update( vmCvar );
		if ( vmCvar->modificationCount != modCount )
		{
			onCvarModified( vmCvar->string );
#ifdef JPLUA
			JPLua::Cvar_Update( name.c_str() );
#endif

			if ( trackChange )
			{
#if defined(PROJECT_GAME)
				trap->SendServerCommand( -1, va( "print \"Server: %s changed to %s\n\"",
					name.c_str(), vmCvar->string )
					);
#else
				Com_Printf( "%s changed to %s\n", name.c_str(), vmCvar->string );
#endif
			}
		}
	}
}

bool xcvar::isDefault() const
{
	return string == defaultString;
}

int xcvar::modificationCount() const
{
	const vmCvar_t *vmCvar = &internalCvar;
	if ( vmCvar )
	{
		return vmCvar->modificationCount;
	}
	return 0;
}

void xcvar::setModificationCallback( xcvarModificationCallback_t newModCallback )
{
	modificationCallback = newModCallback;
}
void xcvar::setQBoolean( qboolean newVal )
{
	trap->Cvar_Set( name.c_str(), va( "%i", !!newVal ? 1 : 0 ) );
	update();
}
void xcvar::setBool( bool newVal )
{
	trap->Cvar_Set( name.c_str(), va( "%i", newVal ? 1 : 0 ) );
	update();
}
void xcvar::setInt( int32_t newVal )
{
	trap->Cvar_Set( name.c_str(), va( "%i", newVal ) );
	update();
}
void xcvar::setBits( uint32_t newVal )
{
	trap->Cvar_Set( name.c_str(), va( "%u", newVal ) );
	update();
}
void xcvar::setFloat( float newVal )
{
	trap->Cvar_Set( name.c_str(), va( "%f", newVal ) );
	update();
}
void xcvar::setString( const char* newStr )
{
	trap->Cvar_Set( name.c_str(), newStr );
	update();
}

void XCVAR_RegisterXCvars()
{
	xcvar* c = xCvars;
	while ( c )
	{
		c->registerInternal();
		c = c->next;
	}
}
void XCVAR_UpdateXCvars()
{
	xcvar* c = xCvars;
	while ( c )
	{
		c->update();
		c = c->next;
	}
}
