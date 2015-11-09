#include "qcommon/q_shared.h"
#ifdef PROJECT_CGAME
#include "cg_local.h"
#endif

// cgame helper

#if ARCH_WIDTH == 64 || defined(MACOS_X) || defined(QARCH_ARM)

// wat do?

#else

#ifdef PROJECT_CGAME
Q_CABI Q_CDECL Q_EXPORT void CrashReport( int fileHandle ) {
	char text[] = "Test from cgame\n";
	Q_FSWriteString( fileHandle, text );
}

#endif

#endif //ARCH_WIDTH == 32
