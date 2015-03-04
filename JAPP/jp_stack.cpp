#include "qcommon/q_shared.h"
#include "jp_stack.h"

void Stack_Push( jpStack_t **s, void *p ) {
	jpStack_t *next;

	next = *s ? *s : (jpStack_t *)malloc( sizeof(jpStack_t) );

	*s = (jpStack_t *)malloc( sizeof(jpStack_t) );
	memset( *s, 0, sizeof(jpStack_t) );
	(*s)->next = next;
	(*s)->data = p;
}

void Stack_Pop( jpStack_t **s ) {
	jpStack_t *next;

	// nothing more to pop?
	if ( !*s )
		return;

	next = (*s)->next;

	free( (*s)->data );
	(*s)->data = NULL;
	free( (void *)*s );

	(*s) = next;
}

void *Stack_GetTop( const jpStack_t **s ) {
	return *s ? ( *s )->data : NULL;
}
