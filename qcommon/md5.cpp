#include "q_shared.h"

namespace Crypto {

	static const size_t chunkSize = 64;

	struct MD5Context {
		uint32_t lo, hi;
		uint32_t a, b, c, d;
		uint8_t buffer[chunkSize];
		uint32_t block[16];
	};

	typedef uint32_t (*md5Func_t)(uint32_t x, uint32_t y, uint32_t z);

	static uint32_t F( uint32_t x, uint32_t y, uint32_t z ) {
		return z ^ (x & (y ^ z));
	}
	static uint32_t G( uint32_t x, uint32_t y, uint32_t z ) {
		return y ^ (z & (x ^ y));
	}
	static uint32_t H( uint32_t x, uint32_t y, uint32_t z ) {
		return (x ^ y) ^ z;
	}
	static uint32_t H2( uint32_t x, uint32_t y, uint32_t z ) {
		return x ^ (y ^ z);
	}
	static uint32_t I( uint32_t x, uint32_t y, uint32_t z ) {
		return y ^ (x | ~z);
	}

	static void MD5_Step( md5Func_t func, uint32_t *a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t t, uint32_t s ) {
		*a += func( b, c, d ) + x + t;
		*a = ((*a) << s) | (((*a) & 0xFFFFFFFFu) >> (32 - s));
		*a += b;
	}

	#ifdef Q3_LITTLE_ENDIAN

	#define SET( n ) (*(uint32_t *)&ptr[(n) * 4])
	#define GET( n ) SET( n )

	#else // Q3_LITTLE_ENDIAN

	#define SET( n ) \
		(ctx->block[(n)] = \
		(uint32_t)ptr[(n) * 4] | \
		((uint32_t)ptr[(n) * 4 + 1] << 8) | \
		((uint32_t)ptr[(n) * 4 + 2] << 16) | \
		((uint32_t)ptr[(n) * 4 + 3] << 24))
	#define GET( n ) \
		(ctx->block[(n)])
	#endif

	static const void *MD5_Body( MD5Context *ctx, const void *data, unsigned long size ) {
		const uint8_t *ptr = (const uint8_t *)data;

		uint32_t a = ctx->a;
		uint32_t b = ctx->b;
		uint32_t c = ctx->c;
		uint32_t d = ctx->d;

		uint32_t saved_a, saved_b, saved_c, saved_d;
		do {
			saved_a = a;
			saved_b = b;
			saved_c = c;
			saved_d = d;

			MD5_Step( F, &a, b, c, d, SET( 0 ), 0xd76aa478u, 7 );
			MD5_Step( F, &d, a, b, c, SET( 1 ), 0xe8c7b756u, 12 );
			MD5_Step( F, &c, d, a, b, SET( 2 ), 0x242070dbu, 17 );
			MD5_Step( F, &b, c, d, a, SET( 3 ), 0xc1bdceeeu, 22 );
			MD5_Step( F, &a, b, c, d, SET( 4 ), 0xf57c0fafu, 7 );
			MD5_Step( F, &d, a, b, c, SET( 5 ), 0x4787c62au, 12 );
			MD5_Step( F, &c, d, a, b, SET( 6 ), 0xa8304613u, 17 );
			MD5_Step( F, &b, c, d, a, SET( 7 ), 0xfd469501u, 22 );
			MD5_Step( F, &a, b, c, d, SET( 8 ), 0x698098d8u, 7 );
			MD5_Step( F, &d, a, b, c, SET( 9 ), 0x8b44f7afu, 12 );
			MD5_Step( F, &c, d, a, b, SET( 10 ), 0xffff5bb1u, 17 );
			MD5_Step( F, &b, c, d, a, SET( 11 ), 0x895cd7beu, 22 );
			MD5_Step( F, &a, b, c, d, SET( 12 ), 0x6b901122u, 7 );
			MD5_Step( F, &d, a, b, c, SET( 13 ), 0xfd987193u, 12 );
			MD5_Step( F, &c, d, a, b, SET( 14 ), 0xa679438eu, 17 );
			MD5_Step( F, &b, c, d, a, SET( 15 ), 0x49b40821u, 22 );
			MD5_Step( G, &a, b, c, d, GET( 1 ), 0xf61e2562u, 5 );
			MD5_Step( G, &d, a, b, c, GET( 6 ), 0xc040b340u, 9 );
			MD5_Step( G, &c, d, a, b, GET( 11 ), 0x265e5a51u, 14 );
			MD5_Step( G, &b, c, d, a, GET( 0 ), 0xe9b6c7aau, 20 );
			MD5_Step( G, &a, b, c, d, GET( 5 ), 0xd62f105du, 5 );
			MD5_Step( G, &d, a, b, c, GET( 10 ), 0x02441453u, 9 );
			MD5_Step( G, &c, d, a, b, GET( 15 ), 0xd8a1e681u, 14 );
			MD5_Step( G, &b, c, d, a, GET( 4 ), 0xe7d3fbc8u, 20 );
			MD5_Step( G, &a, b, c, d, GET( 9 ), 0x21e1cde6u, 5 );
			MD5_Step( G, &d, a, b, c, GET( 14 ), 0xc33707d6u, 9 );
			MD5_Step( G, &c, d, a, b, GET( 3 ), 0xf4d50d87u, 14 );
			MD5_Step( G, &b, c, d, a, GET( 8 ), 0x455a14edu, 20 );
			MD5_Step( G, &a, b, c, d, GET( 13 ), 0xa9e3e905u, 5 );
			MD5_Step( G, &d, a, b, c, GET( 2 ), 0xfcefa3f8u, 9 );
			MD5_Step( G, &c, d, a, b, GET( 7 ), 0x676f02d9u, 14 );
			MD5_Step( G, &b, c, d, a, GET( 12 ), 0x8d2a4c8au, 20 );
			MD5_Step( H, &a, b, c, d, GET( 5 ), 0xfffa3942u, 4 );
			MD5_Step( H2, &d, a, b, c, GET( 8 ), 0x8771f681u, 11 );
			MD5_Step( H, &c, d, a, b, GET( 11 ), 0x6d9d6122u, 16 );
			MD5_Step( H2, &b, c, d, a, GET( 14 ), 0xfde5380cu, 23 );
			MD5_Step( H, &a, b, c, d, GET( 1 ), 0xa4beea44u, 4 );
			MD5_Step( H2, &d, a, b, c, GET( 4 ), 0x4bdecfa9u, 11 );
			MD5_Step( H, &c, d, a, b, GET( 7 ), 0xf6bb4b60u, 16 );
			MD5_Step( H2, &b, c, d, a, GET( 10 ), 0xbebfbc70u, 23 );
			MD5_Step( H, &a, b, c, d, GET( 13 ), 0x289b7ec6u, 4 );
			MD5_Step( H2, &d, a, b, c, GET( 0 ), 0xeaa127fau, 11 );
			MD5_Step( H, &c, d, a, b, GET( 3 ), 0xd4ef3085u, 16 );
			MD5_Step( H2, &b, c, d, a, GET( 6 ), 0x04881d05u, 23 );
			MD5_Step( H, &a, b, c, d, GET( 9 ), 0xd9d4d039u, 4 );
			MD5_Step( H2, &d, a, b, c, GET( 12 ), 0xe6db99e5u, 11 );
			MD5_Step( H, &c, d, a, b, GET( 15 ), 0x1fa27cf8u, 16 );
			MD5_Step( H2, &b, c, d, a, GET( 2 ), 0xc4ac5665u, 23 );
			MD5_Step( I, &a, b, c, d, GET( 0 ), 0xf4292244u, 6 );
			MD5_Step( I, &d, a, b, c, GET( 7 ), 0x432aff97u, 10 );
			MD5_Step( I, &c, d, a, b, GET( 14 ), 0xab9423a7u, 15 );
			MD5_Step( I, &b, c, d, a, GET( 5 ), 0xfc93a039u, 21 );
			MD5_Step( I, &a, b, c, d, GET( 12 ), 0x655b59c3u, 6 );
			MD5_Step( I, &d, a, b, c, GET( 3 ), 0x8f0ccc92u, 10 );
			MD5_Step( I, &c, d, a, b, GET( 10 ), 0xffeff47du, 15 );
			MD5_Step( I, &b, c, d, a, GET( 1 ), 0x85845dd1u, 21 );
			MD5_Step( I, &a, b, c, d, GET( 8 ), 0x6fa87e4fu, 6 );
			MD5_Step( I, &d, a, b, c, GET( 15 ), 0xfe2ce6e0u, 10 );
			MD5_Step( I, &c, d, a, b, GET( 6 ), 0xa3014314u, 15 );
			MD5_Step( I, &b, c, d, a, GET( 13 ), 0x4e0811a1u, 21 );
			MD5_Step( I, &a, b, c, d, GET( 4 ), 0xf7537e82u, 6 );
			MD5_Step( I, &d, a, b, c, GET( 11 ), 0xbd3af235u, 10 );
			MD5_Step( I, &c, d, a, b, GET( 2 ), 0x2ad7d2bbu, 15 );
			MD5_Step( I, &b, c, d, a, GET( 9 ), 0xeb86d391u, 21 );

			a += saved_a;
			b += saved_b;
			c += saved_c;
			d += saved_d;

			ptr += chunkSize;
		} while ( size -= chunkSize );

		ctx->a = a;
		ctx->b = b;
		ctx->c = c;
		ctx->d = d;

		return ptr;
	}

	static void MD5_Init( MD5Context *ctx ) {
		ctx->a = 0x67452301;
		ctx->b = 0xefcdab89;
		ctx->c = 0x98badcfe;
		ctx->d = 0x10325476;

		ctx->lo = 0;
		ctx->hi = 0;
	}

	static void MD5_Update( MD5Context *ctx, const void *data, unsigned long size ) {
		uint32_t saved_lo = ctx->lo;
		if ( (ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo ) {
			ctx->hi++;
		}
		ctx->hi += size >> 29;

		unsigned long used = saved_lo & 0x3f;

		if ( used ) {
			unsigned long available = chunkSize - used;

			if ( size < available ) {
				memcpy( &ctx->buffer[used], data, size );
				return;
			}

			memcpy( &ctx->buffer[used], data, available );
			data = (const uint8_t *)data + available;
			size -= available;
			MD5_Body( ctx, ctx->buffer, chunkSize );
		}

		if ( size >= chunkSize ) {
			data = MD5_Body( ctx, data, size & ~(unsigned long)0x3f );
			size &= 0x3f;
		}

		memcpy( ctx->buffer, data, size );
	}

	static void MD5_Final( MD5Context *ctx, uint8_t *result ) {
		unsigned long used = ctx->lo & 0x3f;

		ctx->buffer[used++] = 0x80;

		unsigned long available = chunkSize - used;

		if ( available < 8 ) {
			memset( &ctx->buffer[used], 0, available );
			MD5_Body( ctx, ctx->buffer, chunkSize );
			used = 0;
			available = chunkSize;
		}

		memset( &ctx->buffer[used], 0, available - 8 );

		ctx->lo <<= 3;
		ctx->buffer[56] = ctx->lo;
		ctx->buffer[57] = ctx->lo >> 8;
		ctx->buffer[58] = ctx->lo >> 16;
		ctx->buffer[59] = ctx->lo >> 24;
		ctx->buffer[60] = ctx->hi;
		ctx->buffer[61] = ctx->hi >> 8;
		ctx->buffer[62] = ctx->hi >> 16;
		ctx->buffer[63] = ctx->hi >> 24;

		MD5_Body( ctx, ctx->buffer, chunkSize );

		result[0] = ctx->a;
		result[1] = ctx->a >> 8;
		result[2] = ctx->a >> 16;
		result[3] = ctx->a >> 24;
		result[4] = ctx->b;
		result[5] = ctx->b >> 8;
		result[6] = ctx->b >> 16;
		result[7] = ctx->b >> 24;
		result[8] = ctx->c;
		result[9] = ctx->c >> 8;
		result[10] = ctx->c >> 16;
		result[11] = ctx->c >> 24;
		result[12] = ctx->d;
		result[13] = ctx->d >> 8;
		result[14] = ctx->d >> 16;
		result[15] = ctx->d >> 24;

		memset( ctx, 0, sizeof(*ctx) );
	}

	void ChecksumMD5( const char *in, size_t inLen, char out[33] ) {
		MD5Context md5 = {};
		uint8_t digest[16] = {};

		MD5_Init( &md5 );
		MD5_Update( &md5, (uint8_t *)in, inLen );
		MD5_Final( &md5, digest );

		out[0] = '\0';
		for ( uint32_t i = 0; i < 16u; i++ ) {
			Q_strcat( out, 33, va( "%02X", digest[i] ) );
		}
	}

} // namespace Crypto
