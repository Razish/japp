#pragma once

// root element is the top of the stack
typedef struct jpStack_s {
	struct jpStack_s *next;
	void *data;
} jpStack_t;

void Stack_Push( jpStack_t **s, void *p );
void Stack_Pop( jpStack_t **s );
void *Stack_GetTop( const jpStack_t **s );
