#pragma once

#ifdef JPLUA
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(_WIN32) || defined (__WIN32__)
#define NOCRYPT
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else

#if MAC_OS_X_VERSION_MIN_REQUIRED == 1020
// needed for socklen_t on OSX 10.2
#        define _BSD_SOCKLEN_T_
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef MACOS_X
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>         // for 'struct sockaddr_dl'
#endif

#ifdef __sun
#include <sys/filio.h>
#endif

#define INVALID_SOCKET                -1
#define SOCKET_ERROR                        -1
#define closesocket                                close
#define ioctlsocket                                ioctl
#define socketError                                errno

#endif

namespace JPLua {
	typedef enum {
		JPLUA_SOCKET_TCP = 0,
		JPLUA_SOCKET_UDP
	} jpluaSocketProtocol_t;

	typedef struct jpluaSocket_s {
		unsigned int socket;
		jpluaSocketProtocol_t protocol;
		sockaddr_in *sockinfo;
	} jpluaSocket_t;

#ifdef JPLUA_INTERNALS
	void Socket_CreateRef(lua_State *L, unsigned int socket);
	int CreateSocket(lua_State *L);
	int GetSocket(lua_State *L);
	jpluaSocket_t *CheckSocket(lua_State *L, int idx);
	void Register_Socket(lua_State *L);
#endif // JPLUA_INTERNALS

}

#endif // JPLUA