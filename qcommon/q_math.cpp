// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_math.c -- stateless support routines that are included in each code module
#include "qcommon/q_shared.h"

vector3	vec3_origin = { 0, 0, 0 };
vector3	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


const vector4 colorBlack = { 0.0f, 0.0f, 0.0f, 1.0f };
const vector4 colorRed = { 1.0f, 0.0f, 0.0f, 1.0f };
const vector4 colorGreen = { 0.0f, 1.0f, 0.0f, 1.0f };
const vector4 colorBlue = { 0.0f, 0.0f, 1.0f, 1.0f };
const vector4 colorYellow = { 1.0f, 1.0f, 0.0f, 1.0f };
const vector4 colorMagenta = { 1.0f, 0.0f, 1.0f, 1.0f };
const vector4 colorCyan = { 0.0f, 1.0f, 1.0f, 1.0f };
const vector4 colorWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
const vector4 colorLtGrey = { 0.75f, 0.75f, 0.75f, 1.0f };
const vector4 colorMdGrey = { 0.5f, 0.5f, 0.5f, 1.0f };
const vector4 colorDkGrey = { 0.25f, 0.25f, 0.25f, 1.0f };
const vector4 colorLtBlue = { 0.367f, 0.261f, 0.722f, 1.0f };
const vector4 colorDkBlue = { 0.199f, 0.0f, 0.398f, 1.0f };

const vector4 g_color_table[Q_COLOR_BITS + 1] = {
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // black
	{ 1.0f, 0.0f, 0.0f, 1.0f }, // red
	{ 0.0f, 1.0f, 0.0f, 1.0f }, // green
	{ 1.0f, 1.0f, 0.0f, 1.0f }, // yellow
	{ 0.0f, 0.0f, 1.0f, 1.0f }, // blue
	{ 0.0f, 1.0f, 1.0f, 1.0f }, // cyan
	{ 1.0f, 0.0f, 1.0f, 1.0f }, // magenta
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // white
	{ 1.0f, 0.5f, 0.0f, 1.0f }, // orange
	{ 0.5f, 0.5f, 0.5f, 1.0f },	// md.grey
};

const vector3 bytedirs[NUMVERTEXNORMALS] = {
	{ -0.525731f, 0.000000f, 0.850651f },
	{ -0.442863f, 0.238856f, 0.864188f },
	{ -0.295242f, 0.000000f, 0.955423f },
	{ -0.309017f, 0.500000f, 0.809017f },
	{ -0.162460f, 0.262866f, 0.951056f },
	{ 0.000000f, 0.000000f, 1.000000f },
	{ 0.000000f, 0.850651f, 0.525731f },
	{ -0.147621f, 0.716567f, 0.681718f },
	{ 0.147621f, 0.716567f, 0.681718f },
	{ 0.000000f, 0.525731f, 0.850651f },
	{ 0.309017f, 0.500000f, 0.809017f },
	{ 0.525731f, 0.000000f, 0.850651f },
	{ 0.295242f, 0.000000f, 0.955423f },
	{ 0.442863f, 0.238856f, 0.864188f },
	{ 0.162460f, 0.262866f, 0.951056f },
	{ -0.681718f, 0.147621f, 0.716567f },
	{ -0.809017f, 0.309017f, 0.500000f },
	{ -0.587785f, 0.425325f, 0.688191f },
	{ -0.850651f, 0.525731f, 0.000000f },
	{ -0.864188f, 0.442863f, 0.238856f },
	{ -0.716567f, 0.681718f, 0.147621f },
	{ -0.688191f, 0.587785f, 0.425325f },
	{ -0.500000f, 0.809017f, 0.309017f },
	{ -0.238856f, 0.864188f, 0.442863f },
	{ -0.425325f, 0.688191f, 0.587785f },
	{ -0.716567f, 0.681718f, -0.147621f },
	{ -0.500000f, 0.809017f, -0.309017f },
	{ -0.525731f, 0.850651f, 0.000000f },
	{ 0.000000f, 0.850651f, -0.525731f },
	{ -0.238856f, 0.864188f, -0.442863f },
	{ 0.000000f, 0.955423f, -0.295242f },
	{ -0.262866f, 0.951056f, -0.162460f },
	{ 0.000000f, 1.000000f, 0.000000f },
	{ 0.000000f, 0.955423f, 0.295242f },
	{ -0.262866f, 0.951056f, 0.162460f },
	{ 0.238856f, 0.864188f, 0.442863f },
	{ 0.262866f, 0.951056f, 0.162460f },
	{ 0.500000f, 0.809017f, 0.309017f },
	{ 0.238856f, 0.864188f, -0.442863f },
	{ 0.262866f, 0.951056f, -0.162460f },
	{ 0.500000f, 0.809017f, -0.309017f },
	{ 0.850651f, 0.525731f, 0.000000f },
	{ 0.716567f, 0.681718f, 0.147621f },
	{ 0.716567f, 0.681718f, -0.147621f },
	{ 0.525731f, 0.850651f, 0.000000f },
	{ 0.425325f, 0.688191f, 0.587785f },
	{ 0.864188f, 0.442863f, 0.238856f },
	{ 0.688191f, 0.587785f, 0.425325f },
	{ 0.809017f, 0.309017f, 0.500000f },
	{ 0.681718f, 0.147621f, 0.716567f },
	{ 0.587785f, 0.425325f, 0.688191f },
	{ 0.955423f, 0.295242f, 0.000000f },
	{ 1.000000f, 0.000000f, 0.000000f },
	{ 0.951056f, 0.162460f, 0.262866f },
	{ 0.850651f, -0.525731f, 0.000000f },
	{ 0.955423f, -0.295242f, 0.000000f },
	{ 0.864188f, -0.442863f, 0.238856f },
	{ 0.951056f, -0.162460f, 0.262866f },
	{ 0.809017f, -0.309017f, 0.500000f },
	{ 0.681718f, -0.147621f, 0.716567f },
	{ 0.850651f, 0.000000f, 0.525731f },
	{ 0.864188f, 0.442863f, -0.238856f },
	{ 0.809017f, 0.309017f, -0.500000f },
	{ 0.951056f, 0.162460f, -0.262866f },
	{ 0.525731f, 0.000000f, -0.850651f },
	{ 0.681718f, 0.147621f, -0.716567f },
	{ 0.681718f, -0.147621f, -0.716567f },
	{ 0.850651f, 0.000000f, -0.525731f },
	{ 0.809017f, -0.309017f, -0.500000f },
	{ 0.864188f, -0.442863f, -0.238856f },
	{ 0.951056f, -0.162460f, -0.262866f },
	{ 0.147621f, 0.716567f, -0.681718f },
	{ 0.309017f, 0.500000f, -0.809017f },
	{ 0.425325f, 0.688191f, -0.587785f },
	{ 0.442863f, 0.238856f, -0.864188f },
	{ 0.587785f, 0.425325f, -0.688191f },
	{ 0.688191f, 0.587785f, -0.425325f },
	{ -0.147621f, 0.716567f, -0.681718f },
	{ -0.309017f, 0.500000f, -0.809017f },
	{ 0.000000f, 0.525731f, -0.850651f },
	{ -0.525731f, 0.000000f, -0.850651f },
	{ -0.442863f, 0.238856f, -0.864188f },
	{ -0.295242f, 0.000000f, -0.955423f },
	{ -0.162460f, 0.262866f, -0.951056f },
	{ 0.000000f, 0.000000f, -1.000000f },
	{ 0.295242f, 0.000000f, -0.955423f },
	{ 0.162460f, 0.262866f, -0.951056f },
	{ -0.442863f, -0.238856f, -0.864188f },
	{ -0.309017f, -0.500000f, -0.809017f },
	{ -0.162460f, -0.262866f, -0.951056f },
	{ 0.000000f, -0.850651f, -0.525731f },
	{ -0.147621f, -0.716567f, -0.681718f },
	{ 0.147621f, -0.716567f, -0.681718f },
	{ 0.000000f, -0.525731f, -0.850651f },
	{ 0.309017f, -0.500000f, -0.809017f },
	{ 0.442863f, -0.238856f, -0.864188f },
	{ 0.162460f, -0.262866f, -0.951056f },
	{ 0.238856f, -0.864188f, -0.442863f },
	{ 0.500000f, -0.809017f, -0.309017f },
	{ 0.425325f, -0.688191f, -0.587785f },
	{ 0.716567f, -0.681718f, -0.147621f },
	{ 0.688191f, -0.587785f, -0.425325f },
	{ 0.587785f, -0.425325f, -0.688191f },
	{ 0.000000f, -0.955423f, -0.295242f },
	{ 0.000000f, -1.000000f, 0.000000f },
	{ 0.262866f, -0.951056f, -0.162460f },
	{ 0.000000f, -0.850651f, 0.525731f },
	{ 0.000000f, -0.955423f, 0.295242f },
	{ 0.238856f, -0.864188f, 0.442863f },
	{ 0.262866f, -0.951056f, 0.162460f },
	{ 0.500000f, -0.809017f, 0.309017f },
	{ 0.716567f, -0.681718f, 0.147621f },
	{ 0.525731f, -0.850651f, 0.000000f },
	{ -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000f, -0.809017f, -0.309017f },
	{ -0.262866f, -0.951056f, -0.162460f },
	{ -0.850651f, -0.525731f, 0.000000f },
	{ -0.716567f, -0.681718f, -0.147621f },
	{ -0.716567f, -0.681718f, 0.147621f },
	{ -0.525731f, -0.850651f, 0.000000f },
	{ -0.500000f, -0.809017f, 0.309017f },
	{ -0.238856f, -0.864188f, 0.442863f },
	{ -0.262866f, -0.951056f, 0.162460f },
	{ -0.864188f, -0.442863f, 0.238856f },
	{ -0.809017f, -0.309017f, 0.500000f },
	{ -0.688191f, -0.587785f, 0.425325f },
	{ -0.681718f, -0.147621f, 0.716567f },
	{ -0.442863f, -0.238856f, 0.864188f },
	{ -0.587785f, -0.425325f, 0.688191f },
	{ -0.309017f, -0.500000f, 0.809017f },
	{ -0.147621f, -0.716567f, 0.681718f },
	{ -0.425325f, -0.688191f, 0.587785f },
	{ -0.162460f, -0.262866f, 0.951056f },
	{ 0.442863f, -0.238856f, 0.864188f },
	{ 0.162460f, -0.262866f, 0.951056f },
	{ 0.309017f, -0.500000f, 0.809017f },
	{ 0.147621f, -0.716567f, 0.681718f },
	{ 0.000000f, -0.525731f, 0.850651f },
	{ 0.425325f, -0.688191f, 0.587785f },
	{ 0.587785f, -0.425325f, 0.688191f },
	{ 0.688191f, -0.587785f, 0.425325f },
	{ -0.955423f, 0.295242f, 0.000000f },
	{ -0.951056f, 0.162460f, 0.262866f },
	{ -1.000000f, 0.000000f, 0.000000f },
	{ -0.850651f, 0.000000f, 0.525731f },
	{ -0.955423f, -0.295242f, 0.000000f },
	{ -0.951056f, -0.162460f, 0.262866f },
	{ -0.864188f, 0.442863f, -0.238856f },
	{ -0.951056f, 0.162460f, -0.262866f },
	{ -0.809017f, 0.309017f, -0.500000f },
	{ -0.864188f, -0.442863f, -0.238856f },
	{ -0.951056f, -0.162460f, -0.262866f },
	{ -0.809017f, -0.309017f, -0.500000f },
	{ -0.681718f, 0.147621f, -0.716567f },
	{ -0.681718f, -0.147621f, -0.716567f },
	{ -0.850651f, 0.000000f, -0.525731f },
	{ -0.688191f, 0.587785f, -0.425325f },
	{ -0.587785f, 0.425325f, -0.688191f },
	{ -0.425325f, 0.688191f, -0.587785f },
	{ -0.425325f, -0.688191f, -0.587785f },
	{ -0.587785f, -0.425325f, -0.688191f },
	{ -0.688191f, -0.587785f, -0.425325f }
};

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return (Q_rand( seed ) & 0xffff) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0f * (Q_random( seed ) - 0.5f);
}

//i wrote this function in a console test app and it appeared faster
//in debug and release than the standard crossproduct asm generated
//by the compiler. however, when inlining the crossproduct function
//the compiler performs further optimizations and generally ends up
//being faster than this asm version. but feel free to try this one
//and see if you're heavily crossproducting in an area and looking
//for a way to optimize. -rww
#if 0
void CrossProductA( float *v1, float *v2, float *cross ) {
#if 1
	static float scratch1, scratch2, scratch3, scratch4, scratch5, scratch6;

	__asm mov   eax, v1
	__asm mov   ecx, v2
	__asm mov   edx, cross

	__asm fld   dword ptr[eax + 4]
		__asm fmul  dword ptr[ecx + 8]
		__asm fstp  scratch1

	__asm fld   dword ptr[eax + 8]
		__asm fmul  dword ptr[ecx + 4]
		__asm fstp  scratch2

	__asm fld   dword ptr[eax + 8]
		__asm fmul  dword ptr[ecx]
		__asm fstp  scratch3

	__asm fld   dword ptr[eax]
		__asm fmul  dword ptr[ecx + 8]
		__asm fstp  scratch4

	__asm fld   dword ptr[eax]
		__asm fmul  dword ptr[ecx + 4]
		__asm fstp  scratch5

	__asm fld   dword ptr[eax + 4]
		__asm fmul  dword ptr[ecx]
		__asm fstp  scratch6

	__asm fld   scratch1
	__asm fsub  scratch2
	__asm fstp  dword ptr[edx]

		__asm fld   scratch3
	__asm fsub  scratch4
	__asm fstp  dword ptr[edx + 4]

		__asm fld   scratch5
	__asm fsub  scratch6
	__asm fstp  dword ptr[edx + 8]
#else //doesn't require use of statics, but not nearly as fast.
	__asm mov   eax, v1
	__asm mov   ecx, v2
	__asm mov   edx, cross

	__asm fld   dword ptr[eax + 4]
		__asm fmul  dword ptr[ecx + 8]
		__asm fld   dword ptr[eax + 8]
		__asm fmul  dword ptr[ecx + 4]
		__asm fsubp st( 1 ), st
	__asm fstp  dword ptr[edx]

		__asm fld   dword ptr[eax + 8]
		__asm fmul  dword ptr[ecx]
		__asm fld   dword ptr[eax]
		__asm fmul  dword ptr[ecx + 8]
		__asm fsubp st( 1 ), st
	__asm fstp  dword ptr[edx + 4]

		__asm fld   dword ptr[eax]
		__asm fmul  dword ptr[ecx + 4]
		__asm fld   dword ptr[eax + 4]
		__asm fmul  dword ptr[ecx]
		__asm fsubp st( 1 ), st
	__asm fstp  dword ptr[edx + 8]
#endif
}
#endif

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}


// this isn't a real cheap function to call!
int DirToByte( vector3 *dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for ( i = 0; i<NUMVERTEXNORMALS; i++ ) {
		d = DotProduct( dir, &bytedirs[i] );
		if ( d > bestd ) {
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vector3 *dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( &vec3_origin, dir );
		return;
	}
	VectorCopy( &bytedirs[b], dir );
}


unsigned ColorBytes3( float r, float g, float b ) {
	unsigned	i;

	((byte *)&i)[0] = r * 255;
	((byte *)&i)[1] = g * 255;
	((byte *)&i)[2] = b * 255;

	return i;
}

unsigned ColorBytes4( float r, float g, float b, float a ) {
	unsigned	i;

	((byte *)&i)[0] = r * 255;
	((byte *)&i)[1] = g * 255;
	((byte *)&i)[2] = b * 255;
	((byte *)&i)[3] = a * 255;

	return i;
}

float NormalizeColor( const vector3 *in, vector3 *out ) {
	float	max;

	max = in->r;
	if ( in->g > max ) {
		max = in->g;
	}
	if ( in->b > max ) {
		max = in->b;
	}

	if ( !max ) {
		VectorClear( out );
	}
	else {
		out->r = in->r / max;
		out->g = in->g / max;
		out->b = in->b / max;
	}
	return max;
}

// Returns false if the triangle is degenerate.
// The normal will point out of the clock for clockwise ordered points
qboolean PlaneFromPoints( vector4 *plane, const vector3 *a, const vector3 *b, const vector3 *c ) {
	vector3	d1, d2;

	VectorSubtract( b, a, &d1 );
	VectorSubtract( c, a, &d2 );
	CrossProduct( &d2, &d1, (vector3*)plane );
	if ( VectorNormalize( (vector3*)plane ) == 0 ) {
		return qfalse;
	}

	plane->w = DotProduct( a, (vector3*)plane );
	return qtrue;
}

// This is not implemented very well...
void RotatePointAroundVector( vector3 *dst, const vector3 *dir, const vector3 *point, float degrees ) {
	vector3 m[3], im[3], zrot[3], tmpmat[3], rot[3];
	vector3 vr, vup, vf;
	int	i;
	float	rad;

	vf.x = dir->x;
	vf.y = dir->y;
	vf.z = dir->z;

	PerpendicularVector( &vr, dir );
	CrossProduct( &vr, &vf, &vup );

	m[0].x = vr.x;
	m[1].x = vr.y;
	m[2].x = vr.z;

	m[0].y = vup.x;
	m[1].y = vup.y;
	m[2].y = vup.z;

	m[0].z = vf.x;
	m[1].z = vf.y;
	m[2].z = vf.z;

	memcpy( im, m, sizeof(im) );

	im[0].y = m[1].x;
	im[0].z = m[2].x;
	im[1].x = m[0].y;
	im[1].z = m[2].y;
	im[2].x = m[0].z;
	im[2].y = m[1].z;

	memset( zrot, 0, sizeof(zrot) );
	zrot[0].x = zrot[1].y = zrot[2].z = 1.0f;

	rad = DEG2RAD( degrees );
	zrot[0].x =  cosf( rad );
	zrot[0].y =  sinf( rad );
	zrot[1].x = -sinf( rad );
	zrot[1].y =  cosf( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ ) {
		dst->raw[i] = DotProduct( &rot[i], point );
	}
}

void RotateAroundDirection( vector3 axis[3], float yaw ) {

	// create an arbitrary axis[1]
	PerpendicularVector( &axis[1], &axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vector3	temp;

		VectorCopy( &axis[1], &temp );
		RotatePointAroundVector( &axis[1], &axis[0], &temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( &axis[0], &axis[1], &axis[2] );
}

void vectoangles( const vector3 *vec, vector3 *angles ) {
	float forward, yaw, pitch;

	if ( vec->y == 0 && vec->x == 0 ) {
		yaw = 0.0f;
		if ( vec->z > 0.0f ) {
			pitch = 90.0f;
		}
		else {
			pitch = 270.0f;
		}
	}
	else {
		if ( vec->x ) {
			yaw = atan2f( vec->y, vec->x ) * 180.0f / M_PI;
		}
		else if ( vec->y > 0.0f ) {
			yaw = 90.0f;
		}
		else {
			yaw = 270.0f;
		}
		if ( yaw < 0 ) {
			yaw += 360.0f;
		}

		forward = sqrtf( vec->x*vec->x + vec->y*vec->y );
		pitch = atan2f( vec->z, forward ) * 180.0f / M_PI;
		if ( pitch < 0 ) {
			pitch += 360.0f;
		}
	}

	angles->pitch = -pitch;
	angles->yaw = yaw;
	angles->roll = 0.0f;
}

float vectoyaw( const vector3 *vec ) {
	float yaw;

	if ( vec->yaw == 0 && vec->pitch == 0 ) {
		yaw = 0;
	}
	else {
		if ( vec->pitch ) {
			yaw = atan2f( vec->yaw, vec->pitch ) * 180 / M_PI;
		}
		else if ( vec->yaw > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}
	}

	return yaw;
}

void AnglesToAxis( const vector3 *angles, vector3 axis[3] ) {
	vector3	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, &axis[0], &right, &axis[2] );
	VectorSubtract( &vec3_origin, &right, &axis[1] );
}

void AxisClear( vector3 axis[3] ) {
	axis[0].x = 1;
	axis[0].y = 0;
	axis[0].z = 0;

	axis[1].x = 0;
	axis[1].y = 1;
	axis[1].z = 0;

	axis[2].x = 0;
	axis[2].y = 0;
	axis[2].z = 1;
}

void AxisCopy( vector3 in[3], vector3 out[3] ) {
	VectorCopy( &in[0], &out[0] );
	VectorCopy( &in[1], &out[1] );
	VectorCopy( &in[2], &out[2] );
}

void ProjectPointOnPlane( vector3 *dst, const vector3 *p, const vector3 *normal ) {
	float d;
	vector3 n;
	float inv_denom;

	inv_denom = DotProduct( normal, normal );
	assert( Q_fabs( inv_denom ) != 0.0f );
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n.x = normal->x * inv_denom;
	n.y = normal->y * inv_denom;
	n.z = normal->z * inv_denom;

	dst->x = p->x - d * n.x;
	dst->y = p->y - d * n.y;
	dst->z = p->z - d * n.z;
}

// Given a normalized forward vector, create two other perpendicular vectors
void MakeNormalVectors( const vector3 *forward, vector3 *right, vector3 *up ) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right->y = -forward->x;
	right->z = forward->y;
	right->x = forward->z;

	d = DotProduct( right, forward );
	VectorMA( right, -d, forward, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
}


void VectorRotate( vector3 *in, vector3 matrix[3], vector3 *out ) {
	out->x = DotProduct( in, &matrix[0] );
	out->y = DotProduct( in, &matrix[1] );
	out->z = DotProduct( in, &matrix[2] );
}

float Q_rsqrt( float number ) {
	byteAlias_t ba;
	float x2;
	const float threehalfs = 1.5f;

	x2 = number * 0.5F;
	ba.f = number;
	ba.i = 0x5f3759df - (ba.i >> 1);               // what the fuck?
	float y = ba.f;
	y = y * (threehalfs - (x2 * y * y));   // 1st iteration
	//y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

#if defined(__linux__)
	assert( !std::isnan( y ) );
#endif

	return y;
}

float Q_fabs( float f ) {
	byteAlias_t ba;
	ba.f = f;
	ba.i &= 0x7FFFFFFF;
	return ba.f;
}

float LerpAngle( float from, float to, float frac ) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}


// Always returns a value from -180 to 180
float AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	a = fmod( a, 360 );//chop it down quickly, then level it out
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vector3 *v1, vector3 *v2, vector3 *v3 ) {
	v3->x = AngleSubtract( v1->x, v2->x );
	v3->y = AngleSubtract( v1->y, v2->y );
	v3->z = AngleSubtract( v1->z, v2->z );
}


float AngleMod( float a ) {
	a = (360.0f / 65536) * ((int)(a*(65536 / 360.0f)) & 65535);
	return a;
}

// returns angle normalized to the range [0 <= angle < 360]
float AngleNormalize360( float angle ) {
	return (360.0f / 65536) * ((int)(angle * (65536 / 360.0f)) & 65535);
}

// returns angle normalized to the range [-180 < angle <= 180]
float AngleNormalize180( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0f ) {
		angle -= 360.0f;
	}
	return angle;
}

// returns the normalized delta from angle1 to angle2
float AngleDelta( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

void SetPlaneSignbits( cplane_t *out ) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for ( j = 0; j < 3; j++ ) {
		if ( out->normal.raw[j] < 0 ) {
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}


#if !defined(QARCH_X86) || defined(C_ONLY) || defined(MACOS_X)

	int BoxOnPlaneSide( vector3 *emins, vector3 *emaxs, struct cplane_s *p ) {
		// fast axial cases
		if ( p->type < 3 ) {
			if ( p->dist <= emins->raw[p->type] ) {
				return 1;
			}
			if ( p->dist >= emaxs->raw[p->type] ) {
				return 2;
			}
			return 3;
		}

		// general case
		float dist1, dist2;
		switch ( p->signbits ) {
		case 0:
			dist1 = p->normal.x*emaxs->x + p->normal.y*emaxs->y + p->normal.z*emaxs->z;
			dist2 = p->normal.x*emins->x + p->normal.y*emins->y + p->normal.z*emins->z;
			break;
		case 1:
			dist1 = p->normal.x*emins->x + p->normal.y*emaxs->y + p->normal.z*emaxs->z;
			dist2 = p->normal.x*emaxs->x + p->normal.y*emins->y + p->normal.z*emins->z;
			break;
		case 2:
			dist1 = p->normal.x*emaxs->x + p->normal.y*emins->y + p->normal.z*emaxs->z;
			dist2 = p->normal.x*emins->x + p->normal.y*emaxs->y + p->normal.z*emins->z;
			break;
		case 3:
			dist1 = p->normal.x*emins->x + p->normal.y*emins->y + p->normal.z*emaxs->z;
			dist2 = p->normal.x*emaxs->x + p->normal.y*emaxs->y + p->normal.z*emins->z;
			break;
		case 4:
			dist1 = p->normal.x*emaxs->x + p->normal.y*emaxs->y + p->normal.z*emins->z;
			dist2 = p->normal.x*emins->x + p->normal.y*emins->y + p->normal.z*emaxs->z;
			break;
		case 5:
			dist1 = p->normal.x*emins->x + p->normal.y*emaxs->y + p->normal.z*emins->z;
			dist2 = p->normal.x*emaxs->x + p->normal.y*emins->y + p->normal.z*emaxs->z;
			break;
		case 6:
			dist1 = p->normal.x*emaxs->x + p->normal.y*emins->y + p->normal.z*emins->z;
			dist2 = p->normal.x*emins->x + p->normal.y*emaxs->y + p->normal.z*emaxs->z;
			break;
		case 7:
			dist1 = p->normal.x*emins->x + p->normal.y*emins->y + p->normal.z*emins->z;
			dist2 = p->normal.x*emaxs->x + p->normal.y*emaxs->y + p->normal.z*emaxs->z;
			break;
		default:
			dist1 = dist2 = 0;		// shut up compiler
			break;
		}

		int sides = 0;
		if ( dist1 >= p->dist )
			sides = 1;
		if ( dist2 < p->dist )
			sides |= 2;

		return sides;
	}

#else

	#if defined(_MSC_VER)
		#pragma warning( push )
		#pragma warning( disable: 4035 )
	#elif defined(__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wreturn-type"
	#endif // _MSC_VER

	Q_NAKED int BoxOnPlaneSide( vector3 *emins, vector3 *emaxs, struct cplane_s *p ) {
		qnakedstart(BOPS)

		static int Q_USED bops_initialized;
		static int Q_USED Ljmptab[8];

	#if defined(_MSC_VER)
		qasm1( push ebx )
		qasm2( cmp bops_initialized, 1 )
		qasm1( je initialized )
		qasm2( mov bops_initialized, 1 )
	#elif defined(__GNUC__)
		__asm__( "push %ebx" );
		__asm__( "cmp %0, 1\n" : : "r" (bops_initialized) );
		qasm1( je initialized )
		__asm__( "mov %0, 1\n" : "=r" (bops_initialized) : );
	#endif

	#if defined(_MSC_VER)
		qasm2( mov Ljmptab[0 * 4], offset Lcase0 )
		qasm2( mov Ljmptab[1 * 4], offset Lcase1 )
		qasm2( mov Ljmptab[2 * 4], offset Lcase2 )
		qasm2( mov Ljmptab[3 * 4], offset Lcase3 )
		qasm2( mov Ljmptab[4 * 4], offset Lcase4 )
		qasm2( mov Ljmptab[5 * 4], offset Lcase5 )
		qasm2( mov Ljmptab[6 * 4], offset Lcase6 )
		qasm2( mov Ljmptab[7 * 4], offset Lcase7 )
	#elif defined(__GNUC__)
		__asm__( "mov %0, offset Lcase0\n" : "=m" (Ljmptab[0]) : );
		__asm__( "mov %0, offset Lcase1\n" : "=m" (Ljmptab[1]) : );
		__asm__( "mov %0, offset Lcase2\n" : "=m" (Ljmptab[2]) : );
		__asm__( "mov %0, offset Lcase3\n" : "=m" (Ljmptab[3]) : );
		__asm__( "mov %0, offset Lcase4\n" : "=m" (Ljmptab[4]) : );
		__asm__( "mov %0, offset Lcase5\n" : "=m" (Ljmptab[5]) : );
		__asm__( "mov %0, offset Lcase6\n" : "=m" (Ljmptab[6]) : );
		__asm__( "mov %0, offset Lcase7\n" : "=m" (Ljmptab[7]) : );
	#endif

		qasmL(initialized:)
		qasm2( mov edx, dword ptr[4 + 12 + esp] )
		qasm2( mov ecx, dword ptr[4 + 4 + esp] )
		qasm2( xor eax, eax )
		qasm2( mov ebx, dword ptr[4 + 8 + esp] )
		qasm2( mov al, byte ptr[17 + edx] )
		qasm2( cmp al, 8 )
		qasm1( jge Lerror )
		qasm1( fld dword ptr[0 + edx] )
		qasm1( fld st( 0 ) )
	#if defined(_MSC_VER)
		qasm1( jmp dword ptr[Ljmptab + eax * 4] )
	#elif defined(__GNUC__)
		__asm__( "jmp dword ptr[%0 + eax * 4]\n" : : "r" (Ljmptab) );
	#endif

		qasmL(Lcase0:)
		qasm1( fmul dword ptr[ebx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( jmp LSetSides )

		qasmL(Lcase1:)
		qasm1( fmul dword ptr[ecx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( jmp LSetSides )

		qasmL(Lcase2:)
		qasm1( fmul dword ptr[ebx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( jmp LSetSides )

		qasmL(Lcase3:)
		qasm1( fmul dword ptr[ecx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( jmp LSetSides )

		qasmL(Lcase4:)
		qasm1( fmul dword ptr[ebx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( jmp LSetSides )

		qasmL(Lcase5:)
		qasm1( fmul dword ptr[ecx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( jmp LSetSides )

		qasmL(Lcase6:)
		qasm1( fmul dword ptr[ebx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ecx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( jmp LSetSides )

		qasmL(Lcase7:)
		qasm1( fmul dword ptr[ecx] )
		qasm1( fld dword ptr[0 + 4 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[4 + ecx] )
		qasm1( fld dword ptr[0 + 8 + edx] )
		qasm1( fxch st( 2 ) )
		qasm1( fmul dword ptr[4 + ebx] )
		qasm1( fxch st( 2 ) )
		qasm1( fld st( 0 ) )
		qasm1( fmul dword ptr[8 + ecx] )
		qasm1( fxch st( 5 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fmul dword ptr[8 + ebx] )
		qasm1( fxch st( 1 ) )
		qasm2( faddp st( 3 ), st( 0 ) )
		qasm1( fxch st( 3 ) )
		qasm2( faddp st( 2 ), st( 0 ) )

		qasmL(LSetSides:)
		qasm2( faddp st( 2 ), st( 0 ) )
		qasm1( fcomp dword ptr[12 + edx] )
		qasm2( xor ecx, ecx )
		qasm1( fnstsw ax )
		qasm1( fcomp dword ptr[12 + edx] )
		qasm2( and ah, 1 )
		qasm2( xor ah, 1 )
		qasm2( add cl, ah )
		qasm1( fnstsw ax )
		qasm2( and ah, 1 )
		qasm2( add ah, ah )
		qasm2( add cl, ah )
	#if defined(_MSC_VER)
		qasm1( pop ebx )
	#elif defined(__GNUC__)
		__asm__( "pop %ebx" );
	#endif
		qasm2( mov eax, ecx )
		qasm1( ret )

		qasmL(Lerror:)
		qasm1( int 3 )

		qnakedend(BOPS)
	}
	#if defined(_MSC_VER)
		#pragma warning( pop )
	#elif defined(__GNUC__)
		#pragma GCC diagnostic pop
	#endif // _MSC_VER

#endif // C_ONLY

float RadiusFromBounds( const vector3 *mins, const vector3 *maxs ) {
	int		i;
	vector3	corner;
	float	a, b;

	for ( i = 0; i<3; i++ ) {
		a = Q_fabs( mins->raw[i] );
		b = Q_fabs( maxs->raw[i] );
		corner.raw[i] = a > b ? a : b;
	}

	return VectorLength( &corner );
}


void ClearBounds( vector3 *mins, vector3 *maxs ) {
	mins->x = mins->y = mins->z = 99999;
	maxs->x = maxs->y = maxs->z = -99999;
}

float DistanceHorizontal( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return sqrtf( v.x*v.x + v.y*v.y );	//Leave off the z component
}

float DistanceHorizontalSquared( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return v.x*v.x + v.y*v.y;	//Leave off the z component
}

void AddPointToBounds( const vector3 *v, vector3 *mins, vector3 *maxs ) {
	if ( v->x < mins->x )	mins->x = v->x;
	if ( v->x > maxs->x )	maxs->x = v->x;

	if ( v->y < mins->y )	mins->y = v->y;
	if ( v->y > maxs->y )	maxs->y = v->y;

	if ( v->z < mins->z )	mins->z = v->z;
	if ( v->z > maxs->z )	maxs->z = v->z;
}

void VectorAdd( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut ) {
#ifdef USE_SSE
	__asm {
		mov ecx, vec1
			movss xmm0, [ecx]
			movhps xmm0, [ecx + 4]

			mov edx, vec2
			movss xmm1, [edx]
			movhps xmm1, [edx + 4]

			addps xmm0, xmm1

			mov eax, vecOut
			movss[eax], xmm0
			movhps[eax + 4], xmm0
	}
#else
	vecOut->x = vec1->x + vec2->x;
	vecOut->y = vec1->y + vec2->y;
	vecOut->z = vec1->z + vec2->z;
#endif
}

void VectorSubtract( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut ) {
#ifdef USE_SSE
	__asm {
		mov ecx, vec1
			movss xmm0, [ecx]
			movhps xmm0, [ecx + 4]

			mov edx, vec2
			movss xmm1, [edx]
			movhps xmm1, [edx + 4]

			subps xmm0, xmm1

			mov eax, vecOut
			movss[eax], xmm0
			movhps[eax + 4], xmm0
	}
#else
	vecOut->x = vec1->x - vec2->x;
	vecOut->y = vec1->y - vec2->y;
	vecOut->z = vec1->z - vec2->z;
#endif
}

void VectorNegate( const vector3 *vecIn, vector3 *vecOut ) {
	vecOut->x = -vecIn->x;
	vecOut->y = -vecIn->y;
	vecOut->z = -vecIn->z;
}

void VectorScale( const vector3 *vecIn, float scale, vector3 *vecOut ) {
#ifdef USE_SSE
	__asm {
		movss xmm0, scale
			shufps xmm0, xmm0, 0x0

			mov edx, vecIn
			movss xmm1, [edx]
			movhps xmm1, [edx + 4]

			mulps xmm0, xmm1

			mov eax, vecOut
			movss[eax], xmm0
			movhps[eax + 4], xmm0
	}
#else
	vecOut->x = vecIn->x * scale;
	vecOut->y = vecIn->y * scale;
	vecOut->z = vecIn->z * scale;
#endif
}

void VectorScale4( const vector4 *vecIn, float scale, vector4 *vecOut ) {
	vecOut->x = vecIn->x * scale;
	vecOut->y = vecIn->y * scale;
	vecOut->z = vecIn->z * scale;
	vecOut->w = vecIn->w * scale;
}

void VectorScaleVector( const vector3 *vecIn, const vector3 *vecScale, vector3 *vecOut ) {
	vecOut->x = vecIn->x * vecScale->x;
	vecOut->y = vecIn->y * vecScale->y;
	vecOut->z = vecIn->z * vecScale->z;
}

void VectorMA( const vector3 *vec1, float scale, const vector3 *vec2, vector3 *vecOut ) {
	vecOut->x = vec1->x + vec2->x*scale;
	vecOut->y = vec1->y + vec2->y*scale;
	vecOut->z = vec1->z + vec2->z*scale;
}

void VectorLerp( const vector3 *vec1, float frac, const vector3 *vec2, vector3 *vecOut ) {
	vecOut->x = vec1->x + (vec2->x - vec1->x)*frac;
	vecOut->y = vec1->y + (vec2->y - vec1->y)*frac;
	vecOut->z = vec1->z + (vec2->z - vec1->z)*frac;
}

void VectorLerp4( const vector4 *vec1, float frac, const vector4 *vec2, vector4 *vecOut ) {
	vecOut->x = vec1->x + (vec2->x - vec1->x)*frac;
	vecOut->y = vec1->y + (vec2->y - vec1->y)*frac;
	vecOut->z = vec1->z + (vec2->z - vec1->z)*frac;
	vecOut->w = vec1->w + (vec2->w - vec1->w)*frac;
}

float VectorLength( const vector3 *vec ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec
			movss xmm1, [edx]
			movhps xmm1, [edx + 4]

			movaps xmm2, xmm1

			mulps xmm1, xmm2

			movaps xmm0, xmm1

			shufps xmm0, xmm0, 0x32
			addps xmm1, xmm0

			shufps xmm0, xmm0, 0x32
			addps xmm1, xmm0

			sqrtss xmm1, xmm1
			movss[res], xmm1
	}

	return res;
#else
	return sqrtf( vec->x*vec->x + vec->y*vec->y + vec->z*vec->z );
#endif
}

float VectorLengthSquared( const vector3 *vec ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec
			movss xmm1, [edx]
			movhps xmm1, [edx + 4]

			movaps xmm2, xmm1

			mulps xmm1, xmm2

			movaps xmm0, xmm1

			shufps xmm0, xmm0, 0x32
			addps xmm1, xmm0

			shufps xmm0, xmm0, 0x32
			addps xmm1, xmm0

			movss[res], xmm1
	}

	return res;
#else
	return (vec->x*vec->x + vec->y*vec->y + vec->z*vec->z);
#endif
}

float Distance( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return VectorLength( &v );
}

float DistanceSquared( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
void VectorNormalizeFast( vector3 *vec ) {
	float ilength = Q_rsqrt( DotProduct( vec, vec ) );

	vec->x *= ilength;
	vec->y *= ilength;
	vec->z *= ilength;
}

float VectorNormalize( vector3 *vec ) {
	float length = DotProduct( vec, vec );

	if ( length ) {
		float reciprocal = 1.0f / (float)sqrtf( length );
		length *= reciprocal;
		VectorScale( vec, reciprocal, vec );
	}

	return length;
}

float VectorNormalize2( const vector3 *vec, vector3 *vecOut ) {
	float length = DotProduct( vec, vec );

	if ( length ) {
		float reciprocal = 1.0f / (float)sqrtf( length );
		length *= reciprocal;
		VectorScale( vec, reciprocal, vecOut );
	}
	else
		VectorClear( vecOut );

	return length;
}

void VectorCopy( const vector3 *vecIn, vector3 *vecOut ) {
	vecOut->x = vecIn->x;
	vecOut->y = vecIn->y;
	vecOut->z = vecIn->z;
}
void IVectorCopy( const ivector3 *vecIn, ivector3 *vecOut ) {
	vecOut->x = vecIn->x;
	vecOut->y = vecIn->y;
	vecOut->z = vecIn->z;
}

void VectorCopy4( const vector4 *vecIn, vector4 *vecOut ) {
	vecOut->x = vecIn->x;
	vecOut->y = vecIn->y;
	vecOut->z = vecIn->z;
	vecOut->w = vecIn->w;
}

void VectorSet( vector3 *vec, float x, float y, float z ) {
	vec->x = x;
	vec->y = y;
	vec->z = z;
}

void VectorSet4( vector4 *vec, float x, float y, float z, float w ) {
	vec->x = x;
	vec->y = y;
	vec->z = z;
	vec->w = w;
}

void VectorClear( vector3 *vec ) {
	vec->x = vec->y = vec->z = 0.0f;
}

void VectorClear4( vector4 *vec ) {
	vec->x = vec->y = vec->z = vec->w = 0.0f;
}

void VectorInc( vector3 *vec ) {
	vec->x += 1.0f;
	vec->y += 1.0f;
	vec->z += 1.0f;
}

void VectorDec( vector3 *vec ) {
	vec->x -= 1.0f;
	vec->y -= 1.0f;
	vec->z -= 1.0f;
}

void VectorInverse( vector3 *vec ) {
	vec->x = -vec->x;
	vec->y = -vec->y;
	vec->z = -vec->z;
}

void VectorAverage( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut ) {
	vecOut->x = (vec1->x + vec2->x) * 0.5f;
	vecOut->y = (vec1->y + vec2->y) * 0.5f;
	vecOut->z = (vec1->z + vec2->z) * 0.5f;
}

void CrossProduct( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut ) {
	vecOut->x = (vec1->y * vec2->z) - (vec1->z * vec2->y);
	vecOut->y = (vec1->z * vec2->x) - (vec1->x * vec2->z);
	vecOut->z = (vec1->x * vec2->y) - (vec1->y * vec2->x);
}

float DotProduct( const vector3 *vec1, const vector3 *vec2 ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec1
			movss xmm1, [edx]
			movhps xmm1, [edx + 4]

			mov edx, vec2
			movss xmm2, [edx]
			movhps xmm2, [edx + 4]

			mulps xmm1, xmm2

			movaps xmm0, xmm1

			shufps xmm0, xmm0, 0x32
			addps xmm1, xmm0

			shufps xmm0, xmm0, 0x32
			addps xmm1, xmm0

			movss[res], xmm1
	}

	return res;
#else
	return vec1->x*vec2->x + vec1->y*vec2->y + vec1->z*vec2->z;
#endif
}

bool VectorCompare( const vector3 *vec1, const vector3 *vec2 ) {
	return flcmp( vec1->x, vec2->x ) && flcmp( vec1->y, vec2->y ) && flcmp( vec1->z, vec2->z );
}

#ifdef _MSC_VER // windows

#include <float.h>
#pragma fenv_access( on )

static float roundfloat( float n ) {
	return (n < 0.0f) ? ceilf( n - 0.5f ) : floorf( n + 0.5f );
}

#else // linux, mac

#include <fenv.h>

#endif

// Round a vector to integers for more efficient network transmission
void VectorSnap( vector3 *v ) {
#ifdef _MSC_VER
	unsigned int oldcontrol, newcontrol;

	_controlfp_s( &oldcontrol, 0, 0 );
	_controlfp_s( &newcontrol, _RC_NEAR, _MCW_RC );

	v->x = roundfloat( v->x );
	v->y = roundfloat( v->y );
	v->z = roundfloat( v->z );

	_controlfp_s( &newcontrol, oldcontrol, _MCW_RC );
#else // pure c99
	int oldround = fegetround();
	fesetround( FE_TONEAREST );

	v->x = nearbyint( v->x );
	v->y = nearbyint( v->y );
	v->z = nearbyint( v->z );

	fesetround( oldround );
#endif
}

// Round a vector to integers for more efficient network transmission, but make sure that it rounds towards a given
//	point rather than blindly truncating.
// This prevents it from truncating into a wall.
void VectorSnapTowards( vector3 *v, vector3 *to ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( to->raw[i] <= v->raw[i] )
			v->raw[i] = floorf( v->raw[i] );
		else
			v->raw[i] = ceilf( v->raw[i] );
	}
}

int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( (val >>= 1) != 0 ) {
		answer++;
	}
	return answer;
}

void MatrixMultiply( const vector3 in1[3], const vector3 in2[3], vector3 out[3] ) {
	out[0].x = in1[0].x*in2[0].x + in1[0].y*in2[1].x + in1[0].z*in2[2].x;
	out[0].y = in1[0].x*in2[0].y + in1[0].y*in2[1].y + in1[0].z*in2[2].y;
	out[0].z = in1[0].x*in2[0].z + in1[0].y*in2[1].z + in1[0].z*in2[2].z;
	out[1].x = in1[1].x*in2[0].x + in1[1].y*in2[1].x + in1[1].z*in2[2].x;
	out[1].y = in1[1].x*in2[0].y + in1[1].y*in2[1].y + in1[1].z*in2[2].y;
	out[1].z = in1[1].x*in2[0].z + in1[1].y*in2[1].z + in1[1].z*in2[2].z;
	out[2].x = in1[2].x*in2[0].x + in1[2].y*in2[1].x + in1[2].z*in2[2].x;
	out[2].y = in1[2].x*in2[0].y + in1[2].y*in2[1].y + in1[2].z*in2[2].y;
	out[2].z = in1[2].x*in2[0].z + in1[2].y*in2[1].z + in1[2].z*in2[2].z;
}


void AngleVectors( const vector3 *angles, vector3 *forward, vector3 *right, vector3 *up ) {
	float angle, sr, sp, sy, cr, cp, cy;

	angle = angles->yaw * (M_PI * 2 / 360);
	sy = sinf( angle );
	cy = cosf( angle );
	angle = angles->pitch * (M_PI * 2 / 360);
	sp = sinf( angle );
	cp = cosf( angle );
	angle = angles->roll * (M_PI * 2 / 360);
	sr = sinf( angle );
	cr = cosf( angle );

	if ( forward ) {
		forward->x = cp*cy;
		forward->y = cp*sy;
		forward->z = -sp;
	}
	if ( right ) {
		right->x = (-1 * sr*sp*cy + -1 * cr*-sy);
		right->y = (-1 * sr*sp*sy + -1 * cr*cy);
		right->z = -1 * sr*cp;
	}
	if ( up ) {
		up->x = (cr*sp*cy + -sr*-sy);
		up->y = (cr*sp*sy + -sr*cy);
		up->z = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vector3 *dst, const vector3 *src ) {
	int	pos;
	int i;
	float minelem = 1.0F;
	vector3 tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ ) {
		if ( Q_fabs( src->raw[i] ) < minelem ) {
			pos = i;
			minelem = Q_fabs( src->raw[i] );
		}
	}
	tempvec.x = tempvec.y = tempvec.z = 0.0f;
	tempvec.raw[pos] = 1.0f;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, &tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

/*
** NormalToLatLong
**
** We use two byte encoded normals in some space critical applications.
** Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
** Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format
**
*/
//rwwRMG - added
void NormalToLatLong( const vector3 *normal, byte bytes[2] ) {
	// check for singularities
	if ( !normal->x && !normal->y ) {
		if ( normal->z > 0.0f ) {
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		}
		else {
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	}
	else {
		int	a, b;

		a = (int)(RAD2DEG( (float)atan2f( normal->y, normal->x ) ) * (255.0f / 360.0f));
		a &= 0xff;

		b = (int)(RAD2DEG( (float)acosf( normal->z ) ) * (255.0f / 360.0f));
		b &= 0xff;

		bytes[0] = b;	// longitude
		bytes[1] = a;	// lattitude
	}
}

// This is the VC libc version of rand() without multiple seeds per thread or 12 levels
// of subroutine calls.
// Both calls have been designed to minimise the inherent number of float <--> int
// conversions and the additional math required to get the desired value.
// eg the typical tint = (rand() * 255) / 32768
// becomes tint = irand(0, 255)

static uint32_t	holdrand = 0x89abcdef; // 64 bit support for OpenJK

void Rand_Init( int seed ) {
	holdrand = seed;
}

// Returns a float min <= x < max (exclusive; will get max - 0.00001f; but never max)

float flrand( float min, float max ) {
	float	result;

	holdrand = (holdrand * 214013L) + 2531011L;
	result = (float)(holdrand >> 17);						// 0 - 32767 range
	result = ((result * (max - min)) / (float)QRAND_MAX) + min;

	return(result);
}

// Returns an integer min <= x <= max (ie inclusive)

int irand( int min, int max ) {
	int		result;

	assert( (max - min) < QRAND_MAX );

	max++;
	holdrand = (holdrand * 214013L) + 2531011L;
	result = holdrand >> 17;
	result = ((result * (max - min)) >> 15) + min;
	return(result);
}

int Q_irand( int value1, int value2 ) {
	return irand( value1, value2 );
}

float Q_powf( float x, int y ) {
	float r = x;
	for ( y--; y > 0; y-- )
		r *= x;
	return r;
}

float DotProductNormalize( const vector3 *inVec1, const vector3 *inVec2 ) {
	vector3	v1, v2;

	VectorNormalize2( inVec1, &v1 );
	VectorNormalize2( inVec2, &v2 );

	return DotProduct( &v1, &v2 );
}

qboolean G_FindClosestPointOnLineSegment( const vector3 *start, const vector3 *end, const vector3 *from, vector3 *result ) {
	vector3	vecStart2From, vecStart2End, vecEnd2Start, vecEnd2From;
	float	distEnd2From, distEnd2Result, theta, cos_theta, dot;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract( from, start, &vecStart2From );
	VectorSubtract( end, start, &vecStart2End );

	dot = DotProductNormalize( &vecStart2From, &vecStart2End );

	if ( dot <= 0 ) {
		//The perpendicular would be beyond or through the start point
		VectorCopy( start, result );
		return qfalse;
	}

	if ( dot == 1 ) {
		//parallel, closer of 2 points will be the target
		if ( (VectorLengthSquared( &vecStart2From )) < (VectorLengthSquared( &vecStart2End )) ) {
			VectorCopy( from, result );
		}
		else {
			VectorCopy( end, result );
		}
		return qfalse;
	}

	//Try other end
	VectorSubtract( from, end, &vecEnd2From );
	VectorSubtract( start, end, &vecEnd2Start );

	dot = DotProductNormalize( &vecEnd2From, &vecEnd2Start );

	if ( dot <= 0 ) {//The perpendicular would be beyond or through the start point
		VectorCopy( end, result );
		return qfalse;
	}

	if ( dot == 1 ) {//parallel, closer of 2 points will be the target
		if ( (VectorLengthSquared( &vecEnd2From )) < (VectorLengthSquared( &vecEnd2Start )) ) {
			VectorCopy( from, result );
		}
		else {
			VectorCopy( end, result );
		}
		return qfalse;
	}

	//		      /|
	//		  c  / |
	//		    /  |a
	//	theta  /)__|
	//		      b
	//cos(theta) = b / c
	//solve for b
	//b = cos(theta) * c

	//angle between vecs end2from and end2start, should be between 0 and 90
	theta = 90 * (1 - dot);//theta

	//Get length of side from End2Result using sine of theta
	distEnd2From = VectorLength( &vecEnd2From );//c
	cos_theta = cosf( DEG2RAD( theta ) );//cos(theta)
	distEnd2Result = cos_theta * distEnd2From;//b

	//Extrapolate to find result
	VectorNormalize( &vecEnd2Start );
	VectorMA( end, distEnd2Result, &vecEnd2Start, result );

	//perpendicular intersection is between the 2 endpoints
	return qtrue;
}

float G_PointDistFromLineSegment( const vector3 *start, const vector3 *end, const vector3 *from ) {
	vector3	vecStart2From, vecStart2End, vecEnd2Start, vecEnd2From, intersection;
	float	distEnd2From, distStart2From, distEnd2Result, theta, cos_theta, dot;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract( from, start, &vecStart2From );
	VectorSubtract( end, start, &vecStart2End );
	VectorSubtract( from, end, &vecEnd2From );
	VectorSubtract( start, end, &vecEnd2Start );

	dot = DotProductNormalize( &vecStart2From, &vecStart2End );

	distStart2From = Distance( start, from );
	distEnd2From = Distance( end, from );

	if ( dot <= 0 ) {
		//The perpendicular would be beyond or through the start point
		return distStart2From;
	}

	if ( dot == 1 ) {
		//parallel, closer of 2 points will be the target
		return ((distStart2From < distEnd2From) ? distStart2From : distEnd2From);
	}

	//Try other end

	dot = DotProductNormalize( &vecEnd2From, &vecEnd2Start );

	if ( dot <= 0 ) {//The perpendicular would be beyond or through the end point
		return distEnd2From;
	}

	if ( dot == 1 ) {//parallel, closer of 2 points will be the target
		return ((distStart2From < distEnd2From) ? distStart2From : distEnd2From);
	}

	//		      /|
	//		  c  / |
	//		    /  |a
	//	theta  /)__|
	//		      b
	//cos(theta) = b / c
	//solve for b
	//b = cos(theta) * c

	//angle between vecs end2from and end2start, should be between 0 and 90
	theta = 90 * (1 - dot);//theta

	//Get length of side from End2Result using sine of theta
	cos_theta = cosf( DEG2RAD( theta ) );//cos(theta)
	distEnd2Result = cos_theta * distEnd2From;//b

	//Extrapolate to find result
	VectorNormalize( &vecEnd2Start );
	VectorMA( end, distEnd2Result, &vecEnd2Start, &intersection );

	//perpendicular intersection is between the 2 endpoints, return dist to it from from
	return Distance( &intersection, from );
}


//TODO: unit test precision at different ranges (and double conversion/promotion?)
bool flcmp( const float &f1, const float &f2, const float epsilon ) {
	const float delta = std::abs( f2 - f1 );

	return delta < epsilon;
}

bool flcmp_old( const float &f1, const float &f2, const float epsilon ) {
	return Q_fabs( f2 - f1 ) < epsilon;
}

bool dblcmp( const double &f1, const double &f2, const double epsilon ) {
	const double delta = std::abs( f2 - f1 );

	return delta < epsilon;
}
