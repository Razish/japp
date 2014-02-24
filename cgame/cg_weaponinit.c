#include "cg_local.h"
#include "fx_local.h"

// The server says this item is used on this level
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	const gitem_t	*item, *ammo;
	char			path[MAX_QPATH];
	vector3			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 )
		return;

	if ( weaponInfo->registered )
		return;

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item=bg_itemlist+1; item->classname; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}

	if ( !item->classname )
		trap->Error( ERR_DROP, "Couldn't find weapon %i", weaponNum );

	CG_RegisterItemVisuals( item-bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap->R_RegisterModel( item->world_model[0] );
	// load in-view model also
	weaponInfo->viewModel = trap->R_RegisterModel( item->view_model );

	// calc midpoint for rotation
	trap->R_ModelBounds( weaponInfo->weaponModel, &mins, &maxs );
	for ( i=0; i<3; i++ )
		weaponInfo->weaponMidpoint.data[i] = mins.data[i] + 0.5f * (maxs.data[i] - mins.data[i]);

	weaponInfo->weaponIcon = trap->R_RegisterShaderNoMip( item->icon );
	weaponInfo->ammoIcon = trap->R_RegisterShaderNoMip( item->icon );

	for ( ammo=bg_itemlist+1; ammo->classname; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum )
			break;
	}
	if ( ammo->classname && ammo->world_model[0] )
		weaponInfo->ammoModel = trap->R_RegisterModel( ammo->world_model[0] );

	weaponInfo->flashModel = NULL_HANDLE;

	if ( weaponNum == WP_DISRUPTOR || weaponNum == WP_FLECHETTE || weaponNum == WP_REPEATER
		|| weaponNum == WP_ROCKET_LAUNCHER || weaponNum == WP_CONCUSSION )
	{
		Q_strncpyz( path, item->view_model, sizeof( path ) );
		COM_StripExtension( path, path, sizeof( path ) );
		strcat( path, "_barrel.md3" );
		weaponInfo->barrelModel = trap->R_RegisterModel( path );
	}
	else if ( weaponNum == WP_STUN_BATON ) {
		// only weapon with more than 1 barrel..
		trap->R_RegisterModel( "models/weapons2/stun_baton/baton_barrel.md3" );
		trap->R_RegisterModel( "models/weapons2/stun_baton/baton_barrel2.md3" );
		trap->R_RegisterModel( "models/weapons2/stun_baton/baton_barrel3.md3" );
	}
	else
		weaponInfo->barrelModel = NULL_HANDLE;

	if ( weaponNum != WP_SABER ) {
		Q_strncpyz( path, item->view_model, sizeof( path ) );
		COM_StripExtension( path, path, sizeof( path ) );
		strcat( path, "_hand.md3" );
		weaponInfo->handsModel = trap->R_RegisterModel( path );
	}
	else
		weaponInfo->handsModel = NULL_HANDLE;

	switch ( weaponNum ) {
	case WP_STUN_BATON:
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/baton/fire.mp3" );
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/baton/fire.mp3" );

	//	trap->R_RegisterShader( "gfx/effects/stunPass" );
		trap->FX_RegisterEffect( "stunBaton/flesh_impact" );
		trap->S_RegisterSound( "sound/weapons/baton/idle.wav" );
		break;

	case WP_MELEE:
		break;

	case WP_SABER:
		weaponInfo->firingSound			= trap->S_RegisterSound( "sound/weapons/saber/saberhum1.wav" );
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/saber/saber_w.glm" );
		break;

	case WP_CONCUSSION:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/concussion/select.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "concussion/muzzle_flash" );
		weaponInfo->missileTrailFunc	= FX_ConcussionProjectileThink;
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/bryar/altcharge.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "concussion/altmuzzle_flash" );
		weaponInfo->altMissileTrailFunc = FX_ConcussionProjectileThink;

		trap->R_RegisterShader( "gfx/effects/blueLine" );
		trap->R_RegisterShader( "gfx/misc/whiteline2" );
		break;

	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/bryar/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/bryar/fire.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "bryar/muzzle_flash" );
		weaponInfo->missileTrailFunc	= FX_BryarProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/bryar/alt_fire.wav" );
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/bryar/altcharge.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "bryar/muzzle_flash" );
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;

		trap->FX_RegisterEffect( "blaster/wall_impact.efx" );
		trap->FX_RegisterEffect( "blaster/flesh_impact.efx" );
		break;

	case WP_BLASTER:
	case WP_EMPLACED_GUN:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/blaster/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/blaster/fire.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "blaster/muzzle_flash" );
		weaponInfo->missileTrailFunc	= FX_BlasterProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/blaster/alt_fire.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "blaster/muzzle_flash" );
		weaponInfo->altMissileTrailFunc = FX_BlasterProjectileThink;

		trap->FX_RegisterEffect( "blaster/deflect" );
		break;

	case WP_DISRUPTOR:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/disruptor/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/disruptor/fire.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "disruptor/muzzle_flash" );
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/disruptor/alt_fire.wav" );
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/disruptor/altCharge.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "disruptor/muzzle_flash" );

		trap->R_RegisterShader( "gfx/effects/redLine" );
		trap->R_RegisterShader( "gfx/misc/whiteline2" );
		trap->R_RegisterShader( "gfx/effects/smokeTrail" );
		trap->S_RegisterSound( "sound/weapons/disruptor/zoomstart.wav" );
		trap->S_RegisterSound( "sound/weapons/disruptor/zoomend.wav" );
		break;

	case WP_BOWCASTER:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/bowcaster/select.wav" );
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/bowcaster/fire.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->altMissileTrailFunc	= FX_BowcasterProjectileThink;
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/bowcaster/fire.wav" );
		weaponInfo->chargeSound			= trap->S_RegisterSound( "sound/weapons/bowcaster/altcharge.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->missileTrailFunc	= FX_BowcasterAltProjectileThink;

		trap->FX_RegisterEffect( "bowcaster/deflect" );
		break;

	case WP_REPEATER:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/repeater/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/repeater/fire.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "repeater/muzzle_flash" );
		weaponInfo->missileTrailFunc	= FX_RepeaterProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/repeater/alt_fire.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "repeater/muzzle_flash" );
		weaponInfo->altMissileTrailFunc = FX_RepeaterAltProjectileThink;
		break;

	case WP_DEMP2:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/demp2/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/demp2/fire.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "demp2/muzzle_flash" );
		weaponInfo->missileTrailFunc	= FX_DEMP2_ProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/demp2/altfire.wav" );
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/demp2/altCharge.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "demp2/muzzle_flash" );
		break;

	case WP_FLECHETTE:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/flechette/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/flechette/fire.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/golan_arms/projectileMain.md3" );
		weaponInfo->missileTrailFunc	= FX_FlechetteProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/flechette/alt_fire.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/golan_arms/projectile.md3" );
		weaponInfo->altMissileTrailFunc = FX_FlechetteAltProjectileThink;
		break;

	case WP_ROCKET_LAUNCHER:
		VectorSet( &weaponInfo->missileDlightColor, 1.0f, 1.0f, 0.5f );
		VectorSet( &weaponInfo->altMissileDlightColor, 1.0f, 1.0f, 0.5f );

		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/rocket/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/rocket/fire.wav" );
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "rocket/muzzle_flash" ); //trap->FX_RegisterEffect( "rocket/muzzle_flash2" );
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->missileSound		= trap->S_RegisterSound( "sound/weapons/rocket/missleloop.wav" );
		weaponInfo->missileDlight		= 125;
		weaponInfo->missileTrailFunc	= FX_RocketProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/rocket/alt_fire.wav" );
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "rocket/altmuzzle_flash" );
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->altMissileSound		= trap->S_RegisterSound( "sound/weapons/rocket/missleloop.wav" );
		weaponInfo->altMissileDlight	= 125;
		weaponInfo->altMissileTrailFunc = FX_RocketAltProjectileThink;

		trap->R_RegisterShaderNoMip( "gfx/2d/wedge" );
		trap->R_RegisterShaderNoMip( "gfx/2d/lock" );
		trap->S_RegisterSound( "sound/weapons/rocket/lock.wav" );
		trap->S_RegisterSound( "sound/weapons/rocket/tick.wav" );
		break;

	case WP_THERMAL:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/thermal/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/thermal/fire.wav" );
		weaponInfo->chargeSound			= trap->S_RegisterSound( "sound/weapons/thermal/charge.wav" );
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/thermal/fire.wav" );
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/thermal/charge.wav" );
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );

		trap->S_RegisterSound( "sound/weapons/thermal/thermloop.wav" );
		trap->S_RegisterSound( "sound/weapons/thermal/warning.wav" );
		break;

	case WP_TRIP_MINE:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/detpack/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/laser_trap/fire.wav" );
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/laser_trap/fire.wav" );

		trap->FX_RegisterEffect( "tripMine/explosion" );
		trap->S_RegisterSound( "sound/weapons/laser_trap/stick.wav" );
		trap->S_RegisterSound( "sound/weapons/laser_trap/warning.wav" );
		break;

	case WP_DET_PACK:
		weaponInfo->selectSound			= trap->S_RegisterSound( "sound/weapons/detpack/select.wav" );
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/detpack/fire.wav" );
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/detpack/fire.wav" );
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );

		trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		trap->S_RegisterSound( "sound/weapons/detpack/stick.wav" );
		trap->S_RegisterSound( "sound/weapons/detpack/warning.wav" );
		trap->S_RegisterSound( "sound/weapons/explosions/explode5.wav" );
		break;

	case WP_TURRET:
		weaponInfo->missileTrailFunc	= FX_TurretProjectileThink;

		trap->FX_RegisterEffect( "effects/blaster/wall_impact.efx" );
		trap->FX_RegisterEffect( "effects/blaster/flesh_impact.efx" );
		break;

	default:
		VectorSet( &weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap->S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav" );
		break;
	}
}
