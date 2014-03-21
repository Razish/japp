#pragma once

// Cross-platform (Linux & Windows) Asm defines

#if defined(_WIN32) && !defined(MINGW32)

#define qasm1( a )			__asm a
#define qasm2( a, b )		__asm a, b
#define qasm3( a, b, c )	__asm a, b, c
#define qasmL( a )			__asm a

#elif defined(MACOS_X)

//In order to use intel syntax need "Codewarrior-style inline assembly" enabled

// Linux has a few issues with defines in function defines
// So we'll create a double define to ensure the defines are properly processed
#define qasm1i( a )			__asm { a }
#define qasm2i( a, b )		__asm { a, b }
#define qasm3i( a, b, c )	__asm { a, b, c }
// proxy defines
#define qasm1( a )			qasm1i( a )
#define qasm2( a, b )		qasm2i( a, b )
#define qasm3( a, b, c )	qasm3i( a, b, c )

#define qasmL( a )      	__asm__( #a "\n" );

#else

// Linux has a few issues with defines in function defines
// So we'll create a double define to ensure the defines are properly processed
#define qasm1i( a )			__asm__( #a "\n" );
#define qasm2i( a, b )		__asm__( #a ", " #b "\n" );
#define qasm3i( a, b, c )	__asm__( #a ", " #b ", " #c "\n" );
// proxy defines
#define qasm1( a )			qasm1i( a )
#define qasm2( a, b )		qasm2i( a, b )
#define qasm3( a, b, c )	qasm3i( a, b, c )

#define qasmL( a )			__asm__( #a "\n" );

#endif
