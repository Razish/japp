
#include "cg_local.h"
#include "ui/menudef.h"
#include "ui/keycodes.h"
#include "bg_luaevent.h"
#include "cg_media.h"
#include "ui/ui_shared.h"

// qcommon.h
#define	MAX_EDIT_LINE 256
typedef struct field_s {
	int		cursor, scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
} field_t;

#define MAX_CHATBOX_ENTRIES (512)
#define MAX_CHATBOX_IDENTIFIER_SIZE (32)

#define EXAMPLE_TIMESTAMP "[^800:00:00^8] "
#define TIMESTAMP_LENGTH sizeof(EXAMPLE_TIMESTAMP)

#define EXAMPLE_TIMESTAMP_CLEAN "[00:00:00] "
#define TIMESTAMP_LENGTH_CLEAN sizeof(EXAMPLE_TIMESTAMP_CLEAN)

#define CHAT_MESSAGE_LENGTH (MAX_SAY_TEXT)

//CHATBOX OBJECTS
typedef struct chatEntry_s {
	char		timeStamp[TIMESTAMP_LENGTH];
	char		message[CHAT_MESSAGE_LENGTH];
	qboolean	isUsed;
	int			time;

	struct urlLocation {
		size_t	start, length;
		vector2	pos, size;
		char	text[CHAT_MESSAGE_LENGTH];

		urlLocation *next;
	} *URLs;
} chatEntry_t;

typedef struct chatBox_s {
	// a logical message may be fragmented into multiple chatEntry_t structures, based on line width.
	//	this is to allow scrolling cleanly but has the down-side of line splitting only being computed once
	chatEntry_t			chatBuffer[MAX_CHATBOX_ENTRIES];

	int					numActiveLines; // totaly amount of lines used
	int					scrollAmount; // how many lines we've scrolled upwards
	qboolean			notification; // flashes when qtrue

	// for fetching the cb object
	char				shortname[MAX_CHATBOX_IDENTIFIER_SIZE]; // may be channel name, or preset names
	char				identifier[MAX_CHATBOX_IDENTIFIER_SIZE];
	struct chatBox_s	*prev, *next; // for scrolling
} chatBox_t;
static chatBox_t *chatboxList = NULL;
static chatBox_t *currentChatbox = NULL;

// chatbox history
typedef struct chatHistory_s {
	struct chatHistory_s *next, *prev;

	char message[MAX_EDIT_LINE];
} chatHistory_t;
static chatHistory_t *chatHistory = NULL;
static chatHistory_t *currentHistory = NULL;

// chatbox input
field_t chatField;
messageMode_e chatMode = CHAT_ALL;
static int chatTargetClient = -1;
static bool chatActive = false;

// for mouse input
static float inputLinePosY = 0.0f;
static float inputLineWidth = 0.0f;
static float inputLineHeight = 0.0f;
static float chatboxHeight = 0.0f;
static float chatboxWidth = 0.0f;

static int CG_GetChatboxFont( void ) {
	return Q_clampi( FONT_SMALL, cg_chatboxFont.integer, FONT_NUM_FONTS );
}

//CHATBOX OBJECT API

static chatBox_t *CG_CreateChatboxObject( const char *cbName ) {
	chatBox_t *cb = (chatBox_t *)malloc( sizeof(chatBox_t) );
	memset( cb, 0, sizeof(chatBox_t) );
	Q_strncpyz( cb->shortname, cbName, sizeof(cb->shortname) );
	return cb;
}

static chatBox_t *CG_GetChatboxByName( const char *cbName ) {
	chatBox_t *cb = chatboxList;
	chatBox_t *prev = cb;

	//No root object. Create one IMMEDIATELY
	if ( !cb )
		prev = cb = chatboxList = CG_CreateChatboxObject( "normal" );

	// just return the default tab if we don't want multiple tabs
	if ( !cg_chatboxTabs.integer )
		return cb;

	while ( cb ) {
		if ( !Q_stricmp( cbName, cb->shortname ) )
			return cb;

		prev = cb;
		cb = cb->next;
	}

	//Didn't find a match, create one
	// assume it's a secure channel
	cb = CG_CreateChatboxObject( cbName );
	Q_strncpyz( cb->identifier, cbName, sizeof(cb->identifier) );
	prev->next = cb; // Attach it to the list
	cb->prev = prev;

	return cb;
}

// these match g_cmds.c
#define EC			"\x19"
#define CHANNEL_EC	"\x10"
#define ADMIN_EC	"\x11"
#define PRIVATE_EC	"\x12"

qboolean CG_ContainsChannelEscapeChar( char *text ) {
	char *s = text, c = 0;
	//RAZTODO: strchr?
	while ( (c = *s++) != '\0' ) {
		if ( c == *CHANNEL_EC )
			return qtrue;
	}
	return qfalse;
}

char *CG_RemoveChannelEscapeChar( char *text ) {
#if 0
	static char ident[4][MAX_CHATBOX_IDENTIFIER_SIZE] = { { 0 } };
	static int index;
	char *identPos = strstr( text, CHANNEL_EC );
	char *colonPos = strstr( identPos, ":" );
	char *buf = (char *)&ident[index++ & 3];
	int identSize = identPos - colonPos;
	int endLen = strlen( colonPos );

	Q_strncpyz( buf, identPos, min( identSize, MAX_CHATBOX_IDENTIFIER_SIZE ) );
	Q_strncpyz( identPos, colonPos, endLen );

	return buf;
#else
	static char ident[4][MAX_CHATBOX_IDENTIFIER_SIZE] = { { 0 } };
	static int index;
	char *buf = (char *)&ident[index++ & 3];
	char *identPos = strstr( text, CHANNEL_EC ) + 1;
	char *identEnd = strstr( identPos, CHANNEL_EC );
	int identSize = (identEnd - identPos) + 1;
	int endLen = strlen( identEnd );

	Q_strncpyz( buf, identPos, std::min( identSize, MAX_CHATBOX_IDENTIFIER_SIZE ) );
	Q_strncpyz( identPos - 1, identEnd + 1, endLen );

	return buf;
#endif
}

void CG_ChatboxInit( void ) {
	chatBox_t *cb = NULL;
	cb = currentChatbox = chatboxList = CG_CreateChatboxObject( "normal" );
	if ( cgs.gametype >= GT_TEAM ) {
		cb->next = CG_CreateChatboxObject( "team" );
		cb->next->prev = cb;
	}
	memset( &chatField, 0, sizeof(chatField) );
}

static chatHistory_t *CG_CreateChatHistoryObject( const char *message ) {
	chatHistory_t *ch = (chatHistory_t *)malloc( sizeof(chatHistory_t) );
	memset( ch, 0, sizeof(chatHistory_t) );
	Q_strncpyz( ch->message, message, sizeof(ch->message) );
	return ch;
}

static chatHistory_t *CG_GetNewChatHistory( const char *message ) {
	chatHistory_t *ch = NULL, *prev = chatHistory;

	ch = CG_CreateChatHistoryObject( message );
	if ( prev )
		chatHistory = prev->prev = ch; // Attach it to the start of the list
	else
		chatHistory = ch;

	ch->next = prev;
	return ch;
}

void CG_ChatboxOutgoing( void ) {
	char *msg = chatField.buffer;

	// remove the key catcher
	CG_ChatboxClose();

	// commit the current line to history
	CG_GetNewChatHistory( chatField.buffer );

	msg = JPLua::Event_ChatMessageSent( msg, chatMode, chatTargetClient );

	// lua event ate it
	if ( !msg || !msg[0] )
		return;

	switch ( chatMode ) {
	default:
	case CHAT_ALL:
		trap->SendClientCommand( va( "say %s", msg ) );
		break;
	case CHAT_TEAM:
		trap->SendClientCommand( va( "say_team %s", msg ) );
		break;
	case CHAT_WHISPER:
		trap->SendClientCommand( va( "tell %i %s", chatTargetClient, msg ) );
		break;
	}
}

static const char *preMatches[] = {
	"www.",
	"http://",
	"https://"
};
static const char *postMatches[] = {
	".com",
	".org",
	".net"
};

// returns 0 if no URLs were found
// else return an offset into message where the first URL is located
// call multiple times to parse multiple urls
static size_t CG_ParseURLs( char *message ) {
	char scratch[CHAT_MESSAGE_LENGTH];
	std::memcpy( scratch, message, sizeof(scratch) );

	const char *delim = " ";
	for ( char *p = strtok( scratch, delim ); p; p = strtok( NULL, delim ) ) {
		// skip colour codes
		while ( Q_IsColorString( p ) ) {
			p += 2;
		}

		ptrdiff_t offset = p - scratch;
		size_t len = strlen( p );
		for ( const char *match : preMatches ) {
			size_t matchLen = strlen( match );
			if ( len < matchLen ) {
				break;
			}
			if ( !Q_strncmp( p, match, matchLen ) ) {
				// got a pre match, try to verify with a post match
				for ( char *dot = p; (dot = strchr( dot, '.' )) != nullptr; dot++ ) {
					for ( const char *match : postMatches ) {
						size_t matchLen = strlen( match );
						if ( len < matchLen ) {
							break;
						}
						if ( !Q_strncmp( dot, match, matchLen ) ) {
							return offset;
						}
					}
				}
			}
		}

		// no pre matches, can still try post matches
		for ( char *dot = p; (dot = strchr( dot, '.' )) != nullptr; dot++ ) {
			for ( const char *match : postMatches ) {
				size_t matchLen = strlen( match );
				if ( len < matchLen ) {
					break;
				}
				if ( !Q_strncmp( dot, match, matchLen ) ) {
					return offset;
				}
			}
		}
	}

	// no pre or post match
	return 0u;
}

// This function is called recursively when a logical message has to be split into multiple lines
void CG_ChatboxAddMessage( const char *message, qboolean multiLine, const char *cbName ) {
	chatBox_t *cb = CG_GetChatboxByName( cbName );
	chatEntry_t *chat = &cb->chatBuffer[MAX_CHATBOX_ENTRIES - 1];
	int strLength = 0;
	int i = 0;
	float accumLength = 0.0f;
	char buf[CHAT_MESSAGE_LENGTH] = { 0 };
	struct tm *timeinfo;
	time_t tm;

	accumLength = cg_chatboxTimeShow.integer
		? Text_Width( EXAMPLE_TIMESTAMP_CLEAN, cg.chatbox.size.scale, CG_GetChatboxFont(), false )
		: 0.0f;

	cb->numActiveLines++;

	if ( cb != currentChatbox ) {
		cb->notification = qtrue;
	}

	// Stop scrolling up if we've already scrolled, similar to console behaviour
	if ( cb->scrollAmount < 0 ) {
		cb->scrollAmount = std::max( cb->scrollAmount - 1, cb->numActiveLines >= cg_chatboxLineCount.integer
			? ( (std::min( cb->numActiveLines, MAX_CHATBOX_ENTRIES ) - cg_chatboxLineCount.integer) * -1 )
			: 0 );
	}

	for ( i = 0, strLength = strlen( message ); i<strLength && i<CHAT_MESSAGE_LENGTH; i++ ) {
		char *p = (char*)&message[i];
		Com_sprintf( buf, sizeof(buf), "%c", *p );
		if ( !Q_IsColorString( p ) && (i > 0 && !Q_IsColorString( p - 1 )) ) {
			accumLength += Text_Width( buf, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
		}

		if ( accumLength > std::max( cg.chatbox.size.width, 128 ) && (i > 0 && !Q_IsColorString( p - 1 )) ) {
			char lastColor = COLOR_GREEN;
			int j = i;
			int savedOffset = i;
			char tempMessage[CHAT_MESSAGE_LENGTH];

			//Attempt to back-track, find a space (' ') within X characters or pixels
			//RAZTODO: Another method using character width? Meh
			while ( message[i] != ' ' ) {
				if ( i <= 0 || i < savedOffset - 16 ) {
					i = j = savedOffset;
					break;
				}
				i--;
				j--;
			}

			memmove( &cb->chatBuffer[0], &cb->chatBuffer[1], sizeof(cb->chatBuffer) - sizeof(chatEntry_t) );
			memset( chat, 0, sizeof(chatEntry_t) ); //Clear the last element, ready for writing
			Q_strncpyz( chat->message, message, i + 1 );
			chat->time = cg.time + cg_chatbox.integer;

			// Insert time-stamp, only for entries on the first line
			if ( !multiLine ) {
				// local time
				if ( cg_chatboxTimeShow.integer == 1 ) {
					time( &tm );
					timeinfo = localtime( &tm );

					if ( !cg.japp.timestamp24Hour && timeinfo->tm_hour > 12 ) {
						timeinfo->tm_hour -= 12;
					}

					Com_sprintf( chat->timeStamp, sizeof(chat->timeStamp), "[^%c%02i:%02i:%02i" S_COLOR_WHITE "] ",
						*(char *)cg_chatboxTimeColour.string, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
				}
				// server time
				else if ( cg_chatboxTimeShow.integer == 2 ) {
					int msec, seconds, mins, hours;

					msec = cg.time - cgs.levelStartTime;	seconds = msec / 1000;
					mins = seconds / 60;					seconds -= mins * 60;
					hours = mins / 60;					mins -= hours * 60;

					Com_sprintf( chat->timeStamp, sizeof(chat->timeStamp), "[^%c%02i:%02i:%02i" S_COLOR_WHITE "] ",
						*(char *)cg_chatboxTimeColour.string, hours, mins, seconds );
				}
			}

			chat->isUsed = qtrue;
			for ( j = i; j >= 0; j-- ) {
				const char *tmp = &message[i];
				if ( Q_IsColorString( tmp ) ) {// == '^' && message[j+1] >= '0' && message[j+1] <= '9' )
					lastColor = message[j + 1];
					break;
				}
			}
			Com_sprintf( tempMessage, sizeof(tempMessage), "^%c%s", lastColor, (const char *)(message + i) );

			//Recursively insert until we don't have to split the message
			CG_ChatboxAddMessage( tempMessage, qtrue, cbName );
			return;
		}
	}

	memmove( &cb->chatBuffer[0], &cb->chatBuffer[1], sizeof(cb->chatBuffer) - sizeof(chatEntry_t) );
	memset( chat, 0, sizeof(chatEntry_t) ); //Clear the last element, ready for writing
	Q_strncpyz( chat->message, message, i + 1 );

	size_t offset = 0u, tmpOffset = 0u;
	while ( (tmpOffset = CG_ParseURLs( chat->message + offset )) != 0u ) {
		chatEntry_t::urlLocation *loc = new chatEntry_t::urlLocation{};
		loc->start = offset + tmpOffset;
		offset = tmpOffset;
		const char *p = strchr( chat->message + offset, ' ' );
		if ( p ) {
			loc->length = p - (chat->message + offset) + 1;
		}
		else {
			loc->length = strlen( chat->message + offset ) + 1;
		}
		Q_strncpyz( loc->text, chat->message + loc->start, loc->length );
		Q_CleanString( loc->text, STRIP_COLOUR );
		loc->next = chat->URLs;
		chat->URLs = loc;
	}

	chat->time = cg.time + cg_chatbox.integer;

	// Insert time-stamp, only for entries on the first line
	if ( !multiLine ) {
		// local time
		if ( cg_chatboxTimeShow.integer == 1 ) {
			time( &tm );
			timeinfo = localtime( &tm );

			if ( !cg.japp.timestamp24Hour && timeinfo->tm_hour > 12 )
				timeinfo->tm_hour -= 12;

			Com_sprintf( chat->timeStamp, sizeof(chat->timeStamp), "[^%c%02i:%02i:%02i" S_COLOR_WHITE "] ",
				*(char *)cg_chatboxTimeColour.string, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
		}
		// server time
		else if ( cg_chatboxTimeShow.integer == 2 ) {
			int msec, seconds, mins, hours;

			msec = cg.time - cgs.levelStartTime;	seconds = msec / 1000;
			mins = seconds / 60;					seconds -= mins * 60;
			hours = mins / 60;					mins -= hours * 60;

			Com_sprintf( chat->timeStamp, sizeof(chat->timeStamp), "[^%c%02i:%02i:%02i" S_COLOR_WHITE "] ",
				*(char *)cg_chatboxTimeColour.string, hours, mins, seconds );
		}
	}

	chat->isUsed = qtrue;
}

static void CG_ChatboxDrawTabs( void ) {
	chatBox_t *cb = chatboxList;
	float xOffset = 0.0f;

	while ( cb ) {
		const char *name = cb->shortname;
		float textWidth = Text_Width( name, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
		float textHeight = Text_Height( name, cg.chatbox.size.scale, CG_GetChatboxFont(), false );

		CG_FillRect( cg.chatbox.pos.x + xOffset,
			cg.chatbox.pos.y + (cg_chatboxLineHeight.value * cg_chatboxLineCount.integer) + (textHeight * 0.25f),
			textWidth + 16.0f, cg_chatboxLineHeight.value,
			(cb == currentChatbox) ? &colorTable[CT_DKGREY] : &colorTable[CT_BLACK] );

		Text_Paint( cg.chatbox.pos.x + xOffset + 8.0f,
			cg.chatbox.pos.y + (cg_chatboxLineHeight.value * cg_chatboxLineCount.integer),
			cg.chatbox.size.scale, &colorWhite, va( "^%c%s", (cb == currentChatbox)
			? COLOR_GREEN
			: (cb->notification && ((int)(trap->Milliseconds() >> 8) & 1)) ? COLOR_RED : COLOR_WHITE, cb->shortname ),
			0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, CG_GetChatboxFont(), false );

		xOffset += textWidth + 16.0f;
		cb = cb->next;
	}
}

static const char *GetPreText( qboolean clean ) {
	switch ( chatMode ) {
	default:
	case CHAT_ALL:
		return clean
			? "Say: "
			: "Say: %s";

	case CHAT_TEAM:
		return clean
			? "Team: "
			: "Team: %s";

	case CHAT_WHISPER:
		if ( clean ) {
			char name[MAX_NETNAME];
			Q_strncpyz( name, cgs.clientinfo[chatTargetClient].name, sizeof(name) );
			Q_CleanString( name, STRIP_COLOUR );
			return va( "Tell %s: ", name );
		}
		return va( "Tell %s: %%s", cgs.clientinfo[chatTargetClient].name );
	}
}

void CG_ChatboxDraw( void ) {
	int i = MAX_CHATBOX_ENTRIES - std::min( cg_chatboxLineCount.integer, currentChatbox->numActiveLines );
	int numLines = 0, done = 0, j = 0;
	//chatEntry_t *last = NULL;
	bool skipDraw = cg.scoreBoardShowing && !(cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION);

	//i is the ideal index. Now offset for scrolling
	i += currentChatbox->scrollAmount;

	currentChatbox->notification = qfalse;

	if ( cg_chatboxTabs.integer ) {
		CG_ChatboxDrawTabs();
	}

	if ( currentChatbox->numActiveLines == 0 ) {
		skipDraw = true;
	}

	float yAccum = 0.0f;

	const char *scrollMsg = va( S_COLOR_YELLOW "Scrolled lines: " S_COLOR_CYAN "%i\n",
		currentChatbox->scrollAmount * -1
	);
	float height = Text_Height( scrollMsg, cg.chatbox.size.scale, CG_GetChatboxFont(), false );

	if ( !skipDraw && currentChatbox->scrollAmount < 0 && CG_ChatboxActive() ) {
		Text_Paint( cg.chatbox.pos.x, cg.chatbox.pos.y + yAccum - (height / 2.0f),
			cg.chatbox.size.scale, &colorWhite, scrollMsg, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, CG_GetChatboxFont(), false
		);
	}
	yAccum += height;

	// accumulate line heights
	chatboxWidth = 0.0f;
	chatboxHeight = 0.0f;
	if ( !skipDraw ) {
		int saved = i;
		for ( done = 0; done < cg_chatboxLineCount.integer && i < MAX_CHATBOX_ENTRIES; i++, done++ ) {
			chatEntry_t *chat = &currentChatbox->chatBuffer[i];
			if ( chat->isUsed ) {
				const char *msg = va( "%s%s", (cg_chatboxTimeShow.integer ? chat->timeStamp : ""), chat->message );
				chatboxHeight += Text_Height( msg, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
				float tmp = Text_Width( msg, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
				if ( tmp > chatboxWidth ) {
					chatboxWidth = tmp;
				}
			}
		}
		i = saved;
	}

	// check to see if background should be drawn
	if ( !skipDraw ) {
		if ( cg.chatbox.background.a > 0.0f ) {
			for ( done = 0; done < cg_chatboxLineCount.integer && i < MAX_CHATBOX_ENTRIES; i++, done++ ) {
				chatEntry_t *chat = &currentChatbox->chatBuffer[i];
				if ( chat->isUsed
					&& (chat->time >= cg.time - cg_chatbox.integer
						|| currentChatbox->scrollAmount
						|| CG_ChatboxActive()) )
				{
					CG_FillRect( cg.chatbox.pos.x, cg.chatbox.pos.y + yAccum, cg.chatbox.size.width/*chatboxWidth*/,
						chatboxHeight, &cg.chatbox.background
					);
					break;
				}
			}
		}
	}

	// draw each line
	if ( !skipDraw ) {
		for ( done = 0; done < cg_chatboxLineCount.integer && i < MAX_CHATBOX_ENTRIES; i++, done++ ) {
			chatEntry_t *chat = &currentChatbox->chatBuffer[i];
			if ( chat->isUsed ) {
				//	last = chat;
				if ( chat->time >= cg.time - cg_chatbox.integer
					|| (currentChatbox->scrollAmount && CG_ChatboxActive())
					|| CG_ChatboxActive() )
				{
					const char *tmp = va( "%s%s", (cg_chatboxTimeShow.integer ? chat->timeStamp : ""), chat->message );

					// retrieve any stored URL positions
					const float height = Text_Height( tmp, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
					//const float width = Text_Width( tmp, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
					Text_Paint(
						cg.chatbox.pos.x, cg.chatbox.pos.y + yAccum - (height / 2.0f),
						cg.chatbox.size.scale, &colorWhite, tmp, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, CG_GetChatboxFont(),
						false
					);
					for ( chatEntry_t::urlLocation *url = chat->URLs; url; url = url->next ) {
						char scratch[CHAT_MESSAGE_LENGTH];
						Q_strncpyz( scratch, tmp, url->start + 1 );

						//FIXME: somehow this isn't accurate when using r_aspectCorrectFonts?!
						url->pos.x = cg.chatbox.pos.x
							+ Text_Width( scratch, cg.chatbox.size.scale, CG_GetChatboxFont(), false );

						Q_strncpyz( scratch, tmp + url->start, url->length );
						url->size.x = Text_Width( scratch, cg.chatbox.size.scale, CG_GetChatboxFont(), false );

						url->pos.y = cg.chatbox.pos.y + yAccum - (height / 2.0f);
						url->size.y = height;

						Text_Paint( url->pos.x, url->pos.y, cg.chatbox.size.scale, &colorTable[CT_LTBLUE2], scratch,
							0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, CG_GetChatboxFont(), false
						);
						CG_FillRect( url->pos.x, url->pos.y + height + 2.0f, url->size.x, 1.0f, &colorTable[CT_LTBLUE2] );
					}
					yAccum += height;
					numLines++;
				}
			}
		}
	}

	// draw the input line
	if ( CG_ChatboxActive() ) {
		inputLinePosY = cg.chatbox.pos.y + yAccum;

		const char *pre = GetPreText( qfalse );
		const char *cleanPre = GetPreText( qtrue );
		char msg[MAX_EDIT_LINE];
		Com_sprintf( msg, sizeof(msg), pre, chatField.buffer );
		const float height = Text_Height( msg, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
		inputLineHeight = height;
		inputLineWidth = Text_Width( msg, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
		Text_Paint( cg.chatbox.pos.x, cg.chatbox.pos.y + yAccum - (height / 2.0f), cg.chatbox.size.scale,
			&g_color_table[ColorIndex( COLOR_WHITE )], msg, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, CG_GetChatboxFont(), false
		);
		if ( ((trap->Milliseconds() >> 8) & 1) ) {
			const float cursorPre = Text_Width( cleanPre, cg.chatbox.size.scale, CG_GetChatboxFont(), false );
			float cursorOffset = 0.0f;

			Q_CleanString( msg, STRIP_COLOUR );
			for ( j = 0; j < chatField.cursor; j++ ) {
				cursorOffset += Text_Width( va( "%c", msg[j] ), cg.chatbox.size.scale, CG_GetChatboxFont(), false );
			}

			const float cursorHeight = Text_Height( "_", cg.chatbox.size.scale, CG_GetChatboxFont(), false );
			Text_Paint( cg.chatbox.pos.x + cursorPre + cursorOffset, cg.chatbox.pos.y + yAccum - (cursorHeight / 2.0f),
				cg.chatbox.size.scale, &g_color_table[ColorIndex( COLOR_WHITE )], "_", 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED,
				CG_GetChatboxFont(), false
			);
		}
		yAccum += height;
	}
}

void CG_ChatboxScroll( int direction ) {
	int scrollAmount = currentChatbox->scrollAmount;
	int numActiveLines = currentChatbox->numActiveLines;

	// down
	if ( direction == 0 ) {
		currentChatbox->scrollAmount = std::min( scrollAmount + 1, 0 );
	}
	// up
	else {
		currentChatbox->scrollAmount = std::max( scrollAmount - 1, numActiveLines >= cg_chatboxLineCount.integer
			? ((std::min( numActiveLines, MAX_CHATBOX_ENTRIES ) - cg_chatboxLineCount.integer) * -1)
			: 0 );
	}
}

void CG_ChatboxTabComplete( void ) {
	if ( cg_chatboxCompletion.integer ) {
		int i = 0, match = -1, numMatches = 0;
		char currWord[MAX_INFO_STRING] = { 0 };
		char matches[MAX_CLIENTS][MAX_NETNAME] = { { 0 } }; // because cgs.clientinfo[i].name uses MAX_QPATH...wtf...
		char *p = &chatField.buffer[0];//[chatField->cursor];

		p = &chatField.buffer[chatField.cursor];
		//find current word
		while ( p > &chatField.buffer[0] && *(p - 1) != ' ' ) {
			p--;
		}

		if ( !*p ) {
			return;
		}

		Q_strncpyz( currWord, p, sizeof(currWord) );
		Q_CleanString( currWord, STRIP_COLOUR );
		Q_strlwr( currWord );

		for ( i = 0; i < cgs.maxclients; i++ ) {
			if ( cgs.clientinfo[i].infoValid ) {
				char name[MAX_QPATH/*MAX_NETNAME*/] = { 0 }; // because cgs.clientinfo[i].name uses MAX_QPATH...wtf...

				Q_strncpyz( name, cgs.clientinfo[i].name, sizeof(name) );
				Q_CleanString( name, STRIP_COLOUR );
				Q_strlwr( name );
				if ( strstr( name, currWord ) ) {
					match = i;
					Q_strncpyz( matches[numMatches++], cgs.clientinfo[i].name, sizeof(matches[0]) );
				}
			}
		}

		if ( numMatches == 1 ) {
			size_t oldCursor = chatField.cursor;
			ptrdiff_t delta = &chatField.buffer[oldCursor] - p;
			const char *str = va( "%s " S_COLOR_GREEN, cgs.clientinfo[match].name );
			size_t drawLen, len;

			Q_strncpyz( p, str, sizeof(chatField.buffer) - (p - &chatField.buffer[0]) );
			chatField.cursor = oldCursor - delta + strlen( str );

			//make sure cursor is visible
			drawLen = chatField.widthInChars - 1;
			len = strlen( chatField.buffer );
			if ( chatField.scroll + drawLen > len ) {
				chatField.scroll = (int)(len - drawLen);
				if ( chatField.scroll < 0 ) {
					chatField.scroll = 0;
				}
			}
		}
		else if ( numMatches > 1 ) {
			CG_ChatboxAddMessage( va( "Several matches found for '%s':", currWord ), qfalse, "normal" );
			for ( i = 0; i </*min( 3, numMatches )*/numMatches; i++ ) {
				CG_ChatboxAddMessage( va( S_COLOR_GREEN "- " S_COLOR_WHITE "%s", matches[i] ), qfalse, "normal" );
			}
			/*
			if ( numMatches > 3 ) {
				CG_ChatboxAddMessage( S_COLOR_GREEN "- " S_COLOR_WHITE" [...] truncated", qfalse, "normal" );
			}
			*/
			trap->S_StartLocalSound( media.sounds.interface.talk, CHAN_LOCAL_SOUND );
		}
	}
}

void CG_ChatboxSelectTabNextNoKeys( void ) {
	currentChatbox = currentChatbox->next ? currentChatbox->next : chatboxList;
}

void CG_ChatboxSelectTabPrevNoKeys( void ) {
	if ( currentChatbox->prev ) {
		currentChatbox = currentChatbox->prev;
	}
	else {
		chatBox_t *cb = chatboxList;
		while ( cb->next ) {
			cb = cb->next;
		}
		currentChatbox = cb;
	}
}

void CG_ChatboxSelect( char *cbName ) {
	currentChatbox = CG_GetChatboxByName( cbName );
}

void CG_ChatboxHistoryUp( void ) {
	if ( currentHistory ) {
		if ( currentHistory->next ) {
			currentHistory = currentHistory->next;
		}
	}
	else if ( chatHistory ) {
		currentHistory = chatHistory;
	}
	else {
		return;
	}

	Q_strncpyz( chatField.buffer, currentHistory->message, sizeof(chatField.buffer) );
	chatField.cursor = strlen( chatField.buffer );
}

static void Field_Clear( field_t *edit ) {
	edit->buffer[0] = 0;
	edit->cursor = 0;
	edit->scroll = 0;
}

void CG_ChatboxHistoryDn( void ) {
	if ( currentHistory ) {
		currentHistory = currentHistory->prev;
	}
	if ( currentHistory ) {
		Q_strncpyz( chatField.buffer, currentHistory->message, sizeof(chatField.buffer) );
		chatField.cursor = strlen( chatField.buffer );
	}
	else {
		Field_Clear( &chatField );
	}
}

void CG_ChatboxClear( void ) {
	if ( !trap->Key_IsDown( A_CTRL ) ) {
		return;
	}

	currentChatbox->numActiveLines = 0;
	memset( currentChatbox->chatBuffer, 0, sizeof(currentChatbox->chatBuffer) );
	currentChatbox->scrollAmount = 0;

	//TODO: Clear chat history?
}

void CG_ChatboxOpen( messageMode_e mode ) {
	if ( chatActive ) {
		return;
	}

	switch ( mode ) {
	case CHAT_ALL:
	case CHAT_TEAM:
		chatTargetClient = -1;
		break;

	case CHAT_WHISPER:
		if ( (chatTargetClient = CG_CrosshairPlayer()) == -1 ) {
			return;
		}
		break;

	default:
		break;
	}

	chatActive = true;
	chatMode = mode;
	Field_Clear( &chatField );
	trap->Key_SetCatcher( trap->Key_GetCatcher() | KEYCATCH_CGAME );
}

void CG_ChatboxClose( void ) {
	chatActive = false;
	trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_CGAME );
}

static void Field_CharEvent( field_t *edit, int key ) {
	int fieldLen = strlen( edit->buffer );

	//key = tolower( key );
	//trap->Print( JAPP_FUNCTION " %d\n", key );

	// handle shortcuts
	//RAZTODO: advanced text editing
	if ( trap->Key_IsDown( A_CTRL ) ) {
		switch ( key ) {
		case 'a': // home
			edit->cursor = 0;
			edit->scroll = 0;
			break;

		case A_BACKSPACE: // clear
		case 'c': // clear
			Field_Clear( edit );
			break;

		case 'e': // end
			edit->cursor = fieldLen;
			edit->scroll = edit->cursor - edit->widthInChars;
			break;

		case 'h': // backspace
			if ( edit->cursor > 0 ) {
				memmove( edit->buffer + edit->cursor - 1, edit->buffer + edit->cursor, fieldLen + 1 - edit->cursor );
				edit->cursor--;
				if ( edit->cursor < edit->scroll ) {
					edit->scroll--;
				}
			}
			break;

		case 'v': // paste
			//RAZTODO: Field_Paste
			break;

		default:
			break;
		}

		return;
	}

	if ( !(key & K_CHAR_FLAG) ) {
		switch ( key ) {
		case A_ENTER:
		case A_KP_ENTER:
			CG_ChatboxOutgoing();
			break;

		case A_TAB:
			CG_ChatboxTabComplete();
			break;

		case A_PAGE_DOWN:
		case A_MWHEELDOWN:
			CG_ChatboxScroll( 0 );
			break;

		case A_PAGE_UP:
		case A_MWHEELUP:
			CG_ChatboxScroll( 1 );
			break;

		case A_CURSOR_UP:
			CG_ChatboxHistoryUp();
			break;

		case A_CURSOR_DOWN:
			CG_ChatboxHistoryDn();
			break;

		case A_CURSOR_RIGHT:
			if ( edit->cursor < fieldLen ) {
				edit->cursor++;
			}
			break;

		case A_CURSOR_LEFT:
			if ( edit->cursor > 0 ) {
				edit->cursor--;
			}
			break;

		case A_INSERT:
			//RAZTODO: chatbox overstrike mode
			//kg.key_overstrikeMode = (qboolean)!kg.key_overstrikeMode;
			break;

		case A_HOME:
			edit->cursor = 0;
			edit->scroll = 0;
			break;

		case A_END:
			edit->cursor = fieldLen;
			edit->scroll = edit->cursor - edit->widthInChars;
			break;

		case A_BACKSPACE:
			if ( edit->cursor > 0 ) {
				memmove( edit->buffer + edit->cursor - 1, edit->buffer + edit->cursor, fieldLen + 1 - edit->cursor );
				edit->cursor--;
				if ( edit->cursor < edit->scroll ) {
					edit->scroll--;
				}
			}
			break;

		case A_DELETE:
			if ( edit->cursor < fieldLen ) {
				memmove( edit->buffer + edit->cursor, edit->buffer + edit->cursor + 1, fieldLen - edit->cursor );
			}
			break;

		case A_MOUSE1:
			if ( Q_PointInBounds( cgs.cursorX, cgs.cursorY, cg.chatbox.pos.x, inputLinePosY,
					inputLineWidth, inputLineHeight ) )
			{
				//TODO: positioning cursor
				//TODO: drag-selection
			}
			if ( Q_PointInBounds( cgs.cursorX, cgs.cursorY, cg.chatbox.pos.x, cg.chatbox.pos.y,
					chatboxWidth, chatboxHeight ) )
			{
				// check for URLs
				int i = MAX_CHATBOX_ENTRIES - std::min( cg_chatboxLineCount.integer, currentChatbox->numActiveLines );
				int done = 0;
				i += currentChatbox->scrollAmount;
				for ( done = 0; done < cg_chatboxLineCount.integer && i < MAX_CHATBOX_ENTRIES; i++, done++ ) {
					chatEntry_t *chat = &currentChatbox->chatBuffer[i];
					if ( chat->isUsed ) {
						for ( const chatEntry_t::urlLocation *url = chat->URLs; url; url = url->next ) {
							if ( Q_PointInBounds( cgs.cursorX, cgs.cursorY, url->pos.x, url->pos.y, url->size.x,
								url->size.y ) )
							{
								Q_OpenURL( url->text );
							}
						}
					}
				}
			}
			break;

		default:
			break;
		}

		// Change scroll if cursor is no longer visible
		if ( edit->cursor < edit->scroll ) {
			edit->scroll = edit->cursor;
		}
		else if ( edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= fieldLen ) {
			edit->scroll = edit->cursor - edit->widthInChars + 1;
		}

		return;
	}

	key &= ~K_CHAR_FLAG;

	// ignore any other non printable chars
	if ( key < 32 ) {
		return;
	}

	//RAZTODO: chatbox overstrike mode
	// - 2 to leave room for the leading slash and trailing \0
	if ( fieldLen == MAX_EDIT_LINE - 1 ) {
		return; // all full
	}
	memmove( edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor, fieldLen + 1 - edit->cursor );
	edit->buffer[edit->cursor++] = key;

	if ( edit->cursor >= edit->widthInChars ) {
		edit->scroll++;
	}

	if ( edit->cursor == fieldLen + 1 ) {
		edit->buffer[edit->cursor] = 0;
	}
}

void CG_ChatboxChar( int key ) {
	Field_CharEvent( &chatField, key );
}

qboolean CG_ChatboxActive( void ) {
	return chatActive && (trap->Key_GetCatcher() & KEYCATCH_CGAME);
}
