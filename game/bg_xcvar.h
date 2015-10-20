#pragma once

#include "qcommon/q_shared.h"

#include <string>

class xcvar
{
	std::string name;
	std::string defaultString;
	std::string string;
	float		value;
	union
	{
		int32_t			integer;
		uint32_t		bits;
	};
	uint32_t	flags;
	qboolean	trackChange;

	xcvar * next;

	struct vmCvar_s internalCvar;

	typedef void( *xcvarModificationCallback_t )(const xcvar* aCvar);

	xcvarModificationCallback_t modificationCallback;

	void onCvarModified( const char* newText );
	void registerInternal();

public:
	xcvar( const char *newName, const char *newDefaultStr, uint32_t newCvarFlags = CVAR_NONE, xcvarModificationCallback_t newModCallback = nullptr, qboolean newTrackChange = qfalse );
	~xcvar();

	void update();

	bool isDefault() const;

	int modificationCount() const;  // temporarily needed for se_language changes in UI

	void setModificationCallback( xcvarModificationCallback_t newModCallback );
	void setQBoolean( qboolean newVal );
	void setBool( bool newVal );
	void setInt( int32_t newVal );
	void setBits( uint32_t newVal );
	void setFloat( float newVal );
	void setString( const char* newStr );

	qboolean getQboolean() const
	{
		return !!integer ? qtrue : qfalse;
	}

	bool getBool() const
	{
		return !!integer;
	}

	int32_t getInt() const
	{
		return integer;
	}
	uint32_t getBits() const
	{
		return bits;
	}
	float getFloat() const
	{
		return value;
	}
	const char* getStr() const
	{
		return string.c_str();
	}
	size_t stringlen() const
	{
		return string.length();
	}
	const char* getDefaultStr() const
	{
		return defaultString.c_str();
	}

	xcvar& operator=( const xcvar &other ) = default;

	xcvar& operator=( const int32_t i )
	{
		setInt( i );
		return *this;
	}

	xcvar& operator=( const float f )
	{
		setFloat( f );
		return *this;
	}

	xcvar& operator=( const char* newStr )
	{
		setString( newStr );
		return *this;
	}

	xcvar& operator=( const std::string &newStr )
	{
		setString( newStr.c_str() );
		return *this;
	}

	void operator+=( int32_t i )
	{
		setInt( integer + i );
	}

	void operator+=( float f )
	{
		setFloat( value + f );
	}

	void operator+=( const char *newStr )
	{
		std::string val( string );
		val.append( newStr );
		setString( val.c_str() );
	}

	void operator-=( int32_t i )
	{
		setInt( integer - i );
	}

	void operator-=( float f )
	{
		setFloat( value - f );
	}

	// called once on every module startup
	friend void XCVAR_RegisterXCvars( void );

	// called every module update frame because
	// engine cannot invoke update callbacks in modules
	friend void XCVAR_UpdateXCvars( void );
};

void XCVAR_RegisterXCvars( void );
void XCVAR_UpdateXCvars( void );
