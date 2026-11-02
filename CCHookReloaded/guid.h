#pragma once
#include <cstdint>
#include <cstring>

namespace guid
{
	namespace detail
	{
		struct MD5Context
		{
			uint32_t buf[4];
			uint32_t bits[2];
			unsigned char in[64];
		};

#define GUID_F1(x, y, z) (z ^ (x & (y ^ z)))
#define GUID_F2(x, y, z) GUID_F1(z, x, y)
#define GUID_F3(x, y, z) (x ^ y ^ z)
#define GUID_F4(x, y, z) (y ^ (x | ~z))

#define GUID_MD5STEP(f, w, x, y, z, data, s) \
		(w += f(x, y, z) + data, w = w << s | w >> (32 - s), w += x)

		// Assumes little-endian
		static void MD5Transform(uint32_t buf[4], const uint32_t in[16])
		{
			uint32_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];

			GUID_MD5STEP(GUID_F1, a, b, c, d, in[0]  + 0xd76aa478,  7);
			GUID_MD5STEP(GUID_F1, d, a, b, c, in[1]  + 0xe8c7b756, 12);
			GUID_MD5STEP(GUID_F1, c, d, a, b, in[2]  + 0x242070db, 17);
			GUID_MD5STEP(GUID_F1, b, c, d, a, in[3]  + 0xc1bdceee, 22);
			GUID_MD5STEP(GUID_F1, a, b, c, d, in[4]  + 0xf57c0faf,  7);
			GUID_MD5STEP(GUID_F1, d, a, b, c, in[5]  + 0x4787c62a, 12);
			GUID_MD5STEP(GUID_F1, c, d, a, b, in[6]  + 0xa8304613, 17);
			GUID_MD5STEP(GUID_F1, b, c, d, a, in[7]  + 0xfd469501, 22);
			GUID_MD5STEP(GUID_F1, a, b, c, d, in[8]  + 0x698098d8,  7);
			GUID_MD5STEP(GUID_F1, d, a, b, c, in[9]  + 0x8b44f7af, 12);
			GUID_MD5STEP(GUID_F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
			GUID_MD5STEP(GUID_F1, b, c, d, a, in[11] + 0x895cd7be, 22);
			GUID_MD5STEP(GUID_F1, a, b, c, d, in[12] + 0x6b901122,  7);
			GUID_MD5STEP(GUID_F1, d, a, b, c, in[13] + 0xfd987193, 12);
			GUID_MD5STEP(GUID_F1, c, d, a, b, in[14] + 0xa679438e, 17);
			GUID_MD5STEP(GUID_F1, b, c, d, a, in[15] + 0x49b40821, 22);

			GUID_MD5STEP(GUID_F2, a, b, c, d, in[1]  + 0xf61e2562,  5);
			GUID_MD5STEP(GUID_F2, d, a, b, c, in[6]  + 0xc040b340,  9);
			GUID_MD5STEP(GUID_F2, c, d, a, b, in[11] + 0x265e5a51, 14);
			GUID_MD5STEP(GUID_F2, b, c, d, a, in[0]  + 0xe9b6c7aa, 20);
			GUID_MD5STEP(GUID_F2, a, b, c, d, in[5]  + 0xd62f105d,  5);
			GUID_MD5STEP(GUID_F2, d, a, b, c, in[10] + 0x02441453,  9);
			GUID_MD5STEP(GUID_F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
			GUID_MD5STEP(GUID_F2, b, c, d, a, in[4]  + 0xe7d3fbc8, 20);
			GUID_MD5STEP(GUID_F2, a, b, c, d, in[9]  + 0x21e1cde6,  5);
			GUID_MD5STEP(GUID_F2, d, a, b, c, in[14] + 0xc33707d6,  9);
			GUID_MD5STEP(GUID_F2, c, d, a, b, in[3]  + 0xf4d50d87, 14);
			GUID_MD5STEP(GUID_F2, b, c, d, a, in[8]  + 0x455a14ed, 20);
			GUID_MD5STEP(GUID_F2, a, b, c, d, in[13] + 0xa9e3e905,  5);
			GUID_MD5STEP(GUID_F2, d, a, b, c, in[2]  + 0xfcefa3f8,  9);
			GUID_MD5STEP(GUID_F2, c, d, a, b, in[7]  + 0x676f02d9, 14);
			GUID_MD5STEP(GUID_F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

			GUID_MD5STEP(GUID_F3, a, b, c, d, in[5]  + 0xfffa3942,  4);
			GUID_MD5STEP(GUID_F3, d, a, b, c, in[8]  + 0x8771f681, 11);
			GUID_MD5STEP(GUID_F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
			GUID_MD5STEP(GUID_F3, b, c, d, a, in[14] + 0xfde5380c, 23);
			GUID_MD5STEP(GUID_F3, a, b, c, d, in[1]  + 0xa4beea44,  4);
			GUID_MD5STEP(GUID_F3, d, a, b, c, in[4]  + 0x4bdecfa9, 11);
			GUID_MD5STEP(GUID_F3, c, d, a, b, in[7]  + 0xf6bb4b60, 16);
			GUID_MD5STEP(GUID_F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
			GUID_MD5STEP(GUID_F3, a, b, c, d, in[13] + 0x289b7ec6,  4);
			GUID_MD5STEP(GUID_F3, d, a, b, c, in[0]  + 0xeaa127fa, 11);
			GUID_MD5STEP(GUID_F3, c, d, a, b, in[3]  + 0xd4ef3085, 16);
			GUID_MD5STEP(GUID_F3, b, c, d, a, in[6]  + 0x04881d05, 23);
			GUID_MD5STEP(GUID_F3, a, b, c, d, in[9]  + 0xd9d4d039,  4);
			GUID_MD5STEP(GUID_F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
			GUID_MD5STEP(GUID_F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
			GUID_MD5STEP(GUID_F3, b, c, d, a, in[2]  + 0xc4ac5665, 23);

			GUID_MD5STEP(GUID_F4, a, b, c, d, in[0]  + 0xf4292244,  6);
			GUID_MD5STEP(GUID_F4, d, a, b, c, in[7]  + 0x432aff97, 10);
			GUID_MD5STEP(GUID_F4, c, d, a, b, in[14] + 0xab9423a7, 15);
			GUID_MD5STEP(GUID_F4, b, c, d, a, in[5]  + 0xfc93a039, 21);
			GUID_MD5STEP(GUID_F4, a, b, c, d, in[12] + 0x655b59c3,  6);
			GUID_MD5STEP(GUID_F4, d, a, b, c, in[3]  + 0x8f0ccc92, 10);
			GUID_MD5STEP(GUID_F4, c, d, a, b, in[10] + 0xffeff47d, 15);
			GUID_MD5STEP(GUID_F4, b, c, d, a, in[1]  + 0x85845dd1, 21);
			GUID_MD5STEP(GUID_F4, a, b, c, d, in[8]  + 0x6fa87e4f,  6);
			GUID_MD5STEP(GUID_F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
			GUID_MD5STEP(GUID_F4, c, d, a, b, in[6]  + 0xa3014314, 15);
			GUID_MD5STEP(GUID_F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
			GUID_MD5STEP(GUID_F4, a, b, c, d, in[4]  + 0xf7537e82,  6);
			GUID_MD5STEP(GUID_F4, d, a, b, c, in[11] + 0xbd3af235, 10);
			GUID_MD5STEP(GUID_F4, c, d, a, b, in[2]  + 0x2ad7d2bb, 15);
			GUID_MD5STEP(GUID_F4, b, c, d, a, in[9]  + 0xeb86d391, 21);

			buf[0] += a; buf[1] += b; buf[2] += c; buf[3] += d;
		}

#undef GUID_F1
#undef GUID_F2
#undef GUID_F3
#undef GUID_F4
#undef GUID_MD5STEP

		static void MD5Update(MD5Context *ctx, const unsigned char *buf, unsigned len)
		{
			uint32_t t = ctx->bits[0];
			if ((ctx->bits[0] = t + (static_cast<uint32_t>(len) << 3)) < t)
				ctx->bits[1]++;
			ctx->bits[1] += len >> 29;

			t = (t >> 3) & 0x3f;
			if (t)
			{
				unsigned char *p = ctx->in + t;
				t = 64 - t;
				if (len < t)
				{
					memcpy(p, buf, len);
					return;
				}
				memcpy(p, buf, t);
				MD5Transform(ctx->buf, reinterpret_cast<const uint32_t *>(ctx->in));
				buf += t;
				len -= t;
			}
			while (len >= 64)
			{
				memcpy(ctx->in, buf, 64);
				MD5Transform(ctx->buf, reinterpret_cast<const uint32_t *>(ctx->in));
				buf += 64;
				len -= 64;
			}
			memcpy(ctx->in, buf, len);
		}

		static void MD5Final(MD5Context *ctx, unsigned char digest[16])
		{
			unsigned count = (ctx->bits[0] >> 3) & 0x3f;
			unsigned char *p = ctx->in + count;
			*p++ = 0x80;
			count = 64 - 1 - count;
			if (count < 8)
			{
				memset(p, 0, count);
				MD5Transform(ctx->buf, reinterpret_cast<const uint32_t *>(ctx->in));
				memset(ctx->in, 0, 56);
			}
			else
			{
				memset(p, 0, count - 8);
			}
			reinterpret_cast<uint32_t *>(ctx->in)[14] = ctx->bits[0];
			reinterpret_cast<uint32_t *>(ctx->in)[15] = ctx->bits[1];
			MD5Transform(ctx->buf, reinterpret_cast<const uint32_t *>(ctx->in));
			memcpy(digest, ctx->buf, 16);
			SecureZeroMemory(ctx, sizeof(*ctx));
		}

		static void SeededMD5(const char *key, uint32_t seed, char out[33])
		{
			static constexpr char hex[] = "0123456789abcdef";
			unsigned char digest[16];

			MD5Context ctx;
			ctx.bits[0] = ctx.bits[1] = 0;
			ctx.buf[0] = 0x67452301 + seed * 11;
			ctx.buf[1] = 0xefcdab89 + seed * 71;
			ctx.buf[2] = 0x98badcfe + seed * 37;
			ctx.buf[3] = 0x10325476 + seed * 97;

			MD5Update(&ctx, reinterpret_cast<const unsigned char *>(key), static_cast<unsigned>(strlen(key)));
			MD5Final(&ctx, digest);

			for (int i = 0; i < 16; i++)
			{
				out[i * 2]     = hex[digest[i] >> 4];
				out[i * 2 + 1] = hex[digest[i] & 0x0f];
			}
			out[32] = '\0';
		}
	}

	// Etkey needs 28 bytes, outGuid needs 33
	static void Calculate(const char *etKey, char outGuid[33])
	{
		char key[19] = {};
		memcpy(key, etKey + 10, 18);

		char firstHash[33];
		detail::SeededMD5(key, 0x00B684A3, firstHash);
		// Hash the lowercase hex result again
		detail::SeededMD5(firstHash, 0x00051A56, outGuid);

		for (int i = 0; outGuid[i]; i++)
			outGuid[i] = static_cast<char>(toupper(static_cast<unsigned char>(outGuid[i])));

		SecureZeroMemory(key, sizeof(key));
		SecureZeroMemory(firstHash, sizeof(firstHash));
	}
}
