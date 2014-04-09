#pragma once

// root element is the top of the stack
typedef struct stack_s {
	struct stack_s *next;
	void *data;
} stack_t;

void Stack_Push( stack_t **s, void *p );
void Stack_Pop( stack_t **s );
void *Stack_GetTop( const stack_t **s );
