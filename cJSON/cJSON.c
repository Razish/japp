/*
	JSON framework in C

	Copyright (c) 2010 Lourens "BobaFett" Elzinga

	Based on cJSON by Dave Gamble

	Changes:

	* Cleaned up code formatting to make it much more readable
	* Renamed print to serialize
	* Serializer now uses a stringbuilder instead of a tremendous amount of mallocs
	* Added usage of dynamic arrays and hashtables to drastically speed up lookup times for arrays and objects
	* JSON Type #defines are now internal
	* Array and Object functions now contain sanity checks (to ensure an array/object is being used)
	* cJSON struct is now internal, and is typedef'd as void
	* Replaced all instances of sprintf by snprintf to remove the risk of overflows (#defined as sprintf_s for windows compiles)
	* Added functions to obtain item values as a specific type (with default value in case of an error or incompatible type)
	* Added functions to determine the type of an item
	* Removed 'references'. They are unsafe and not very useful.
	* Added item duplication
	* Added new create functions (for booleans and integers)
	* The string serializer now supports unprintable characters ( < ANSI 32 without \x equivalent )
	* Deleting linked nodes is no longer possible
	* Added a safe version of cJSON_Delete which also clears the pointer if deleting was successful
	* Added function to insert items in arrays
	* Added function to swap 2 items in arrays
	* Added functions to clear arrays and objects
	* Added extended lookup, to allow retreival of deeply nested items in 1 call
	* Added 64 bit integer support
*/

// cJSON
// JSON parser in C.

// Sprintf security fix
// All instances of sprintf are change to snprintf to avoid buffer overflows
// On windows, it's called sprintf_s instead of snprintf, so we'll do a little define here
//Raz: also, VS2015's stdio.h does not allow macro redefinition of standard library functions
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf sprintf_s
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#define __STDC_FORMAT_MACROS // older compilers need this
#include <inttypes.h>
#define __cJSON_INTERNAL

// cJSON Types:
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6

#define cJSON_HashTableSize 32		// An object's hashtable has cJSON_HashTableSize slots
#define cJSON_ArrayBlockSize 16		// Arrays are alloced with cJSON_ArrayBlockSize slots at a time

// The cJSON structure:
typedef struct cJSON_s {
	/* Item walking */
	struct cJSON_s *next, *prev;	// next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem
	struct cJSON_s *child;		// An array or object item will have a child pointer pointing to a chain of the items in the array/object.

	/* Array/Object lookup */
	struct cJSON_s **table;		// Dynamic array or hashtable for quick item lookups
	size_t tablesize;			// Size of the dynamic array or hashtable, depending on type
	size_t arraysize;			// Size of an array/object, if applicable

	/* Item type */
	int type;					// The type of the item, as above.

	/* Item data */
	char *valuestring;			// The item's string, if type == cJSON_String
	int valueint;				// The item's number, if type == cJSON_Number
	double valuedouble;			// The item's number, if type == cJSON_Number
	int64_t valuelong;		// The item's number, if type == cJSON_Number

	/* Subitem data */
	char *string;				// The item's name string, if this item is the child of, or is in the list of subitems of an object.
	int linked;					// 1 if this node is linked somewhere. If set, attempts to add it to another object or array will fail.
	struct cJSON_s *hashnext;	// Next entry in the hashtable (if applicable)
	struct cJSON_s *lastentry;		// Latest entry in the object, so we know which item to link to
} cJSON;

#include "cJSON.h"

static int cJSON_strcasecmp( const char *s1, const char *s2 ) {
	if ( !s1 ) return (s1 == s2) ? 0 : 1;
	if ( !s2 ) return 1;
	for ( ; tolower( *s1 ) == tolower( *s2 ); ++s1, ++s2 ) {
		if ( *s1 == 0 ) {
			return 0;
		}
	}
	return tolower( *(const unsigned char *)s1 ) - tolower( *(const unsigned char *)s2 );
}

// override memory allocator

#if defined(_MSC_VER)

	// required to fix C4232: address of dllimport 'malloc' is not static, identity not guaranteed
	//	does not happen with runtime symbol resolution

	static void *local_malloc( size_t sz ) {
		return malloc( sz );
	}
	static void *local_realloc( void *ptr, size_t sz ) {
		return realloc( ptr, sz );
	}
	static void local_free( void *ptr ) {
		free( ptr );
	}

	void *(*cJSON_malloc)	( size_t sz )				= local_malloc;
	void *(*cJSON_realloc)	( void *ptr, size_t sz )	= local_realloc;
	void  (*cJSON_free)		( void *ptr )				= local_free;

#else

	static void *(*cJSON_malloc)	( size_t sz )				= malloc;
	static void *(*cJSON_realloc)	( void *ptr, size_t sz )	= realloc;
	static void  (*cJSON_free)		( void *ptr )				= free;

#endif

static uint32_t cJSON_GenerateHashValue( const char *name, const int size ) {
	int		i;
	uint32_t	hash;

	hash = 0;
	i = 0;
	while ( name[i] != '\0' ) {
		char letter = (char)tolower( name[i] );
		hash += (uint32_t)(letter)*(i + 119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size - 1);
	return hash;
}

static char* cJSON_strdup( const char* str ) {
	size_t len;
	char* copy;

	if ( !str ) {
		return 0;
	}

	len = strlen( str ) + 1;
	if ( !(copy = (char*)cJSON_malloc( len )) ) {
		return 0;
	}
	memcpy( copy, str, len );
	return copy;
}

void cJSON_InitHooks( cJSON_Hooks* hooks ) {
	if ( !hooks ) {/* Reset hooks */
		cJSON_malloc = malloc;
		cJSON_realloc = realloc;
		cJSON_free = free;
		return;
	}

	cJSON_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
	cJSON_realloc = (hooks->realloc_fn) ? hooks->realloc_fn : realloc;
	cJSON_free = (hooks->free_fn) ? hooks->free_fn : free;
}

/* String builder */
#define SBBLOCKSIZE 512	// Alloc SBBLOCKSIZE bytes at a time for the stringbuilder

typedef struct sb_s {	// StringBuilder structure for the serializer
	char	*buffer;
	size_t	bufferlen;
	size_t	stringlen;
	char	finalized;
} cJSON_StringBuilder;

static void cJSON_SB_Init( cJSON_StringBuilder *sb )	// Initializes the stringbuilder structure (Do not call this more than once, as it causes memory leaks)
{
	sb->buffer = cJSON_malloc( SBBLOCKSIZE );
	sb->bufferlen = SBBLOCKSIZE;
	sb->stringlen = 0;
	sb->finalized = 0;
}

static int cJSON_SB_CheckSpace( cJSON_StringBuilder *sb, size_t space )	// Ensures at least 'space' bytes can be appended to the buffer (and enlarges it if needed)
{
	int spaceToAlloc = 0;
	void *newptr;

	while ( sb->bufferlen + spaceToAlloc < sb->stringlen + space ) {
		spaceToAlloc += SBBLOCKSIZE;
	}
	if ( !spaceToAlloc ) {
		return 1;	// Enough space
	}
	// We need to alloc space, so do it now
	newptr = cJSON_realloc( sb->buffer, sb->bufferlen + spaceToAlloc );
	if ( !newptr ) {
		return 0;	// Failed to alloc
	}
	else {
		sb->buffer = (char *)newptr;
		sb->bufferlen += spaceToAlloc;
		return 1;
	}
}

static void cJSON_SB_AddChar( cJSON_StringBuilder *sb, char ch ) {
	if ( !cJSON_SB_CheckSpace( sb, 1 ) ) {
		return;
	}
	sb->buffer[sb->stringlen++] = ch;
}

static void cJSON_SB_AddCharRep( cJSON_StringBuilder *sb, char ch, int reps ) {
	if ( reps < 1 ) {
		return;
	}
	if ( !cJSON_SB_CheckSpace( sb, reps ) ) {
		return;
	}
	while ( reps ) {
		sb->buffer[sb->stringlen++] = ch;
		reps--;
	}
}

static void cJSON_SB_AddStringN( cJSON_StringBuilder *sb, const char *str, size_t len ) {
	if ( !cJSON_SB_CheckSpace( sb, len ) ) {
		return;
	}
	strcpy( &sb->buffer[sb->stringlen], str );
	sb->stringlen += len;
}

static void cJSON_SB_AddString( cJSON_StringBuilder *sb, const char *str ) {
	int len = strlen( str );
	cJSON_SB_AddStringN( sb, str, len );
}

static const char *cJSON_SB_Finalize( cJSON_StringBuilder *sb ) {
	void *newptr;
	if ( !cJSON_SB_CheckSpace( sb, 1 ) ) {
		return 0;
	}
	sb->buffer[sb->stringlen++] = 0;	// NULL terminator
	newptr = realloc( sb->buffer, sb->stringlen );
	if ( !newptr ) {
		return 0;
	}
	else {
		sb->finalized = 1;
		sb->bufferlen = sb->stringlen;
		sb->buffer = newptr;
		return newptr;
	}
}

/* End of stringbuilder */


// Internal constructor.
static cJSON *cJSON_New_Item( void ) {
	cJSON* node = (cJSON*)cJSON_malloc( sizeof(cJSON) );
	if ( node ) {
		memset( node, 0, sizeof(cJSON) );
	}
	return node;
}

// Delete a cJSON structure.
void cJSON_Delete( cJSON *c ) {
	cJSON *next;

	if ( !c ) {
		return;
	}

	if ( c->linked ) {
		return;		// Cannot delete linked items
	}

	while ( c ) {
		next = c->next;

		if ( c->child ) {
			c->child->linked = 0;	// Mark it as unlinked so we can delete it
			cJSON_Delete( c->child );
		}
		if ( c->valuestring ) {
			cJSON_free( c->valuestring );
		}
		if ( c->string ) {
			cJSON_free( c->string );
		}
		if ( c->table ) {
			cJSON_free( c->table );
		}

		cJSON_free( c );
		c = next;
	}
}

// Delete a cJSON item and clear the pointer to it (if it can be deleted that is)
void cJSON_SafeDelete( cJSON **c ) {
	if ( c && *c && !(*c)->linked ) {
		cJSON_Delete( *c );
		*c = 0;
	}
}

// Parse the input text to generate a number, and populate the result into item.
static const char *parse_number( cJSON *item, const char *num ) {
	long double n = 0, sign = 1, scale = 0;

	int subscale = 0, signsubscale = 1;

	// Could use sscanf for this?
	if ( *num == '-' ) { // Has sign?
		sign = -1;
		num++;
	}
	if ( *num == '0' ) { // is zero
		num++;
	}
	if ( *num >= '1' && *num <= '9' ) { // Number?
		do {
			n = (n * 10.0) + (*num++ - '0');
		} while ( *num >= '0' && *num <= '9' );
	}
	if ( *num == '.' )	// Fractional part?
	{
		num++;
		do {
			n = (n * 10.0) + (*num++ - '0');
			scale--;
		} while ( *num >= '0' && *num <= '9' );
	}
	if ( *num == 'e' || *num == 'E' )		// Exponent?
	{
		num++;
		if ( *num == '+' )				// With sign?
		{
			num++;
		}
		else if ( *num == '-' ) {
			signsubscale = -1;
			num++;
		}
		while ( *num >= '0' && *num <= '9' ) {
			subscale = (subscale * 10) + (*num++ - '0');	// Number?
		}
	}

	n = sign * n * pow( 10.0, (scale + subscale * signsubscale) );	// number = +/- number.fraction * 10^+/- exponent

	item->valuedouble = (double)n;
	item->valueint = (int)n;
	item->valuelong = (int64_t)n;
	item->type = cJSON_Number;
	return num;
}

// Serialize the number nicely from the given item into a string.
static void serialize_number( cJSON *item, cJSON_StringBuilder *sb ) {
	char str[64];
	double d = item->valuedouble;
	if ( fabs( ((double)item->valueint) - d ) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN ) {
		snprintf( str, 21, "%d", item->valueint );
	}
	else {
		if ( fabs( floor( d ) - d ) <= DBL_EPSILON ) {
			snprintf( str, 64, "%.0f", d );
		}
		else if ( fabs( d ) < 1.0e-6 || fabs( d ) > 1.0e9 ) {
			snprintf( str, 64, "%e", d );
		}
		else {
			snprintf( str, 64, "%f", d );
		}
	}

	cJSON_SB_AddString( sb, str );
}

// Parse the input text into an unescaped cstring, and populate item.
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string( cJSON *item, const char *str ) {
	const char *ptr = str + 1;
	char *ptr2;
	char *out;
	int len = 0;
	unsigned uc;

	if ( *str != '\"' ) {
		return 0;	// not a string!
	}

	while ( *ptr != '\"' && (unsigned char)*ptr > 31 && ++len ) {
		if ( *ptr++ == '\\' ) {
			ptr++;	// Skip escaped quotes.
		}
	}

	out = (char*)cJSON_malloc( len + 1 );	// This is how long we need for the string, roughly.
	if ( !out ) {
		return 0;	// malloc failed
	}

	ptr = str + 1;
	ptr2 = out;

	while ( *ptr != '\"' && (unsigned char)*ptr > 31 ) {
		if ( *ptr != '\\' ) {
			*ptr2++ = *ptr++;
		}
		else {
			ptr++;
			switch ( *ptr ) {
			case 'b':
				*ptr2++ = '\b';
				break;
			case 'f':
				*ptr2++ = '\f';
				break;
			case 'n':
				*ptr2++ = '\n';
				break;
			case 'r':
				*ptr2++ = '\r';
				break;
			case 't':
				*ptr2++ = '\t';
				break;
			case 'u':	 // transcode utf16 to utf8. DOES NOT SUPPORT SURROGATE PAIRS CORRECTLY.
				sscanf( ptr + 1, "%4x", &uc );	// get the unicode char.

				if ( uc < 0x80 ) {
					len = 1;
				}
				else if ( uc < 0x800 ) {
					len = 2;
				}
				else {
					len = 3;
				}

				ptr2 += len;

				switch ( len ) {
				case 3: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
				case 2: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
				case 1: *--ptr2 = (char)(uc | firstByteMark[len]);
				default: break;
				}
				ptr2 += len;
				ptr += 4;
				break;
			default:
				*ptr2++ = *ptr;
				break;
			}
			ptr++;
		}
	}
	*ptr2 = 0;
	if ( *ptr == '\"' ) {
		ptr++;
	}
	item->valuestring = out;
	item->type = cJSON_String;
	return ptr;
}

// Serialize the cstring provided to an escaped version that can printed/stored.
static void serialize_string_ptr( const char *str, cJSON_StringBuilder *sb ) {
	const char *ptr;
	char ubuff[5];

	if ( !str ) {
		cJSON_SB_AddStringN( sb, "\"\"", 2 );	// Add "" and bail
		return;
	}

	ptr = str;
	cJSON_SB_AddChar( sb, '\"' );

	while ( *ptr ) {
		if ( (unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\' ) {
			cJSON_SB_AddChar( sb, *ptr );
		}
		else {
			cJSON_SB_AddChar( sb, '\\' );
			switch ( *ptr++ ) {
			case '\\':
				cJSON_SB_AddChar( sb, '\\' );
				break;
			case '\"':
				cJSON_SB_AddChar( sb, '\"' );
				break;
			case '\b':
				cJSON_SB_AddChar( sb, 'b' );
				break;
			case '\f':
				cJSON_SB_AddChar( sb, 'f' );
				break;
			case '\n':
				cJSON_SB_AddChar( sb, 'n' );
				break;
			case '\r':
				cJSON_SB_AddChar( sb, 'r' );
				break;
			case '\t':
				cJSON_SB_AddChar( sb, 't' );
				break;
			default:
				cJSON_SB_AddChar( sb, 'u' );
				snprintf( ubuff, 5, "%04x", (unsigned int)*(ptr - 1) );
				cJSON_SB_AddStringN( sb, ubuff, 4 );
				break;
			}
		}
		ptr++;
	}

	cJSON_SB_AddChar( sb, '\"' );
}
// Invoke print_string_ptr on an item to serialize it's string value.
__inline static void serialize_string( cJSON *item, cJSON_StringBuilder *sb ) {
	serialize_string_ptr( item->valuestring, sb );
}

// Predeclare these prototypes.
static const char *parse_value( cJSON *item, const char *value );
static const char *parse_array( cJSON *item, const char *value );
static const char *parse_object( cJSON *item, const char *value );

static void serialize_value( cJSON *item, int depth, int fmt, cJSON_StringBuilder *sb );
static void serialize_array( cJSON *item, int depth, int fmt, cJSON_StringBuilder *sb );
static void serialize_object( cJSON *item, int depth, int fmt, cJSON_StringBuilder *sb );

// Utility to jump whitespace and cr/lf
static const char *skip( const char *in ) {
	while ( in && (unsigned char)*in <= 32 ) {
		in++;
	}
	return in;
}

// Parse an object - create a new root, and populate.
cJSON *cJSON_Parse( const char *value ) {
	cJSON *c = cJSON_New_Item();
	if ( !c ) {
		return 0;       /* memory fail */
	}

	if ( !parse_value( c, skip( value ) ) ) {
		cJSON_Delete( c );
		return 0;
	}
	return c;
}

// Serialize a cJSON item/entity/structure to text.
const char *cJSON_Serialize( cJSON *item, int format ) {
	cJSON_StringBuilder sb;
	cJSON_SB_Init( &sb );
	serialize_value( item, 0, format, &sb );
	return cJSON_SB_Finalize( &sb );
}


// Parser core - when encountering text, process appropriately.
static const char *parse_value( cJSON *item, const char *value ) {
	if ( !value ) {
		return 0;	// Fail on null.
	}
	if ( !strncmp( value, "null", 4 ) ) {
		item->type = cJSON_NULL;
		return value + 4;
	}
	if ( !strncmp( value, "false", 5 ) ) {
		item->type = cJSON_False;
		return value + 5;
	}
	if ( !strncmp( value, "true", 4 ) ) {
		item->type = cJSON_True;
		item->valueint = 1;
		return value + 4;
	}
	if ( *value == '\"' ) {
		return parse_string( item, value );
	}
	if ( *value == '-' || (*value >= '0' && *value <= '9') ) {
		return parse_number( item, value );
	}
	if ( *value == '[' ) {
		return parse_array( item, value );
	}
	if ( *value == '{' ) {
		return parse_object( item, value );
	}

	return 0;	// failure.
}

// Serialize a value to text.
static void serialize_value( cJSON *item, int depth, int fmt, cJSON_StringBuilder *sb ) {
	if ( !item ) {
		return;
	}
	switch ( item->type ) {
	case cJSON_NULL:
		cJSON_SB_AddStringN( sb, "null", 4 );
		break;
	case cJSON_False:
		cJSON_SB_AddStringN( sb, "false", 5 );
		break;
	case cJSON_True:
		cJSON_SB_AddStringN( sb, "true", 4 );
		break;
	case cJSON_Number:
		serialize_number( item, sb );
		break;
	case cJSON_String:
		serialize_string( item, sb );
		break;
	case cJSON_Array:
		serialize_array( item, depth, fmt, sb );
		break;
	case cJSON_Object:
		serialize_object( item, depth, fmt, sb );
		break;
	default:
		break;
	}
}

// Build an arry from input text.
static const char *parse_array( cJSON *item, const char *value ) {
	cJSON *child;

	if ( *value != '[' ) {
		return 0;	// not an arry!
	}
	item->type = cJSON_Array;

	item->table = cJSON_malloc( cJSON_ArrayBlockSize * sizeof(cJSON *) );
	item->tablesize = cJSON_ArrayBlockSize;

	value = skip( value + 1 );

	if ( *value == ']' ) {
		return value + 1;	// empty arry.
	}
	child = cJSON_New_Item();

	if ( !child ) {
		return 0;		 // memory fail
	}
	value = skip( parse_value( child, skip( value ) ) );	// skip any spacing, get the value.

	if ( !value ) {
		return 0;
	}

	cJSON_AddItemToArray( item, child );

	while ( *value == ',' ) {
		if ( !(child = cJSON_New_Item()) )
			return 0; 	// memory fail

		if ( !(value = skip( parse_value( child, skip( value + 1 ) ) )) )
			return 0;	// memory fail

		cJSON_AddItemToArray( item, child );
	}

	if ( *value == ']' ) {
		return value + 1;	// end of arry
	}
	return 0;	// malformed.
}

// Serialize an array to text
static void serialize_array( cJSON *item, int depth, int fmt, cJSON_StringBuilder *sb ) {
	cJSON *child = item->child;

	cJSON_SB_AddChar( sb, '[' );
	while ( child ) {
		serialize_value( child, depth + 1, fmt, sb );
		if ( child->next ) {
			cJSON_SB_AddChar( sb, ',' );
			if ( fmt ) {
				cJSON_SB_AddChar( sb, ' ' );
			}
		}
		child = child->next;
	}
	cJSON_SB_AddChar( sb, ']' );
}

// Build an object from the text.
static const char *parse_object( cJSON *item, const char *value ) {
	cJSON *child;
	const char *name;
	if ( *value != '{' ) {
		return 0;	// not an object!
	}

	// Create object
	item->type = cJSON_Object;

	item->table = cJSON_malloc( cJSON_HashTableSize * sizeof(cJSON *) );
	memset( item->table, 0, cJSON_HashTableSize * sizeof(cJSON *) );
	item->tablesize = cJSON_HashTableSize;

	value = skip( value + 1 );
	if ( *value == '}' ) {
		return value + 1;	// empty arry.
	}

	child = cJSON_New_Item();
	if ( !child ) {
		return 0;
	}

	value = skip( parse_string( child, skip( value ) ) );
	if ( !value ) {
		return 0;
	}

	name = child->valuestring;
	child->valuestring = 0;

	if ( *value != ':' ) {
		return 0;	// fail!
	}

	value = skip( parse_value( child, skip( value + 1 ) ) );	// skip any spacing, get the value.
	if ( !value ) {
		return 0;
	}

	cJSON_AddItemToObject( item, name, child );
	free( (void *)name );

	while ( *value == ',' ) {
		if ( !(child = cJSON_New_Item()) ) {
			return 0; // memory fail
		}

		value = skip( parse_string( child, skip( value + 1 ) ) );
		if ( !value ) {
			return 0;
		}
		name = child->valuestring;
		child->valuestring = 0;

		if ( *value != ':' ) {
			return 0;	// fail!
		}
		value = skip( parse_value( child, skip( value + 1 ) ) );	// skip any spacing, get the value.
		if ( !value ) {
			return 0;
		}

		cJSON_AddItemToObject( item, name, child );
		free( (void *)name );
	}

	if ( *value == '}' ) {
		return value + 1;	// end of arry
	}
	return 0;	// malformed.
}

// Render an object to text.
static void serialize_object( cJSON *item, int depth, int fmt, cJSON_StringBuilder *sb ) {
	cJSON *child = item->child;

	depth++;

	cJSON_SB_AddChar( sb, '{' );
	if ( fmt ) {
		cJSON_SB_AddChar( sb, '\n' );
	}
	while ( child ) {
		if ( fmt ) {
			cJSON_SB_AddCharRep( sb, '\t', depth );
		}
		serialize_string_ptr( child->string, sb );
		if ( fmt ) {
			cJSON_SB_AddStringN( sb, " : ", 3 );
		}
		else {
			cJSON_SB_AddChar( sb, ':' );
		}
		serialize_value( child, depth, fmt, sb );
		if ( child->next ) {
			cJSON_SB_AddChar( sb, ',' );
		}
		if ( fmt ) {
			cJSON_SB_AddChar( sb, '\n' );
		}
		child = child->next;
	}
	if ( fmt ) {
		cJSON_SB_AddCharRep( sb, '\t', depth - 1 );
	}
	cJSON_SB_AddChar( sb, '}' );
}

// Get Array size/item / object item.
int cJSON_GetArraySize( cJSON *arry ) {
	// Not an array or object
	if ( !arry || (arry->type != cJSON_Array && arry->type != cJSON_Object) )
		return 0;

	return arry->arraysize;
}

cJSON *cJSON_GetArrayItem( cJSON *arry, int item ) {
	if ( !arry || arry->type != cJSON_Array || item < 0 || (size_t)item >= arry->arraysize ) {
		return 0;	// Not an array or the index is out of bounds
	}
	return arry->table[item];
}

cJSON *cJSON_GetObjectItem( cJSON *object, const char *string ) {
	cJSON *c;
	uint32_t hash;
	if ( !object || object->type != cJSON_Object ) {
		return 0;	// Not an object
	}

	hash = cJSON_GenerateHashValue( string, cJSON_HashTableSize );
	for ( c = object->table[hash]; c; c = c->hashnext ) {
		if ( !cJSON_strcasecmp( c->string, string ) ) {
			return c;
		}
	}
	return 0;
}

// Extended GetItem.
// Allows complex nested lookups
//
// Format example:
// sub[5].myvar
// [2]
// test.sub[24].something[4].var


cJSON *cJSON_GetItemExt( cJSON *item, const char *extitem ) {
	char cmd[256] = { 0 };
	char *t;
	const char *s;
	int state = 0;	// 0 = awaiting initial input, 1 = pending object name, 2 = pending index, 3 = awaiting next item
	cJSON *c;

	c = item;
	t = &cmd[0];
	s = extitem;
	for ( ;; ) {
		if ( !*s ) {
			if ( state == 1 ) {
				*t = 0;
				c = cJSON_GetObjectItem( c, cmd );
				if ( !c ) {
					return 0;	// Invalid index
				}
				state = 3;
				t = &cmd[0];
			}
			else if ( state == 2 ) {
				*t = 0;
				c = cJSON_GetArrayItem( c, atoi( cmd ) );
				if ( !c ) {
					return 0;	// Invalid index
				}
				state = 3;
				t = &cmd[0];
			}
			break;
		}
		else if ( *s == '[' ) {
			// Opening bracket for index
			if ( state == 1 ) {
				// Finalized an object name, do the lookup
				*t = 0;
				c = cJSON_GetObjectItem( c, cmd );
				if ( !c ) {
					return 0;	// Invalid index
				}
				state = 2;
				t = &cmd[0];
			}
			else if ( state == 2 ) {
				// Syntax error
				return 0;
			}
			else {
				state = 2;
			}
		}
		else if ( *s == ']' ) {
			if ( state == 2 ) {
				// Finalized an index name, do the lookup
				*t = 0;
				c = cJSON_GetArrayItem( c, atoi( cmd ) );
				if ( !c ) {
					return 0;	// Invalid index
				}
				state = 3;
				t = &cmd[0];
			}
			else {
				// Syntax error
				return 0;
			}
		}
		else if ( *s == '.' ) {
			if ( state == 1 ) {
				*t = 0;
				c = cJSON_GetObjectItem( c, cmd );
				if ( !c ) {
					return 0;	// Invalid index
				}
				state = 1;
				t = &cmd[0];
			}
			else if ( state == 3 ) {
				state = 1;
			}
			else {
				// Syntax error
				return 0;
			}
		}
		else if ( *s >= '0' && *s <= '9' ) {
			if ( state == 0 || state == 3 ) state = 1;
			*t++ = *s;
		}
		else {
			if ( state == 2 ) {
				// Syntax error, non-number in index
				return 0;
			}
			if ( state == 0 || state == 3 ) state = 1;
			*t++ = *s;
		}
		s++;
	}
	return c;
}

// Add item to arry/object.
void cJSON_AddItemToArray( cJSON *arry, cJSON *item ) {

	cJSON *c;
	if ( arry->type != cJSON_Array ) {
		return;
	}
	if ( item->linked ) {
		return;	// Item is already linked, dont allow a re-link
	}
	if ( arry->tablesize == arry->arraysize ) {
		// Enlarge the table
		void *tmp;
		tmp = cJSON_realloc( arry->table, (arry->tablesize + cJSON_ArrayBlockSize) * sizeof(cJSON *) );
		if ( tmp ) {
			arry->table = tmp;
			arry->tablesize += cJSON_ArrayBlockSize;
		}
		else {
			return;		// Allocation failed
		}
	}

	if ( !arry->child ) {
		arry->child = item;
		arry->table[0] = item;
		arry->arraysize = 1;
	}
	else {
		c = arry->table[arry->arraysize - 1]; // Get the last object in the array

		c->next = item;
		item->prev = c;

		arry->table[arry->arraysize] = item;	// Add it to the array
		arry->arraysize++;
	}
	item->linked = 1;
}

// Add item to arry/object.
void cJSON_InsertItemInArray( cJSON *arry, cJSON *item, int before ) {
	cJSON *c;
	if ( arry->type != cJSON_Array ) {
		return;
	}
	if ( item->linked ) {
		return;	// Item is already linked, dont allow a re-link
	}

	if ( before < 0 ) {	// Clamp insertion point
		before = 0;
	}
	else if ( (size_t)before >= arry->arraysize ) {
		before = arry->arraysize - 1;
	}

	if ( arry->tablesize == arry->arraysize ) {
		// Enlarge the table
		void *tmp;
		tmp = cJSON_realloc( arry->table, (arry->tablesize + cJSON_ArrayBlockSize) * sizeof(cJSON *) );
		if ( tmp ) {
			arry->table = tmp;
			arry->tablesize += cJSON_ArrayBlockSize;
		}
		else {
			return;		// Allocation failed
		}
	}

	if ( !arry->child ) {
		arry->child = item;
		arry->table[0] = item;
		arry->arraysize = 1;
	}
	else {
		if ( before == 0 ) {
			arry->child->prev = item;
			item->next = arry->child;
			arry->child = item;
		}
		else {
			c = arry->table[before - 1]; // Get the object we want to insert it in front of

			item->next = c->next;
			item->next->prev = item;
			item->prev = c;
			c->next = item;
		}
		// Shift the array
		memmove( &arry->table[before + 1], &arry->table[before], (arry->arraysize - before) * sizeof(cJSON *) );

		arry->table[before] = item;	// Add it to the array
		arry->arraysize++;
	}
	item->linked = 1;
}

void cJSON_AddItemToObject( cJSON *object, const char *string, cJSON *item ) {
	uint32_t hash;
	if ( object->type != cJSON_Object ) {
		return;
	}
	if ( item->linked ) {
		return;	// Item is already linked, dont allow a re-link
	}
	if ( item->string ) {
		cJSON_free( item->string );
	}
	item->string = cJSON_strdup( string );

	cJSON_DeleteItemFromObject( object, string );		// If the item already exists, delete it

	hash = cJSON_GenerateHashValue( string, cJSON_HashTableSize );	// Get the hash
	item->hashnext = object->table[hash];							// Link it to the hashtable
	object->table[hash] = item;

	if ( !object->lastentry ) {	// No lastentry = no children
		object->child = item;
		object->lastentry = item;
	}
	else {
		object->lastentry->next = item;
		item->prev = object->lastentry;
		object->lastentry = item;
	}

	item->linked = 1;
	object->arraysize++;
}

cJSON *cJSON_DetachItemFromArray( cJSON *arry, int which ) {
	cJSON *c;
	if ( arry->type != cJSON_Array || which < 0 || (size_t)which >= arry->arraysize ) {
		return 0;
	}

	c = arry->table[which];

	if ( !c ) {
		return 0;
	}
	if ( c->prev ) {
		c->prev->next = c->next;
	}

	if ( c->next ) {
		c->next->prev = c->prev;
	}

	if ( c == arry->child ) {
		arry->child = c->next;
	}
	c->prev = c->next = 0;

	if ( (size_t)which != arry->arraysize - 1 ) {
		// Shift it out of the dynamic array
		memmove( &arry->table[which], &arry->table[which + 1], (arry->arraysize - (which + 1)) * sizeof(cJSON *) );
	}

	arry->arraysize--;
	if ( arry->tablesize - arry->arraysize > 31 ) {
		// Got enough slack, lets shrink the array
		void *tmp;
		tmp = realloc( arry->table, (arry->tablesize - cJSON_ArrayBlockSize) * sizeof(cJSON *) );
		if ( tmp ) {
			arry->table = tmp;
			arry->tablesize = arry->tablesize - cJSON_ArrayBlockSize;
		}
	}

	c->linked = 0;
	return c;
}

void cJSON_DeleteItemFromArray( cJSON *arry, int which ) {
	cJSON_Delete( cJSON_DetachItemFromArray( arry, which ) );
}

cJSON *cJSON_DetachItemFromObject( cJSON *object, const char *string ) {
	cJSON *c = 0, *p = 0;
	uint32_t hash;

	hash = cJSON_GenerateHashValue( string, cJSON_HashTableSize );

	for ( c = object->table[hash]; c; c = c->hashnext ) {
		if ( !cJSON_strcasecmp( c->string, string ) ) {
			if ( c->prev ) {
				c->prev->next = c->next;
			}

			if ( c->next ) {
				c->next->prev = c->prev;
			}

			if ( c == object->child ) {
				object->child = c->next;
			}
			if ( c == object->lastentry ) {
				object->lastentry = c->prev;
			}
			c->prev = c->next = 0;

			if ( p ) {	// Unlink it from the hashtable
				p->hashnext = c->hashnext;
			}
			else {
				object->table[hash] = c->hashnext;
			}

			c->linked = 0;
			object->arraysize--;
			return c;
		}
		p = c;
	}
	return 0;
}

void cJSON_DeleteItemFromObject( cJSON *object, const char *string ) {
	cJSON_Delete( cJSON_DetachItemFromObject( object, string ) );
}

void cJSON_ClearItemsFromObject( cJSON *object ) {
	if ( object->type != cJSON_Object ) {
		return;
	}
	if ( object->child ) {
		object->child->linked = 0;
		cJSON_Delete( object->child );
		object->child = 0;
	}
	object->arraysize = 0;
	memset( object->table, 0, cJSON_HashTableSize * sizeof(cJSON *) );
	return;
}


void cJSON_ClearItemsFromArray( cJSON *arry ) {
	void *tmp;
	if ( arry->type != cJSON_Array ) {
		return;
	}
	if ( arry->child ) {
		arry->child->linked = 0;
		cJSON_Delete( arry->child );
		arry->child = 0;
	}
	tmp = realloc( arry->table, cJSON_ArrayBlockSize * sizeof(cJSON *) );
	if ( tmp ) {
		arry->table = tmp;
		arry->tablesize = cJSON_ArrayBlockSize;
	}
	arry->arraysize = 0;
	return;
}


// Replace arry/object items with new ones.
void cJSON_ReplaceItemInArray( cJSON *arry, int which, cJSON *newitem ) {
	cJSON *c;
	if ( arry->type != cJSON_Array || which < 0 || (size_t)which >= arry->arraysize ) {
		return;
	}

	c = arry->table[which];

	if ( !c ) {
		return;
	}

	newitem->next = c->next;
	newitem->prev = c->prev;

	if ( newitem->next ) {
		newitem->next->prev = newitem;
	}

	if ( c == arry->child ) {
		arry->child = newitem;
	}
	else {
		newitem->prev->next = newitem;
	}
	c->next = c->prev = 0;

	arry->table[which] = newitem;

	cJSON_Delete( c );
}

__inline void cJSON_ReplaceItemInObject( cJSON *object, const char *string, cJSON *newitem ) {
	// AddItemToObject already deletes the existing item, so lets just call that
	cJSON_AddItemToObject( object, string, newitem );
}


void cJSON_SwapItemsInArray( cJSON *arry, int item1, int item2 ) {
	cJSON *i1, *i2;
	cJSON *n, *p;
	if ( arry->type != cJSON_Array || item1 < 0 || (size_t)item1 >= arry->arraysize || item2 < 0 || (size_t)item2 >= arry->arraysize ) {
		return;
	}

	i1 = arry->table[item1];
	i2 = arry->table[item2];

	if ( !i1 || !i2 ) {
		return;
	}

	// Swap the linkage
	n = i1->next;
	p = i1->prev;
	i1->next = i2->next;
	i1->prev = i2->prev;
	i2->next = n;
	i2->prev = p;

	// Swap the array indexes
	arry->table[item1] = i2;
	arry->table[item2] = i1;

	// Fix links TO the swapped items

	if ( i1->prev ) {
		i1->prev->next = i1;
	}
	else {
		// Only way prev is NULL, is if it's the first item
		arry->child = i1;
	}
	if ( i1->next ) {
		i1->next->prev = i1;
	}

	if ( i2->prev ) {
		i2->prev->next = i2;
	}
	else {
		arry->child = i2;
	}
	if ( i2->next ) {
		i2->next->prev = i2;
	}
}

// Duplicate items

cJSON *cJSON_DuplicateItem( cJSON *item ) {
	cJSON *c = 0;	// Current, New Used for copying child items
	cJSON *newitem = cJSON_New_Item();

	newitem->type = item->type;
	newitem->valuedouble = item->valuedouble;
	newitem->valueint = item->valueint;
	newitem->valuestring = cJSON_strdup( item->valuestring );

	if ( newitem->type == cJSON_Array ) {
		newitem->table = cJSON_malloc( cJSON_ArrayBlockSize * sizeof(cJSON *) );
		newitem->tablesize = cJSON_ArrayBlockSize;
	}
	else if ( item->type == cJSON_Object ) {
		newitem->table = cJSON_malloc( cJSON_HashTableSize * sizeof(cJSON *) );
		memset( newitem->table, 0, cJSON_HashTableSize * sizeof(cJSON *) );
		newitem->tablesize = cJSON_HashTableSize;
	}
	else {
		return newitem;
	}

	if ( item->child ) {
		for ( c = item->child; c; c = c->next ) {
			cJSON *n = cJSON_DuplicateItem( c );

			if ( item->type == cJSON_Object ) {
				cJSON_AddItemToObject( newitem, c->string, n );
			}
			else {
				cJSON_AddItemToArray( newitem, n );
			}
		}
	}
	return newitem;
}

// Create basic types:
cJSON *cJSON_CreateNull( void ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_NULL;
	return item;
}

cJSON *cJSON_CreateTrue( void ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_True;
	return item;
}

cJSON *cJSON_CreateFalse( void ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_False;
	return item;
}

cJSON *cJSON_CreateBoolean( int boolean ) {
	cJSON *item = cJSON_New_Item();
	item->type = boolean ? cJSON_True : cJSON_False;
	return item;
}

cJSON *cJSON_CreateNumber( double num ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_Number;
	item->valuedouble = num;
	item->valueint = (int)num;
	return item;
}

cJSON *cJSON_CreateInteger( int num ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_Number;
	item->valuedouble = (double)num;
	item->valueint = num;
	return item;
}

cJSON *cJSON_CreateLongInteger(int64_t num){
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_Number;
	item->valuedouble = (double)num;
	item->valueint = (int)num;
	item->valuelong = num;
	return item;
}

cJSON *cJSON_CreateString( const char *string ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_String;
	item->valuestring = cJSON_strdup( string );
	return item;
}

cJSON *cJSON_CreateArray( void ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_Array;

	item->table = cJSON_malloc( cJSON_ArrayBlockSize * sizeof(cJSON *) );
	item->tablesize = cJSON_ArrayBlockSize;
	return item;
}

cJSON *cJSON_CreateObject( void ) {
	cJSON *item = cJSON_New_Item();
	item->type = cJSON_Object;

	item->table = cJSON_malloc( cJSON_HashTableSize * sizeof(cJSON *) );
	memset( item->table, 0, cJSON_HashTableSize * sizeof(cJSON *) );
	item->tablesize = cJSON_HashTableSize;
	return item;
}

// Create Arrays:
cJSON *cJSON_CreateIntArray( int *numbers, int count ) {
	int i;
	cJSON *a = cJSON_CreateArray();

	for ( i = 0; i < count; i++ ) {
		cJSON *n = cJSON_CreateNumber( numbers[i] );
		cJSON_AddItemToArray( a, n );
	}
	return a;
}

cJSON *cJSON_CreateFloatArray( float *numbers, int count ) {
	int i;
	cJSON *a = cJSON_CreateArray();

	for ( i = 0; i < count; i++ ) {
		cJSON *n = cJSON_CreateNumber( numbers[i] );
		cJSON_AddItemToArray( a, n );
	}
	return a;
}

cJSON *cJSON_CreateDoubleArray( double *numbers, int count ) {
	int i;
	cJSON *a = cJSON_CreateArray();

	for ( i = 0; i < count; i++ ) {
		cJSON *n = cJSON_CreateNumber( numbers[i] );
		cJSON_AddItemToArray( a, n );
	}
	return a;
}

cJSON *cJSON_CreateLongIntArray(int64_t *numbers, int count) {
	int i;
	cJSON *a = cJSON_CreateArray();

	for (i = 0; i < count; i++) {
		cJSON *n = cJSON_CreateLongInteger(numbers[i]);
		cJSON_AddItemToArray(a, n);
	}
	return a;
}


cJSON *cJSON_CreateStringArray( const char **strings, int count ) {
	int i;
	cJSON *n = 0, *a = cJSON_CreateArray();

	for ( i = 0; i < count; i++ ) {
		if ( !strings[i] ) {
			n = cJSON_CreateNull();
		}
		else {
			n = cJSON_CreateString( strings[i] );
		}
		cJSON_AddItemToArray( a, n );
	}
	return a;
}

// Type checking (inlined)

__inline int cJSON_IsNULL( cJSON *item ) {
	return item->type == cJSON_NULL;
}

__inline int cJSON_IsTrue( cJSON *item ) {
	return item->type == cJSON_True;
}

__inline int cJSON_IsFalse( cJSON *item ) {
	return item->type == cJSON_False;
}

__inline int cJSON_IsBoolean( cJSON *item ) {
	return item->type == cJSON_True || item->type == cJSON_False;
}

__inline int cJSON_IsNumber( cJSON *item ) {
	return item->type == cJSON_Number;
}

__inline int cJSON_IsString( cJSON *item ) {
	return item->type == cJSON_String;
}

__inline int cJSON_IsArray( cJSON *item ) {
	return item->type == cJSON_Array;
}

__inline int cJSON_IsObject( cJSON *item ) {
	return item->type == cJSON_Object;
}

__inline int cJSON_IsLinked( cJSON *item ) {
	return item->linked;
}

int cJSON_ToBooleanOpt( cJSON *item, int defval ) {
	if ( !item ) {
		return defval;
	}
	switch ( item->type ) {
	case cJSON_False:
		return 0;
	case cJSON_True:
		return 1;
	case cJSON_NULL:
		return 0;
	case cJSON_Number:
		return (item->valueint != 0);
	case cJSON_String:
	case cJSON_Array:
	case cJSON_Object:
	default:
		return defval;
	}
}

double cJSON_ToNumberOpt( cJSON *item, double defval ) {
	if ( !item ) {
		return defval;
	}
	switch ( item->type ) {
	case cJSON_False:
	case cJSON_True:
	case cJSON_NULL:
		return defval;
	case cJSON_Number:
		return item->valuedouble;
	case cJSON_String:
		return atof( item->valuestring );
	case cJSON_Array:
	case cJSON_Object:
	default:
		return defval;
	}
}

int cJSON_ToIntegerOpt( cJSON *item, int defval ) {
	if ( !item ) {
		return defval;
	}
	switch ( item->type ) {
	case cJSON_False:
	case cJSON_True:
	case cJSON_NULL:
		return defval;
	case cJSON_Number:
		return item->valueint;
	case cJSON_String:
		return atoi( item->valuestring );
	case cJSON_Array:
	case cJSON_Object:
	default:
		return defval;
	}
}

int64_t cJSON_ToLongIntegerOpt(cJSON *item, int64_t defval){
	if (!item) {
		return defval;
	}
	switch (item->type) {
	case cJSON_False:
	case cJSON_True:
	case cJSON_NULL:
		return defval;
	case cJSON_Number:
		return item->valuelong;
	case cJSON_String:
		return atoll(item->valuestring);
	case cJSON_Array:
	case cJSON_Object:
	default:
		return defval;
	}
}

const char *cJSON_ToStringOpt( cJSON *item, const char *defval ) {
	if ( !item ) {
		return defval;
	}
	switch ( item->type ) {
	case cJSON_False:
	case cJSON_True:
	case cJSON_NULL:
	case cJSON_Number:
		return defval;
	case cJSON_String:
		return item->valuestring;
	case cJSON_Array:
	case cJSON_Object:
	default:
		return defval;
	}
}

__inline int cJSON_ToBoolean( cJSON *item ) {
	return cJSON_ToBooleanOpt( item, 0 );
}

__inline double cJSON_ToNumber( cJSON *item ) {
	return cJSON_ToNumberOpt( item, 0 );
}

__inline int cJSON_ToInteger( cJSON *item ) {
	return cJSON_ToIntegerOpt( item, 0 );
}

__inline int64_t cJSON_ToLongInteger(cJSON *item){
	return cJSON_ToLongIntegerOpt(item, 0);
}

__inline const char * cJSON_ToString( cJSON *item ) {
	return cJSON_ToStringOpt( item, 0 );
}

__inline int cJSON_ToBooleanRaw( cJSON *item ) {
	return item->type == cJSON_True ? 1 : 0;
}

__inline double cJSON_ToNumberRaw( cJSON *item ) {
	return item->valuedouble;
}

__inline int cJSON_ToIntegerRaw( cJSON *item ) {
	return item->valueint;
}

__inline const char * cJSON_ToStringRaw( cJSON *item ) {
	return item->valuestring;
}


void cJSON_SetStringValue( cJSON *item, const char *value ) {
	if ( item->type == cJSON_Array || item->type == cJSON_Object ) {
		return;
	}
	item->type = cJSON_String;

	if ( item->valuestring ) {
		free( item->valuestring );
	}
	item->valuestring = cJSON_strdup( value );
	item->valueint = 0;
	item->valuedouble = 0.0;
}

void cJSON_SetNumberValue( cJSON *item, double number ) {
	if ( item->type == cJSON_Array || item->type == cJSON_Object ) {
		return;
	}
	item->type = cJSON_Number;

	if ( item->valuestring ) {
		free( item->valuestring );
		item->valuestring = 0;
	}

	item->valueint = (int)number;
	item->valuedouble = number;
}

void cJSON_SetIntegerValue( cJSON *item, int integer ) {
	if ( item->type == cJSON_Array || item->type == cJSON_Object ) {
		return;
	}
	item->type = cJSON_Number;

	if ( item->valuestring ) {
		free( item->valuestring );
		item->valuestring = 0;
	}

	item->valueint = integer;
	item->valuedouble = (double)integer;
}

void cJSON_SetBooleanValue( cJSON *item, int boolean ) {
	if ( item->type == cJSON_Array || item->type == cJSON_Object ) {
		return;
	}

	item->type = boolean ? cJSON_True : cJSON_False;

	if ( item->valuestring ) {
		free( item->valuestring );
		item->valuestring = 0;
	}

	item->valueint = 0;
	item->valuedouble = 0;
}

void cJSON_SetNULLValue( cJSON *item ) {
	if ( item->type == cJSON_Array || item->type == cJSON_Object ) {
		return;
	}

	item->type = cJSON_NULL;

	if ( item->valuestring ) {
		free( item->valuestring );
		item->valuestring = 0;
	}

	item->valueint = 0;
	item->valuedouble = 0;
}

