// events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"
#include "cg_media.h"

// set up the appropriate ghoul2 info to a refent
void CG_SetGhoul2InfoRef( refEntity_t *ent, refEntity_t	*s1 ) {
	ent->ghoul2 = s1->ghoul2;
	VectorCopy( &s1->modelScale, &ent->modelScale );
	ent->radius = s1->radius;
	VectorCopy( &s1->angles, &ent->angles );
}

// The server says this item is used on this level
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	const gitem_t	*item;
	int				handle;

	if ( itemNum < 0 || itemNum >= (int)bg_numItems ) {
		trap->Error( ERR_DROP, "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems - 1 );
	}

	itemInfo = &cg_items[itemNum];
	if ( itemInfo->registered )
		return;

	item = &bg_itemlist[itemNum];

	memset( itemInfo, 0, sizeof(*itemInfo) );
	itemInfo->registered = qtrue;

	if ( item->giType == IT_TEAM && (item->giTag == PW_REDFLAG || item->giTag == PW_BLUEFLAG) && cgs.gametype == GT_CTY )
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[1] );
	else if ( item->giType == IT_WEAPON && (item->giTag == WP_THERMAL || item->giTag == WP_TRIP_MINE || item->giTag == WP_DET_PACK) )
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[1] );
	else
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[0] );

	if ( !Q_stricmp( &item->world_model[0][strlen( item->world_model[0] ) - 4], ".glm" ) ) {
		handle = trap->G2API_InitGhoul2Model( &itemInfo->g2Models[0], item->world_model[0], 0, 0, 0, 0, 0 );
		if ( handle < 0 )
			itemInfo->g2Models[0] = NULL;
		else
			itemInfo->radius[0] = 60;
	}
	if ( item->icon )
		itemInfo->icon = trap->R_RegisterShaderNoMip( item->icon );
	else
		itemInfo->icon = 0;

	if ( item->giType == IT_WEAPON )
		CG_RegisterWeapon( item->giTag );

	// powerups have an accompanying ring or sphere
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] )
			itemInfo->models[1] = trap->R_RegisterModel( item->world_model[1] );
	}
}


#define WEAPON_FORCE_BUSY_HOLSTER

#ifdef WEAPON_FORCE_BUSY_HOLSTER
//rww - this was done as a last resort. Forgive me.
static int cgWeapFrame = 0;
static int cgWeapFrameTime = 0;
#endif

static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame, int animNum ) {
	animation_t *animations = bgHumanoidAnimations;
#ifdef WEAPON_FORCE_BUSY_HOLSTER
	if ( cg.snap->ps.forceHandExtend != HANDEXTEND_NONE || cgWeapFrameTime > cg.time ) {
		// the reason for the after delay is so that it doesn't snap the weapon frame to the "idle" (0) frame for a very quick moment
		if ( cgWeapFrame < 6 ) {
			cgWeapFrame = 6;
			cgWeapFrameTime = cg.time + 10;
		}

		else if ( cgWeapFrameTime < cg.time && cgWeapFrame < 10 ) {
			cgWeapFrame++;
			cgWeapFrameTime = cg.time + 10;
		}

		else if ( cg.snap->ps.forceHandExtend != HANDEXTEND_NONE && cgWeapFrame == 10 )
			cgWeapFrameTime = cg.time + 100;

		return cgWeapFrame;
	}
	else {
		cgWeapFrame = 0;
		cgWeapFrameTime = 0;
	}
#endif

	switch ( animNum ) {
	case TORSO_DROPWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 5 )
			return frame - animations[animNum].firstFrame + 6;
		break;

	case TORSO_RAISEWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 4 )
			return frame - animations[animNum].firstFrame + 6 + 4;
		break;

	case BOTH_ATTACK1:
	case BOTH_ATTACK2:
	case BOTH_ATTACK3:
	case BOTH_ATTACK4:
	case BOTH_ATTACK10:
	case BOTH_THERMAL_THROW:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 6 )
			return 1 + (frame - animations[animNum].firstFrame);

		break;

	default:
		break;
	}

	return -1;
}


static void CG_CalculateWeaponPosition( vector3 *origin, vector3 *angles ) {
	float	scale, fracsin;
	int		delta;
	refdef_t *refdef = CG_GetRefdef();

	VectorCopy( &refdef->vieworg, origin );
	VectorCopy( &refdef->viewangles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 )
		scale = -cg.xyspeed;
	else
		scale = cg.xyspeed;

	// gun angles from bobbing
	if ( cg_gunBobEnable.integer ) {
		angles->pitch += cg.xyspeed	* cg.bobfracsin * cg.gunBob.pitch;
		angles->yaw += scale		* cg.bobfracsin * cg.gunBob.yaw;
		angles->roll += scale		* cg.bobfracsin * cg.gunBob.roll;

		// drop the weapon when landing
		delta = cg.time - cg.landTime;
		if ( delta < LAND_DEFLECT_TIME )
			origin->z += cg.landChange*0.25f * delta / LAND_DEFLECT_TIME;
		else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )
			origin->z += cg.landChange*0.25f * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;

#if 0
		// drop the weapon when stair climbing
		delta = cg.time - cg.stepTime;
		if ( delta < STEP_TIME/2 )
			origin->z -= cg.stepChange*0.25f * delta / (STEP_TIME/2);
		else if ( delta < STEP_TIME )
			origin->z -= cg.stepChange*0.25f * (STEP_TIME - delta) / (STEP_TIME/2);
#endif
	}

	// idle drift
	if ( cg_gunIdleDriftEnable.integer ) {
		scale = cg.xyspeed + 40;
		fracsin = sinf( cg.time * cg.gunIdleDrift.speed );
		angles->pitch += scale * fracsin * cg.gunIdleDrift.amount.pitch;
		angles->yaw += scale * fracsin * cg.gunIdleDrift.amount.yaw;
		angles->roll += scale * fracsin * cg.gunIdleDrift.amount.roll;
	}
}

static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	SE_R_AddRefEntityToScene( gun, MAX_CLIENTS );

	// add electrocution shell
	if ( cg.predictedPlayerState.electrifyTime > cg.time ) {
		int preShader = gun->customShader;
		if ( rand() & 1 )
			gun->customShader = media.gfx.world.electricBody;
		else
			gun->customShader = media.gfx.world.electricBody2;
		SE_R_AddRefEntityToScene( gun, MAX_CLIENTS );
		gun->customShader = preShader; //set back just to be safe
	}
}

// Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
// The main player will have this called for BOTH cases, so effects like light and sound should only be done on the
//	world model case.
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, vector3 *newAngles, qboolean thirdPerson ) {
	refEntity_t gun, barrel, flash;
	vector3 angles;
	weapon_t weaponNum;
	weaponInfo_t *weapon;
	centity_t *nonPredictedCent;

	weaponNum = cent->currentState.weapon;

	if ( cent->currentState.weapon == WP_EMPLACED_GUN )
		return;

	// spectator mode, don't draw it...
	if ( cg.predictedPlayerState.pm_type == PM_SPECTATOR && cent->currentState.number == cg.predictedPlayerState.clientNum )
		return;

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	memset( &gun, 0, sizeof(gun) );

	// only do this if we are in first person, since world weapons are now handled on the server by Ghoul2
	if ( !thirdPerson ) {
		// add the weapon
		VectorCopy( &parent->lightingOrigin, &gun.lightingOrigin );
		gun.shadowPlane = parent->shadowPlane;
		gun.renderfx = parent->renderfx;

		// this player, in first person view
		if ( ps )
			gun.hModel = weapon->viewModel;
		else
			gun.hModel = weapon->weaponModel;
		if ( !gun.hModel )
			return;

		if ( !ps ) {
			// add weapon ready sound
			if ( (cent->currentState.eFlags & EF_FIRING) && weapon->firingSound )
				trap->S_AddLoopingSound( cent->currentState.number, &cent->lerpOrigin, &vec3_origin, weapon->firingSound );
			else if ( weapon->readySound )
				trap->S_AddLoopingSound( cent->currentState.number, &cent->lerpOrigin, &vec3_origin, weapon->readySound );
		}

		CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon" );

		if ( !CG_IsMindTricked( cent->currentState.trickedentindex, cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3, cent->currentState.trickedentindex4, cg.snap->ps.clientNum ) ) {
			CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups ); //don't draw the weapon if the player is invisible
		}

		if ( weaponNum == WP_STUN_BATON ) {
			int i;

			for ( i = 0; i < 3; i++ ) {
				memset( &barrel, 0, sizeof(barrel) );
				VectorCopy( &parent->lightingOrigin, &barrel.lightingOrigin );
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				if ( i == 0 )	barrel.hModel = trap->R_RegisterModel( "models/weapons2/stun_baton/baton_barrel.md3" );
				else if ( i == 1 )	barrel.hModel = trap->R_RegisterModel( "models/weapons2/stun_baton/baton_barrel2.md3" );
				else				barrel.hModel = trap->R_RegisterModel( "models/weapons2/stun_baton/baton_barrel3.md3" );
				angles.yaw = 0;
				angles.pitch = 0;
				angles.roll = 0;

				AnglesToAxis( &angles, barrel.axis );

				if ( i == 0 )	CG_PositionRotatedEntityOnTag( &barrel, parent, weapon->handsModel, "tag_barrel" );
				else if ( i == 1 )	CG_PositionRotatedEntityOnTag( &barrel, parent, weapon->handsModel, "tag_barrel2" );
				else				CG_PositionRotatedEntityOnTag( &barrel, parent, weapon->handsModel, "tag_barrel3" );
				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
			}
		}
		else {
			// add the spinning barrel
			if ( weapon->barrelModel ) {
				memset( &barrel, 0, sizeof(barrel) );
				VectorCopy( &parent->lightingOrigin, &barrel.lightingOrigin );
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				barrel.hModel = weapon->barrelModel;
				angles.yaw = 0;
				angles.pitch = 0;
				angles.roll = 0;

				AnglesToAxis( &angles, barrel.axis );

				CG_PositionRotatedEntityOnTag( &barrel, parent, weapon->handsModel, "tag_barrel" );

				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
			}
		}
	}

	memset( &flash, 0, sizeof(flash) );
	CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash" );

	VectorCopy( &flash.origin, &cg.lastFPFlashPoint );

	// Do special charge bits
	// Make the guns do their charging visual in True View.
	if ( (ps || cg.renderingThirdPerson || cg.predictedPlayerState.clientNum != cent->currentState.number || cg_trueGuns.integer) &&
		((cent->currentState.modelindex2 == WEAPON_CHARGING_ALT && cent->currentState.weapon == WP_BRYAR_PISTOL) ||
		(cent->currentState.modelindex2 == WEAPON_CHARGING_ALT && cent->currentState.weapon == WP_BRYAR_OLD) ||
		(cent->currentState.weapon == WP_BOWCASTER && cent->currentState.modelindex2 == WEAPON_CHARGING) ||
		(cent->currentState.weapon == WP_DEMP2 && cent->currentState.modelindex2 == WEAPON_CHARGING_ALT)) ) {
		int shader = 0;
		float val = 0.0f;
		float scale = 1.0f;
		addspriteArgStruct_t fxSArgs;
		vector3 flashorigin, flashdir;

		if ( !thirdPerson ) {
			VectorCopy( &flash.origin, &flashorigin );
			VectorCopy( &flash.axis[0], &flashdir );
		}
		else {
			mdxaBone_t boltMatrix;

			// it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
			if ( !trap->G2API_HasGhoul2ModelOnIndex( &(cent->ghoul2), 1 ) )
				return;

			// Couldn't find bolt point.
			if ( !trap->G2API_GetBoltMatrix( cent->ghoul2, 1, 0, &boltMatrix, newAngles, &cent->lerpOrigin, cg.time,
				cgs.gameModels, &cent->modelScale ) ) {
				return;
			}

			BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, &flashorigin );
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_X, &flashdir );
		}

		if ( cent->currentState.weapon == WP_BRYAR_PISTOL || cent->currentState.weapon == WP_BRYAR_OLD ) {
			// Hardcoded max charge time of 1 second
			val = (cg.time - cent->currentState.constantLight) * 0.001f;
			shader = media.gfx.world.bryarFrontFlash;
		}
		else if ( cent->currentState.weapon == WP_BOWCASTER ) {
			// Hardcoded max charge time of 1 second
			val = (cg.time - cent->currentState.constantLight) * 0.001f;
			shader = media.gfx.world.greenFrontFlash;
		}
		else if ( cent->currentState.weapon == WP_DEMP2 ) {
			val = (cg.time - cent->currentState.constantLight) * 0.001f;
			shader = media.gfx.world.lightningFlash;
			scale = 1.75f;
		}

		if ( val < 0.0f )
			val = 0.0f;
		else if ( val > 1.0f ) {
			val = 1.0f;
			if ( ps && cent->currentState.number == ps->clientNum )
				CGCam_Shake( 0.2f, 100 );
		}
		else if ( ps && cent->currentState.number == ps->clientNum )
			CGCam_Shake( val * val * 0.6f, 100 );

		val += random() * 0.5f;

		VectorCopy( &flashorigin, &fxSArgs.origin );
		VectorClear( &fxSArgs.vel );
		VectorClear( &fxSArgs.accel );
		fxSArgs.scale = 3.0f*val*scale;
		fxSArgs.dscale = 0.0f;
		fxSArgs.sAlpha = 0.7f;
		fxSArgs.eAlpha = 0.7f;
		fxSArgs.rotation = random() * 360;
		fxSArgs.bounce = 0.0f;
		fxSArgs.life = 1.0f;
		fxSArgs.shader = shader;
		fxSArgs.flags = 0x08000000;

		trap->FX_AddSprite( &fxSArgs );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum then this is a fake player (like on the
	//	single player podiums), so go ahead and use the cent
	if ( nonPredictedCent - cg_entities != cent->currentState.clientNum )
		nonPredictedCent = cent;

	// add the flash
	if ( weaponNum != WP_DEMP2 || (nonPredictedCent->currentState.eFlags & EF_FIRING) ) {
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME )
			return;
	}

	if ( ps || cg.renderingThirdPerson || cg_trueGuns.integer
		|| cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		// Make sure we don't do the thirdperson model effects for the local player if we're in first person
		vector3 flashorigin, flashdir;
		refEntity_t	tpflash;

		memset( &tpflash, 0, sizeof(tpflash) );

		if ( !thirdPerson ) {
			CG_PositionEntityOnTag( &tpflash, &gun, gun.hModel, "tag_flash" );
			VectorCopy( &tpflash.origin, &flashorigin );
			VectorCopy( &tpflash.axis[0], &flashdir );
		}
		else {
			mdxaBone_t boltMatrix;

			// it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
			if ( !trap->G2API_HasGhoul2ModelOnIndex( &(cent->ghoul2), 1 ) )
				return;

			// Couldn't find bolt point.
			if ( !trap->G2API_GetBoltMatrix( cent->ghoul2, 1, 0, &boltMatrix, newAngles, &cent->lerpOrigin, cg.time, cgs.gameModels, &cent->modelScale ) )
				return;

			BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, &flashorigin );
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_X, &flashdir );
		}

		if ( cg.time - cent->muzzleFlashTime <= MUZZLE_FLASH_TIME + 10 ) {
			// Handle muzzle flashes
			if ( cent->currentState.eFlags & EF_ALT_FIRING ) {
				// Check the alt firing first.
				if ( weapon->altMuzzleEffect ) {
					if ( !thirdPerson )
						trap->FX_PlayEntityEffectID( weapon->altMuzzleEffect, &flashorigin, tpflash.axis, -1, -1, -1, -1 );
					else
						trap->FX_PlayEffectID( weapon->altMuzzleEffect, &flashorigin, &flashdir, -1, -1, qfalse );
				}
			}
			else {
				// Regular firing
				if ( weapon->muzzleEffect ) {
					if ( !thirdPerson )
						trap->FX_PlayEntityEffectID( weapon->muzzleEffect, &flashorigin, tpflash.axis, -1, -1, -1, -1 );
					else
						trap->FX_PlayEffectID( weapon->muzzleEffect, &flashorigin, &flashdir, -1, -1, qfalse );
				}
			}
		}

		if ( weapon->flashDlightColor.r || weapon->flashDlightColor.g || weapon->flashDlightColor.b ) {
			trap->R_AddLightToScene( &flashorigin, 300 + (rand() & 31), weapon->flashDlightColor.r, weapon->flashDlightColor.g,
				weapon->flashDlightColor.b );
		}
	}
}

// Add the weapon, and flash for the player's view
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t hand;
	centity_t *cent = NULL;
	clientInfo_t *ci = NULL;
	vector3 angles;
	weaponInfo_t *weapon = NULL;
	float cgFov = 0.0f;
	refdef_t *refdef = CG_GetRefdef();

	// no gun if in third person view or a camera is active
	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR || ps->pm_type == PM_INTERMISSION || cg.renderingThirdPerson )
		return;

	if ( !cg.renderingThirdPerson
		&& (cg_trueGuns.integer || cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE)
		&& cg_trueFOV.value
		&& cg.predictedPlayerState.pm_type != PM_SPECTATOR
		&& cg.predictedPlayerState.pm_type != PM_INTERMISSION ) {
		cgFov = cg_fovViewmodel.integer ? cg_fovViewmodel.value : cg_trueFOV.value;
	}
	else
		cgFov = cg_fovViewmodel.integer ? cg_fovViewmodel.value : cg_fov.value;

	cgFov = Q_clampi( 1, cgFov, 180 );

	// allow the gun to be completely removed
	if ( !cg.japp.fakeGun && (!cg_drawGun.integer || cg.predictedPlayerState.zoomMode || cg_trueGuns.integer
		|| cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE) ) {
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun )
		return;

	cent = &cg_entities[cg.predictedPlayerState.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = cg.japp.fakeGun ? &cg_weapons[WP_BRYAR_PISTOL] : &cg_weapons[ps->weapon];

	memset( &hand, 0, sizeof(hand) );

	// set up gun position
	CG_CalculateWeaponPosition( &hand.origin, &angles );

	VectorMA( &hand.origin, cg.gunAlign.x, &refdef->viewaxis[0], &hand.origin );
	VectorMA( &hand.origin, cg.gunAlign.y, &refdef->viewaxis[1], &hand.origin );
	VectorMA( &hand.origin, cg.gunAlign.z, &refdef->viewaxis[2], &hand.origin );

	AnglesToAxis( &angles, hand.axis );

	if ( cg_fovViewmodel.integer ) {
		float fracDistFOV, fracWeapFOV;
		if ( cg_fovAspectAdjust.integer ) {
			// Based on LordHavoc's code for Darkplaces
			// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
			const float baseAspect = 0.75f; // 3/4
			const float aspect = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;
			const float desiredFov = cgFov;

			cgFov = atanf( tanf( desiredFov*M_PI / 360.0f ) * baseAspect*aspect )*360.0f / M_PI;
		}
		fracDistFOV = tanf( CG_GetRefdef()->fov_x * (M_PI / 180) * 0.5f );
		fracWeapFOV = (1.0f / fracDistFOV) * tanf( cgFov * (M_PI / 180) * 0.5f );
		VectorScale( &hand.axis[0], fracWeapFOV, &hand.axis[0] );
	}

	// map torso animations to weapon animations
	if ( cg_debugGunFrame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_debugGunFrame.integer;
		hand.backlerp = 0;
	}
	else {
		float currentFrame;

		// get clientinfo for animation map
		if ( cent->currentState.eType == ET_NPC ) {
			if ( !cent->npcClient )
				return;
			ci = cent->npcClient;
		}
		else
			ci = &cgs.clientinfo[cent->currentState.clientNum];

		//Raz: Smoother first-person anims by eezstreet http://jkhub.org/topic/1499-/
		//		actually ported from SP
#if 1
		// Sil's fix
		trap->G2API_GetBoneFrame( cent->ghoul2, "lower_lumbar", cg.time, &currentFrame, cgs.gameModels, 0 );
		hand.frame = CG_MapTorsoToWeaponFrame( ci, ceil( currentFrame ), cent->currentState.torsoAnim );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, floor( currentFrame ), cent->currentState.torsoAnim );
		hand.backlerp = 1.0f - (currentFrame - floor( currentFrame ));
#else
		// basejka style
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame, cent->currentState.torsoAnim );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame, cent->currentState.torsoAnim );
		hand.backlerp = cent->pe.torso.backlerp;
#endif

		// Handle the fringe situation where oldframe is invalid
		if ( hand.frame == -1 ) {
			hand.frame = 0;
			hand.oldframe = 0;
			hand.backlerp = 0;
		}
		else if ( hand.oldframe == -1 ) {
			hand.oldframe = hand.frame;
			hand.backlerp = 0;
		}
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;// | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], &angles, qfalse );
}

#define ICON_WEAPONS	0
#define ICON_FORCE		1
#define ICON_INVENTORY	2
void CG_DrawIconBackground( void ) {
	int t;
	float inTime = cg.invenSelectTime + WEAPON_SELECT_TIME;
	float wpTime = cg.weaponSelectTime + WEAPON_SELECT_TIME;
	float fpTime = cg.forceSelectTime + WEAPON_SELECT_TIME;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
		return;

	// simple hud
	if ( cg_hudFiles.integer )
		return;

	if ( inTime > wpTime )
		cg.iconSelectTime = cg.invenSelectTime;
	else
		cg.iconSelectTime = cg.weaponSelectTime;

	if ( fpTime > inTime && fpTime > wpTime )
		cg.iconSelectTime = cg.forceSelectTime;

	if ( (cg.iconSelectTime + WEAPON_SELECT_TIME) < cg.time ) {
		// Time is up for the HUD to display
		if ( cg.iconHUDActive ) {
			// The time is up, but we still need to move the prongs back to their original position
			t = cg.time - (cg.iconSelectTime + WEAPON_SELECT_TIME);
			cg.iconHUDPercent = t / 130.0f;
			cg.iconHUDPercent = 1 - cg.iconHUDPercent;

			if ( cg.iconHUDPercent < 0 ) {
				cg.iconHUDActive = qfalse;
				cg.iconHUDPercent = 0;
			}
		}

		return;
	}

	if ( !cg.iconHUDActive ) {
		t = cg.time - cg.iconSelectTime;
		cg.iconHUDPercent = t / 130.0f;

		// Calc how far into opening sequence we are
		if ( cg.iconHUDPercent > 1 ) {
			cg.iconHUDActive = qtrue;
			cg.iconHUDPercent = 1;
		}
		else if ( cg.iconHUDPercent < 0 )
			cg.iconHUDPercent = 0;
	}
	else
		cg.iconHUDPercent = 1;
}

qboolean CG_WeaponCheck( int weap ) {
	if ( cg.snap->ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].energyPerShot &&
		cg.snap->ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].altEnergyPerShot ) {
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_WeaponSelectable( int i ) {
	if ( !i )
		return qfalse;

	if ( cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot
		&& cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < weaponData[i].altEnergyPerShot ) {
		return qfalse;
	}

	if ( i == WP_DET_PACK && cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < 1
		&& !cg.predictedPlayerState.hasDetPackPlanted ) {
		return qfalse;
	}

	if ( !(cg.predictedPlayerState.stats[STAT_WEAPONS] & (1 << i)) )
		return qfalse;

	return qtrue;
}

void CG_DrawWeaponSelect( void ) {
	int i, bits, count, smallIconSize, bigIconSize, holdX, x, y, pad, sideLeftIconCnt, sideRightIconCnt, sideMax,
		holdCount, iconCnt, yOffset = 0;
	qboolean drewConc = qfalse;

	// can't cycle when on a weapon
	if ( cg.predictedPlayerState.emplacedIndex )
		cg.weaponSelectTime = 0;

	// Time is up for the HUD to display
	if ( (cg.weaponSelectTime + WEAPON_SELECT_TIME) < cg.time )
		return;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 )
		return;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	bits = cg.predictedPlayerState.stats[STAT_WEAPONS];

	// count the number of weapons owned
	count = 0;

	// display this weapon that we don't actually "have" as unhighlighted until it's deselected
	// since it's selected we must increase the count to display the proper number of valid selectable weapons
	if ( !CG_WeaponSelectable( cg.weaponSelect ) && (cg.weaponSelect == WP_THERMAL || cg.weaponSelect == WP_TRIP_MINE) )
		count++;

	for ( i = 1; i < WP_NUM_WEAPONS; i++ ) {
		if ( bits & (1 << i) ) {
			if ( CG_WeaponSelectable( i ) || (i != WP_THERMAL && i != WP_TRIP_MINE) )
				count++;
		}
	}

	if ( !count ) // If no weapons, don't display
		return;

	sideMax = 3; // Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if ( holdCount == 0 ) {
		// No icons to either side
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if ( count > (2 * sideMax) ) {
		// Go to the max on each side
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else {
		// Less than max, so do the calc
		sideLeftIconCnt = holdCount / 2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	if ( cg.weaponSelect == WP_CONCUSSION )
		i = WP_FLECHETTE;
	else
		i = cg.weaponSelect - 1;
	if ( i < 1 )
		i = LAST_USEABLE_WEAPON;

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 12;

	x = (SCREEN_WIDTH / 2);
	y = 410;

	// Left side ICONS
	trap->R_SetColor( &colorTable[CT_WHITE] );
	// Work backwards from current icon
	holdX = x - ((bigIconSize / 2) + pad + smallIconSize);
	drewConc = qfalse;

	for ( iconCnt = 1; iconCnt < (sideLeftIconCnt + 1); i-- ) {
		if ( i == WP_CONCUSSION )
			i--;
		else if ( i == WP_FLECHETTE && !drewConc && cg.weaponSelect != WP_CONCUSSION )
			i = WP_CONCUSSION;
		if ( i < 1 )
			i = LAST_USEABLE_WEAPON;

		// Does he have this weapon?
		if ( !(bits & (1 << i)) ) {
			if ( i == WP_CONCUSSION ) {
				drewConc = qtrue;
				i = WP_ROCKET_LAUNCHER;
			}
			continue;
		}

		// Don't show thermal and tripmine when out of them
		if ( !CG_WeaponSelectable( i ) && (i == WP_THERMAL || i == WP_TRIP_MINE) )
			continue;

		++iconCnt;					// Good icon

		if ( media.gfx.interface.weaponIcons[i] ) {
			CG_RegisterWeapon( i );

			trap->R_SetColor( &colorTable[CT_WHITE] );
			if ( !CG_WeaponCheck( i ) )
				CG_DrawPic( holdX, y + 10 + yOffset, smallIconSize, smallIconSize, media.gfx.interface.weaponIconsInactive[i] );
			else
				CG_DrawPic( holdX, y + 10 + yOffset, smallIconSize, smallIconSize, media.gfx.interface.weaponIcons[i] );

			holdX -= (smallIconSize + pad);
		}
		if ( i == WP_CONCUSSION ) {
			drewConc = qtrue;
			i = WP_ROCKET_LAUNCHER;
		}
	}

	// Current Center Icon
	if ( media.gfx.interface.weaponIcons[cg.weaponSelect] ) {
		CG_RegisterWeapon( cg.weaponSelect );

		trap->R_SetColor( &colorTable[CT_WHITE] );
		if ( !CG_WeaponCheck( cg.weaponSelect ) )
			CG_DrawPic( x - (bigIconSize / 2), (y - ((bigIconSize - smallIconSize) / 2)) + 10 + yOffset, bigIconSize, bigIconSize, media.gfx.interface.weaponIconsInactive[cg.weaponSelect] );
		else
			CG_DrawPic( x - (bigIconSize / 2), (y - ((bigIconSize - smallIconSize) / 2)) + 10 + yOffset, bigIconSize, bigIconSize, media.gfx.interface.weaponIcons[cg.weaponSelect] );
	}

	if ( cg.weaponSelect == WP_CONCUSSION )
		i = WP_ROCKET_LAUNCHER;
	else
		i = cg.weaponSelect + 1;

	if ( i > LAST_USEABLE_WEAPON )
		i = 1;

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize / 2) + pad;
	for ( iconCnt = 1; iconCnt<(sideRightIconCnt + 1); i++ ) {
		if ( i == WP_CONCUSSION )
			i++;
		else if ( i == WP_ROCKET_LAUNCHER && !drewConc && cg.weaponSelect != WP_CONCUSSION )
			i = WP_CONCUSSION;
		if ( i > LAST_USEABLE_WEAPON )
			i = 1;

		if ( !(bits & (1 << i)) ) {
			if ( i == WP_CONCUSSION ) {
				drewConc = qtrue;
				i = WP_FLECHETTE;
			}
			continue;
		}

		// Don't show thermal and tripmine when out of them
		if ( !CG_WeaponSelectable( i ) && (i == WP_THERMAL || i == WP_TRIP_MINE) )
			continue;

		++iconCnt; // Good icon

		if ( media.gfx.interface.weaponIcons[i] ) {
			CG_RegisterWeapon( i );
			// No ammo for this weapon?
			trap->R_SetColor( &colorTable[CT_WHITE] );
			if ( !CG_WeaponCheck( i ) )
				CG_DrawPic( holdX, y + 10 + yOffset, smallIconSize, smallIconSize, media.gfx.interface.weaponIconsInactive[i] );
			else
				CG_DrawPic( holdX, y + 10 + yOffset, smallIconSize, smallIconSize, media.gfx.interface.weaponIcons[i] );


			holdX += (smallIconSize + pad);
		}
		if ( i == WP_CONCUSSION ) {
			drewConc = qtrue;
			i = WP_FLECHETTE;
		}
	}

	// draw the selected name
	if ( cg_weapons[cg.weaponSelect].item ) {
		vector4			textColor = { .875f, .718f, .121f, 1.0f };
		char text[1024], upperKey[1024];

		strcpy( upperKey, cg_weapons[cg.weaponSelect].item->classname );
		Q_strupr( upperKey );

		if ( trap->SE_GetStringTextString( va( "SP_INGAME_%s", upperKey ), text, sizeof(text) ) )
			UI_DrawProportionalString( (SCREEN_WIDTH / 2), y + 45 + yOffset, text, UI_CENTER | UI_SMALLFONT, &textColor );
		else
			UI_DrawProportionalString( (SCREEN_WIDTH / 2), y + 45 + yOffset, cg_weapons[cg.weaponSelect].item->classname, UI_CENTER | UI_SMALLFONT, &textColor );
	}

	trap->R_SetColor( NULL );
}


void CG_NextWeapon_f( void ) {
	int i, original;

	if ( !cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.pm_type == PM_SPECTATOR )
		return;

	if ( cg.snap->ps.emplacedIndex )
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if ( cg.weaponSelect == WP_FLECHETTE )
			cg.weaponSelect = WP_CONCUSSION;
		else if ( cg.weaponSelect == WP_CONCUSSION )
			cg.weaponSelect = WP_ROCKET_LAUNCHER;
		else if ( cg.weaponSelect == WP_DET_PACK )
			cg.weaponSelect = WP_BRYAR_OLD;
		else
			cg.weaponSelect++;

		if ( cg.weaponSelect == WP_NUM_WEAPONS )
			cg.weaponSelect = 0;
		if ( CG_WeaponSelectable( cg.weaponSelect ) )
			break;
	}
	if ( i == WP_NUM_WEAPONS )
		cg.weaponSelect = original;
	else
		trap->S_MuteSound( cg.snap->ps.clientNum, CHAN_WEAPON );
}

void CG_PrevWeapon_f( void ) {
	int i, original;

	if ( !cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.pm_type == PM_SPECTATOR )
		return;

	if ( cg.snap->ps.emplacedIndex )
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if ( cg.weaponSelect == WP_ROCKET_LAUNCHER )
			cg.weaponSelect = WP_CONCUSSION;
		else if ( cg.weaponSelect == WP_CONCUSSION )
			cg.weaponSelect = WP_FLECHETTE;
		else if ( cg.weaponSelect == WP_BRYAR_OLD )
			cg.weaponSelect = WP_DET_PACK;
		else
			cg.weaponSelect--;
		if ( cg.weaponSelect == -1 )
			cg.weaponSelect = WP_NUM_WEAPONS - 1;
		if ( CG_WeaponSelectable( cg.weaponSelect ) )
			break;
	}
	if ( i == WP_NUM_WEAPONS )
		cg.weaponSelect = original;
	else
		trap->S_MuteSound( cg.snap->ps.clientNum, CHAN_WEAPON );
}

void CG_Weapon_f( void ) {
	int num;

	if ( !cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.snap->ps.emplacedIndex )
		return;

	num = atoi( CG_Argv( 1 ) );
	if ( num < 1 || num > LAST_USEABLE_WEAPON )
		return;

	if ( num == 1 && cg.snap->ps.weapon == WP_SABER ) {
		if ( cg.snap->ps.weaponTime < 1 )
			trap->SendConsoleCommand( "sv_saberswitch\n" );
		return;
	}

	//rww - hack to make weapon numbers same as single player
	if ( num > WP_STUN_BATON )
		num += 2;
	else {
		if ( cg.snap->ps.stats[STAT_WEAPONS] & (1 << WP_SABER) )
			num = WP_SABER;
		else
			num = WP_MELEE;
	}

	if ( num > LAST_USEABLE_WEAPON + 1 )
		return;

	if ( num >= WP_THERMAL && num <= WP_DET_PACK ) {
		int weap, i = 0;

		// already in cycle range so start with next cycle item
		if ( cg.snap->ps.weapon >= WP_THERMAL && cg.snap->ps.weapon <= WP_DET_PACK )
			weap = cg.snap->ps.weapon + 1;
		else // not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;

		// prevent an endless loop
		while ( i <= 4 ) {
			if ( weap > WP_DET_PACK )
				weap = WP_THERMAL;

			if ( CG_WeaponSelectable( weap ) ) {
				num = weap;
				break;
			}

			weap++;
			i++;
		}
	}

	if ( !CG_WeaponSelectable( num ) )
		return;

	cg.weaponSelectTime = cg.time;

	if ( !(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)) ) {
		if ( num == WP_SABER ) {
			// don't have saber, try melee on the same slot
			num = WP_MELEE;

			if ( !(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)) )
				return;
		}
		else
			return; // don't have the weapon
	}

	if ( cg.weaponSelect != num )
		trap->S_MuteSound( cg.snap->ps.clientNum, CHAN_WEAPON );

	cg.weaponSelect = num;
}

// Version of the above which doesn't add +2 to a weapon. The above can't trigger WP_MELEE or WP_STUN_BATON.
//	Derogatory comments go here.
void CG_WeaponClean_f( void ) {
	int num;

	if ( !cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.snap->ps.emplacedIndex )
		return;

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > LAST_USEABLE_WEAPON )
		return;

	if ( num == 1 && cg.snap->ps.weapon == WP_SABER ) {
		if ( cg.predictedPlayerState.weaponTime < 1 )
			trap->SendConsoleCommand( "sv_saberswitch\n" );
		return;
	}

	if ( num == WP_STUN_BATON ) {
		if ( cg.snap->ps.stats[STAT_WEAPONS] & (1 << WP_SABER) )
			num = WP_SABER;
		else
			num = WP_MELEE;
	}

	// other weapons are off limits due to not actually being weapon weapons
	if ( num > LAST_USEABLE_WEAPON + 1 )
		return;

	if ( num >= WP_THERMAL && num <= WP_DET_PACK ) {
		int weap, i;

		if ( cg.snap->ps.weapon >= WP_THERMAL && cg.snap->ps.weapon <= WP_DET_PACK )
			weap = cg.snap->ps.weapon + 1;
		else
			weap = WP_THERMAL;

		// prevent an endless loop
		for ( i = 0; i <= 4; i++ ) {
			if ( weap > WP_DET_PACK )
				weap = WP_THERMAL;

			if ( CG_WeaponSelectable( weap ) ) {
				num = weap;
				break;
			}

			weap++;
		}
	}

	if ( !CG_WeaponSelectable( num ) )
		return;

	cg.weaponSelectTime = cg.time;

	if ( !(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)) ) {
		if ( num == WP_SABER ) {
			// don't have saber, try melee on the same slot
			num = WP_MELEE;

			if ( !(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)) )
				return;
		}
		else
			return; // don't have the weapon
	}

	if ( cg.weaponSelect != num )
		trap->S_MuteSound( cg.snap->ps.clientNum, CHAN_WEAPON );

	cg.weaponSelect = num;
}

// The current weapon has just run out of ammo
void CG_OutOfAmmoChange( int oldWeapon ) {
	int i;

	cg.weaponSelectTime = cg.time;

	for ( i = LAST_USEABLE_WEAPON; i > 0; i-- ) {
		// We don't want the emplaced or turret
		if ( CG_WeaponSelectable( i ) ) {
			//rww - Don't we want to make sure i != one of these if autoswitch is 1 (safe)?
			if ( cg_autoSwitch.integer != 1 || (i != WP_TRIP_MINE && i != WP_DET_PACK && i != WP_THERMAL && i != WP_ROCKET_LAUNCHER) ) {
				if ( i != oldWeapon ) {
					// don't even do anything if we're just selecting the weapon we already have/had
					cg.weaponSelect = i;
					break;
				}
			}
		}
	}

	trap->S_MuteSound( cg.snap->ps.clientNum, CHAN_WEAPON );
}

void CG_GetWeaponMuzzleBolt( int clIndex, vector3 *to ) {
	centity_t *cent;
	mdxaBone_t	boltMatrix;

	if ( clIndex < 0 || clIndex >= MAX_CLIENTS )
		return;

	cent = &cg_entities[clIndex];

	if ( !cent || !cent->ghoul2 || !trap->G2_HaveWeGhoul2Models( cent->ghoul2 )
		|| !trap->G2API_HasGhoul2ModelOnIndex( &(cent->ghoul2), 1 ) ) {
		return;
	}

	trap->G2API_GetBoltMatrix( cent->ghoul2, 1, 0, &boltMatrix, &cent->turAngles, &cent->lerpOrigin, cg.time, cgs.gameModels, &cent->modelScale );
	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, to );
}

// Caused by an EV_FIRE_WEAPON event
void CG_FireWeapon( centity_t *cent, qboolean altFire ) {
	entityState_t *ent;
	int c;
	weaponInfo_t *weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE )
		return;
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		trap->Error( ERR_DROP, "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ent->weapon];

	// mark the entity as muzzle flashing, so when it is added it will append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	if ( cg.predictedPlayerState.clientNum == cent->currentState.number ) {
		if ( (altFire && (ent->weapon == WP_BRYAR_PISTOL || ent->weapon == WP_BRYAR_OLD || ent->weapon == WP_DEMP2)) ||
			(!altFire && ent->weapon == WP_BOWCASTER) ) {
			float val = (cg.time - cent->currentState.constantLight) * 0.001f;

			if ( val > 3 )
				val = 3;
			if ( val < 0.2f )
				val = 0.2f;

			val *= 2;

			CGCam_Shake( val, 250 );
		}
		else if ( ent->weapon == WP_ROCKET_LAUNCHER ||
			(ent->weapon == WP_REPEATER && altFire) ||
			ent->weapon == WP_FLECHETTE ||
			(ent->weapon == WP_CONCUSSION && !altFire) ) {
			if ( ent->weapon == WP_CONCUSSION ) {
				// gives an advantage to being in 3rd person, but would look silly otherwise
				if ( !cg.renderingThirdPerson ) {
					// kick the view back
					cg.kick_angles.pitch = flrand( -10, -15 );
					cg.kick_time = cg.time;
				}
			}
			else if ( ent->weapon == WP_ROCKET_LAUNCHER )
				CGCam_Shake( flrand( 2, 3 ), 350 );
			else if ( ent->weapon == WP_REPEATER )
				CGCam_Shake( flrand( 2, 3 ), 350 );
			else if ( ent->weapon == WP_FLECHETTE ) {
				if ( altFire )
					CGCam_Shake( flrand( 2, 3 ), 350 );
				else
					CGCam_Shake( 1.5f, 250 );
			}
		}
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & (1 << PW_QUAD) ) {
		//	trap->S_StartSound( NULL, cent->currentState.number, CHAN_ITEM, media.snd.quad );
	}

	// play a sound
	if ( altFire ) {
		// play a sound
		for ( c = 0; c<4; c++ ) {
			if ( !weap->altFlashSound[c] )
				break;
		}
		if ( c > 0 ) {
			c = rand() % c;
			if ( weap->altFlashSound[c] )
				trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound[c] );
		}
	}
	else {
		// play a sound
		for ( c = 0; c<4; c++ ) {
			if ( !weap->flashSound[c] )
				break;
		}
		if ( c > 0 ) {
			c = rand() % c;
			if ( weap->flashSound[c] )
				trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}
}

qboolean CG_VehicleWeaponImpact( centity_t *cent ) {
	// see if this is a missile entity that's owned by a vehicle and should do a special, overridden impact effect
	if ( (cent->currentState.eFlags & EF_JETPACK_ACTIVE) //hack so we know we're a vehicle Weapon shot
		&& cent->currentState.otherEntityNum2
		&& g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX ) {
		// missile is from a special vehWeapon
		vector3 normal;
		ByteToDir( cent->currentState.eventParm, &normal );

		trap->FX_PlayEffectID( g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX, &cent->lerpOrigin, &normal, -1, -1, qfalse );
		return qtrue;
	}
	return qfalse;
}

// Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
void CG_MissileHitWall( int weapon, int clientNum, vector3 *origin, vector3 *dir, impactSound_t soundType, qboolean altFire, int charge ) {
	int parm;
	static const vector3 up = { 0, 0, 1 };

	switch ( weapon ) {
	case WP_BRYAR_PISTOL:
		if ( altFire ) {
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
			FX_BryarHitWall( origin, dir );
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall( origin, dir );
		break;

	case WP_BRYAR_OLD:
		if ( altFire ) {
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
			FX_BryarHitWall( origin, dir );
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
			FX_RepeaterAltHitWall( origin, dir );
		else
			FX_RepeaterHitWall( origin, dir );
		break;

	case WP_DEMP2:
		if ( altFire )
			trap->FX_PlayEffectID( media.efx.demp2.altDetonate, origin, dir, -1, -1, qfalse );
		else
			FX_DEMP2_HitWall( origin, dir );
		break;

	case WP_FLECHETTE:
		if ( !altFire )
			FX_FlechetteWeaponHitWall( origin, dir );
		break;

	case WP_ROCKET_LAUNCHER:
		FX_RocketHitWall( origin, dir );
		break;

	case WP_THERMAL:
		trap->FX_PlayEffectID( media.efx.thermal.explosion, origin, dir, -1, -1, qfalse );
		trap->FX_PlayEffectID( media.efx.thermal.shockwave, origin, &up, -1, -1, qfalse );
		break;

	case WP_EMPLACED_GUN:
		FX_BlasterWeaponHitWall( origin, dir );
		//FIXME: Give it its own hit wall effect
		break;

	default:
		break;
	}
}

void CG_MissileHitPlayer( int weapon, vector3 *origin, vector3 *dir, int entityNum, qboolean altFire ) {
	qboolean humanoid = qtrue;
	static const vector3 up = { 0, 0, 1 };

	// some weapons will make an explosion with the blood, while others will just make the blood
	switch ( weapon ) {
	case WP_BRYAR_PISTOL:
		if ( altFire )
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		else
			FX_BryarHitPlayer( origin, dir, humanoid );
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitPlayer( origin, dir, humanoid );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		else
			FX_BryarHitPlayer( origin, dir, humanoid );
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
			FX_RepeaterAltHitPlayer( origin, dir, humanoid );
		else
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		break;

	case WP_DEMP2:
		if ( altFire )
			trap->FX_PlayEffectID( media.efx.demp2.altDetonate, origin, dir, -1, -1, qfalse );
		else
			FX_DEMP2_HitPlayer( origin, dir, humanoid );
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		FX_RocketHitPlayer( origin, dir, humanoid );
		break;

	case WP_THERMAL:
		trap->FX_PlayEffectID( media.efx.thermal.explosion, origin, dir, -1, -1, qfalse );
		trap->FX_PlayEffectID( media.efx.thermal.shockwave, origin, &up, -1, -1, qfalse );
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}

qboolean CG_CalcMuzzlePoint( int entityNum, vector3 *muzzle ) {
	vector3 forward, right, gunpoint;
	centity_t *cent;
	int anim;
	refdef_t *refdef = CG_GetRefdef();

	if ( entityNum == cg.snap->ps.clientNum ) {
		// I'm not exactly sure why we'd be rendering someone else's crosshair, but hey.
		int weapontype = cg.snap->ps.weapon;
		vector3 weaponMuzzle;
		centity_t *pEnt = &cg_entities[cg.predictedPlayerState.clientNum];

		VectorCopy( &WP_MuzzlePoint[weapontype], &weaponMuzzle );

		if ( weapontype == WP_DISRUPTOR || weapontype == WP_STUN_BATON || weapontype == WP_MELEE || weapontype == WP_SABER )
			VectorClear( &weaponMuzzle );

		if ( cg.renderingThirdPerson ) {
			VectorCopy( &pEnt->lerpOrigin, &gunpoint );
			AngleVectors( &pEnt->lerpAngles, &forward, &right, NULL );
		}
		else {
			VectorCopy( &refdef->vieworg, &gunpoint );
			AngleVectors( &refdef->viewangles, &forward, &right, NULL );
		}

		if ( weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex ) {
			centity_t *gunEnt = &cg_entities[cg.snap->ps.emplacedIndex];

			if ( gunEnt ) {
				vector3 pitchConstraint;

				VectorCopy( &gunEnt->lerpOrigin, &gunpoint );
				gunpoint.z += 46;

				if ( cg.renderingThirdPerson )
					VectorCopy( &pEnt->lerpAngles, &pitchConstraint );
				else
					VectorCopy( &refdef->viewangles, &pitchConstraint );

				if ( pitchConstraint.pitch > 40 )
					pitchConstraint.pitch = 40;
				AngleVectors( &pitchConstraint, &forward, &right, NULL );
			}
		}

		VectorCopy( &gunpoint, muzzle );

		VectorMA( muzzle, weaponMuzzle.x, &forward, muzzle );
		VectorMA( muzzle, weaponMuzzle.y, &right, muzzle );

		if ( weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex ) {
			// ...
		}
		else if ( cg.renderingThirdPerson )
			muzzle->z += cg.snap->ps.viewheight + weaponMuzzle.z;
		else
			muzzle->z += weaponMuzzle.z;

		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid )
		return qfalse;

	VectorCopy( &cent->currentState.pos.trBase, muzzle );

	AngleVectors( &cent->currentState.apos.trBase, &forward, NULL, NULL );
	anim = cent->currentState.legsAnim;
	if ( anim == BOTH_CROUCH1WALK || anim == BOTH_CROUCH1IDLE )
		muzzle->z += CROUCH_VIEWHEIGHT;
	else
		muzzle->z += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzle, 14, &forward, muzzle );

	return qtrue;

}

// create one instance of all the weapons we are going to use so we can just copy this info into each clients gun ghoul2
//	object in fast way
static void *g2WeaponInstances[MAX_WEAPONS];

void CG_InitG2Weapons( void ) {
	int i = 0;
	const gitem_t *item;

	memset( g2WeaponInstances, 0, sizeof(g2WeaponInstances) );

	for ( item = bg_itemlist + 1; item->classname; item++ ) {
		if ( item->giType == IT_WEAPON ) {
			assert( item->giTag < MAX_WEAPONS );

			trap->G2API_InitGhoul2Model( &g2WeaponInstances[item->giTag], item->world_model[0], 0, 0, 0, 0, 0 );
			if ( g2WeaponInstances[item->giTag] ) {
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap->G2API_SetBoltInfo( g2WeaponInstances[item->giTag], 0, 0 );
				// now set up the gun bolt on it
				if ( item->giTag == WP_SABER )
					trap->G2API_AddBolt( g2WeaponInstances[item->giTag], 0, "*blade1" );
				else
					trap->G2API_AddBolt( g2WeaponInstances[item->giTag], 0, "*flash" );
				i++;
			}
			if ( i == MAX_WEAPONS ) {
				assert( 0 );
				break;
			}
		}
	}
}

// clean out any g2 models we instanciated for copying purposes
void CG_ShutDownG2Weapons( void ) {
	int i;
	for ( i = 0; i < MAX_WEAPONS; i++ )
		trap->G2API_CleanGhoul2Models( &g2WeaponInstances[i] );
}

void *CG_G2WeaponInstance( centity_t *cent, int weapon ) {
	clientInfo_t *ci = NULL;

	if ( weapon != WP_SABER )
		return g2WeaponInstances[weapon];

	if ( cent->currentState.eType != ET_PLAYER && cent->currentState.eType != ET_NPC )
		return g2WeaponInstances[weapon];

	if ( cent->currentState.eType == ET_NPC )
		ci = cent->npcClient;
	else
		ci = &cgs.clientinfo[cent->currentState.number];

	if ( !ci )
		return g2WeaponInstances[weapon];

	// Try to return the custom saber instance if we can.
	if ( ci->saber[0].model[0] && ci->ghoul2Weapons[0] )
		return ci->ghoul2Weapons[0];

	// If no custom then just use the default.
	return g2WeaponInstances[weapon];
}

// what ghoul2 model do we want to copy ?
void CG_CopyG2WeaponInstance( centity_t *cent, int weaponNum, void *toGhoul2 ) {
	//rww - the -1 is because there is no "weapon" for WP_NONE
	assert( weaponNum < MAX_WEAPONS );
	if ( CG_G2WeaponInstance( cent, weaponNum ) ) {
		if ( weaponNum == WP_SABER ) {
			clientInfo_t *ci = NULL;

			if ( cent->currentState.eType == ET_NPC )
				ci = cent->npcClient;
			else
				ci = &cgs.clientinfo[cent->currentState.number];

			if ( !ci )
				trap->G2API_CopySpecificGhoul2Model( CG_G2WeaponInstance( cent, weaponNum ), 0, toGhoul2, 1 );
			else {
				// Try both the left hand saber and the right hand saber
				int i;

				for ( i = 0; i < MAX_SABERS; i++ ) {
					if ( ci->saber[i].model[0] && ci->ghoul2Weapons[i] )
						trap->G2API_CopySpecificGhoul2Model( ci->ghoul2Weapons[i], 0, toGhoul2, i + 1 );
					else if ( ci->ghoul2Weapons[i] ) {
						// if the second saber has been removed, then be sure to remove it and free the instance.
						qboolean g2HasSecondSaber = trap->G2API_HasGhoul2ModelOnIndex( &(toGhoul2), 2 );

						// remove it now since we're switching away from sabers
						if ( g2HasSecondSaber )
							trap->G2API_RemoveGhoul2Model( &(toGhoul2), 2 );
						trap->G2API_CleanGhoul2Models( &ci->ghoul2Weapons[i] );
					}
				}
			}
		}
		else {
			qboolean g2HasSecondSaber = trap->G2API_HasGhoul2ModelOnIndex( &(toGhoul2), 2 );

			// remove it now since we're switching away from sabers
			if ( g2HasSecondSaber )
				trap->G2API_RemoveGhoul2Model( &(toGhoul2), 2 );

			if ( weaponNum == WP_EMPLACED_GUN ) {
				// a bit of a hack to remove gun model when using an emplaced weap
				if ( trap->G2API_HasGhoul2ModelOnIndex( &(toGhoul2), 1 ) )
					trap->G2API_RemoveGhoul2Model( &(toGhoul2), 1 );
			}
			else if ( weaponNum == WP_MELEE ) {
				// don't want a weapon on the model for this one
				if ( trap->G2API_HasGhoul2ModelOnIndex( &(toGhoul2), 1 ) )
					trap->G2API_RemoveGhoul2Model( &(toGhoul2), 1 );
			}
			else
				trap->G2API_CopySpecificGhoul2Model( CG_G2WeaponInstance( cent, weaponNum ), 0, toGhoul2, 1 );
		}
	}
}

void CG_CheckPlayerG2Weapons( playerState_t *ps, centity_t *cent ) {
	if ( !ps ) {
		assert( !"CG_CheckPlayerG2Weapons: NULL ps" );
		return;
	}

	if ( ps->pm_flags & PMF_FOLLOW )
		return;

	if ( cent->currentState.eType == ET_NPC ) {
		assert( !"CG_CheckPlayerG2Weapons on NPC" );
		return;
	}

	// should we change the gun model on this player?
	if ( cent->currentState.saberInFlight )
		cent->ghoul2weapon = CG_G2WeaponInstance( cent, WP_SABER );

	if ( cent->currentState.eFlags & EF_DEAD ) {
		// no updating weapons when dead
		cent->ghoul2weapon = NULL;
		return;
	}

	if ( cent->torsoBolt ) {
		// got our limb cut off, no updating weapons until it's restored
		cent->ghoul2weapon = NULL;
		return;
	}

	if ( cgs.clientinfo[ps->clientNum].team == TEAM_SPECTATOR || ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		cent->ghoul2weapon = cg_entities[ps->clientNum].ghoul2weapon = NULL;
		cent->weapon = cg_entities[ps->clientNum].weapon = 0;
		return;
	}

	if ( cent->ghoul2 && cent->ghoul2weapon != CG_G2WeaponInstance( cent, ps->weapon )
		&& ps->clientNum == cent->currentState.number ) //don't want spectator mode forcing one client's weapon instance over another's
	{
		CG_CopyG2WeaponInstance( cent, ps->weapon, cent->ghoul2 );
		cent->ghoul2weapon = CG_G2WeaponInstance( cent, ps->weapon );
		if ( cent->weapon == WP_SABER && cent->weapon != ps->weapon && !ps->saberHolstered ) {
			// switching away from the saber
			//	trap->S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" ) );
			if ( cgs.clientinfo[ps->clientNum].saber[0].soundOff && !ps->saberHolstered )
				trap->S_StartSound( &cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOff );

			if ( cgs.clientinfo[ps->clientNum].saber[1].soundOff && cgs.clientinfo[ps->clientNum].saber[1].model[0]
				&& !ps->saberHolstered ) {
				trap->S_StartSound( &cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOff );
			}
		}
		else if ( ps->weapon == WP_SABER && cent->weapon != ps->weapon && !cent->saberWasInFlight ) {
			// switching to the saber
			//	trap->S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberon.wav" ) );
			if ( cgs.clientinfo[ps->clientNum].saber[0].soundOn )
				trap->S_StartSound( &cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOn );

			if ( cgs.clientinfo[ps->clientNum].saber[1].soundOn )
				trap->S_StartSound( &cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOn );

			BG_SI_SetDesiredLength( &cgs.clientinfo[ps->clientNum].saber[0], 0, -1 );
			BG_SI_SetDesiredLength( &cgs.clientinfo[ps->clientNum].saber[1], 0, -1 );
		}
		cent->weapon = ps->weapon;
	}
}
