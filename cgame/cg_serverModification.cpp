#include "cg_local.h"

qboolean Server_Supports(uint32_t supportFlag) { return !!(cg.japp.SSF & supportFlag); }
