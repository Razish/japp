#pragma once

#ifdef JPLUA

#include "cJSON/cJSON.h"

namespace JPLua {

	// Serialiser instance userdata type
	struct serialiser_t {
		fileHandle_t fileHandle;
		char fileName[MAX_QPATH];
		cJSON *inRoot, *outRoot;
		qboolean read, write;
	};

#ifdef JPLUA_INTERNALS
	void Serialiser_CreateRef( lua_State *L, const char *path, fsMode_t mode );
	int GetSerialiser( lua_State *L );
	serialiser_t *CheckSerialiser( lua_State *L, int idx );
	void Register_Serialiser( lua_State *L );
#endif

} // namespace JPLua

#endif // JPLUA
