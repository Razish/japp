#include "g_local.h"

typedef struct jpMalloc_s
{
	struct	jpMalloc_s *next;
	void	*data;
} jpMalloc_t;
static jpMalloc_t *root = NULL;

void *JP_Alloc( int size )
{
	void		*data	= malloc( size );
	jpMalloc_t	*ptr	= (jpMalloc_t *)malloc( sizeof( jpMalloc_t ) );

	ptr->data = data;

	if ( root )
		ptr->next = root;
	root = ptr;

	return ptr->data;
}

void JP_Free( void *data )
{
	jpMalloc_t *ptr = root;
	while ( ptr )
	{
		if ( (unsigned int)data == (unsigned int)ptr->data )
		{//Found it, remove the entry
			free( ptr->data );
			if ( ptr->next )
				ptr = ptr->next->next;
			break;
		}
		ptr = ptr->next;
	}
}
