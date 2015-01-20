/*
	Triangle/triangle intersection test routine by Tomas Moller, 1997.
	See article "A Fast Triangle-Triangle Intersection Test", Journal of Graphics Tools, 2(2), 1997
	Updated June 1999: removed the divisions -- a little faster now!
	Updated October 1999: added {} to CROSS and SUB macros
*/

#include <math.h>
#include "qcommon/q_shared.h"
#include "g_local.h"

static const float epsilon = 0.000001f;

// sort so that a <= b
static void sort( float *a, float *b ) {
	if ( *a > *b ) {
		const float c = *a;
		*a = *b;
		*b = c;
	}
}

static qboolean coplanar_tri_tri( vector3 *N,
	vector3 *V0, vector3 *V1, vector3 *V2,
	vector3 *U0, vector3 *U1, vector3 *U2 )
{
	vector3 A;
	short i0, i1;

	A.x = Q_fabs( N->x );
	A.y = Q_fabs( N->y );
	A.z = Q_fabs( N->z );

	if ( A.x > A.y ) {
		if ( A.x > A.z ) {
			i0 = 1;
			i1 = 2;
		}
		else {
			i0 = 0;
			i1 = 1;
		}
	}
	else {
		if ( A.z > A.y ) {
			i0 = 0;
			i1 = 1;
		}
		else {
			i0 = 0;
			i1 = 2;
		}
	}

	// edge_against_tri_edges
	{
		float Ax, Ay, Bx, By, Cx, Cy, e, d, f;
		Ax = V1->raw[i0] - V0->raw[i0];
		Ay = V1->raw[i1] - V0->raw[i1];
		Bx = U0->raw[i0] - U1->raw[i0];
		By = U0->raw[i1] - U1->raw[i1];
		Cx = V0->raw[i0] - U0->raw[i0];
		Cy = V0->raw[i1] - U0->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
		Bx = U1->raw[i0] - U2->raw[i0];
		By = U1->raw[i1] - U2->raw[i1];
		Cx = V0->raw[i0] - U1->raw[i0];
		Cy = V0->raw[i1] - U1->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
		Bx = U2->raw[i0] - U0->raw[i0];
		By = U2->raw[i1] - U0->raw[i1];
		Cx = V0->raw[i0] - U2->raw[i0];
		Cy = V0->raw[i1] - U2->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
	}
	// edge_against_tri_edges
	{
		float Ax, Ay, Bx, By, Cx, Cy, e, d, f;
		Ax = V2->raw[i0] - V1->raw[i0];
		Ay = V2->raw[i1] - V1->raw[i1];
		Bx = U0->raw[i0] - U1->raw[i0];
		By = U0->raw[i1] - U1->raw[i1];
		Cx = V1->raw[i0] - U0->raw[i0];
		Cy = V1->raw[i1] - U0->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
		Bx = U1->raw[i0] - U2->raw[i0];
		By = U1->raw[i1] - U2->raw[i1];
		Cx = V1->raw[i0] - U1->raw[i0];
		Cy = V1->raw[i1] - U1->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
		Bx = U2->raw[i0] - U0->raw[i0];
		By = U2->raw[i1] - U0->raw[i1];
		Cx = V1->raw[i0] - U2->raw[i0];
		Cy = V1->raw[i1] - U2->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
	}
	// edge_against_tri_edges
	{
		float Ax,Ay,Bx,By,Cx,Cy,e,d,f;
		Ax = V0->raw[i0] - V2->raw[i0];
		Ay = V0->raw[i1] - V2->raw[i1];
		Bx = U0->raw[i0] - U1->raw[i0];
		By = U0->raw[i1] - U1->raw[i1];
		Cx = V2->raw[i0] - U0->raw[i0];
		Cy = V2->raw[i1] - U0->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
		Bx = U1->raw[i0] - U2->raw[i0];
		By = U1->raw[i1] - U2->raw[i1];
		Cx = V2->raw[i0] - U1->raw[i0];
		Cy = V2->raw[i1] - U1->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
		Bx = U2->raw[i0] - U0->raw[i0];
		By = U2->raw[i1] - U0->raw[i1];
		Cx = V2->raw[i0] - U2->raw[i0];
		Cy = V2->raw[i1] - U2->raw[i1];
		f = Ay * Bx - Ax * By;
		d = By * Cx - Bx * Cy;
		if ( (f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f) ) {
			e = Ax * Cy - Ay * Cx;
			if ( f > 0 ) {
				if ( e >= 0 && e <= f ) {
					return qtrue;
				}
			}
			else {
				if ( e <= 0 && e >= f ) {
					return qtrue;
				}
			}
		}
	}

	// point_in_tri
	{
		float a, b, c, d0, d1, d2;
		a = U1->raw[i1] - U0->raw[i1];
		b = -(U1->raw[i0] - U0->raw[i0]);
		c = -a * U0->raw[i0] - b * U0->raw[i1];
		d0 = a * V0->raw[i0] + b * V0->raw[i1] + c;
		a = U2->raw[i1] - U1->raw[i1];
		b = -(U2->raw[i0] - U1->raw[i0]);
		c = -a * U1->raw[i0] - b * U1->raw[i1];
		d1 = a * V0->raw[i0] + b * V0->raw[i1] + c;
		a = U0->raw[i1] - U2->raw[i1];
		b = -(U0->raw[i0] - U2->raw[i0]);
		c = -a * U2->raw[i0] - b * U2->raw[i1];
		d2 = a * V0->raw[i0] + b * V0->raw[i1] + c;
		if ( d0 * d1 > 0.0f ) {
			if ( d0 * d2 > 0.0f ) {
				return qtrue;
			}
		}
	}

	// point_in_tri
	{
		float a, b, c, d0, d1, d2;
		a = V1->raw[i1] - V0->raw[i1];
		b = -(V1->raw[i0] - V0->raw[i0]);
		c = -a * V0->raw[i0] - b * V0->raw[i1];
		d0 = a * U0->raw[i0] + b * U0->raw[i1] + c;
		a = V2->raw[i1] - V1->raw[i1];
		b = -(V2->raw[i0] - V1->raw[i0]);
		c = -a * V1->raw[i0] - b * V1->raw[i1];
		d1 = a * U0->raw[i0] + b * U0->raw[i1] + c;
		a = V0->raw[i1] - V2->raw[i1];
		b = -(V0->raw[i0] - V2->raw[i0]);
		c = -a * V2->raw[i0] - b * V2->raw[i1];
		d2 = a * U0->raw[i0] + b * U0->raw[i1] + c;
		if ( d0 * d1 > 0.0f ) {
			if ( d0 * d2 > 0.0f ) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

qboolean tri_tri_intersect( vector3 *V0, vector3 *V1, vector3 *V2, vector3 *U0, vector3 *U1, vector3 *U2 ) {
	vector3 E1, E2, N1, N2, D;
	float d1, d2;
	float du0, du1, du2, dv0, dv1, dv2;
	float isect1[2], isect2[2];
	float du0du1, du0du2, dv0dv1, dv0dv2;
	short index;
	float vp0, vp1, vp2, up0, up1, up2;
	float bb, cc, max;
	float a, b, c, d, e, f, x0, x1, y0, y1;
	float xx, yy, xxyy, tmp;

	// compute plane of tri(V0, V1, V2)
	VectorSubtract( V1, V0, &E1 );
	VectorSubtract( V2, V0, &E2 );
	CrossProduct( &E1, &E2, &N1 );
	d1 = -DotProduct( &N1, V0 );

	// put U0,U1,U2 into plane equation 1 to compute signed distances to the plane
	du0 = DotProduct( &N1, U0 ) + d1;
	du1 = DotProduct( &N1, U1 ) + d1;
	du2 = DotProduct( &N1, U2 ) + d1;

	// coplanarity robustness check
	if ( Q_fabs( du0 ) < epsilon ) {
		du0 = 0.0f;
	}
	if ( Q_fabs( du1 ) < epsilon ) {
		du1 = 0.0f;
	}
	if ( Q_fabs( du2 ) < epsilon ) {
		du2 = 0.0f;
	}

	du0du1 = du0 * du1;
	du0du2 = du0 * du2;

	// same sign on all of them + not equal 0, no intersection occurs
	if ( du0du1 > 0.0f && du0du2 > 0.0f ) {
		return qfalse;
	}

	// compute plane of tri(U0, U1, U2)
	VectorSubtract( U1, U0, &E1 );
	VectorSubtract( U2, U0, &E2 );
	CrossProduct( &E1, &E2, &N2 );
	d2 = -DotProduct( &N2, U0 );

	// put V0,V1,V2 into plane equation 1 to compute signed distances to the plane
	dv0 = DotProduct( &N2, V0 ) + d2;
	dv1 = DotProduct( &N2, V1 ) + d2;
	dv2 = DotProduct( &N2, V2 ) + d2;

	// coplanarity robustness check
	if ( Q_fabs( dv0 ) < epsilon ) {
		dv0 = 0.0f;
	}
	if ( Q_fabs( dv1 ) < epsilon ) {
		dv1 = 0.0f;
	}
	if ( Q_fabs( dv2 ) < epsilon ) {
		dv2 = 0.0f;
	}

	dv0dv1 = dv0 * dv1;
	dv0dv2 = dv0 * dv2;

	// same sign on all of them + not equal 0, no intersection occurs
	if ( dv0dv1 > 0.0f && dv0dv2 > 0.0f ) {
		return qfalse;
	}

	// compute direction of intersection line
	CrossProduct( &D, &N1, &N2 );

	// compute and index to the largest component of D
	max = Q_fabs( D.x );
	index = 0;
	bb = Q_fabs( D.y );
	cc = Q_fabs( D.z );
	if ( bb > max ) {
		max = bb;
		index = 1;
	}
	if ( cc > max ) {
		max = cc;
		index = 2;
	}

	// this is the simplified projection onto L
	vp0 = V0->raw[index];
	vp1 = V1->raw[index];
	vp2 = V2->raw[index];

	up0 = U0->raw[index];
	up1 = U1->raw[index];
	up2 = U2->raw[index];

	// compute_intervals
	if ( dv0dv1 > 0.0f ) {
		a = vp2;
		b = (vp0 - vp2) * dv2;
		c = (vp1 - vp2) * dv2;
		x0 = dv2 - dv0;
		x1 = dv2 - dv1;
	}
	else if ( dv0dv2 > 0.0f ) {
		a = vp1;
		b = (vp0 - vp1) * dv1;
		c = (vp2 - vp1) * dv1;
		x0 = dv1 - dv0;
		x1 = dv1 - dv2;
	}
	else if ( dv1 * dv2 > 0.0f || dv0 != 0.0f ) {
		a = vp0;
		b = (vp1 - vp0) * dv0;
		c = (vp2 - vp0) * dv0;
		x0 = dv0 - dv1;
		x1 = dv0 - dv2;
	}
	else if ( dv1 != 0.0f ) {
		a = vp1;
		b = (vp0 - vp1) * dv1;
		c = (vp2 - vp1) * dv1;
		x0 = dv1 - dv0;
		x1 = dv1 - dv2;
	}
	else if ( dv2 != 0.0f ) {
		a = vp2;
		b = (vp0 - vp2) * dv2;
		c = (vp1 - vp2) * dv2;
		x0 = dv2 - dv0;
		x1 = dv2 - dv1;
	}
	else {
		return coplanar_tri_tri( &N1, V0, V1, V2, U0, U1, U2 );
	}

	// compute_intervals
	if ( du0du1 > 0.0f ) {
		d = up2;
		e = (up0 - up2) * du2;
		f = (up1 - up2) * du2;
		y0 = du2 - du0;
		y1 = du2 - du1;
	}
	else if ( du0du2 > 0.0f ) {
		d = up1;
		e = (up0 - up1) * du1;
		f = (up2 - up1) * du1;
		y0 = du1 - du0;
		y1 = du1 - du2;
	}
	else if ( du1 * du2 > 0.0f || du0 != 0.0f ) {
		d = up0;
		e = (up1 - up0) * du0;
		f = (up2 - up0) * du0;
		y0 = du0 - du1;
		y1 = du0 - du2;
	}
	else if ( du1 != 0.0f ) {
		d = up1;
		e = (up0 - up1) * du1;
		f = (up2 - up1) * du1;
		y0 = du1 - du0;
		y1 = du1 - du2;
	}
	else if ( du2 != 0.0f ) {
		d = up2;
		e = (up0 - up2) * du2;
		f = (up1 - up2) * du2;
		y0 = du2 - du0;
		y1 = du2 - du1;
	}
	else {
		return coplanar_tri_tri( &N1, V0, V1, V2, U0, U1, U2 );
	}

	xx = x0 * x1;
	yy = y0 * y1;
	xxyy = xx * yy;

	tmp = a * xxyy;
	isect1[0] = tmp + b * x1 * yy;
	isect1[1] = tmp + c * x0 * yy;

	tmp = d * xxyy;
	isect2[0] = tmp + e * xx * y1;
	isect2[1] = tmp + f * xx * y0;

	// sort so that a <= b
	sort( &isect1[0], &isect1[1] );
	sort( &isect2[0], &isect2[1] );

	if ( isect1[1] < isect2[0] || isect2[1] < isect1[0] ) {
		return qfalse;
	}

	return qtrue;
}
