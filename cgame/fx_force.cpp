#include "cg_local.h"
#include "cg_media.h"

// This effect is not generic because of possible enhancements
void FX_ForceDrained( vector3 *origin, vector3 *dir ) {
	VectorScale( dir, -1.0f, dir );
	trap->FX_PlayEffectID( media.efx.force.drained, origin, dir, -1, -1, qfalse );
}
