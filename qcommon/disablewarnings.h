#pragma once

/*
Raz:	I only want warnings disabled if we're NOT going to work around them *for a reason*
This is simply because things have to be done a certain way, but MSVC doesn't like it. Nit-picky.
If there's only one specific case, you may want to push the warning and pop	back on for later warnings...

#pragma warning( push )
#pragma warning( disable : 1337 )
...Some code...
#pragma warning( pop )	//revert anything changed in the pushed segment of the code
*/

#if defined(_WIN32) && !defined(MINGW32)
#pragma warning( error : 4013 ) // 'Func' undefined; assuming extern returning int
#pragma warning( error : 4024 ) // different types for formal and actual parameter x
#pragma warning( error : 4026 ) // function declared with formal parameter list
#pragma warning( error : 4028 ) // formal parameter x different from declaration
#pragma warning( error : 4029 ) // declared formal parameter list different from definition
#pragma warning( error : 4033 ) // 'Func' must return a value
#pragma warning( error : 4047 ) // '=' : 'x' differs in levels of indirection from 'int'
#pragma warning( error : 4053 ) // one void operand for '?:'
#pragma warning( error : 4087 ) // 'Func' declared with 'void' parameter list
#pragma warning( error : 4098 ) // 'void' function returning a value
#pragma warning( error : 4245 ) // conversion from 'x' to 'y', signed/unsigned mismatch
#pragma warning( error : 4305 ) // truncation from 'x' to 'y'
#pragma warning( error : 4700 ) // uninitialized local variable 'x' used

#pragma warning( disable	: 4100 )	// unreferenced formal parameter
#pragma warning( disable	: 4127 )	// conditional expression is constant
#pragma warning( disable	: 4244 )	// conversion from 'x' to 'y', possible loss of data
#pragma warning( disable	: 4706 )	// assignment within conditional expression
#pragma warning( disable	: 4131 )	// 'x' : uses old-style declarator
#pragma warning( disable	: 4996 )	// This function or variable may be unsafe

#endif //_WIN32 && !MINGW32
