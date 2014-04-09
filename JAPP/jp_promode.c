#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "jp_promode.h"

// Physics
float	cpm_pm_airstopaccelerate = 1;
float	cpm_pm_aircontrol = 0;
float	cpm_pm_strafeaccelerate = 1.0f;
float	cpm_pm_wishspeed = 400;

void CPM_UpdateSettings( int num ) {
	// num = 0: normal quake 3
	// num = 1: pro mode

	// Physics
	cpm_pm_airstopaccelerate = 1;
	cpm_pm_aircontrol = 0;
	cpm_pm_strafeaccelerate = 1.0f;
	cpm_pm_wishspeed = 400;
	pm_accelerate = 10.0f;
	pm_friction = 6.0f;

	if ( num ) {
		// Physics
		cpm_pm_airstopaccelerate = 2.5f;
		cpm_pm_aircontrol = 150;
		cpm_pm_strafeaccelerate = 70;
		cpm_pm_wishspeed = 30;
		pm_accelerate = 15;
		pm_friction = 8;
	}
}

void CPM_PM_Aircontrol( pmove_t *pmove, vector3 *wishdir, float wishspeed ) {
	float zspeed, speed, dot, k;

	// can't control movement if not moving forward or backward
	if ( (pmove->ps->movementDir && pmove->ps->movementDir != 4) || (int)wishspeed == 0 )
		return;

	zspeed = pmove->ps->velocity.z;
	pmove->ps->velocity.z = 0;
	speed = VectorNormalize( &pmove->ps->velocity );

	dot = DotProduct( &pmove->ps->velocity, wishdir );
	k = 32;
	k *= cpm_pm_aircontrol*dot*dot*pml.frametime;


	if ( dot > 0.0f ) {
		// we can't change direction while slowing down
		pmove->ps->velocity.x = pmove->ps->velocity.x*speed + wishdir->x*k;
		pmove->ps->velocity.y = pmove->ps->velocity.y*speed + wishdir->y*k;
		VectorNormalize( &pmove->ps->velocity );
	}

	pmove->ps->velocity.x *= speed;
	pmove->ps->velocity.y *= speed;
	pmove->ps->velocity.z = zspeed;
}
