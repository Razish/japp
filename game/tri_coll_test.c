/* Triangle/triangle intersection test routine,
 * by Tomas Moller, 1997.
 * See article "A Fast Triangle-Triangle Intersection Test",
 * Journal of Graphics Tools, 2(2), 1997
 *
 * int tri_tri_intersect(float V0[3],float V1[3],float V2[3],
 *                         float U0[3],float U1[3],float U2[3])
 *
 * parameters: vertices of triangle 1: V0,V1,V2
 *             vertices of triangle 2: U0,U1,U2
 * result    : returns 1 if the triangles intersect, otherwise 0
 *
 */

#include <math.h>
#include "qcommon/q_shared.h"
#include "g_local.h"

/* if USE_EPSILON_TEST is true then we do a check:
if |dv|<EPSILON then dv=0.0f;
else no check is done (which is less robust)
*/
#define USE_EPSILON_TEST 1
#define EPSILON 0.000001f


// Cross product
#if 1
static void CROSS( vector3 *dest, const vector3 *v1, const vector3 *v2 ) {
	dest->x = v1->y*v2->z - v1->z*v2->y;
	dest->y = v1->z*v2->x - v1->x*v2->z;
	dest->z = v1->x*v2->y - v1->y*v2->x;
}
#else
#define CROSS( dest, v1, v2 ) \
	dest[0] = v1[1]*v2[2] - v1[2]*v2[1]; \
	dest[1] = v1[2]*v2[0] - v1[0]*v2[2]; \
	dest[2] = v1[0]*v2[1] - v1[1]*v2[0];
#endif

// Dot product
#if 1
static float DOT( vector3 *v1, vector3 *v2 ) {
	return (v1->x*v2->x + v1->y*v2->y + v1->z*v2->z);
}
#else
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#endif

// Subtract
#if 1
static void SUB( vector3 *dest, const vector3 *v1, const vector3 *v2 ) {
	dest->x = v1->x - v2->x;
	dest->y = v1->y - v2->y;
	dest->z = v1->z - v2->z;
}
#else
#define SUB( dest, v1, v2 ) \
	dest[0] = v1[0]-v2[0]; \
	dest[1] = v1[1]-v2[1]; \
	dest[2] = v1[2]-v2[2];
#endif

// Intersect
static void ISECT( float VV0, float VV1, float VV2, float D0, float D1, float D2, float *isect0, float *isect1 ) {
	*isect0 = VV0 + (VV1 - VV0)*D0 / (D0 - D1);
	*isect1 = VV0 + (VV2 - VV0)*D0 / (D0 - D2);
}

// Edge-Edge test
/* this edge to edge test is based on Franlin Antonio's gem:
"Faster Line Segment Intersection", in Graphics Gems III,
pp. 199-202 */
#define EDGE_EDGE_TEST( V0, U0, U1 ) \
	Bx = U0->data[i0]-U1->data[i0]; \
	By = U0->data[i1]-U1->data[i1]; \
	Cx = V0->data[i0]-U0->data[i0]; \
	Cy = V0->data[i1]-U0->data[i1]; \
	f = Ay*Bx - Ax*By; \
	d = By*Cx - Bx*Cy; \
	if( (f > 0 && d >= 0 && d <= f) || \
	(f < 0 && d <= 0 && d >= f) ) \
{ \
	e = Ax*Cy - Ay*Cx; \
	if ( f > 0 ) \
{ \
	if ( e >= 0 && e <= f ) \
	return qtrue; \
} \
		else \
{ \
	if ( e <= 0 && e >= f ) \
	return qtrue; \
} \
}

#define EDGE_AGAINST_TRI_EDGES( V0, V1, U0, U1, U2 ) \
{ \
	float Ax, Ay, Bx, By, Cx, Cy, e, d, f; \
	Ax = V1->data[i0]-V0->data[i0]; \
	Ay = V1->data[i1]-V0->data[i1]; \
	/* test edge U0,U1 against V0,V1 */ \
	EDGE_EDGE_TEST( V0, U0, U1 ); \
	/* test edge U1,U2 against V0,V1 */ \
	EDGE_EDGE_TEST( V0, U1, U2 ); \
	/* test edge U2,U1 against V0,V1 */ \
	EDGE_EDGE_TEST( V0, U2, U0 ); \
}

#define POINT_IN_TRI( V0, U0, U1, U2 ) \
{ \
	float a, b, c, d0, d1, d2; \
	/* is T1 completly inside T2? */ \
	/* check if V0 is inside tri(U0,U1,U2) */ \
	a = U1->data[i1]-U0->data[i1]; \
	b = -(U1->data[i0]-U0->data[i0]); \
	c = -a*U0->data[i0]-b*U0->data[i1]; \
	d0 = a*V0->data[i0]+b*V0->data[i1]+c; \
	\
	a = U2->data[i1]-U1->data[i1]; \
	b = -(U2->data[i0]-U1->data[i0]); \
	c = -a*U1->data[i0]-b*U1->data[i1]; \
	d1 = a*V0->data[i0]+b*V0->data[i1]+c; \
	\
	a = U0->data[i1]-U2->data[i1]; \
	b = -(U0->data[i0]-U2->data[i0]); \
	c = -a*U2->data[i0]-b*U2->data[i1]; \
	d2 = a*V0->data[i0]+b*V0->data[i1]+c; \
	if ( d0*d1 > 0.0f ) \
{ \
	if ( d0*d2 > 0.0f ) \
	return qtrue; \
} \
}

qboolean coplanar_tri_tri( vector3 *N, vector3 *V0, vector3 *V1, vector3 *V2, vector3 *U0, vector3 *U1, vector3 *U2 ) {
	vector3 A;
	short i0, i1;

	/* first project onto an axis-aligned plane, that maximizes the area */
	/* of the triangles, compute indices: i0,i1. */
	A.x = fabsf( N->x );
	A.y = fabsf( N->y );
	A.z = fabsf( N->z );

	if ( A.x > A.y ) {
		if ( A.x > A.z ) {
			i0 = 1;      /* A[0] is greatest */
			i1 = 2;
		}
		else {
			i0 = 0;      /* A[2] is greatest */
			i1 = 1;
		}
	}
	else   /* A[0]<=A[1] */
	{
		if ( A.z > A.y ) {
			i0 = 0;      /* A[2] is greatest */
			i1 = 1;
		}
		else {
			i0 = 0;      /* A[1] is greatest */
			i1 = 2;
		}
	}

	/* test all edges of triangle 1 against the edges of triangle 2 */
	EDGE_AGAINST_TRI_EDGES( V0, V1, U0, U1, U2 );
	EDGE_AGAINST_TRI_EDGES( V1, V2, U0, U1, U2 );
	EDGE_AGAINST_TRI_EDGES( V2, V0, U0, U1, U2 );

	/* finally, test if tri1 is totally contained in tri2 or vice versa */
	POINT_IN_TRI( V0, U0, U1, U2 );
	POINT_IN_TRI( U0, V0, V1, V2 );

	return qfalse;
}

// Compute intervals
static int COMPUTE_INTERVALS( float VV0, float VV1, float VV2, float D0, float D1, float D2, float D0D1, float D0D2, float *isect0, float *isect1,
	vector3 *N1, vector3 *V0, vector3 *V1, vector3 *V2, vector3 *U0, vector3 *U1, vector3 *U2 ) {
	if ( D0D1 > 0.0f )
		ISECT( VV2, VV0, VV1, D2, D0, D1, isect0, isect1 );
	else if ( D0D2 > 0.0f )
		ISECT( VV1, VV0, VV2, D1, D0, D2, isect0, isect1 );
	else if ( D1*D2 > 0.0f || D0 != 0.0f )
		ISECT( VV0, VV1, VV2, D0, D1, D2, isect0, isect1 );
	else if ( D1 != 0.0f )
		ISECT( VV1, VV0, VV2, D1, D0, D2, isect0, isect1 );
	else if ( D2 != 0.0f )
		ISECT( VV2, VV0, VV1, D2, D0, D1, isect0, isect1 );
	else
		return coplanar_tri_tri( N1, V0, V1, V2, U0, U1, U2 );
	return -1;
}

qboolean tri_tri_intersect( vector3 *V0, vector3 *V1, vector3 *V2, vector3 *U0, vector3 *U1, vector3 *U2 ) {
	vector3 E1, E2;
	vector3 N1, N2;
	float d1, d2;
	float du0, du1, du2, dv0, dv1, dv2;
	vector3 D;
	float isect1[2], isect2[2];
	float du0du1, du0du2, dv0dv1, dv0dv2;
	short index;
	float vp0, vp1, vp2;
	float up0, up1, up2;
	float b, c, max;
	int tmp;

	/* compute plane equation of triangle(V0,V1,V2) */
	SUB( &E1, V1, V0 );
	SUB( &E2, V2, V0 );
	CROSS( &N1, &E1, &E2 );
	d1 = -DOT( &N1, V0 );
	/* plane equation 1: N1.X+d1=0 */

	/* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
	du0 = DOT( &N1, U0 ) + d1;
	du1 = DOT( &N1, U1 ) + d1;
	du2 = DOT( &N1, U2 ) + d1;

	/* coplanarity robustness check */
#if USE_EPSILON_TEST
	if ( fabsf( du0 ) < EPSILON ) du0 = 0.0f;
	if ( fabsf( du1 ) < EPSILON ) du1 = 0.0f;
	if ( fabsf( du2 ) < EPSILON ) du2 = 0.0f;
#endif
	du0du1 = du0*du1;
	du0du2 = du0*du2;

	if ( du0du1 > 0.0f && du0du2 > 0.0f ) // same sign on all of them + not equal 0 ?
		return qfalse; // no intersection occurs

	/* compute plane of triangle (U0,U1,U2) */
	SUB( &E1, U1, U0 );
	SUB( &E2, U2, U0 );
	CROSS( &N2, &E1, &E2 );
	d2 = -DOT( &N2, U0 );
	/* plane equation 2: N2.X+d2=0 */

	/* put V0,V1,V2 into plane equation 2 */
	dv0 = DOT( &N2, V0 ) + d2;
	dv1 = DOT( &N2, V1 ) + d2;
	dv2 = DOT( &N2, V2 ) + d2;

#if USE_EPSILON_TEST
	if ( fabsf( dv0 ) < EPSILON ) dv0 = 0.0f;
	if ( fabsf( dv1 ) < EPSILON ) dv1 = 0.0f;
	if ( fabsf( dv2 ) < EPSILON ) dv2 = 0.0f;
#endif

	dv0dv1 = dv0*dv1;
	dv0dv2 = dv0*dv2;

	if ( dv0dv1 > 0.0f && dv0dv2 > 0.0f ) // same sign on all of them + not equal 0 ?
		return qfalse; // no intersection occurs

	/* compute direction of intersection line */
	CROSS( &D, &N1, &N2 );

	/* compute and index to the largest component of D */
	max = fabsf( D.x );
	index = 0;
	b = fabsf( D.y );
	c = fabsf( D.z );
	if ( b > max ) {
		max = b;
		index = 1;
	}
	if ( c > max ) {
		max = c;
		index = 2;
	}

	/* this is the simplified projection onto L*/
	vp0 = V0->data[index];
	vp1 = V1->data[index];
	vp2 = V2->data[index];

	up0 = U0->data[index];
	up1 = U1->data[index];
	up2 = U2->data[index];

	/* compute interval for triangle 1 */
	tmp = COMPUTE_INTERVALS( vp0, vp1, vp2, dv0, dv1, dv2, dv0dv1, dv0dv2, &isect1[0], &isect1[1], &N1, V0, V1, V2, U0, U1, U2 );
	if ( tmp != -1 ) return (qboolean)tmp;

	/* compute interval for triangle 2 */
	tmp = COMPUTE_INTERVALS( up0, up1, up2, du0, du1, du2, du0du1, du0du2, &isect2[0], &isect2[1], &N1, V0, V1, V2, U0, U1, U2 );
	if ( tmp != -1 ) return (qboolean)tmp;

	// sort
	if ( isect1[0] > isect1[1] ) {
		float c = isect1[0];
		isect1[0] = isect1[1];
		isect1[1] = c;
	}
	if ( isect2[0] > isect2[1] ) {
		float c = isect2[0];
		isect2[0] = isect2[1];
		isect2[1] = c;
	}

	if ( isect1[1] < isect2[0] ||
		isect2[1] < isect1[0] )
		return qtrue;

	return qfalse;
}
