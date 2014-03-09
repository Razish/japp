#pragma once

void CPM_UpdateSettings(int num);

// Physics
extern float	cpm_pm_airstopaccelerate;
extern float	cpm_pm_aircontrol;
extern float	cpm_pm_strafeaccelerate;
extern float	cpm_pm_wishspeed;

void CPM_PM_Aircontrol ( pmove_t *pm, vector3 *wishdir, float wishspeed );
