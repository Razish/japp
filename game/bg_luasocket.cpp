#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif
#include "bg_luainternal.h"

#ifdef JPLUA
namespace JPLua {

	static const char SOCKET_META[] = "Socket.meta";

	void InitSocket(const char *ip, const char *protocol, const unsigned int port) {
#if defined(_WIN32)
		int iResult = 0;
		WSADATA wsaData;
		unsigned int sock = INVALID_SOCKET;

		struct addrinfo hints, *result = NULL;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		char buffer[20];
		_itoa_s(port, buffer, sizeof(buffer), 10);
		LPCSTR p = buffer;

		// Get address info
		if ((iResult = getaddrinfo(NULL, p, &hints, &result)) != 0) {
			trap->Print("^2InitSocket: ^1getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return;
		}

		// Init Socket
		if ((sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == INVALID_SOCKET) {
			trap->Print("^2InitSocket: ^1socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return;
		}

		if ((iResult = bind(sock, result->ai_addr, (int)result->ai_addrlen)) == SOCKET_ERROR) {
			trap->Print("^2InitSocket: ^1bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(sock);
			WSACleanup();
			return;
		}

		// Listen!
		if ((iResult = listen(sock, 66)) == SOCKET_ERROR) { // 32 + 32 players + 2 servers
			trap->Print("^2InitSocket: ^1listen failed with error: %d\n", WSAGetLastError());
			closesocket(sock);
			WSACleanup();
			return;
		}
#elif(__linux__)

#endif
	}

	void JPLua::Socket_CreateRef(lua_State * L, unsigned int socket) {


	}

	int CreateSocket(lua_State * L) {
		StackCheck st(L);
		char ip[15] = { '\0' }; // bind to specific ip
		char protocol[3] = { '\0' }; // tcp / udp
		unsigned int port = 0; // port

		if (lua_type(L, 1) != LUA_TTABLE) {
			trap->Print("^2ERROR: ^1 JPLua::CreateSocket failed, not a table\n");
			return 0;
		}

		{
			lua_getfield(L, 1, "ip");
			Q_strncpyz(ip, lua_tostring(L, -1), sizeof(ip));
			lua_getfield(L, 1, "protocol");
			Q_strncpyz(protocol, lua_tostring(L, -1), sizeof(protocol));
			lua_getfield(L, 1, "port");
			port = (unsigned int)lua_tointeger(L, -1);
			lua_pop(L, 3);
		}


		return 1;
	}

	int GetSocket(lua_State * L){

		return 0;
	}

	jpluaSocket_t * JPLua::CheckSocket(lua_State * L, int idx){

		return nullptr;
	}

	void Register_Socket(lua_State * L){

	}


}


#endif