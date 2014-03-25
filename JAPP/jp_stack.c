#include "qcommon/q_shared.h"
#include "jp_stack.h"

void Stack_Push( stack_t **s, void *p ) {
	stack_t *next;

	next = *s ? *s : (stack_t *)malloc( sizeof(stack_t) );

	*s = (stack_t *)malloc( sizeof(stack_t) );
	memset( *s, 0, sizeof(stack_t) );
	(*s)->next = next;
	(*s)->data = p;
}

void Stack_Pop( stack_t **s ) {
	stack_t *next;

	// nothing more to pop?
	if ( !*s )
		return;

	next = (*s)->next;

	free( (*s)->data );
	(*s)->data = NULL;
	free( (void *)*s );

	(*s) = next;
}

void *Stack_GetTop( const stack_t **s ) {
	return *s ? ( *s )->data : NULL;
}
