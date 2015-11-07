#include "cg_local.h"
#include "Ghoul2/G2.h"

#define		MAX_TRUEVIEW_INFO_SIZE		4096
char		true_view_info[MAX_TRUEVIEW_INFO_SIZE];
int			true_view_valid;

void CG_TrueViewInit( void ) {
	int len = 0;
	fileHandle_t f;

	len = trap->FS_Open( "trueview.cfg", &f, FS_READ );

	if ( !f ) {
		//	trap->Print("Error: File Not Found: trueview.cfg\n");
		true_view_valid = 0;
		return;
	}

	if ( len <= 0 || len >= MAX_TRUEVIEW_INFO_SIZE ) {
		//	trap->Print("Error: trueview.cfg is over the filesize limit. (%i)\n", MAX_TRUEVIEW_INFO_SIZE);
		trap->FS_Close( f );
		true_view_valid = 0;
		return;
	}

	trap->FS_Read( true_view_info, len, f );
	true_view_valid = 1;
	trap->FS_Close( f );
}

int BG_SiegeGetPairedValue( const char *buf, const char *key, char *outbuf );

//Tries to adjust the eye position from the data in cfg file if possible.
void CG_AdjustEyePos( const char *modelName ) {
	char eyepos[MAX_QPATH];

	if ( true_view_valid ) {
		if ( BG_SiegeGetPairedValue( true_view_info, (char *)modelName, eyepos ) ) {
			trap->Print( "True View Eye Adjust Loaded for %s.\n", modelName );
			trap->Cvar_Set( "cg_trueEyePosition", eyepos );
		}
		else
			trap->Cvar_Set( "cg_trueEyePosition", "0" );
	}
	else
		trap->Cvar_Set( "cg_trueEyePosition", "0" );
}

// Get the point in the leg animation and return a percentage of the current point in the anim between 0 and the total
//	anim length (0.0f - 1.0f)
static float GetSelfLegAnimPoint( void ) {
	return BG_GetLegsAnimPoint( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex );
}

// Get the point in the torso animation and return a percentage of the current point in the anim between 0 and the total
//	anim length (0.0f - 1.0f)
static float GetSelfTorsoAnimPoint( void ) {
	return BG_GetTorsoAnimPoint( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex );
}

// Uses the currently setup model-based First Person View to calculation the final viewangles.
// Features the following:
//	1. Simulates allowable eye movement by makes a deadzone around the inputed viewangles vs the desired viewangles of
//		refdef->viewangles
//	2. Prevents the sudden view flipping during moves where your camera is suppose to flip 360 on the pitch (x) pitch
//		(x) axis.
static void CG_SmoothTrueView( vector3 *eyeAngles ) {
	float legAnimPoint = GetSelfLegAnimPoint();
	float torsoAnimPoint = GetSelfTorsoAnimPoint();
	int i;
	float angDiff; //refdef->viewangles in relation to eyeAngles
	qboolean eyeRange = qtrue, useRefDef = qfalse, didSpecial = qfalse;
	refdef_t *refdef = CG_GetRefdef();

	//RAFIXME: See if I can find a link this to the prediction stuff.  I think the snap is of just the last gamestate snap

	// Rolls
	if ( cg_trueRoll.integer ) {
		if ( cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_STOP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_STOP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_LEFT
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_RIGHT ) {
			// Roll moves that look good with eye range
			eyeRange = qtrue;
			didSpecial = qtrue;
		}
		else if ( cg_trueRoll.integer == 1 ) {
			// Use simple roll for the more complicated rolls
			if ( cg.predictedPlayerState.legsAnim == BOTH_FLIP_L || cg.predictedPlayerState.legsAnim == BOTH_ROLL_L ) {
				// Left rolls
				VectorCopy( &refdef->viewangles, eyeAngles );
				eyeAngles->roll += AngleNormalize180( (360 * legAnimPoint) );
				AngleNormalize180( eyeAngles->roll );
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
			else if ( cg.predictedPlayerState.legsAnim == BOTH_FLIP_R || cg.predictedPlayerState.legsAnim == BOTH_ROLL_R ) {
				// Right rolls
				VectorCopy( &refdef->viewangles, eyeAngles );
				eyeAngles->roll += AngleNormalize180( (360 - (360 * legAnimPoint)) );
				AngleNormalize180( eyeAngles->roll );
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
		}
		else {
			// You're here because you're using cg_trueRoll.integer == 2
			if ( cg.predictedPlayerState.legsAnim == BOTH_FLIP_L || cg.predictedPlayerState.legsAnim == BOTH_ROLL_L
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_R || cg.predictedPlayerState.legsAnim == BOTH_ROLL_R ) {
				// Roll animation, lock the eyemovement
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
		}
	}
	else if ( cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_STOP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_STOP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_LEFT
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_RIGHT
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_L || cg.predictedPlayerState.legsAnim == BOTH_ROLL_L
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_R || cg.predictedPlayerState.legsAnim == BOTH_ROLL_R ) {
		// you don't want rolling so use refdef->viewangles as the view
		useRefDef = qtrue;
	}

	// Flips
	if ( cg_trueFlip.integer ) {
		if ( cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_BACK1 ) {
			// Flip moves that look good with the eyemovement locked
			eyeRange = qfalse;
			didSpecial = qtrue;
		}
		else if ( cg_trueFlip.integer == 1 ) {
			// Use simple flip for the more complicated flips
			if ( cg.predictedPlayerState.legsAnim == BOTH_FLIP_F || cg.predictedPlayerState.legsAnim == BOTH_ROLL_F ) {
				// forward flips
				VectorCopy( &refdef->viewangles, eyeAngles );
				eyeAngles->pitch += AngleNormalize180( 360 - (360 * legAnimPoint) );
				AngleNormalize180( eyeAngles->pitch );
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
			else if ( cg.predictedPlayerState.legsAnim == BOTH_FLIP_B || cg.predictedPlayerState.legsAnim == BOTH_ROLL_B
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK1 ) {
				// back flips
				VectorCopy( &refdef->viewangles, eyeAngles );
				eyeAngles->pitch += AngleNormalize180( (360 * legAnimPoint) );
				AngleNormalize180( eyeAngles->pitch );
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
		}
		else {
			// You're here because you're using cg_trueFlip.integer = 2
			if ( cg.predictedPlayerState.legsAnim == BOTH_FLIP_F || cg.predictedPlayerState.legsAnim == BOTH_ROLL_F
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_B || cg.predictedPlayerState.legsAnim == BOTH_ROLL_B
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK1 ) {
				// Flip animation and using cg_trueFlip.integer = 2, lock the eyemovement
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
		}
	}
	else if ( cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_BACK1 || cg.predictedPlayerState.legsAnim == BOTH_FLIP_F
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F || cg.predictedPlayerState.legsAnim == BOTH_FLIP_B
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_B || cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK1 ) {
		// you don't want flipping so use refdef->viewangles as the view
		useRefDef = qtrue;
	}

	if ( cg_trueSpin.integer ) {
		if ( cg_trueSpin.integer == 1 ) {
			// Do a simulated Spin for the more complicated spins
			if ( cg.predictedPlayerState.torsoAnim == BOTH_T1_TL_BR || cg.predictedPlayerState.torsoAnim == BOTH_T1__L_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1__L__R || cg.predictedPlayerState.torsoAnim == BOTH_T1_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_BL__R || cg.predictedPlayerState.torsoAnim == BOTH_T1_BL_TR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2__L_BR || cg.predictedPlayerState.torsoAnim == BOTH_T2_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2_BL__R || cg.predictedPlayerState.torsoAnim == BOTH_T3__L_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3_BL_BR || cg.predictedPlayerState.torsoAnim == BOTH_T3_BL__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4__L_BR || cg.predictedPlayerState.torsoAnim == BOTH_T4_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4_BL__R || cg.predictedPlayerState.torsoAnim == BOTH_T5_TL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5__L_BR || cg.predictedPlayerState.torsoAnim == BOTH_T5__L__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BL_BR || cg.predictedPlayerState.torsoAnim == BOTH_T5_BL__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BL_TR || cg.predictedPlayerState.torsoAnim == BOTH_ATTACK_BACK
				|| cg.predictedPlayerState.torsoAnim == BOTH_CROUCHATTACKBACK1
				|| cg.predictedPlayerState.torsoAnim == BOTH_BUTTERFLY_LEFT
				|| cg.predictedPlayerState.legsAnim == BOTH_FJSS_TR_BL ) {
				// Left Spins
				VectorCopy( &refdef->viewangles, eyeAngles );
				eyeAngles->yaw += AngleNormalize180( (360 - (360 * torsoAnimPoint)) );
				AngleNormalize180( eyeAngles->yaw );
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
			else if ( cg.predictedPlayerState.torsoAnim == BOTH_T1_BR_BL || cg.predictedPlayerState.torsoAnim == BOTH_T1__R__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1__R_BL || cg.predictedPlayerState.torsoAnim == BOTH_T1_TR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_BR_TL || cg.predictedPlayerState.torsoAnim == BOTH_T1_BR__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2_BR__L || cg.predictedPlayerState.torsoAnim == BOTH_T2_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2__R_BL || cg.predictedPlayerState.torsoAnim == BOTH_T3_BR__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3_BR_BL || cg.predictedPlayerState.torsoAnim == BOTH_T3__R_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4_BR__L || cg.predictedPlayerState.torsoAnim == BOTH_T4_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4__R_BL || cg.predictedPlayerState.torsoAnim == BOTH_T5_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5__R__L || cg.predictedPlayerState.torsoAnim == BOTH_T5__R_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_TR_BL || cg.predictedPlayerState.torsoAnim == BOTH_T5_BR_TL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BR__L || cg.predictedPlayerState.legsAnim == BOTH_BUTTERFLY_RIGHT
				|| cg.predictedPlayerState.legsAnim == BOTH_FJSS_TL_BR ) {
				// Right Spins
				VectorCopy( &refdef->viewangles, eyeAngles );
				eyeAngles->yaw += AngleNormalize180( (360 * torsoAnimPoint) );
				AngleNormalize180( eyeAngles->yaw );
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
		}
		else {
			// You're here because you're using cg_trueSpin.integer == 2
			if ( BG_SpinningSaberAnim( cg.predictedPlayerState.torsoAnim )
				&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSLASHDOWN1
				&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSTABDOWN ) {
				// Flip animation and using cg_trueFlip.integer = 2, lock the eyemovement
				eyeRange = qfalse;
				didSpecial = qtrue;
			}
		}
	}
	else if ( BG_SpinningSaberAnim( cg.predictedPlayerState.torsoAnim )
		&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSLASHDOWN1
		&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSTABDOWN ) {
		// you don't want spinning so use refdef->viewangles as the view
		useRefDef = qtrue;
	}
	else if ( cg.predictedPlayerState.legsAnim == BOTH_JUMPATTACK6 )
		useRefDef = qtrue;

	// Prevent camera flicker while landing.
	if ( cg.predictedPlayerState.legsAnim == BOTH_LAND1 || cg.predictedPlayerState.legsAnim == BOTH_LAND2
		|| cg.predictedPlayerState.legsAnim == BOTH_LANDBACK1 || cg.predictedPlayerState.legsAnim == BOTH_LANDLEFT1
		|| cg.predictedPlayerState.legsAnim == BOTH_LANDRIGHT1 ) {
		useRefDef = qtrue;
	}

	// Prevent the camera flicker while switching to the saber.
	if ( cg.predictedPlayerState.torsoAnim == BOTH_STAND2TO1 || cg.predictedPlayerState.torsoAnim == BOTH_STAND1TO2 )
		useRefDef = qtrue;

	// special camera view for blue backstab
	if ( cg.predictedPlayerState.torsoAnim == BOTH_A2_STABBACK1 ) {
		eyeRange = qfalse;
		didSpecial = qtrue;
	}

	if ( cg.predictedPlayerState.torsoAnim == BOTH_JUMPFLIPSLASHDOWN1
		|| cg.predictedPlayerState.torsoAnim == BOTH_JUMPFLIPSTABDOWN ) {
		eyeRange = qfalse;
		didSpecial = qtrue;
	}


	if ( useRefDef )
		VectorCopy( &refdef->viewangles, eyeAngles );
	else {
		// Movement Roll dampener
		if ( !didSpecial ) {
			if ( !cg_trueMoveRoll.integer )
				eyeAngles->roll = refdef->viewangles.roll;
			else if ( cg_trueMoveRoll.integer == 1 )
				eyeAngles->roll *= 0.05f;
		}

		// eye movement
		if ( eyeRange ) {
			// allow eye motion
			for ( i = 0; i<2; i++ ) {
				float fov = cg_trueFOV.value ? cg_trueFOV.value : cg_fov.value;

				angDiff = eyeAngles->raw[i] - refdef->viewangles.raw[i];
				angDiff = AngleNormalize180( angDiff );
				if ( fabsf( angDiff ) > fov ) {
					if ( angDiff < 0 )	eyeAngles->raw[i] += fov;
					else				eyeAngles->raw[i] -= fov;
				}
				else
					eyeAngles->raw[i] = refdef->viewangles.raw[i];
				AngleNormalize180( eyeAngles->raw[i] );
			}
		}
	}
}

static const vector3 cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static const vector3 cameramaxs = {  CAMERA_SIZE,  CAMERA_SIZE,  CAMERA_SIZE };

// Checks to see if the current camera position is valid based on the last known safe location.  If it's not safe, place
//	the camera at the last position safe location
static void CheckCameraLocation( vector3 *OldeyeOrigin ) {
	trace_t trace;
	refdef_t *refdef = CG_GetRefdef();

	CG_Trace( &trace, OldeyeOrigin, &cameramins, &cameramaxs, &refdef->vieworg, cg.snap->ps.clientNum, CG_GetCameraClip() );
	if ( trace.fraction <= 1.0f )
		VectorCopy( &trace.endpos, &refdef->vieworg );
}

void CG_TrueView( centity_t *cent ) {
	refdef_t *refdef = CG_GetRefdef();

	// Restrict True View Model changes to the player and do the True View camera view work.
	if ( cg.snap && cent->currentState.number == cg.snap->ps.clientNum ) {
		if ( !cg.renderingThirdPerson && (cg_trueGuns.integer || cent->currentState.weapon == WP_SABER
			|| cent->currentState.weapon == WP_MELEE) && !cg.predictedPlayerState.zoomMode && !cg_fakeGun.integer ) {
			mdxaBone_t 		eyeMatrix;
			vector3			eyeAngles, eyeAxis[3], oldEyeOrigin;
			qhandle_t		eyesBolt;
			qboolean		boneBased = qfalse;

			// make the player's be based on the ghoul2 model

			// grab the location data for the "*head_eyes" tag surface
			eyesBolt = trap->G2API_AddBolt( cent->ghoul2, 0, "*head_eyes" );
			if ( !trap->G2API_GetBoltMatrix( cent->ghoul2, 0, eyesBolt, &eyeMatrix, &cent->turAngles, &cent->lerpOrigin,
				cg.time, cgs.gameModels, &cent->modelScale ) ) {
				// Something prevented you from getting the "*head_eyes" information.  The model probably doesn't have a
				//	*head_eyes tag surface.  Try using *head_front instead
				eyesBolt = trap->G2API_AddBolt( cent->ghoul2, 0, "*head_front" );
				if ( !trap->G2API_GetBoltMatrix( cent->ghoul2, 0, eyesBolt, &eyeMatrix, &cent->turAngles, &cent->lerpOrigin,
					cg.time, cgs.gameModels, &cent->modelScale ) ) {
					eyesBolt = trap->G2API_AddBolt( cent->ghoul2, 0, "reye" );
					boneBased = qtrue;
					if ( !trap->G2API_GetBoltMatrix( cent->ghoul2, 0, eyesBolt, &eyeMatrix, &cent->turAngles,
						&cent->lerpOrigin, cg.time, cgs.gameModels, &cent->modelScale ) ) {
						if ( !cg.japp.trueviewWarning ) {
							// first failure.  Do a single warning then turn the warnings off.
							trap->Print( "WARNING:  This Model seems to have missing the *head_eyes and *head_front tag "
								"surfaces.  True View Disabled.\n" );
							cg.japp.trueviewWarning = qtrue;
						}

						return;
					}
				}
			}

			//Set the original eye Origin
			VectorCopy( &refdef->vieworg, &oldEyeOrigin );

			//set the player's view origin
			BG_GiveMeVectorFromMatrix( &eyeMatrix, ORIGIN, &refdef->vieworg );

			// Find the orientation of the eye tag surface I based this on coordsys.h that I found at
			//	http://www.xs4all.nl/~hkuiper/cwmtx/html/coordsys_8h-source.html
			// According to the file, Harry Kuiper, Will DeVore deserve credit for making that file that I based this on.

			if ( boneBased ) {
				// the eye bone has different default axis orientation than the tag surfaces.
				eyeAxis[0].x = eyeMatrix.matrix[0][1];
				eyeAxis[1].x = eyeMatrix.matrix[1][1];
				eyeAxis[2].x = eyeMatrix.matrix[2][1];
				eyeAxis[0].y = eyeMatrix.matrix[0][0];
				eyeAxis[1].y = eyeMatrix.matrix[1][0];
				eyeAxis[2].y = eyeMatrix.matrix[2][0];
				eyeAxis[0].z = -eyeMatrix.matrix[0][2];
				eyeAxis[1].z = -eyeMatrix.matrix[1][2];
				eyeAxis[2].z = -eyeMatrix.matrix[2][2];
			}
			else {
				eyeAxis[0].x = eyeMatrix.matrix[0][0];
				eyeAxis[1].x = eyeMatrix.matrix[1][0];
				eyeAxis[2].x = eyeMatrix.matrix[2][0];
				eyeAxis[0].y = eyeMatrix.matrix[0][1];
				eyeAxis[1].y = eyeMatrix.matrix[1][1];
				eyeAxis[2].y = eyeMatrix.matrix[2][1];
				eyeAxis[0].z = eyeMatrix.matrix[0][2];
				eyeAxis[1].z = eyeMatrix.matrix[1][2];
				eyeAxis[2].z = eyeMatrix.matrix[2][2];
			}

			eyeAngles.yaw = (atan2f( eyeAxis[1].x, eyeAxis[0].x ) * 180 / M_PI);

			//I want asin but it's not setup in the libraries so I'm useing the statement asin x = (M_PI / 2) - acos x
			eyeAngles.pitch = (((M_PI / 2) - acosf( -eyeAxis[2].x )) * 180 / M_PI);
			eyeAngles.roll = (atan2f( eyeAxis[2].y, eyeAxis[2].z ) * 180 / M_PI);

			AngleVectors( &eyeAngles, &eyeAxis[0], NULL, NULL );
			VectorMA( &refdef->vieworg, cg_trueEyePosition.value, &eyeAxis[0], &refdef->vieworg );
			if ( cg.snap->ps.emplacedIndex )
				VectorMA( &refdef->vieworg, 10, &eyeAxis[2], &refdef->vieworg );

			// Trace to see if the bolt eye origin is ok to move to.  If it's not, place it at the last safe position.
			CheckCameraLocation( &oldEyeOrigin );

			// Do all the Eye "movement" and simplified moves here.
			CG_SmoothTrueView( &eyeAngles );

			//set the player view angles
			VectorCopy( &eyeAngles, &refdef->viewangles );

			//set the player view axis
			AnglesToAxis( &refdef->viewangles, refdef->viewaxis );

			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "head_eyes_mouth", TURN_OFF );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada_eyes_mouth", TURN_OFF );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "head", TURN_OFF );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada", TURN_OFF );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada_face", TURN_OFF );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb", TURN_OFF );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb_face", TURN_OFF );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb_eyes_mouth", TURN_OFF );
		}
		else {
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "head_eyes_mouth", TURN_ON );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada_eyes_mouth", TURN_ON );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "head", TURN_ON );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada", TURN_ON );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada_face", TURN_ON );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb", TURN_ON );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb_face", TURN_ON );
			trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb_eyes_mouth", TURN_ON );
		}
	}
	else if ( !(cent->torsoBolt & (1 << (G2_MODELPART_HEAD - 10))) ) {
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "head_eyes_mouth", TURN_ON );
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada_eyes_mouth", TURN_ON );
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "head", TURN_ON );
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada", TURN_ON );
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "heada_face", TURN_ON );
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb", TURN_ON );
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb_face", TURN_ON );
		trap->G2API_SetSurfaceOnOff( cent->ghoul2, "headb_eyes_mouth", TURN_ON );
	}
}
