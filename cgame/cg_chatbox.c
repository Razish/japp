
#include "cg_local.h"
#include "ui/menudef.h"
#include "ui/keycodes.h"
#include "cg_luaevent.h"

#define MAX_CHATBOX_ENTRIES (512)
#define MAX_CHATBOX_IDENTIFIER_SIZE (32)

#define EXAMPLE_TIMESTAMP "[^800:00:00^8] "
#define TIMESTAMP_LENGTH sizeof( EXAMPLE_TIMESTAMP )

#define EXAMPLE_TIMESTAMP_CLEAN "[00:00:00] "
#define TIMESTAMP_LENGTH_CLEAN sizeof( EXAMPLE_TIMESTAMP_CLEAN )

#define CHAT_MESSAGE_LENGTH MAX_SAY_TEXT

//CHATBOX OBJECTS
typedef struct chatEntry_s {
	char		timeStamp[TIMESTAMP_LENGTH];
	char		message[CHAT_MESSAGE_LENGTH];
	qboolean	isUsed;
	int			time;
} chatEntry_t;

typedef struct chatBox_s {
	chatEntry_t			chatBuffer[MAX_CHATBOX_ENTRIES]; // A logical message may be fragmented into multiple chatEntry_t structures, based on line width.
														// This is to allow scrolling cleanly but has the down-side of line splitting only being computed once
	int					numActiveLines; // totaly amount of lines used
	int					scrollAmount; // how many lines we've scrolled upwards
	qboolean			notification; // flashes when qtrue

	//For fetching the cb object
	char				shortname[MAX_CHATBOX_IDENTIFIER_SIZE]; //May be channel name, or preset names
	char				identifier[MAX_CHATBOX_IDENTIFIER_SIZE];
	struct chatBox_s	*prev; // for scrolling
	struct chatBox_s	*next;
} chatBox_t;
static chatBox_t *chatboxList = NULL;
static chatBox_t *currentChatbox = NULL;

static QINLINE int JP_GetChatboxFont( void ) {
	return Com_Clampi( FONT_SMALL, FONT_NUM_FONTS, cg_chatboxFont.integer );
}

//CHATBOX OBJECT API

static chatBox_t *CreateChatboxObject( const char *cbName )
{
	chatBox_t *cb = (chatBox_t *)malloc( sizeof( chatBox_t ) );
	memset( cb, 0, sizeof( chatBox_t ) );
	Q_strncpyz( cb->shortname, cbName, sizeof( cb->shortname ) );
	return cb;
}

static chatBox_t *GetChatboxByName( const char *cbName )
{
	chatBox_t *cb = chatboxList;
	chatBox_t *prev = cb;

	//No root object. Create one IMMEDIATELY
	if ( !cb )
		prev = cb = chatboxList = CreateChatboxObject( "normal" );

	// just return the default tab if we don't want multiple tabs
	if ( !cg_chatboxTabs.integer )
		return cb;

	while ( cb )
	{
		if ( !Q_stricmp( cbName, cb->shortname ) )
			return cb;

		prev = cb;
		cb = cb->next;
	}

	//Didn't find a match, create one
	// assume it's a secure channel
	cb = CreateChatboxObject( cbName );
	Q_strncpyz( cb->identifier, cbName, sizeof( cb->identifier ) );
	prev->next = cb; // Attach it to the list
	cb->prev = prev;

	return cb;
}

// these match g_cmds.c
#define EC			"\x19"
#define CHANNEL_EC	"\x10"
#define ADMIN_EC	"\x11"
#define PRIVATE_EC	"\x12"

qboolean CG_ContainsChannelEscapeChar( char *text )
{
	char *s = text, c=0;
	while ( (c = *s++) != '\0' )
	{
		if ( c == *CHANNEL_EC )
			return qtrue;
	}
	return qfalse;
}

char *CG_RemoveChannelEscapeChar( char *text )
{
#if 0
	static char ident[4][MAX_CHATBOX_IDENTIFIER_SIZE] = { {0} };
	static int index;
	char *identPos = strstr( text, CHANNEL_EC );
	char *colonPos = strstr( identPos, ":" );
	char *buf = (char *)&ident[index++ & 3];
	int identSize = identPos-colonPos;
	int endLen = strlen( colonPos );

	Q_strncpyz( buf, identPos, min( identSize, MAX_CHATBOX_IDENTIFIER_SIZE ) );
	Q_strncpyz( identPos, colonPos, endLen );

	return buf;
#else
	static char ident[4][MAX_CHATBOX_IDENTIFIER_SIZE] = { {0} };
	static int index;
	char *buf = (char *)&ident[index++ & 3];
	char *identPos = strstr( text, CHANNEL_EC )+1;
	char *identEnd = strstr( identPos, CHANNEL_EC );
	int identSize = (identEnd-identPos)+1;
	int endLen = strlen( identEnd );

	Q_strncpyz( buf, identPos, min( identSize, MAX_CHATBOX_IDENTIFIER_SIZE ) );
	Q_strncpyz( identPos-1, identEnd+1, endLen );

	return buf;
#endif
}

void JP_ChatboxInit( void )
{
	chatBox_t *cb = NULL;
	cb = currentChatbox = chatboxList = CreateChatboxObject( "normal" );
	if ( cgs.gametype >= GT_TEAM )
	{
		cb->next = CreateChatboxObject( "team" );
		cb->next->prev = cb;
		cb = cb->next;
	}
}

// This function is called recursively when a logical message has to be split into multiple lines
void JP_ChatboxAdd( const char *message, qboolean multiLine, char *cbName )
{
	chatBox_t *cb = GetChatboxByName( cbName );
	chatEntry_t *chat = &cb->chatBuffer[MAX_CHATBOX_ENTRIES-1];
	int strLength = 0;
	int i = 0;
	float accumLength = 0.0f; //cg_chatTimeStamp.integer ? CG_Text_Width( EXAMPLE_TIMESTAMP, cg_hudChatS.value, JP_GetChatboxFont() ) : 0.0f;
	char buf[CHAT_MESSAGE_LENGTH] = { 0 };
	struct tm *timeinfo;
	time_t tm;

	accumLength = cg_chatboxTimeShow.integer ? CG_Text_Width( EXAMPLE_TIMESTAMP_CLEAN, cg.chatbox.size.scale, JP_GetChatboxFont() ) : 0.0f;

	cb->numActiveLines++;

	if ( cb != currentChatbox )
		cb->notification = qtrue;

	//Stop scrolling up if we've already scrolled, similar to console behaviour
	if ( cb->scrollAmount < 0 )
		cb->scrollAmount = max( cb->scrollAmount - 1, cb->numActiveLines >= cg_chatboxLineCount.integer ? ((min(cb->numActiveLines,MAX_CHATBOX_ENTRIES)-cg_chatboxLineCount.integer)*-1) : 0 ); //cb->scrollAmount--;

	for ( i=0, strLength=strlen( message );
		 i<strLength && i<CHAT_MESSAGE_LENGTH;
		 i++ )
	{
		char *p = (char*)&message[i];
		Com_sprintf( buf, sizeof( buf ), "%c", *p );
		if ( !Q_IsColorString( p ) && (i > 0 && !Q_IsColorString( p-1 )) )
			accumLength += CG_Text_Width( buf, cg.chatbox.size.scale, JP_GetChatboxFont() );
		//HACK: Compensate for ^x effectively being 0
	//	if ( Q_IsColorString( p ) )
	//		accumLength -= CG_Text_Width( "^0",  chatVars.scale, JP_GetChatboxFont() );

		if ( accumLength > max( cg.chatbox.size.width, 192.0f ) && (i>0 && !Q_IsColorString( p-1 )) )
		{
			char lastColor = '2';
			int j = i;
			int savedOffset = i;
			char tempMessage[CHAT_MESSAGE_LENGTH];

			//Attempt to back-track, find a space (' ') within X characters or pixels
			//RAZTODO: Another method using character width? Meh
			while ( message[i] != ' ' )
			{
				if ( i <= 0 || i < savedOffset-16 )
				{
					i = j = savedOffset;
					break;
				}
				i--;
				j--;
			}

			memmove( &cb->chatBuffer[0], &cb->chatBuffer[1], sizeof( cb->chatBuffer ) - sizeof( chatEntry_t ) );
			memset( chat, 0, sizeof( chatEntry_t ) ); //Clear the last element, ready for writing
			Q_strncpyz( chat->message, message, i+1 );
			chat->time = cg.time + cg_chatbox.integer;

			if ( !multiLine )
			{//Insert time-stamp, only for entries on the first line
				if ( cg_chatboxTimeShow.integer == 1 )
				{
					time( &tm );
					timeinfo = localtime ( &tm );

					if ( !cg.japp.timestamp24Hour && timeinfo->tm_hour > 12 )
						timeinfo->tm_hour -= 12;

					Com_sprintf( chat->timeStamp, sizeof( chat->timeStamp ), "[^%c%02i:%02i:%02i^7] ", *(char *)cg_chatboxTimeColour.string, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
				}
				else if ( cg_chatboxTimeShow.integer == 2 )
				{
					int msec, seconds, mins, hours;

					msec	= cg.time-cgs.levelStartTime;	seconds = msec / 1000;
					mins	= seconds / 60;					seconds	-= mins * 60;
					hours	= mins / 60;					mins -= hours * 60;

					Com_sprintf( chat->timeStamp, sizeof( chat->timeStamp ), "[^%c%02i:%02i:%02i^7] ", *(char *)cg_chatboxTimeColour.string, hours, mins, seconds );
				}
			}

			chat->isUsed = qtrue;
			for ( j=i; j>=0; j-- )
			{
				if ( message[j] == '^' && message[j+1] >= '0' && message[j+1] <= '9' )
				{
					lastColor = message[j+1];
					break;
				}
			}
			Com_sprintf( tempMessage, sizeof( tempMessage ), "^%c%s", lastColor, (const char *)(message + i) );

			//Recursively insert until we don't have to split the message
			JP_ChatboxAdd( tempMessage, qtrue, cbName );
			return;
		}
	}

	memmove( &cb->chatBuffer[0], &cb->chatBuffer[1], sizeof( cb->chatBuffer ) - sizeof( chatEntry_t ) );
	memset( chat, 0, sizeof( chatEntry_t ) ); //Clear the last element, ready for writing
	Q_strncpyz( chat->message, message, i+1 );

	chat->time = cg.time + cg_chatbox.integer;

	if ( !multiLine )
	{//Insert time-stamp, only for entries on the first line
		if ( cg_chatboxTimeShow.integer == 1 )
		{
			time( &tm );
			timeinfo = localtime ( &tm );

			if ( !cg.japp.timestamp24Hour && timeinfo->tm_hour > 12 )
				timeinfo->tm_hour -= 12;

			Com_sprintf( chat->timeStamp, sizeof( chat->timeStamp ), "[^%c%02i:%02i:%02i^7] ", *(char *)cg_chatboxTimeColour.string, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
		}
		else if ( cg_chatboxTimeShow.integer == 2 )
		{
			int msec, seconds, mins, hours;

			msec	= cg.time-cgs.levelStartTime;	seconds = msec / 1000;
			mins	= seconds / 60;					seconds	-= mins * 60;
			hours	= mins / 60;					mins -= hours * 60;

			Com_sprintf( chat->timeStamp, sizeof( chat->timeStamp ), "[^%c%02i:%02i:%02i^7] ", *(char *)cg_chatboxTimeColour.string, hours, mins, seconds );
		}
	}

	chat->isUsed = qtrue;
}

static void DrawChatboxTabs( void )
{
	chatBox_t *cb = chatboxList;
	float xOffset = 0.0f;
	int cls_realtime = *(int *)0x8AF224;

	while ( cb )
	{
		char *name = cb->shortname;
		float textWidth = CG_Text_Width( name, cg.chatbox.size.scale, JP_GetChatboxFont() );
		float textHeight = CG_Text_Height( name, cg.chatbox.size.scale, JP_GetChatboxFont() );

		CG_FillRect( cg.chatbox.pos.x + xOffset, cg.chatbox.pos.y + (cg_chatboxLineHeight.value*cg_chatboxLineCount.integer) + (textHeight*0.25f), textWidth+16.0f, cg_chatboxLineHeight.value, (cb==currentChatbox) ? &colorTable[CT_DKGREY] : &colorTable[CT_BLACK] );
		CG_Text_Paint( cg.chatbox.pos.x + xOffset+8.0f, cg.chatbox.pos.y + (cg_chatboxLineHeight.value*cg_chatboxLineCount.integer), cg.chatbox.size.scale, &colorWhite, va( "^%c%s", (cb==currentChatbox) ? '2' : (cb->notification && ( (int)( cls_realtime >> 8 ) & 1 ) ) ? '1' : '7', cb->shortname ), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, JP_GetChatboxFont() );

		xOffset += textWidth + 16.0f;
		cb = cb->next;
	}
}

void JP_ChatboxDraw( void )
{
	int i = MAX_CHATBOX_ENTRIES-min( cg_chatboxLineCount.integer, currentChatbox->numActiveLines );
	int numLines = 0;
	int done = 0;
	chatEntry_t *last = NULL;

	if ( cg.scoreBoardShowing && !(cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION) )
		return;

	//i is the ideal index. Now offset for scrolling
	i += currentChatbox->scrollAmount;

//	CG_FillRect( cg_hudChatX.value, cg_hudChatY.value+1.75f, max( cg_hudChatW.value, 192.0f ), cg_chatLH.value*cg_chatLines.integer, backgroundColour );

	currentChatbox->notification = qfalse;

	if ( cg_chatboxTabs.integer )
		DrawChatboxTabs();

	if ( currentChatbox->numActiveLines == 0 )
		return;

	if ( currentChatbox->scrollAmount < 0 )
		CG_Text_Paint( cg.chatbox.pos.x, cg.chatbox.pos.y - (cg_chatboxLineHeight.value*1), cg.chatbox.size.scale, &colorWhite, va( "^3Scrolled lines: ^5%i\n", currentChatbox->scrollAmount*-1 ), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, JP_GetChatboxFont() );

	for ( done = 0; done<cg_chatboxLineCount.integer && i<MAX_CHATBOX_ENTRIES; i++, done++ )
	{//Check to see if background should be drawn
		chatEntry_t *chat = &currentChatbox->chatBuffer[i];
		if ( chat->isUsed && (chat->time >= cg.time-cg_chatbox.integer || currentChatbox->scrollAmount || cg_chatbox.integer == 1 || (trap->Key_GetCatcher() & KEYCATCH_MESSAGE)) )
		{
			CG_FillRect( cg.chatbox.pos.x, cg.chatbox.pos.y+1.75f, max( cg.chatbox.size.width, 192.0f ), cg_chatboxLineHeight.value*cg_chatboxLineCount.integer, &cg.chatbox.background );
			break;
		}
	}

	for ( done = 0; done<cg_chatboxLineCount.integer && i<MAX_CHATBOX_ENTRIES; i++, done++ )
	{
		chatEntry_t *chat = &currentChatbox->chatBuffer[i];
		if ( chat->isUsed )
		{
			last = chat;
			if ( chat->time >= cg.time-cg_chatbox.integer || currentChatbox->scrollAmount || cg_chatbox.integer == 1 || (trap->Key_GetCatcher() & KEYCATCH_MESSAGE) )
			{
				CG_Text_Paint( cg.chatbox.pos.x, cg.chatbox.pos.y + (cg_chatboxLineHeight.value * numLines), cg.chatbox.size.scale, &colorWhite, va( "%s%s", (cg_chatboxTimeShow.integer ? chat->timeStamp : ""), chat->message ), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, JP_GetChatboxFont() );
				numLines++;
			}
		}
	}

//	if ( last->isUsed && last->time < cg.time-cg_chatBox.integer && !currentChatbox->scrollAmount && cg_chatBox.integer > 1 )
//		CG_Text_Paint( cg_hudChatX.value, cg_hudChatY.value + (cg_chatLH.value * numLines), cg_hudChatS.value, colorWhite, va( "%s%s", (cg_chatTimeStamp.integer ? last->timeStamp : ""), last->message ), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, JP_GetChatboxFont() );
}

void JP_ChatboxScroll( int direction )
{
	int scrollAmount = currentChatbox->scrollAmount;
	int numActiveLines = currentChatbox->numActiveLines;

	if ( direction == 0 )
	{//down
		currentChatbox->scrollAmount = min( scrollAmount + 1, 0 );
	}
	else
	{//up
		currentChatbox->scrollAmount = max( scrollAmount - 1, numActiveLines >= cg_chatboxLineCount.integer ? ((min(numActiveLines,MAX_CHATBOX_ENTRIES)-cg_chatboxLineCount.integer)*-1) : 0 );
	}
}

void JP_ChatboxSelectTabNextNoKeys( void )
{
	if ( currentChatbox->next )
		currentChatbox = currentChatbox->next;
	else
		currentChatbox = chatboxList;
}

void JP_ChatboxSelectTabPrevNoKeys( void )
{
	if ( currentChatbox->prev )
		currentChatbox = currentChatbox->prev;
	else
	{
		chatBox_t *cb = chatboxList;
		while ( cb->next )
			cb = cb->next;
		currentChatbox = cb;
	}
}

void JP_ChatboxClear( void )
{
	currentChatbox->numActiveLines = 0;
	memset( currentChatbox->chatBuffer, 0, sizeof( currentChatbox->chatBuffer ) );
	currentChatbox->scrollAmount = 0;
}

void JP_ChatboxSelect( char *cbName )
{
	currentChatbox = GetChatboxByName( cbName );
}
