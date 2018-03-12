
#include "cg_local.h"
#include "ui/menudef.h"
#include "ui/keycodes.h"
#include "bg_luaevent.h"
#include "cg_media.h"
#include "ui/ui_shared.h"
#include "ui/ui_fonts.h"

// qcommon.h
#define	MAX_EDIT_LINE 256
typedef struct field_s {
	int		cursor, scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
} field_t;

#define MAX_CHATBOX_ENTRIES (512)

#define EXAMPLE_TIMESTAMP "[^800:00:00^8] "
#define TIMESTAMP_LENGTH sizeof(EXAMPLE_TIMESTAMP)

#define EXAMPLE_TIMESTAMP_CLEAN "[00:00:00] "
#define TIMESTAMP_LENGTH_CLEAN sizeof(EXAMPLE_TIMESTAMP_CLEAN)

//CHATBOX OBJECTS
typedef struct chatEntry_s {
	char		timeStamp[TIMESTAMP_LENGTH];
	char		message[MAX_SAY_TEXT];
	qboolean	isUsed;
	int			time;

	struct urlLocation {
		size_t	start, length;
		vector2	pos, size;
		char	text[MAX_SAY_TEXT];

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
static bool overstrike = false;
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

	// no root object - create one immediately
	if ( !cb ) {
		prev = cb = chatboxList = CG_CreateChatboxObject( "normal" );
	}

	// just return the default tab if we don't want multiple tabs
	if ( !cg_chatboxTabs.integer ) {
		return cb;
	}

	while ( cb ) {
		if ( !Q_stricmp( cbName, cb->shortname ) ) {
			return cb;
		}

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

// these match g_cmds.cpp
#define SAY_EC		"\x19" // end of medium
#define TAB_EC		"\x11" // device control 1
#define UNUSED02_EC	"\x12" // devuce control 2
#define UNUSED01_EC	"\x13" // devuce control 3
#define MOD_EC		"\x14" // device control 4

bool CG_ContainsChatTabEscapeChar( const char *text ) {
	return strchr( text, *TAB_EC ) != nullptr;
}

const char *CG_ExtractChatTabEscapeChar( const char *text ) {
	static char ident[4][MAX_CHATBOX_IDENTIFIER_SIZE] = { { 0 } };
	static int index;
	char *buf = (char *)&ident[index++ & 3];
	const char *identPos = strstr( text, TAB_EC ) + 1;
	const char *identEnd = strstr( identPos, TAB_EC );
	int identSize = (identEnd - identPos) + 1;
	//int endLen = strlen( identEnd );

	Q_strncpyz( buf, identPos, std::min( identSize, MAX_CHATBOX_IDENTIFIER_SIZE ) );
	//Q_strncpyz( identPos - 1, identEnd + 1, endLen );

	return buf;
}

void CG_RemoveChatEscapeChars( char *text ) {
#if 0
	int wi = 0;
	for ( int ri = 0; text[ri]; ri++ ) {
		if ( text[ri] != *SAY_EC && text[ri] != *TAB_EC && text[ri] != *UNUSED02_EC && text[ri] != *UNUSED01_EC && text[ri] != *MOD_EC ) {
			text[wi++] = text[ri];
		}
	}
	text[wi] = '\0';
#endif
	Q_strstrip( text, SAY_EC TAB_EC UNUSED02_EC UNUSED01_EC MOD_EC, nullptr );
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

	// pass to lua event, which may consume it
	msg = JPLua::Event_ChatMessageSent( msg, chatMode, chatTargetClient, currentChatbox->shortname, currentChatbox->identifier );
	if ( !msg || !msg[0] ) {
		return;
	}

	// hijack global say when using tabbed chatbox
	if ( chatMode == CHAT_ALL ) {
		if ( currentChatbox->shortname[0] ) {
			if ( !strcmp( currentChatbox->shortname, "normal" ) ) {
				// global say to "normal" is default behaviour, let it go
				assert( chatTargetClient == -1 );
				//chatMode = CHAT_ALL;
			}
			else if ( !strcmp( currentChatbox->shortname, "team" ) ) {
				// redirect global say from "team" tab
				chatMode = CHAT_TEAM;
			}
			else {
				// this will happen if we're using global reply to an unknown tab
				// we will hazard a guess that this is meant to be a private message
				// if that fails, will fall back to team chat and hope the server can work some redirect magic (e.g. say_team_mod)w

				// check if "@username"
				if ( currentChatbox->shortname[0] == '@' ) {
					for ( int i = 0; i < cgs.maxclients; i++ ) {
						if ( cgs.clientinfo[i].infoValid ) {
							char theirName[MAX_NETNAME]{};
							Q_strncpyz( theirName, cgs.clientinfo[i].name, sizeof(theirName) );
							Q_CleanString( theirName, STRIP_COLOUR );
							if ( !Q_strncmp( theirName, &currentChatbox->shortname[1], std::min( (size_t)(MAX_CHATBOX_IDENTIFIER_SIZE-1), sizeof(theirName) ) ) ) {
								chatMode = CHAT_WHISPER;
								chatTargetClient = i;
								break;
							}
						}
					}
				}

				chatMode = CHAT_TEAM;
			}
		}
	}

	switch ( chatMode ) {

	default:
	case CHAT_ALL: {
		trap->SendClientCommand( va( "say %s", msg ) );
	} break;

	case CHAT_TEAM: {
		trap->SendClientCommand( va( "say_team %s", msg ) );
	} break;

	case CHAT_WHISPER: {
		trap->SendClientCommand( va( "tell %i %s", chatTargetClient, msg ) );
	} break;

	}
}

static const char *preMatches[] = {
	"www.",
	"://"
};
static const char *postMatches[] = {
	".com",
	".org",
	".net",
	".ru",
	".co.uk",
	".ua",
	".tk",
	".biz",
	".tk"
};

// returns 0 if no URLs were found
// else return an offset into message where the first URL is located
// call multiple times to parse multiple urls
static size_t CG_ParseURLs( char *message ) {
	char scratch[MAX_SAY_TEXT];
	std::memcpy( scratch, message, sizeof(scratch) );

	const char *delim = " ";
	for ( char *p = strtok( scratch, delim ); p; p = strtok( NULL, delim ) ) {
		// skip colour codes
		while ( Q_IsColorString( p ) ) {
			p += 2;
		}

		ptrdiff_t offset = p - scratch;
		size_t len = strlen( p );
		for ( const char *preMatch : preMatches ) {
			size_t matchLen = strlen( preMatch );
			if ( len < matchLen ) {
				break;
			}
			if ( !Q_strncmp( p, preMatch, matchLen ) ) {
				// got a pre match, try to verify with a post match
				for ( char *dot = p; (dot = strchr( dot, '.' )) != nullptr; dot++ ) {
					for ( const char *postMatch : postMatches ) {
						size_t matchLen = strlen( postMatch );
						if ( len < matchLen ) {
							break;
						}
						if ( !Q_strncmp( dot, postMatch, matchLen ) ) {
							const char *p2 = postMatch;
							while ( p2-p > 0 ) {
								p2--;
								if ( *--p2 == ' ' ) {
									size_t realPreLength = postMatch - p2;
									offset = (p - scratch) - realPreLength;
									break;
								}
							}
							return offset;
						}
					}
				}
			}
		}

		// no pre matches, can still try post matches
		for ( char *dot = p; (dot = strchr( dot, '.' )) != nullptr; dot++ ) {
			for ( const char *postMatch : postMatches ) {
				size_t matchLen = strlen( postMatch );
				if ( len < matchLen ) {
					break;
				}
				if ( !Q_strncmp( dot, postMatch, matchLen ) ) {
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
	char buf[MAX_SAY_TEXT] = { 0 };
	struct tm *timeinfo;
	time_t tm;
	const Font font( CG_GetChatboxFont(), cg.chatbox.size.scale );

	accumLength = cg_chatboxTimeShow.integer ? font.Width( EXAMPLE_TIMESTAMP_CLEAN ) : 0.0f;

	cb->numActiveLines++;

	if ( cb != currentChatbox ) {
		cb->notification = qtrue;
		if ( cg_chatboxTabs.integer == 2 && strcmp( cbName, "normal" ) ) {
			// auto-switch to this tab if it's not global chat
			CG_ChatboxSelect( cbName );
		}
	}

	// Stop scrolling up if we've already scrolled, similar to console behaviour
	if ( cb->scrollAmount < 0 ) {
		cb->scrollAmount = std::max( cb->scrollAmount - 1, cb->numActiveLines >= cg_chatboxLineCount.integer
			? ( (std::min( cb->numActiveLines, MAX_CHATBOX_ENTRIES ) - cg_chatboxLineCount.integer) * -1 )
			: 0 );
	}

	for ( i = 0, strLength = strlen( message ); i<strLength && i<MAX_SAY_TEXT; i++ ) {
		char *p = (char*)&message[i];
		Com_sprintf( buf, sizeof(buf), "%c", *p );
		if ( !Q_IsColorString( p ) && (i > 0 && !Q_IsColorString( p - 1 )) ) {
			accumLength += font.Width( buf );
		}

		if ( accumLength > std::max( cg.chatbox.size.width, 128 ) && (i > 0 && !Q_IsColorString( p - 1 )) ) {
			char lastColor = COLOR_GREEN;
			int j = i;
			int savedOffset = i;
			char tempMessage[MAX_SAY_TEXT];

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
				const char *tmp = &message[j];
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

	// parse out URLs
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
			hours = mins / 60;						mins -= hours * 60;

			Com_sprintf( chat->timeStamp, sizeof(chat->timeStamp), "[^%c%02i:%02i:%02i" S_COLOR_WHITE "] ",
				*(char *)cg_chatboxTimeColour.string, hours, mins, seconds );
		}
	}

	chat->isUsed = qtrue;
}

static void CG_ChatboxDrawTabs( void ) {
	chatBox_t *cb = chatboxList;
	float xOffset = 0.0f;
	const Font font( CG_GetChatboxFont(), cg.chatbox.size.scale, false );

	while ( cb ) {
		const char *name = cb->shortname;
		float textWidth = font.Width( name );
		float textHeight = font.Height( name );

		CG_FillRect(
			cg.chatbox.pos.x + cg.chatbox.size.width - textWidth - 16.0f - xOffset,
			cg.chatbox.pos.y + chatboxHeight + textHeight,
			textWidth + 16.0f,
			textHeight,
			(cb == currentChatbox) ? &colorTable[CT_DKGREY] : &colorTable[CT_BLACK]
		);

		char nameColour = COLOR_WHITE;
		if ( cb == currentChatbox ) {
			nameColour = COLOR_GREEN;
		}
		else if ( cb->notification ) {
			if ( (trap->Milliseconds() >> 8) & 1 ) {
				// chatbox demands attention, flicker between white and red
				nameColour = COLOR_RED;
			}
		}
		font.Paint(
			cg.chatbox.pos.x + cg.chatbox.size.width - textWidth - 8.0f - xOffset,
			cg.chatbox.pos.y + chatboxHeight + textHeight,
			va( "^%c%s", nameColour, cb->shortname ),
			&colorWhite,
			ITEM_TEXTSTYLE_OUTLINED
		);

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

static char GetChatColour( void ) {
	switch ( chatMode ) {
	default:
	case CHAT_ALL:
		return COLOR_GREEN;

	case CHAT_TEAM:
		return COLOR_CYAN;

	case CHAT_WHISPER:
		return COLOR_MAGENTA;
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

	if ( currentChatbox->numActiveLines == 0 ) {
		skipDraw = true;
	}

	float yAccum = 0.0f;

	const char *scrollMsg = va( S_COLOR_YELLOW "Scrolled lines: " S_COLOR_CYAN "%i\n",
		currentChatbox->scrollAmount * -1
	);
	const Font font( CG_GetChatboxFont(), cg.chatbox.size.scale, false );
	float height = font.Height( scrollMsg );

	if ( !skipDraw && currentChatbox->scrollAmount < 0 && CG_ChatboxActive() ) {
		font.Paint(
			cg.chatbox.pos.x, cg.chatbox.pos.y + yAccum - (height / 2.0f),
			scrollMsg, &colorWhite, ITEM_TEXTSTYLE_OUTLINED
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
				chatboxHeight += font.Height( msg );
				float tmp = font.Width( msg );
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
					CG_FillRect(
						cg.chatbox.pos.x,
						cg.chatbox.pos.y + yAccum,
						cg.chatbox.size.width,
						chatboxHeight,
						&cg.chatbox.background
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
					const float height = font.Height( tmp );
					//const float width = font.Width( tmp );
					font.Paint(
						cg.chatbox.pos.x, cg.chatbox.pos.y + yAccum - (height / 2.0f),
						tmp, &colorWhite, ITEM_TEXTSTYLE_OUTLINED
					);
					const size_t timestampLength = cg_chatboxTimeShow.integer ? strlen( chat->timeStamp ) : 0u;
					for ( chatEntry_t::urlLocation *url = chat->URLs; url; url = url->next ) {
						char scratch[MAX_SAY_TEXT];
						Q_strncpyz( scratch, tmp, url->start + timestampLength + 1 );

						//FIXME: somehow this isn't accurate when using r_aspectCorrectFonts?!
						url->pos.x = cg.chatbox.pos.x
							+ font.Width( scratch );

						Q_strncpyz( scratch, tmp + url->start + timestampLength, url->length );
						url->size.x = font.Width( scratch );

						url->pos.y = cg.chatbox.pos.y + yAccum - (height / 2.0f);
						url->size.y = height;

						font.Paint(
							url->pos.x, url->pos.y, scratch, &colorTable[CT_LTBLUE2], ITEM_TEXTSTYLE_OUTLINED
						);
						CG_FillRect( url->pos.x, url->pos.y + height + 2.0f, url->size.x, 1.0f, &colorTable[CT_LTBLUE2] );
					}
					yAccum += height;
					numLines++;
				}
			}
		}
	}

	if ( cg_chatboxTabs.integer ) {
		CG_ChatboxDrawTabs();
	}

	// draw the input line
	if ( CG_ChatboxActive() ) {
		inputLinePosY = cg.chatbox.pos.y + yAccum;
		const char *pre = GetPreText( qfalse );
		const char *cleanPre = GetPreText( qtrue );
		char msg[MAX_EDIT_LINE];
		Com_sprintf( msg, sizeof(msg), pre, va( S_COLOR_ESCAPE "%c%s", GetChatColour(), chatField.buffer ) );
		const float height = font.Height( msg );
		inputLineHeight = height;
		inputLineWidth = font.Width( msg );
		font.Paint(
			cg.chatbox.pos.x, cg.chatbox.pos.y + yAccum - (height / 2.0f), msg,
			&g_color_table[ColorIndex( COLOR_WHITE )], ITEM_TEXTSTYLE_OUTLINED
		);
		if ( (trap->Milliseconds() >> 8) & 1 ) {
			const float cursorPre = font.Width( cleanPre );
			float cursorOffset = 0.0f;

			Q_CleanString( msg, STRIP_COLOUR );
			for ( j = 0; j < chatField.cursor; j++ ) {
				cursorOffset += font.Width( va( "%c", msg[j] ) );
			}

			const char *cursorChar = overstrike ? "#" : "|";
			const float cursorWidth = font.Width( cursorChar );
			const float cursorHeight = font.Height( cursorChar );
			if ( !overstrike ) {
				cursorOffset -= cursorWidth / 2.0f;
			}
			if ( overstrike ) {
				CG_FillRect(
					cg.chatbox.pos.x + cursorPre + cursorOffset, cg.chatbox.pos.y + yAccum,
					cursorWidth, cursorHeight / 1.5f, &g_color_table[ColorIndex( COLOR_WHITE )]
				);
			}
			else {
				font.Paint(
					cg.chatbox.pos.x + cursorPre + cursorOffset, cg.chatbox.pos.y + yAccum - (cursorHeight / 2.0f),
					cursorChar, &g_color_table[ColorIndex( COLOR_WHITE )], ITEM_TEXTSTYLE_OUTLINED
				);
			}
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
		// find current word
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

			// find the last used colour
			char previousColour = GetChatColour();
			char *pc = &chatField.buffer[chatField.cursor];
			while ( pc > &chatField.buffer[0] && *(pc - 1) != '^' ) {
				pc--;
			}
			if ( pc - chatField.buffer > 0u ) {
				previousColour = *pc;
			}

			const char *str = va( "%s " S_COLOR_ESCAPE "%c", cgs.clientinfo[match].name, previousColour );
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

void CG_ChatboxSelectTabNext( void ) {
	currentChatbox = currentChatbox->next ? currentChatbox->next : chatboxList;
}

void CG_ChatboxSelectTabPrev( void ) {
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

void CG_ChatboxSelect( const char *cbName ) {
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

static void ChatField_Clear( field_t *edit ) {
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
		ChatField_Clear( &chatField );
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
	ChatField_Clear( &chatField );
	trap->Key_SetCatcher( trap->Key_GetCatcher() | KEYCATCH_CGAME );
}

void CG_ChatboxClose( void ) {
	chatActive = false;
	trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_CGAME );
}

static void ChatField_CharEvent( field_t *edit, int key ) {
	int fieldLen = strlen( edit->buffer );

	//key = tolower( key );

	// handle shortcuts
	//RAZTODO: advanced text editing
	if ( trap->Key_IsDown( A_CTRL ) ) {
		switch ( key ) {

		// home
		case 'a': {
			edit->cursor = 0;
			edit->scroll = 0;
		} break;

		// end
		case 'e': {
			edit->cursor = fieldLen;
			edit->scroll = edit->cursor - edit->widthInChars;
		} break;

		// clear
		case A_BACKSPACE: {
			ChatField_Clear( edit );
		} break;

		// scroll to first/last message
		case A_HOME: {
			const int numActiveLines = currentChatbox->numActiveLines;
			currentChatbox->scrollAmount = numActiveLines >= cg_chatboxLineCount.integer
				? (std::min( numActiveLines, MAX_CHATBOX_ENTRIES ) - cg_chatboxLineCount.integer) * -1
				: 0;
		} break;
		case A_END: {
			currentChatbox->scrollAmount = 0;
		} break;

		// scroll through chat tabs
		case A_PAGE_UP:
		case A_MWHEELUP: {
			CG_ChatboxSelectTabNext();
		} break;
		case A_PAGE_DOWN:
		case A_MWHEELDOWN: {
			CG_ChatboxSelectTabPrev();
		} break;

		// backspace
		case 'h': {
			if ( edit->cursor > 0 ) {
				memmove( edit->buffer + edit->cursor - 1, edit->buffer + edit->cursor, fieldLen + 1 - edit->cursor );
				edit->cursor--;
				if ( edit->cursor < edit->scroll ) {
					edit->scroll--;
				}
			}
		} break;

		// copy/paste
		case 'c': {
			//RAZTODO: ChatField_Copy
			CG_ChatboxAddMessage( S_COLOR_YELLOW "Clipboard support is not available for this chatbox yet", qfalse, "normal" );
		} break;
		case 'v': {
			//RAZTODO: ChatField_Paste
			CG_ChatboxAddMessage( S_COLOR_YELLOW "Clipboard support is not available for this chatbox yet", qfalse, "normal" );
		} break;

		default: {
		} break;

		}

		return;
	}

	if ( !(key & K_CHAR_FLAG) ) {
		switch ( key ) {

		case A_ENTER:
		case A_KP_ENTER: {
			CG_ChatboxOutgoing();
		} break;

		case A_TAB: {
			CG_ChatboxTabComplete();
		} break;

		case A_PAGE_DOWN:
		case A_MWHEELDOWN: {
			CG_ChatboxScroll( 0 );
		} break;

		case A_PAGE_UP:
		case A_MWHEELUP: {
			CG_ChatboxScroll( 1 );
		} break;

		case A_CURSOR_UP: {
			CG_ChatboxHistoryUp();
		} break;

		case A_CURSOR_DOWN: {
			CG_ChatboxHistoryDn();
		} break;

		case A_CURSOR_RIGHT: {
			if ( edit->cursor < fieldLen ) {
				edit->cursor++;
			}
		} break;

		case A_CURSOR_LEFT: {
			if ( edit->cursor > 0 ) {
				edit->cursor--;
			}
		} break;

		case A_INSERT: {
			overstrike = !overstrike;
		} break;

		case A_HOME: {
			edit->cursor = 0;
			edit->scroll = 0;
		} break;

		case A_END: {
			edit->cursor = fieldLen;
			edit->scroll = edit->cursor - edit->widthInChars;
		} break;

		case A_BACKSPACE: {
			if ( edit->cursor > 0 ) {
				memmove( edit->buffer + edit->cursor - 1, edit->buffer + edit->cursor, fieldLen + 1 - edit->cursor );
				edit->cursor--;
				if ( edit->cursor < edit->scroll ) {
					edit->scroll--;
				}
			}
		} break;

		case A_DELETE: {
			if ( edit->cursor < fieldLen ) {
				memmove( edit->buffer + edit->cursor, edit->buffer + edit->cursor + 1, fieldLen - edit->cursor );
			}
		} break;

		case A_MOUSE1: {
			if ( Q_PointInBounds( cgs.cursorX, cgs.cursorY, cg.chatbox.pos.x, inputLinePosY,
					SCREEN_WIDTH, inputLineHeight ) )
			{
				const char *pre = GetPreText( qtrue );
				const size_t bias = strlen( pre );
				char msg[MAX_EDIT_LINE];
				Com_sprintf( msg, sizeof(msg), va( "%s%s", pre, chatField.buffer ) );

				// update the cursor position by checking the entire width character by character
				//	this approach should work well with JA's crappy font code
				bool cursorSet = false;
				char savedChar;
				size_t i = bias + 1u;
				const Font font( CG_GetChatboxFont(), cg.chatbox.size.scale, false );
				do {
					savedChar = msg[i];
					msg[i] = '\0';
					edit->cursor = i - bias - 1;
					//TODO: drag-selection
					float width = font.Width( msg );
					if ( cgs.cursorX < cg.chatbox.pos.x + width ) {
						cursorSet = true;
						break;
					}
					msg[i] = savedChar;
				} while ( i == 1u || msg[i++] );

				if ( !cursorSet ) {
					edit->cursor = fieldLen;
				}
			}
			else if ( Q_PointInBounds( cgs.cursorX, cgs.cursorY, cg.chatbox.pos.x, cg.chatbox.pos.y,
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
		} break;

		default: {
		} break;

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

	// - 2 to leave room for the leading slash and trailing \0
	if ( fieldLen == MAX_EDIT_LINE - 1 ) {
		return; // all full
	}

	if ( !overstrike ) {
		memmove( edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor, fieldLen + 1 - edit->cursor );
	}
	edit->buffer[edit->cursor++] = key;

	if ( edit->cursor >= edit->widthInChars ) {
		edit->scroll++;
	}

	if ( edit->cursor == fieldLen + 1 ) {
		edit->buffer[edit->cursor] = 0;
	}
}

void CG_ChatboxChar( int key ) {
	ChatField_CharEvent( &chatField, key );
}

qboolean CG_ChatboxActive( void ) {
	return chatActive && (trap->Key_GetCatcher() & KEYCATCH_CGAME);
}
