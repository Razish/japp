#include "cg_local.h"

qboolean Server_Supports( unsigned int supportFlag )
{
	return !!(cg.japp.SSF & supportFlag );
}
