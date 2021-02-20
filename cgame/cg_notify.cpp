// Requires:
//	libgtk2.0-dev
//	libnotify-dev
#ifndef NO_NOTIFY
	#include <libnotify/notify.h>
	#include <glib.h>
	#include <unistd.h>
#endif

#include "cg_local.h"
#include "cg_notify.h"

void CG_NotifyInit( void ) {
#ifndef NO_NOTIFY
	notify_init( "JA++" );
#endif
}

bool CG_NotifySend( const char *title, const char *msg, uint32_t timeout, const char *iconName ) {
	bool success = true;

#ifndef NO_NOTIFY
	// workaround for libnotify/dbus issues...ugh
	if ( notify_is_initted() ) {
		CG_NotifyShutdown();
	}
	CG_NotifyInit();
	if ( !notify_is_initted() ) {
		// must be some kind of issue, abandon ship
		return false;
	}

	char titleBuf[256]{ "JA++" };
	if ( title && *title ) {
		Q_strcat( titleBuf, sizeof(titleBuf), va( ": %s", title ) );
	}

	NotifyNotification *n = notify_notification_new( titleBuf, msg, iconName );
	if ( !n ) {
		return false;
	}
	notify_notification_set_timeout( n, timeout );
	GError *err = nullptr;
	if ( !notify_notification_show( n, &err ) ) {
		trap->Print( "Failed to send notification: %s\n", err->message );
		g_error_free( err );
		success = false;
	}
	g_object_unref( G_OBJECT( n ) );
#endif

	return success;
}

void CG_NotifyShutdown( void ) {
#ifndef NO_NOTIFY
	notify_uninit();
#endif
}
