// File: bc7enc16.c - Richard Geldreich, Jr. 4/2018 - MIT license or public domain (see end of file)
#include "bc7enc16.h"
#include <math.h>
#include <memory.h>
#include <assert.h>
#include <limits.h>

// Helpers
static inline int32_t clampi(int32_t value, int32_t low, int32_t high) { if (value < low) value = low; else if (value > high) value = high;	return value; }
static inline float clampf(float value, float low, float high) { if (value < low) value = low; else if (value > high) value = high;	return value; }
static inline float saturate(float value) { return clampf(value, 0, 1.0f); }
static inline uint8_t minimumub(uint8_t a, uint8_t b) { return (a < b) ? a : b; }
static inline uint32_t minimumu(uint32_t a, uint32_t b) { return (a < b) ? a : b; }
static inline float minimumf(float a, float b) { return (a < b) ? a : b; }
static inline uint8_t maximumub(uint8_t a, uint8_t b) { return (a > b) ? a : b; }
static inline uint32_t maximumu(uint32_t a, uint32_t b) { return (a > b) ? a : b; }
static inline float maximumf(float a, float b) { return (a > b) ? a : b; }
static inline int squarei(int i) { return i * i; }
static inline float squaref(float i) { return i * i; }

typedef struct { uint8_t m_c[4]; } color_quad_u8;
typedef struct { float m_c[4]; } vec4F;

static inline color_quad_u8 *color_quad_u8_set_clamped(color_quad_u8 *pRes, int32_t r, int32_t g, int32_t b, int32_t a) { pRes->m_c[0] = (uint8_t)clampi(r, 0, 255); pRes->m_c[1] = (uint8_t)clampi(g, 0, 255); pRes->m_c[2] = (uint8_t)clampi(b, 0, 255); pRes->m_c[3] = (uint8_t)clampi(a, 0, 255); return pRes; }
static inline color_quad_u8 *color_quad_u8_set(color_quad_u8 *pRes, int32_t r, int32_t g, int32_t b, int32_t a) { assert((uint32_t)(r | g | b | a) <= 255); pRes->m_c[0] = (uint8_t)r; pRes->m_c[1] = (uint8_t)g; pRes->m_c[2] = (uint8_t)b; pRes->m_c[3] = (uint8_t)a; return pRes; }
static inline bc7enc16_bool color_quad_u8_notequals(const color_quad_u8 *pLHS, const color_quad_u8 *pRHS) { return (pLHS->m_c[0] != pRHS->m_c[0]) || (pLHS->m_c[1] != pRHS->m_c[1]) || (pLHS->m_c[2] != pRHS->m_c[2]) || (pLHS->m_c[3] != pRHS->m_c[3]); }
static inline vec4F *vec4F_set_scalar(vec4F *pV, float x) {	pV->m_c[0] = x; pV->m_c[1] = x; pV->m_c[2] = x;	pV->m_c[3] = x;	return pV; }
static inline vec4F *vec4F_set(vec4F *pV, float x, float y, float z, float w) {	pV->m_c[0] = x;	pV->m_c[1] = y;	pV->m_c[2] = z;	pV->m_c[3] = w;	return pV; }
static inline vec4F *vec4F_saturate_in_place(vec4F *pV) { pV->m_c[0] = saturate(pV->m_c[0]); pV->m_c[1] = saturate(pV->m_c[1]); pV->m_c[2] = saturate(pV->m_c[2]); pV->m_c[3] = saturate(pV->m_c[3]); return pV; }
static inline vec4F vec4F_saturate(const vec4F *pV) { vec4F res; res.m_c[0] = saturate(pV->m_c[0]); res.m_c[1] = saturate(pV->m_c[1]); res.m_c[2] = saturate(pV->m_c[2]); res.m_c[3] = saturate(pV->m_c[3]); return res; }
static inline vec4F vec4F_from_color(const color_quad_u8 *pC) { vec4F res; vec4F_set(&res, pC->m_c[0], pC->m_c[1], pC->m_c[2], pC->m_c[3]); return res; }
static inline vec4F vec4F_add(const vec4F *pLHS, const vec4F *pRHS) { vec4F res; vec4F_set(&res, pLHS->m_c[0] + pRHS->m_c[0], pLHS->m_c[1] + pRHS->m_c[1], pLHS->m_c[2] + pRHS->m_c[2], pLHS->m_c[3] + pRHS->m_c[3]); return res; }
static inline vec4F vec4F_sub(const vec4F *pLHS, const vec4F *pRHS) { vec4F res; vec4F_set(&res, pLHS->m_c[0] - pRHS->m_c[0], pLHS->m_c[1] - pRHS->m_c[1], pLHS->m_c[2] - pRHS->m_c[2], pLHS->m_c[3] - pRHS->m_c[3]); return res; }
static inline float vec4F_dot(const vec4F *pLHS, const vec4F *pRHS) { return pLHS->m_c[0] * pRHS->m_c[0] + pLHS->m_c[1] * pRHS->m_c[1] + pLHS->m_c[2] * pRHS->m_c[2] + pLHS->m_c[3] * pRHS->m_c[3]; }
static inline vec4F vec4F_mul(const vec4F *pLHS, float s) { vec4F res; vec4F_set(&res, pLHS->m_c[0] * s, pLHS->m_c[1] * s, pLHS->m_c[2] * s, pLHS->m_c[3] * s); return res; }
static inline vec4F *vec4F_normalize_in_place(vec4F *pV) { float s = pV->m_c[0] * pV->m_c[0] + pV->m_c[1] * pV->m_c[1] + pV->m_c[2] * pV->m_c[2] + pV->m_c[3] * pV->m_c[3]; if (s != 0.0f) { s = 1.0f / sqrtf(s); pV->m_c[0] *= s; pV->m_c[1] *= s; pV->m_c[2] *= s; pV->m_c[3] *= s; } return pV; }

// Various BC7 tables
static const uint32_t g_bc7_weights3[8] = { 0, 9, 18, 27, 37, 46, 55, 64 };
static const uint32_t g_bc7_weights4[16] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };
// Precomputed weight constants used during least fit determination. For each entry in g_bc7_weights[]: w * w, (1.0f - w) * w, (1.0f - w) * (1.0f - w), w
static const float g_bc7_weights3x[8 * 4] = { 0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.019775f, 0.120850f, 0.738525f, 0.140625f, 0.079102f, 0.202148f, 0.516602f, 0.281250f, 0.177979f, 0.243896f, 0.334229f, 0.421875f, 0.334229f, 0.243896f, 0.177979f, 0.578125f, 0.516602f, 0.202148f,
	0.079102f, 0.718750f, 0.738525f, 0.120850f, 0.019775f, 0.859375f, 1.000000f, 0.000000f, 0.000000f, 1.000000f };
static const float g_bc7_weights4x[16 * 4] = { 0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.003906f, 0.058594f, 0.878906f, 0.062500f, 0.019775f, 0.120850f, 0.738525f, 0.140625f, 0.041260f, 0.161865f, 0.635010f, 0.203125f, 0.070557f, 0.195068f, 0.539307f, 0.265625f, 0.107666f, 0.220459f,
	0.451416f, 0.328125f, 0.165039f, 0.241211f, 0.352539f, 0.406250f, 0.219727f, 0.249023f, 0.282227f, 0.468750f, 0.282227f, 0.249023f, 0.219727f, 0.531250f, 0.352539f, 0.241211f, 0.165039f, 0.593750f, 0.451416f, 0.220459f, 0.107666f, 0.671875f, 0.539307f, 0.195068f, 0.070557f, 0.734375f,
	0.635010f, 0.161865f, 0.041260f, 0.796875f, 0.738525f, 0.120850f, 0.019775f, 0.859375f, 0.878906f, 0.058594f, 0.003906f, 0.937500f, 1.000000f, 0.000000f, 0.000000f, 1.000000f };
static const uint8_t g_bc7_partition1[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static const uint8_t g_bc7_partition2[64 * 16] =
{
	0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,		0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,		0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,		0,0,0,1,0,0,1,1,0,0,1,1,0,1,1,1,		0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1,		0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,		0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,		0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,		0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,		0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,		0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,		0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,		0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,		0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,		0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,
	0,0,0,0,1,0,0,0,1,1,1,0,1,1,1,1,		0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,		0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,		0,1,1,1,0,0,1,1,0,0,0,1,0,0,0,0,		0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,		0,0,0,0,1,0,0,0,1,1,0,0,1,1,1,0,		0,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0,		0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,
	0,0,1,1,0,0,0,1,0,0,0,1,0,0,0,0,		0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,		0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,		0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,		0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0,		0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,		0,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0,		0,0,1,1,1,0,0,1,1,0,0,1,1,1,0,0,
	0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,		0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,		0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,		0,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0,		0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,		0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,		0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,		0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,
	0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,0,		0,0,0,1,0,0,1,1,1,1,0,0,1,0,0,0,		0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,		0,0,1,1,1,0,1,1,1,1,0,1,1,1,0,0,		0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,		0,0,1,1,1,1,0,0,1,1,0,0,0,0,1,1,		0,1,1,0,0,1,1,0,1,0,0,1,1,0,0,1,		0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,
	0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,0,		0,0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,		0,0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,		0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,0,		0,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1,		0,0,1,1,0,1,1,0,1,1,0,0,1,0,0,1,		0,1,1,0,0,0,1,1,1,0,0,1,1,1,0,0,		0,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0,
	0,1,1,0,1,1,0,0,1,1,0,0,1,0,0,1,		0,1,1,0,0,0,1,1,0,0,1,1,1,0,0,1,		0,1,1,1,1,1,1,0,1,0,0,0,0,0,0,1,		0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1,		0,0,0,0,1,1,1,1,0,0,1,1,0,0,1,1,		0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,		0,0,1,0,0,0,1,0,1,1,1,0,1,1,1,0,		0,1,0,0,0,1,0,0,0,1,1,1,0,1,1,1
};
static const uint8_t g_bc7_table_anchor_index_second_subset[64] = {	15,15,15,15,15,15,15,15,		15,15,15,15,15,15,15,15,		15, 2, 8, 2, 2, 8, 8,15,		2, 8, 2, 2, 8, 8, 2, 2,		15,15, 6, 8, 2, 8,15,15,		2, 8, 2, 2, 2,15,15, 6,		6, 2, 6, 8,15,15, 2, 2,		15,15,15,15,15, 2, 2,15 };
static const uint8_t g_bc7_num_subsets[8] = { 3, 2, 3, 2, 1, 1, 1, 2 };
static const uint8_t g_bc7_partition_bits[8] = { 4, 6, 6, 6, 0, 0, 0, 6 };
static const uint8_t g_bc7_color_index_bitcount[8] = { 3, 3, 2, 2, 2, 2, 4, 2 };
static int get_bc7_color_index_size(int mode, int index_selection_bit) { return g_bc7_color_index_bitcount[mode] + index_selection_bit; }
static const uint8_t g_bc7_mode_has_p_bits[8] = { 1, 1, 0, 1, 0, 0, 1, 1 };
static const uint8_t g_bc7_mode_has_shared_p_bits[8] = { 0, 1, 0, 0, 0, 0, 0, 0 };
static const uint8_t g_bc7_color_precision_table[8] = { 4, 6, 5, 7, 5, 7, 7, 5 };
static const int8_t g_bc7_alpha_precision_table[8] = { 0, 0, 0, 0, 6, 8, 7, 5 };

typedef struct { uint16_t m_error; uint8_t m_lo; uint8_t m_hi; } endpoint_err;

static endpoint_err g_bc7_mode_1_optimal_endpoints[256][2]; // [c][pbit]
static const uint32_t BC7ENC16_MODE_1_OPTIMAL_INDEX = 2;

// Initialize the lookup table used for optimal single color compression in mode 1. Must be called before encoding.
void bc7enc16_compress_block_init()
{
	for (int c = 0; c < 256; c++)
	{
		for (uint32_t lp = 0; lp < 2; lp++)
		{
			endpoint_err best;
			best.m_error = (uint16_t)UINT16_MAX;
			for (uint32_t l = 0; l < 64; l++)
			{
				uint32_t low = ((l << 1) | lp) << 1;
				low |= (low >> 7);
				for (uint32_t h = 0; h < 64; h++)
				{
					uint32_t high = ((h << 1) | lp) << 1;
					high |= (high >> 7);
					const int k = (low * (64 - g_bc7_weights3[BC7ENC16_MODE_1_OPTIMAL_INDEX]) + high * g_bc7_weights3[BC7ENC16_MODE_1_OPTIMAL_INDEX] + 32) >> 6;
					const int err = (k - c) * (k - c);
					if (err < best.m_error)
					{
						best.m_error = (uint16_t)err;
						best.m_lo = (uint8_t)l;
						best.m_hi = (uint8_t)h;
					}
				} // h
			} // l
			g_bc7_mode_1_optimal_endpoints[c][lp] = best;
		} // lp
	} // c
}

static void compute_least_squares_endpoints_rgba(uint32_t N, const uint8_t *pSelectors, const vec4F *pSelector_weights, vec4F *pXl, vec4F *pXh, const color_quad_u8 *pColors)
{
	// Least squares using normal equations: http://www.cs.cornell.edu/~bindel/class/cs3220-s12/notes/lec10.pdf
	// I did this in matrix form first, expanded out all the ops, then optimized it a bit.
	float z00 = 0.0f, z01 = 0.0f, z10 = 0.0f, z11 = 0.0f;
	float q00_r = 0.0f, q10_r = 0.0f, t_r = 0.0f;
	float q00_g = 0.0f, q10_g = 0.0f, t_g = 0.0f;
	float q00_b = 0.0f, q10_b = 0.0f, t_b = 0.0f;
	float q00_a = 0.0f, q10_a = 0.0f, t_a = 0.0f;
	for (uint32_t i = 0; i < N; i++)
	{
		const uint32_t sel = pSelectors[i];
		z00 += pSelector_weights[sel].m_c[0];
		z10 += pSelector_weights[sel].m_c[1];
		z11 += pSelector_weights[sel].m_c[2];
		float w = pSelector_weights[sel].m_c[3];
		q00_r += w * pColors[i].m_c[0]; t_r += pColors[i].m_c[0];
		q00_g += w * pColors[i].m_c[1]; t_g += pColors[i].m_c[1];
		q00_b += w * pColors[i].m_c[2]; t_b += pColors[i].m_c[2];
		q00_a += w * pColors[i].m_c[3]; t_a += pColors[i].m_c[3];
	}

	q10_r = t_r - q00_r;
	q10_g = t_g - q00_g;
	q10_b = t_b - q00_b;
	q10_a = t_a - q00_a;

	z01 = z10;

	float det = z00 * z11 - z01 * z10;
	if (det != 0.0f)
		det = 1.0f / det;

	float iz00, iz01, iz10, iz11;
	iz00 = z11 * det;
	iz01 = -z01 * det;
	iz10 = -z10 * det;
	iz11 = z00 * det;

	pXl->m_c[0] = (float)(iz00 * q00_r + iz01 * q10_r); pXh->m_c[0] = (float)(iz10 * q00_r + iz11 * q10_r);
	pXl->m_c[1] = (float)(iz00 * q00_g + iz01 * q10_g); pXh->m_c[1] = (float)(iz10 * q00_g + iz11 * q10_g);
	pXl->m_c[2] = (float)(iz00 * q00_b + iz01 * q10_b); pXh->m_c[2] = (float)(iz10 * q00_b + iz11 * q10_b);
	pXl->m_c[3] = (float)(iz00 * q00_a + iz01 * q10_a); pXh->m_c[3] = (float)(iz10 * q00_a + iz11 * q10_a);
}

static void compute_least_squares_endpoints_rgb(uint32_t N, const uint8_t *pSelectors, const vec4F *pSelector_weights, vec4F *pXl, vec4F *pXh, const color_quad_u8 *pColors)
{
	float z00 = 0.0f, z01 = 0.0f, z10 = 0.0f, z11 = 0.0f;
	float q00_r = 0.0f, q10_r = 0.0f, t_r = 0.0f;
	float q00_g = 0.0f, q10_g = 0.0f, t_g = 0.0f;
	float q00_b = 0.0f, q10_b = 0.0f, t_b = 0.0f;
	for (uint32_t i = 0; i < N; i++)
	{
		const uint32_t sel = pSelectors[i];
		z00 += pSelector_weights[sel].m_c[0];
		z10 += pSelector_weights[sel].m_c[1];
		z11 += pSelector_weights[sel].m_c[2];
		float w = pSelector_weights[sel].m_c[3];
		q00_r += w * pColors[i].m_c[0]; t_r += pColors[i].m_c[0];
		q00_g += w * pColors[i].m_c[1]; t_g += pColors[i].m_c[1];
		q00_b += w * pColors[i].m_c[2]; t_b += pColors[i].m_c[2];
	}

	q10_r = t_r - q00_r;
	q10_g = t_g - q00_g;
	q10_b = t_b - q00_b;

	z01 = z10;

	float det = z00 * z11 - z01 * z10;
	if (det != 0.0f)
		det = 1.0f / det;

	float iz00, iz01, iz10, iz11;
	iz00 = z11 * det;
	iz01 = -z01 * det;
	iz10 = -z10 * det;
	iz11 = z00 * det;

	pXl->m_c[0] = (float)(iz00 * q00_r + iz01 * q10_r); pXh->m_c[0] = (float)(iz10 * q00_r + iz11 * q10_r);
	pXl->m_c[1] = (float)(iz00 * q00_g + iz01 * q10_g); pXh->m_c[1] = (float)(iz10 * q00_g + iz11 * q10_g);
	pXl->m_c[2] = (float)(iz00 * q00_b + iz01 * q10_b); pXh->m_c[2] = (float)(iz10 * q00_b + iz11 * q10_b);
	pXl->m_c[3] = 255.0f; pXh->m_c[3] = 255.0f;
}

typedef struct
{
	uint32_t m_num_pixels;
	const color_quad_u8 *m_pPixels;
	uint32_t m_num_selector_weights;
	const uint32_t *m_pSelector_weights;
	const vec4F *m_pSelector_weightsx;
	uint32_t m_comp_bits;
	uint32_t m_weights[4];
	bc7enc16_bool m_has_alpha;
	bc7enc16_bool m_has_pbits;
	bc7enc16_bool m_endpoints_share_pbit;
	bc7enc16_bool m_perceptual;
} color_cell_compressor_params;

typedef struct
{
	uint64_t m_best_overall_err;
	color_quad_u8 m_low_endpoint;
	color_quad_u8 m_high_endpoint;
	uint32_t m_pbits[2];
	uint8_t *m_pSelectors;
	uint8_t *m_pSelectors_temp;
} color_cell_compressor_results;

static inline color_quad_u8 scale_color(const color_quad_u8 *pC, const color_cell_compressor_params *pParams)
{
	color_quad_u8 results;

	const uint32_t n = pParams->m_comp_bits + (pParams->m_has_pbits ? 1 : 0);
	assert((n >= 4) && (n <= 8));

	for (uint32_t i = 0; i < 4; i++)
	{
		uint32_t v = pC->m_c[i] << (8 - n);
		v |= (v >> n);
		assert(v <= 255);
		results.m_c[i] = (uint8_t)(v);
	}

	return results;
}

static inline uint64_t compute_color_distance_rgb(const color_quad_u8 *pE1, const color_quad_u8 *pE2, bc7enc16_bool perceptual, const uint32_t weights[4])
{
	int dr, dg, db;

	if (perceptual)
	{
		const int l1 = pE1->m_c[0] * 109 + pE1->m_c[1] * 366 + pE1->m_c[2] * 37;
		const int cr1 = ((int)pE1->m_c[0] << 9) - l1;
		const int cb1 = ((int)pE1->m_c[2] << 9) - l1;
		const int l2 = pE2->m_c[0] * 109 + pE2->m_c[1] * 366 + pE2->m_c[2] * 37;
		const int cr2 = ((int)pE2->m_c[0] << 9) - l2;
		const int cb2 = ((int)pE2->m_c[2] << 9) - l2;
		dr = (l1 - l2) >> 8;
		dg = (cr1 - cr2) >> 8;
		db = (cb1 - cb2) >> 8;
	}
	else
	{
		dr = (int)pE1->m_c[0] - (int)pE2->m_c[0];
		dg = (int)pE1->m_c[1] - (int)pE2->m_c[1];
		db = (int)pE1->m_c[2] - (int)pE2->m_c[2];
	}

	return weights[0] * (uint32_t)(dr * dr) + weights[1] * (uint32_t)(dg * dg) + weights[2] * (uint32_t)(db * db);
}

static inline uint64_t compute_color_distance_rgba(const color_quad_u8 *pE1, const color_quad_u8 *pE2, bc7enc16_bool perceptual, const uint32_t weights[4])
{
	int da = (int)pE1->m_c[3] - (int)pE2->m_c[3];
	return compute_color_distance_rgb(pE1, pE2, perceptual, weights) + (weights[3] * (uint32_t)(da * da));
}

static uint64_t pack_mode1_to_one_color(const color_cell_compressor_params *pParams, color_cell_compressor_results *pResults, uint32_t r, uint32_t g, uint32_t b, uint8_t *pSelectors)
{
	uint32_t best_err = UINT_MAX;
	uint32_t best_p = 0;

	for (uint32_t p = 0; p < 2; p++)
	{
		uint32_t err = g_bc7_mode_1_optimal_endpoints[r][p].m_error + g_bc7_mode_1_optimal_endpoints[g][p].m_error + g_bc7_mode_1_optimal_endpoints[b][p].m_error;
		if (err < best_err)
		{
			best_err = err;
			best_p = p;
		}
	}

	const endpoint_err *pEr = &g_bc7_mode_1_optimal_endpoints[r][best_p];
	const endpoint_err *pEg = &g_bc7_mode_1_optimal_endpoints[g][best_p];
	const endpoint_err *pEb = &g_bc7_mode_1_optimal_endpoints[b][best_p];

	color_quad_u8_set(&pResults->m_low_endpoint, pEr->m_lo, pEg->m_lo, pEb->m_lo, 0);
	color_quad_u8_set(&pResults->m_high_endpoint, pEr->m_hi, pEg->m_hi, pEb->m_hi, 0);
	pResults->m_pbits[0] = best_p;
	pResults->m_pbits[1] = 0;

	memset(pSelectors, BC7ENC16_MODE_1_OPTIMAL_INDEX, pParams->m_num_pixels);

	color_quad_u8 p;
	for (uint32_t i = 0; i < 3; i++)
	{
		uint32_t low = ((pResults->m_low_endpoint.m_c[i] << 1) | pResults->m_pbits[0]) << 1;
		low |= (low >> 7);

		uint32_t high = ((pResults->m_high_endpoint.m_c[i] << 1) | pResults->m_pbits[0]) << 1;
		high |= (high >> 7);

		p.m_c[i] = (uint8_t)((low * (64 - g_bc7_weights3[BC7ENC16_MODE_1_OPTIMAL_INDEX]) + high * g_bc7_weights3[BC7ENC16_MODE_1_OPTIMAL_INDEX] + 32) >> 6);
	}
	p.m_c[3] = 255;

	uint64_t total_err = 0;
	for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		total_err += compute_color_distance_rgb(&p, &pParams->m_pPixels[i], pParams->m_perceptual, pParams->m_weights);

	pResults->m_best_overall_err = total_err;

	return total_err;
}

static uint64_t evaluate_solution(const color_quad_u8 *pLow, const color_quad_u8 *pHigh, const uint32_t pbits[2], const color_cell_compressor_params *pParams, color_cell_compressor_results *pResults)
{
	color_quad_u8 quantMinColor = *pLow;
	color_quad_u8 quantMaxColor = *pHigh;

	if (pParams->m_has_pbits)
	{
		uint32_t minPBit, maxPBit;

		if (pParams->m_endpoints_share_pbit)
			maxPBit = minPBit = pbits[0];
		else
		{
			minPBit = pbits[0];
			maxPBit = pbits[1];
		}

		quantMinColor.m_c[0] = (uint8_t)((pLow->m_c[0] << 1) | minPBit);
		quantMinColor.m_c[1] = (uint8_t)((pLow->m_c[1] << 1) | minPBit);
		quantMinColor.m_c[2] = (uint8_t)((pLow->m_c[2] << 1) | minPBit);
		quantMinColor.m_c[3] = (uint8_t)((pLow->m_c[3] << 1) | minPBit);

		quantMaxColor.m_c[0] = (uint8_t)((pHigh->m_c[0] << 1) | maxPBit);
		quantMaxColor.m_c[1] = (uint8_t)((pHigh->m_c[1] << 1) | maxPBit);
		quantMaxColor.m_c[2] = (uint8_t)((pHigh->m_c[2] << 1) | maxPBit);
		quantMaxColor.m_c[3] = (uint8_t)((pHigh->m_c[3] << 1) | maxPBit);
	}

	color_quad_u8 actualMinColor = scale_color(&quantMinColor, pParams);
	color_quad_u8 actualMaxColor = scale_color(&quantMaxColor, pParams);

	const uint32_t N = pParams->m_num_selector_weights;

	color_quad_u8 weightedColors[16];
	weightedColors[0] = actualMinColor;
	weightedColors[N - 1] = actualMaxColor;

	const uint32_t nc = pParams->m_has_alpha ? 4 : 3;
	for (uint32_t i = 1; i < (N - 1); i++)
		for (uint32_t j = 0; j < nc; j++)
			weightedColors[i].m_c[j] = (uint8_t)((actualMinColor.m_c[j] * (64 - pParams->m_pSelector_weights[i]) + actualMaxColor.m_c[j] * pParams->m_pSelector_weights[i] + 32) >> 6);

	const int lr = actualMinColor.m_c[0];
	const int lg = actualMinColor.m_c[1];
	const int lb = actualMinColor.m_c[2];
	const int dr = actualMaxColor.m_c[0] - lr;
	const int dg = actualMaxColor.m_c[1] - lg;
	const int db = actualMaxColor.m_c[2] - lb;

	uint64_t total_err = 0;

	if (!pParams->m_perceptual)
	{
		if (pParams->m_has_alpha)
		{
			const int la = actualMinColor.m_c[3];
			const int da = actualMaxColor.m_c[3] - la;

			const float f = N / (float)(squarei(dr) + squarei(dg) + squarei(db) + squarei(da) + .00000125f);

			for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
			{
				const color_quad_u8 *pC = &pParams->m_pPixels[i];
				int r = pC->m_c[0];
				int g = pC->m_c[1];
				int b = pC->m_c[2];
				int a = pC->m_c[3];

				int best_sel = (int)((float)((r - lr) * dr + (g - lg) * dg + (b - lb) * db + (a - la) * da) * f + .5f);
				best_sel = clampi(best_sel, 1, N - 1);

				uint64_t err0 = compute_color_distance_rgba(&weightedColors[best_sel - 1], pC, BC7ENC16_FALSE, pParams->m_weights);
				uint64_t err1 = compute_color_distance_rgba(&weightedColors[best_sel], pC, BC7ENC16_FALSE, pParams->m_weights);

				if (err1 > err0)
				{
					err1 = err0;
					--best_sel;
				}
				total_err += err1;

				pResults->m_pSelectors_temp[i] = (uint8_t)best_sel;
			}
		}
		else
		{
			const float f = N / (float)(squarei(dr) + squarei(dg) + squarei(db) + .00000125f);

			for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
			{
				const color_quad_u8 *pC = &pParams->m_pPixels[i];
				int r = pC->m_c[0];
				int g = pC->m_c[1];
				int b = pC->m_c[2];

				int sel = (int)((float)((r - lr) * dr + (g - lg) * dg + (b - lb) * db) * f + .5f);
				sel = clampi(sel, 1, N - 1);

				uint64_t err0 = compute_color_distance_rgb(&weightedColors[sel - 1], pC, BC7ENC16_FALSE, pParams->m_weights);
				uint64_t err1 = compute_color_distance_rgb(&weightedColors[sel], pC, BC7ENC16_FALSE, pParams->m_weights);

				int best_sel = sel;
				uint64_t best_err = err1;
				if (err0 < best_err)
				{
					best_err = err0;
					best_sel = sel - 1;
				}

				total_err += best_err;

				pResults->m_pSelectors_temp[i] = (uint8_t)best_sel;
			}
		}
	}
	else
	{
		for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		{
			uint64_t best_err = UINT64_MAX;
			uint32_t best_sel = 0;

			if (pParams->m_has_alpha)
			{
				for (uint32_t j = 0; j < N; j++)
				{
					uint64_t err = compute_color_distance_rgba(&weightedColors[j], &pParams->m_pPixels[i], BC7ENC16_TRUE, pParams->m_weights);
					if (err < best_err)
					{
						best_err = err;
						best_sel = j;
					}
				}
			}
			else
			{
				for (uint32_t j = 0; j < N; j++)
				{
					uint64_t err = compute_color_distance_rgb(&weightedColors[j], &pParams->m_pPixels[i], BC7ENC16_TRUE, pParams->m_weights);
					if (err < best_err)
					{
						best_err = err;
						best_sel = j;
					}
				}
			}

			total_err += best_err;

			pResults->m_pSelectors_temp[i] = (uint8_t)best_sel;
		}
	}

	if (total_err < pResults->m_best_overall_err)
	{
		pResults->m_best_overall_err = total_err;

		pResults->m_low_endpoint = *pLow;
		pResults->m_high_endpoint = *pHigh;

		pResults->m_pbits[0] = pbits[0];
		pResults->m_pbits[1] = pbits[1];

		memcpy(pResults->m_pSelectors, pResults->m_pSelectors_temp, sizeof(pResults->m_pSelectors[0]) * pParams->m_num_pixels);
	}

	return total_err;
}

static void fixDegenerateEndpoints(uint32_t mode, color_quad_u8 *pTrialMinColor, color_quad_u8 *pTrialMaxColor, const vec4F *pXl, const vec4F *pXh, uint32_t iscale)
{
	if (mode == 1)
	{
		// fix degenerate case where the input collapses to a single colorspace voxel, and we loose all freedom (test with grayscale ramps)
		for (uint32_t i = 0; i < 3; i++)
		{
			if (pTrialMinColor->m_c[i] == pTrialMaxColor->m_c[i])
			{
				if (fabs(pXl->m_c[i] - pXh->m_c[i]) > 0.0f)
				{
					if (pTrialMinColor->m_c[i] > (iscale >> 1))
					{
						if (pTrialMinColor->m_c[i] > 0)
							pTrialMinColor->m_c[i]--;
						else
							if (pTrialMaxColor->m_c[i] < iscale)
								pTrialMaxColor->m_c[i]++;
					}
					else
					{
						if (pTrialMaxColor->m_c[i] < iscale)
							pTrialMaxColor->m_c[i]++;
						else if (pTrialMinColor->m_c[i] > 0)
							pTrialMinColor->m_c[i]--;
					}
				}
			}
		}
	}
}

static uint64_t find_optimal_solution(uint32_t mode, vec4F xl, vec4F xh, const color_cell_compressor_params *pParams, color_cell_compressor_results *pResults)
{
	vec4F_saturate_in_place(&xl); vec4F_saturate_in_place(&xh);

	if (pParams->m_has_pbits)
	{
		const int iscalep = (1 << (pParams->m_comp_bits + 1)) - 1;
		const float scalep = (float)iscalep;

		const int32_t totalComps = pParams->m_has_alpha ? 4 : 3;

		uint32_t best_pbits[2];
		color_quad_u8 bestMinColor, bestMaxColor;

		if (!pParams->m_endpoints_share_pbit)
		{
			float best_err0 = 1e+9;
			float best_err1 = 1e+9;

			for (int p = 0; p < 2; p++)
			{
				color_quad_u8 xMinColor, xMaxColor;

				// Notes: The pbit controls which quantization intervals are selected.
				// total_levels=2^(comp_bits+1), where comp_bits=4 for mode 0, etc.
				// pbit 0: v=(b*2)/(total_levels-1), pbit 1: v=(b*2+1)/(total_levels-1) where b is the component bin from [0,total_levels/2-1] and v is the [0,1] component value
				// rearranging you get for pbit 0: b=floor(v*(total_levels-1)/2+.5)
				// rearranging you get for pbit 1: b=floor((v*(total_levels-1)-1)/2+.5)
				for (uint32_t c = 0; c < 4; c++)
				{
					xMinColor.m_c[c] = (uint8_t)(clampi(((int)((xl.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p, p, iscalep - 1 + p));
					xMaxColor.m_c[c] = (uint8_t)(clampi(((int)((xh.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p, p, iscalep - 1 + p));
				}

				color_quad_u8 scaledLow = scale_color(&xMinColor, pParams);
				color_quad_u8 scaledHigh = scale_color(&xMaxColor, pParams);

				float err0 = 0, err1 = 0;
				for (int i = 0; i < totalComps; i++)
				{
					err0 += squaref(scaledLow.m_c[i] - xl.m_c[i] * 255.0f);
					err1 += squaref(scaledHigh.m_c[i] - xh.m_c[i] * 255.0f);
				}

				if (err0 < best_err0)
				{
					best_err0 = err0;
					best_pbits[0] = p;

					bestMinColor.m_c[0] = xMinColor.m_c[0] >> 1;
					bestMinColor.m_c[1] = xMinColor.m_c[1] >> 1;
					bestMinColor.m_c[2] = xMinColor.m_c[2] >> 1;
					bestMinColor.m_c[3] = xMinColor.m_c[3] >> 1;
				}

				if (err1 < best_err1)
				{
					best_err1 = err1;
					best_pbits[1] = p;

					bestMaxColor.m_c[0] = xMaxColor.m_c[0] >> 1;
					bestMaxColor.m_c[1] = xMaxColor.m_c[1] >> 1;
					bestMaxColor.m_c[2] = xMaxColor.m_c[2] >> 1;
					bestMaxColor.m_c[3] = xMaxColor.m_c[3] >> 1;
				}
			}
		}
		else
		{
			// Endpoints share pbits
			float best_err = 1e+9;

			for (int p = 0; p < 2; p++)
			{
				color_quad_u8 xMinColor, xMaxColor;
				for (uint32_t c = 0; c < 4; c++)
				{
					xMinColor.m_c[c] = (uint8_t)(clampi(((int)((xl.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p, p, iscalep - 1 + p));
					xMaxColor.m_c[c] = (uint8_t)(clampi(((int)((xh.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p, p, iscalep - 1 + p));
				}

				color_quad_u8 scaledLow = scale_color(&xMinColor, pParams);
				color_quad_u8 scaledHigh = scale_color(&xMaxColor, pParams);

				float err = 0;
				for (int i = 0; i < totalComps; i++)
					err += squaref((scaledLow.m_c[i] / 255.0f) - xl.m_c[i]) + squaref((scaledHigh.m_c[i] / 255.0f) - xh.m_c[i]);

				if (err < best_err)
				{
					best_err = err;
					best_pbits[0] = p;
					best_pbits[1] = p;
					for (uint32_t j = 0; j < 4; j++)
					{
						bestMinColor.m_c[j] = xMinColor.m_c[j] >> 1;
						bestMaxColor.m_c[j] = xMaxColor.m_c[j] >> 1;
					}
				}
			}
		}

		fixDegenerateEndpoints(mode, &bestMinColor, &bestMaxColor, &xl, &xh, iscalep >> 1);

		if ((pResults->m_best_overall_err == UINT64_MAX) || color_quad_u8_notequals(&bestMinColor, &pResults->m_low_endpoint) || color_quad_u8_notequals(&bestMaxColor, &pResults->m_high_endpoint) || (best_pbits[0] != pResults->m_pbits[0]) || (best_pbits[1] != pResults->m_pbits[1]))
			evaluate_solution(&bestMinColor, &bestMaxColor, best_pbits, pParams, pResults);
	}
	else
	{
		const int iscale = (1 << pParams->m_comp_bits) - 1;
		const float scale = (float)iscale;

		color_quad_u8 trialMinColor, trialMaxColor;
		color_quad_u8_set_clamped(&trialMinColor, (int)(xl.m_c[0] * scale + .5f), (int)(xl.m_c[1] * scale + .5f), (int)(xl.m_c[2] * scale + .5f), (int)(xl.m_c[3] * scale + .5f));
		color_quad_u8_set_clamped(&trialMaxColor, (int)(xh.m_c[0] * scale + .5f), (int)(xh.m_c[1] * scale + .5f), (int)(xh.m_c[2] * scale + .5f), (int)(xh.m_c[3] * scale + .5f));

		fixDegenerateEndpoints(mode, &trialMinColor, &trialMaxColor, &xl, &xh, iscale);

		if ((pResults->m_best_overall_err == UINT64_MAX) || color_quad_u8_notequals(&trialMinColor, &pResults->m_low_endpoint) || color_quad_u8_notequals(&trialMaxColor, &pResults->m_high_endpoint))
			evaluate_solution(&trialMinColor, &trialMaxColor, pResults->m_pbits, pParams, pResults);
	}

	return pResults->m_best_overall_err;
}

static uint64_t color_cell_compression(uint32_t mode, const color_cell_compressor_params *pParams, color_cell_compressor_results *pResults, const bc7enc16_compress_block_params *pComp_params)
{
	assert((mode == 6) || (!pParams->m_has_alpha));

	pResults->m_best_overall_err = UINT64_MAX;

	// If the partition's colors are all the same in mode 1, then just pack them as a single color.
	if (mode == 1)
	{
		const uint32_t cr = pParams->m_pPixels[0].m_c[0], cg = pParams->m_pPixels[0].m_c[1], cb = pParams->m_pPixels[0].m_c[2];

		bc7enc16_bool allSame = BC7ENC16_TRUE;
		for (uint32_t i = 1; i < pParams->m_num_pixels; i++)
		{
			if ((cr != pParams->m_pPixels[i].m_c[0]) || (cg != pParams->m_pPixels[i].m_c[1]) || (cb != pParams->m_pPixels[i].m_c[2]))
			{
				allSame = BC7ENC16_FALSE;
				break;
			}
		}

		if (allSame)
			return pack_mode1_to_one_color(pParams, pResults, cr, cg, cb, pResults->m_pSelectors);
	}

	// Compute partition's mean color and principle axis.
	vec4F meanColor, axis;
	vec4F_set_scalar(&meanColor, 0.0f);

	for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
	{
		vec4F color = vec4F_from_color(&pParams->m_pPixels[i]);
		meanColor = vec4F_add(&meanColor, &color);
	}

	vec4F meanColorScaled = vec4F_mul(&meanColor, 1.0f / (float)(pParams->m_num_pixels));

	meanColor = vec4F_mul(&meanColor, 1.0f / (float)(pParams->m_num_pixels * 255.0f));
	vec4F_saturate_in_place(&meanColor);

	if (pParams->m_has_alpha)
	{
		// Use incremental PCA for RGBA PCA, because it's simple.
		vec4F_set_scalar(&axis, 0.0f);
		for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		{
			vec4F color = vec4F_from_color(&pParams->m_pPixels[i]);
			color = vec4F_sub(&color, &meanColorScaled);
			vec4F a = vec4F_mul(&color, color.m_c[0]);
			vec4F b = vec4F_mul(&color, color.m_c[1]);
			vec4F c = vec4F_mul(&color, color.m_c[2]);
			vec4F d = vec4F_mul(&color, color.m_c[3]);
			vec4F n = i ? axis : color;
			vec4F_normalize_in_place(&n);
			axis.m_c[0] += vec4F_dot(&a, &n);
			axis.m_c[1] += vec4F_dot(&b, &n);
			axis.m_c[2] += vec4F_dot(&c, &n);
			axis.m_c[3] += vec4F_dot(&d, &n);
		}
		vec4F_normalize_in_place(&axis);
	}
	else
	{
		// Use covar technique for RGB PCA, because it doesn't require per-pixel normalization.
		float cov[6] = { 0, 0, 0, 0, 0, 0 };

		for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		{
			const color_quad_u8 *pV = &pParams->m_pPixels[i];
			float r = pV->m_c[0] - meanColorScaled.m_c[0];
			float g = pV->m_c[1] - meanColorScaled.m_c[1];
			float b = pV->m_c[2] - meanColorScaled.m_c[2];
			cov[0] += r*r; cov[1] += r*g; cov[2] += r*b; cov[3] += g*g; cov[4] += g*b; cov[5] += b*b;
		}

		float vfr = .9f, vfg = 1.0f, vfb = .7f;
		for (uint32_t iter = 0; iter < 3; iter++)
		{
			float r = vfr*cov[0] + vfg*cov[1] + vfb*cov[2];
			float g = vfr*cov[1] + vfg*cov[3] + vfb*cov[4];
			float b = vfr*cov[2] + vfg*cov[4] + vfb*cov[5];

			float m = maximumf(maximumf(fabsf(r), fabsf(g)), fabsf(b));
			if (m > 1e-10f)
			{
				m = 1.0f / m;
				r *= m; g *= m;	b *= m;
			}

			vfr = r; vfg = g; vfb = b;
		}

		float len = vfr*vfr + vfg*vfg + vfb*vfb;
		if (len < 1e-10f)
			vec4F_set_scalar(&axis, 0.0f);
		else
		{
			len = 1.0f / sqrtf(len);
			vfr *= len; vfg *= len; vfb *= len;
			vec4F_set(&axis, vfr, vfg, vfb, 0);
		}
	}

	if (vec4F_dot(&axis, &axis) < .5f)
	{
		if (pParams->m_perceptual)
			vec4F_set(&axis, .213f, .715f, .072f, pParams->m_has_alpha ? .715f : 0);
		else
			vec4F_set(&axis, 1.0f, 1.0f, 1.0f, pParams->m_has_alpha ? 1.0f : 0);
		vec4F_normalize_in_place(&axis);
	}

	float l = 1e+9f, h = -1e+9f;

	for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
	{
		vec4F color = vec4F_from_color(&pParams->m_pPixels[i]);

		vec4F q = vec4F_sub(&color, &meanColorScaled);
		float d = vec4F_dot(&q, &axis);

		l = minimumf(l, d);
		h = maximumf(h, d);
	}

	l *= (1.0f / 255.0f);
	h *= (1.0f / 255.0f);

	vec4F b0 = vec4F_mul(&axis, l);
	vec4F b1 = vec4F_mul(&axis, h);
	vec4F c0 = vec4F_add(&meanColor, &b0);
	vec4F c1 = vec4F_add(&meanColor, &b1);
	vec4F minColor = vec4F_saturate(&c0);
	vec4F maxColor = vec4F_saturate(&c1);

	vec4F whiteVec;
	vec4F_set_scalar(&whiteVec, 1.0f);
	if (vec4F_dot(&minColor, &whiteVec) > vec4F_dot(&maxColor, &whiteVec))
	{
		vec4F temp = minColor;
		minColor = maxColor;
		maxColor = temp;
	}
	// First find a solution using the block's PCA.
	if (!find_optimal_solution(mode, minColor, maxColor, pParams, pResults))
		return 0;

	if (pComp_params->m_try_least_squares)
	{
		// Now try to refine the solution using least squares by computing the optimal endpoints from the current selectors.
		vec4F xl, xh;
		vec4F_set_scalar(&xl, 0.0f);
		vec4F_set_scalar(&xh, 0.0f);
		if (pParams->m_has_alpha)
			compute_least_squares_endpoints_rgba(pParams->m_num_pixels, pResults->m_pSelectors, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);
		else
			compute_least_squares_endpoints_rgb(pParams->m_num_pixels, pResults->m_pSelectors, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);

		xl = vec4F_mul(&xl, (1.0f / 255.0f));
		xh = vec4F_mul(&xh, (1.0f / 255.0f));

		if (!find_optimal_solution(mode, xl, xh, pParams, pResults))
			return 0;
	}

	if (pComp_params->m_uber_level > 0)
	{
		// In uber level 1, try varying the selectors a little, somewhat like cluster fit would. First try incrementing the minimum selectors,
		// then try decrementing the selectrors, then try both.
		uint8_t selectors_temp[16], selectors_temp1[16];
		memcpy(selectors_temp, pResults->m_pSelectors, pParams->m_num_pixels);

		const int max_selector = pParams->m_num_selector_weights - 1;

		uint32_t min_sel = 16;
		uint32_t max_sel = 0;
		for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		{
			uint32_t sel = selectors_temp[i];
			min_sel = minimumu(min_sel, sel);
			max_sel = maximumu(max_sel, sel);
		}

		for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		{
			uint32_t sel = selectors_temp[i];
			if ((sel == min_sel) && (sel < (pParams->m_num_selector_weights - 1)))
				sel++;
			selectors_temp1[i] = (uint8_t)sel;
		}

		vec4F xl, xh;
		vec4F_set_scalar(&xl, 0.0f);
		vec4F_set_scalar(&xh, 0.0f);
		if (pParams->m_has_alpha)
			compute_least_squares_endpoints_rgba(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);
		else
			compute_least_squares_endpoints_rgb(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);

		xl = vec4F_mul(&xl, (1.0f / 255.0f));
		xh = vec4F_mul(&xh, (1.0f / 255.0f));

		if (!find_optimal_solution(mode, xl, xh, pParams, pResults))
			return 0;

		for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		{
			uint32_t sel = selectors_temp[i];
			if ((sel == max_sel) && (sel > 0))
				sel--;
			selectors_temp1[i] = (uint8_t)sel;
		}

		if (pParams->m_has_alpha)
			compute_least_squares_endpoints_rgba(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);
		else
			compute_least_squares_endpoints_rgb(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);

		xl = vec4F_mul(&xl, (1.0f / 255.0f));
		xh = vec4F_mul(&xh, (1.0f / 255.0f));

		if (!find_optimal_solution(mode, xl, xh, pParams, pResults))
			return 0;

		for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
		{
			uint32_t sel = selectors_temp[i];
			if ((sel == min_sel) && (sel < (pParams->m_num_selector_weights - 1)))
				sel++;
			else if ((sel == max_sel) && (sel > 0))
				sel--;
			selectors_temp1[i] = (uint8_t)sel;
		}

		if (pParams->m_has_alpha)
			compute_least_squares_endpoints_rgba(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);
		else
			compute_least_squares_endpoints_rgb(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);

		xl = vec4F_mul(&xl, (1.0f / 255.0f));
		xh = vec4F_mul(&xh, (1.0f / 255.0f));

		if (!find_optimal_solution(mode, xl, xh, pParams, pResults))
			return 0;

		// In uber levels 2+, try taking more advantage of endpoint extrapolation by scaling the selectors in one direction or another.
		const uint32_t uber_err_thresh = (pParams->m_num_pixels * 56) >> 4;
		if ((pComp_params->m_uber_level >= 2) && (pResults->m_best_overall_err > uber_err_thresh))
		{
			const int Q = (pComp_params->m_uber_level >= 4) ? (pComp_params->m_uber_level - 2) : 1;
			for (int ly = -Q; ly <= 1; ly++)
			{
				for (int hy = max_selector - 1; hy <= (max_selector + Q); hy++)
				{
					if ((ly == 0) && (hy == max_selector))
						continue;

					for (uint32_t i = 0; i < pParams->m_num_pixels; i++)
						selectors_temp1[i] = (uint8_t)clampf(floorf((float)max_selector * ((float)selectors_temp[i] - (float)ly) / ((float)hy - (float)ly) + .5f), 0, (float)max_selector);

					//vec4F xl, xh;
					vec4F_set_scalar(&xl, 0.0f);
					vec4F_set_scalar(&xh, 0.0f);
					if (pParams->m_has_alpha)
						compute_least_squares_endpoints_rgba(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);
					else
						compute_least_squares_endpoints_rgb(pParams->m_num_pixels, selectors_temp1, pParams->m_pSelector_weightsx, &xl, &xh, pParams->m_pPixels);

					xl = vec4F_mul(&xl, (1.0f / 255.0f));
					xh = vec4F_mul(&xh, (1.0f / 255.0f));

					if (!find_optimal_solution(mode, xl, xh, pParams, pResults))
						return 0;
				}
			}
		}
	}

	if (mode == 1)
	{
		// Try encoding the partition as a single color by using the optimal singe colors tables to encode the block to its mean.
		color_cell_compressor_results avg_results = *pResults;
		const uint32_t r = (int)(.5f + meanColor.m_c[0] * 255.0f), g = (int)(.5f + meanColor.m_c[1] * 255.0f), b = (int)(.5f + meanColor.m_c[2] * 255.0f);
		uint64_t avg_err = pack_mode1_to_one_color(pParams, &avg_results, r, g, b, pResults->m_pSelectors_temp);
		if (avg_err < pResults->m_best_overall_err)
		{
			*pResults = avg_results;
			memcpy(pResults->m_pSelectors, pResults->m_pSelectors_temp, sizeof(pResults->m_pSelectors[0]) * pParams->m_num_pixels);
			pResults->m_best_overall_err = avg_err;
		}
	}

	return pResults->m_best_overall_err;
}

static uint64_t color_cell_compression_est(uint32_t num_pixels, const color_quad_u8 *pPixels, bc7enc16_bool perceptual, uint32_t pweights[4], uint64_t best_err_so_far)
{
	// Find RGB bounds as an approximation of the block's principle axis
	uint32_t lr = 255, lg = 255, lb = 255;
	uint32_t hr = 0, hg = 0, hb = 0;
	for (uint32_t i = 0; i < num_pixels; i++)
	{
		const color_quad_u8 *pC = &pPixels[i];
		if (pC->m_c[0] < lr) lr = pC->m_c[0];
		if (pC->m_c[1] < lg) lg = pC->m_c[1];
		if (pC->m_c[2] < lb) lb = pC->m_c[2];
		if (pC->m_c[0] > hr) hr = pC->m_c[0];
		if (pC->m_c[1] > hg) hg = pC->m_c[1];
		if (pC->m_c[2] > hb) hb = pC->m_c[2];
	}

	color_quad_u8 lowColor; color_quad_u8_set(&lowColor, lr, lg, lb, 0);
	color_quad_u8 highColor; color_quad_u8_set(&highColor, hr, hg, hb, 0);

	// Place endpoints at bbox diagonals and compute interpolated colors
	const uint32_t N = 8;
	color_quad_u8 weightedColors[8];

	weightedColors[0] = lowColor;
	weightedColors[N - 1] = highColor;
	for (uint32_t i = 1; i < (N - 1); i++)
	{
		weightedColors[i].m_c[0] = (uint8_t)((lowColor.m_c[0] * (64 - g_bc7_weights3[i]) + highColor.m_c[0] * g_bc7_weights3[i] + 32) >> 6);
		weightedColors[i].m_c[1] = (uint8_t)((lowColor.m_c[1] * (64 - g_bc7_weights3[i]) + highColor.m_c[1] * g_bc7_weights3[i] + 32) >> 6);
		weightedColors[i].m_c[2] = (uint8_t)((lowColor.m_c[2] * (64 - g_bc7_weights3[i]) + highColor.m_c[2] * g_bc7_weights3[i] + 32) >> 6);
	}

	// Compute dots and thresholds
	const int ar = highColor.m_c[0] - lowColor.m_c[0];
	const int ag = highColor.m_c[1] - lowColor.m_c[1];
	const int ab = highColor.m_c[2] - lowColor.m_c[2];

	int dots[8];
	for (uint32_t i = 0; i < N; i++)
		dots[i] = weightedColors[i].m_c[0] * ar + weightedColors[i].m_c[1] * ag + weightedColors[i].m_c[2] * ab;

	int thresh[8 - 1];
	for (uint32_t i = 0; i < (N - 1); i++)
		thresh[i] = (dots[i] + dots[i + 1] + 1) >> 1;

	uint64_t total_err = 0;
	if (perceptual)
	{
		// Transform block's interpolated colors to YCbCr
		int l1[8], cr1[8], cb1[8];
		for (int j = 0; j < 8; j++)
		{
			const color_quad_u8 *pE1 = &weightedColors[j];
			l1[j] = pE1->m_c[0] * 109 + pE1->m_c[1] * 366 + pE1->m_c[2] * 37;
			cr1[j] = ((int)pE1->m_c[0] << 9) - l1[j];
			cb1[j] = ((int)pE1->m_c[2] << 9) - l1[j];
		}

		for (uint32_t i = 0; i < num_pixels; i++)
		{
			const color_quad_u8 *pC = &pPixels[i];

			int d = ar * pC->m_c[0] + ag * pC->m_c[1] + ab * pC->m_c[2];

			// Find approximate selector
			uint32_t s = 0;
			if (d >= thresh[6])
				s = 7;
			else if (d >= thresh[5])
				s = 6;
			else if (d >= thresh[4])
				s = 5;
			else if (d >= thresh[3])
				s = 4;
			else if (d >= thresh[2])
				s = 3;
			else if (d >= thresh[1])
				s = 2;
			else if (d >= thresh[0])
				s = 1;

			// Compute error
			const int l2 = pC->m_c[0] * 109 + pC->m_c[1] * 366 + pC->m_c[2] * 37;
			const int cr2 = ((int)pC->m_c[0] << 9) - l2;
			const int cb2 = ((int)pC->m_c[2] << 9) - l2;

			const int dl = (l1[s] - l2) >> 8;
			const int dcr = (cr1[s] - cr2) >> 8;
			const int dcb = (cb1[s] - cb2) >> 8;

			int ie = (pweights[0] * dl * dl) + (pweights[1] * dcr * dcr) + (pweights[2] * dcb * dcb);

			total_err += ie;
			if (total_err > best_err_so_far)
				break;
		}
	}
	else
	{
		for (uint32_t i = 0; i < num_pixels; i++)
		{
			const color_quad_u8 *pC = &pPixels[i];

			int d = ar * pC->m_c[0] + ag * pC->m_c[1] + ab * pC->m_c[2];

			// Find approximate selector
			uint32_t s = 0;
			if (d >= thresh[6])
				s = 7;
			else if (d >= thresh[5])
				s = 6;
			else if (d >= thresh[4])
				s = 5;
			else if (d >= thresh[3])
				s = 4;
			else if (d >= thresh[2])
				s = 3;
			else if (d >= thresh[1])
				s = 2;
			else if (d >= thresh[0])
				s = 1;

			// Compute error
			const color_quad_u8 *pE1 = &weightedColors[s];

			int dr = (int)pE1->m_c[0] - (int)pC->m_c[0];
			int dg = (int)pE1->m_c[1] - (int)pC->m_c[1];
			int db = (int)pE1->m_c[2] - (int)pC->m_c[2];

			total_err += pweights[0] * (dr * dr) + pweights[1] * (dg * dg) + pweights[2] * (db * db);
			if (total_err > best_err_so_far)
				break;
		}
	}

	return total_err;
}

// This table contains bitmasks indicating which "key" partitions must be best ranked before this partition is worth evaluating.
// We first rank the best/most used 14 partitions (sorted by usefulness), record the best one found as the key partition, then use
// that to control the other partitions to evaluate. The quality loss is ~.08 dB RGB PSNR, the perf gain is up to ~11% (at uber level 0).
static const uint32_t g_partition_predictors[35] =
{
	UINT32_MAX,
	UINT32_MAX,
	UINT32_MAX,
	UINT32_MAX,
	UINT32_MAX,
	(1 << 1) | (1 << 2) | (1 << 8),
	(1 << 1) | (1 << 3) | (1 << 7),
	UINT32_MAX,
	UINT32_MAX,
	(1 << 2) | (1 << 8) | (1 << 16),
	(1 << 7) | (1 << 3) | (1 << 15),
	UINT32_MAX,
	(1 << 8) | (1 << 14) | (1 << 16),
	(1 << 7) | (1 << 14) | (1 << 15),
	UINT32_MAX,
	UINT32_MAX,
	UINT32_MAX,
	UINT32_MAX,
	(1 << 14) | (1 << 15),
	(1 << 16) | (1 << 22) | (1 << 14),
	(1 << 17) | (1 << 24) | (1 << 14),
	(1 << 2) | (1 << 14) | (1 << 15) | (1 << 1),
	UINT32_MAX,
	(1 << 1) | (1 << 3) | (1 << 14) | (1 << 16) | (1 << 22),
	UINT32_MAX,
	(1 << 1) | (1 << 2) | (1 << 15) | (1 << 17) | (1 << 24),
	(1 << 1) | (1 << 3) | (1 << 22),
	UINT32_MAX,
	UINT32_MAX,
	UINT32_MAX,
	(1 << 14) | (1 << 15) | (1 << 16) | (1 << 17),
	UINT32_MAX,
	UINT32_MAX,
	(1 << 1) | (1 << 2) | (1 << 3) | (1 << 27) | (1 << 4) | (1 << 24),
	(1 << 14) | (1 << 15) | (1 << 16) | (1 << 11) | (1 << 17) | (1 << 27)
};

// Estimate the partition used by mode 1. This scans through each partition and computes an approximate error for each.
static uint32_t estimate_partition(const color_quad_u8 *pPixels, const bc7enc16_compress_block_params *pComp_params, uint32_t pweights[4])
{
	const uint32_t total_partitions = minimumu(pComp_params->m_max_partitions_mode1, BC7ENC16_MAX_PARTITIONS1);
	if (total_partitions <= 1)
		return 0;

	uint64_t best_err = UINT64_MAX;
	uint32_t best_partition = 0;

	// Partition order sorted by usage frequency across a large test corpus. Pattern 34 (checkerboard) must appear in slot 34.
	// Using a sorted order allows the user to decrease the # of partitions to scan with minimal loss in quality.
	static const uint8_t s_sorted_partition_order[64] =
	{
		1 - 1, 14 - 1, 2 - 1, 3 - 1, 16 - 1, 15 - 1, 11 - 1, 17 - 1,
		4 - 1, 24 - 1, 27 - 1, 7 - 1, 8 - 1, 22 - 1, 20 - 1, 30 - 1,
		9 - 1, 5 - 1, 10 - 1, 21 - 1, 6 - 1, 32 - 1, 23 - 1, 18 - 1,
		19 - 1, 12 - 1, 13 - 1, 31 - 1, 25 - 1, 26 - 1, 29 - 1, 28 - 1,
		33 - 1, 34 - 1, 35 - 1, 46 - 1, 47 - 1, 52 - 1, 50 - 1, 51 - 1,
		49 - 1, 39 - 1, 40 - 1, 38 - 1, 54 - 1, 53 - 1, 55 - 1, 37 - 1,
		58 - 1, 59 - 1, 56 - 1, 42 - 1, 41 - 1, 43 - 1, 44 - 1, 60 - 1,
		45 - 1, 57 - 1, 48 - 1, 36 - 1, 61 - 1, 64 - 1, 63 - 1, 62 - 1
	};

	assert(s_sorted_partition_order[34] == 34);

	int best_key_partition = 0;

	for (uint32_t partition_iter = 0; (partition_iter < total_partitions) && (best_err > 0); partition_iter++)
	{
		const uint32_t partition = s_sorted_partition_order[partition_iter];

		// Check to see if we should bother evaluating this partition at all, depending on the best partition found from the first 14.
		if (pComp_params->m_mode1_partition_estimation_filterbank)
		{
			if ((partition_iter >= 14) && (partition_iter <= 34))
			{
				const uint32_t best_key_partition_bitmask = 1 << (best_key_partition + 1);
				if ((g_partition_predictors[partition] & best_key_partition_bitmask) == 0)
				{
					if (partition_iter == 34)
						break;

					continue;
				}
			}
		}

		const uint8_t *pPartition = &g_bc7_partition2[partition * 16];

		color_quad_u8 subset_colors[2][16];
		uint32_t subset_total_colors[2] = { 0, 0 };
		for (uint32_t index = 0; index < 16; index++)
			subset_colors[pPartition[index]][subset_total_colors[pPartition[index]]++] = pPixels[index];

		uint64_t total_subset_err = 0;
		for (uint32_t subset = 0; (subset < 2) && (total_subset_err < best_err); subset++)
			total_subset_err += color_cell_compression_est(subset_total_colors[subset], &subset_colors[subset][0], pComp_params->m_perceptual, pweights, best_err);

		if (total_subset_err < best_err)
		{
			best_err = total_subset_err;
			best_partition = partition;
		}

		// If the checkerboard pattern doesn't get the highest ranking vs. the previous (lower frequency) patterns, then just stop now because statistically the subsequent patterns won't do well either.
		if ((partition == 34) && (best_partition != 34))
			break;

		if (partition_iter == 13)
			best_key_partition = best_partition;

	} // partition

	return best_partition;
}

static void set_block_bits(uint8_t *pBytes, uint32_t val, uint32_t num_bits, uint32_t *pCur_ofs)
{
	assert((num_bits <= 32) && (val < (1ULL << num_bits)));
	while (num_bits)
	{
		const uint32_t n = minimumu(8 - (*pCur_ofs & 7), num_bits);
		pBytes[*pCur_ofs >> 3] |= (uint8_t)(val << (*pCur_ofs & 7));
		val >>= n;
		num_bits -= n;
		*pCur_ofs += n;
	}
	assert(*pCur_ofs <= 128);
}

typedef struct
{
	uint32_t m_mode;
	uint32_t m_partition;
	uint8_t m_selectors[16];
	color_quad_u8 m_low[2];
	color_quad_u8 m_high[2];
	uint32_t m_pbits[2][2];
} bc7_optimization_results;

static void encode_bc7_block(void *pBlock, const bc7_optimization_results *pResults)
{
	const uint32_t best_mode = pResults->m_mode;
	const uint32_t total_subsets = g_bc7_num_subsets[best_mode];
	const uint32_t total_partitions = 1 << g_bc7_partition_bits[best_mode];
	const uint8_t *pPartition = (total_subsets == 2) ? &g_bc7_partition2[pResults->m_partition * 16] : &g_bc7_partition1[0];

	uint8_t color_selectors[16];
	memcpy(color_selectors, pResults->m_selectors, 16);

	color_quad_u8 low[2], high[2];
	memcpy(low, pResults->m_low, sizeof(low));
	memcpy(high, pResults->m_high, sizeof(high));

	uint32_t pbits[2][2];
	memcpy(pbits, pResults->m_pbits, sizeof(pbits));

	int anchor[2] = { -1, -1 };

	for (uint32_t k = 0; k < total_subsets; k++)
	{
		const uint32_t anchor_index = k ? g_bc7_table_anchor_index_second_subset[pResults->m_partition] : 0;
		anchor[k] = anchor_index;

		const uint32_t color_index_bits = get_bc7_color_index_size(best_mode, 0);
		const uint32_t num_color_indices = 1 << color_index_bits;

		if (color_selectors[anchor_index] & (num_color_indices >> 1))
		{
			for (uint32_t i = 0; i < 16; i++)
				if (pPartition[i] == k)
					color_selectors[i] = (uint8_t)((num_color_indices - 1) - color_selectors[i]);

			color_quad_u8 tmp = low[k];
			low[k] = high[k];
			high[k] = tmp;

			if (!g_bc7_mode_has_shared_p_bits[best_mode])
			{
				uint32_t t = pbits[k][0];
				pbits[k][0] = pbits[k][1];
				pbits[k][1] = t;
			}
		}
	}

	uint8_t *pBlock_bytes = (uint8_t *)(pBlock);
	memset(pBlock_bytes, 0, BC7ENC16_BLOCK_SIZE);

	uint32_t cur_bit_ofs = 0;
	set_block_bits(pBlock_bytes, 1 << best_mode, best_mode + 1, &cur_bit_ofs);

	if (total_partitions > 1)
		set_block_bits(pBlock_bytes, pResults->m_partition, 6, &cur_bit_ofs);

	const uint32_t total_comps = (best_mode >= 4) ? 4 : 3;
	for (uint32_t comp = 0; comp < total_comps; comp++)
	{
		for (uint32_t subset = 0; subset < total_subsets; subset++)
		{
			set_block_bits(pBlock_bytes, low[subset].m_c[comp], (comp == 3) ? g_bc7_alpha_precision_table[best_mode] : g_bc7_color_precision_table[best_mode], &cur_bit_ofs);
			set_block_bits(pBlock_bytes, high[subset].m_c[comp], (comp == 3) ? g_bc7_alpha_precision_table[best_mode] : g_bc7_color_precision_table[best_mode], &cur_bit_ofs);
		}
	}

	for (uint32_t subset = 0; subset < total_subsets; subset++)
	{
		set_block_bits(pBlock_bytes, pbits[subset][0], 1, &cur_bit_ofs);
		if (!g_bc7_mode_has_shared_p_bits[best_mode])
			set_block_bits(pBlock_bytes, pbits[subset][1], 1, &cur_bit_ofs);
	}

	for (int idx = 0; idx < 16; idx++)
	{
		uint32_t n = get_bc7_color_index_size(best_mode, 0);
		if ((idx == anchor[0]) || (idx == anchor[1]))
			n--;
		set_block_bits(pBlock_bytes, color_selectors[idx], n, &cur_bit_ofs);
	}

	assert(cur_bit_ofs == 128);
}

static void handle_alpha_block(void *pBlock, const color_quad_u8 *pPixels, const bc7enc16_compress_block_params *pComp_params, color_cell_compressor_params *pParams)
{
	color_cell_compressor_results results6;

	pParams->m_pSelector_weights = g_bc7_weights4;
	pParams->m_pSelector_weightsx = (const vec4F *)g_bc7_weights4x;
	pParams->m_num_selector_weights = 16;
	pParams->m_comp_bits = 7;
	pParams->m_has_pbits = BC7ENC16_TRUE;
	pParams->m_has_alpha = BC7ENC16_TRUE;
	pParams->m_perceptual = pComp_params->m_perceptual;
	pParams->m_num_pixels = 16;
	pParams->m_pPixels = pPixels;

	bc7_optimization_results opt_results;
	results6.m_pSelectors = opt_results.m_selectors;

	uint8_t selectors_temp[16];
	results6.m_pSelectors_temp = selectors_temp;

	color_cell_compression(6, pParams, &results6, pComp_params);

	opt_results.m_mode = 6;
	opt_results.m_partition = 0;
	opt_results.m_low[0] = results6.m_low_endpoint;
	opt_results.m_high[0] = results6.m_high_endpoint;
	opt_results.m_pbits[0][0] = results6.m_pbits[0];
	opt_results.m_pbits[0][1] = results6.m_pbits[1];

	encode_bc7_block(pBlock, &opt_results);
}

static void handle_opaque_block(void *pBlock, const color_quad_u8 *pPixels, const bc7enc16_compress_block_params *pComp_params, color_cell_compressor_params *pParams)
{
	uint8_t selectors_temp[16];

	// Mode 6
	bc7_optimization_results opt_results;

	pParams->m_pSelector_weights = g_bc7_weights4;
	pParams->m_pSelector_weightsx = (const vec4F *)g_bc7_weights4x;
	pParams->m_num_selector_weights = 16;
	pParams->m_comp_bits = 7;
	pParams->m_has_pbits = BC7ENC16_TRUE;
	pParams->m_endpoints_share_pbit = BC7ENC16_FALSE;
	pParams->m_perceptual = pComp_params->m_perceptual;
	pParams->m_num_pixels = 16;
	pParams->m_pPixels = pPixels;
	pParams->m_has_alpha = BC7ENC16_FALSE;

	color_cell_compressor_results results6;
	results6.m_pSelectors = opt_results.m_selectors;
	results6.m_pSelectors_temp = selectors_temp;

	uint64_t best_err = color_cell_compression(6, pParams, &results6, pComp_params);

	opt_results.m_mode = 6;
	opt_results.m_partition = 0;
	opt_results.m_low[0] = results6.m_low_endpoint;
	opt_results.m_high[0] = results6.m_high_endpoint;
	opt_results.m_pbits[0][0] = results6.m_pbits[0];
	opt_results.m_pbits[0][1] = results6.m_pbits[1];

	// Mode 1
	if ((best_err > 0) && (pComp_params->m_max_partitions_mode1 > 0))
	{
		const uint32_t trial_partition = estimate_partition(pPixels, pComp_params, pParams->m_weights);
		pParams->m_pSelector_weights = g_bc7_weights3;
		pParams->m_pSelector_weightsx = (const vec4F *)g_bc7_weights3x;
		pParams->m_num_selector_weights = 8;
		pParams->m_comp_bits = 6;
		pParams->m_has_pbits = BC7ENC16_TRUE;
		pParams->m_endpoints_share_pbit = BC7ENC16_TRUE;

		const uint8_t *pPartition = &g_bc7_partition2[trial_partition * 16];

		color_quad_u8 subset_colors[2][16];

		uint32_t subset_total_colors1[2] = { 0, 0 };

		uint8_t subset_pixel_index1[2][16];
		uint8_t subset_selectors1[2][16];
		color_cell_compressor_results subset_results1[2];

		for (uint32_t idx = 0; idx < 16; idx++)
		{
			const uint32_t p = pPartition[idx];
			subset_colors[p][subset_total_colors1[p]] = pPixels[idx];
			subset_pixel_index1[p][subset_total_colors1[p]] = (uint8_t)idx;
			subset_total_colors1[p]++;
		}

		uint64_t trial_err = 0;
		for (uint32_t subset = 0; subset < 2; subset++)
		{
			pParams->m_num_pixels = subset_total_colors1[subset];
			pParams->m_pPixels = &subset_colors[subset][0];

			color_cell_compressor_results *pResults = &subset_results1[subset];
			pResults->m_pSelectors = &subset_selectors1[subset][0];
			pResults->m_pSelectors_temp = selectors_temp;
			uint64_t err = color_cell_compression(1, pParams, pResults, pComp_params);
			trial_err += err;
			if (trial_err > best_err)
				break;

		} // subset

		if (trial_err < best_err)
		{
			best_err = trial_err;
			opt_results.m_mode = 1;
			opt_results.m_partition = trial_partition;
			for (uint32_t subset = 0; subset < 2; subset++)
			{
				for (uint32_t i = 0; i < subset_total_colors1[subset]; i++)
					opt_results.m_selectors[subset_pixel_index1[subset][i]] = subset_selectors1[subset][i];
				opt_results.m_low[subset] = subset_results1[subset].m_low_endpoint;
				opt_results.m_high[subset] = subset_results1[subset].m_high_endpoint;
				opt_results.m_pbits[subset][0] = subset_results1[subset].m_pbits[0];
			}
		}
	}

	encode_bc7_block(pBlock, &opt_results);
}

bc7enc16_bool bc7enc16_compress_block(void *pBlock, const void *pPixelsRGBA, const bc7enc16_compress_block_params *pComp_params)
{
	assert(g_bc7_mode_1_optimal_endpoints[255][0].m_hi != 0);

	const color_quad_u8 *pPixels = (const color_quad_u8 *)(pPixelsRGBA);

	color_cell_compressor_params params;
	if (pComp_params->m_perceptual)
	{
		// https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.709_conversion
		const float pr_weight = (.5f / (1.0f - .2126f)) * (.5f / (1.0f - .2126f));
		const float pb_weight = (.5f / (1.0f - .0722f)) * (.5f / (1.0f - .0722f));
		params.m_weights[0] = (int)(pComp_params->m_weights[0] * 4.0f);
		params.m_weights[1] = (int)(pComp_params->m_weights[1] * 4.0f * pr_weight);
		params.m_weights[2] = (int)(pComp_params->m_weights[2] * 4.0f * pb_weight);
		params.m_weights[3] = pComp_params->m_weights[3] * 4;
	}
	else
		memcpy(params.m_weights, pComp_params->m_weights, sizeof(params.m_weights));

	for (uint32_t i = 0; i < 16; i++)
	{
		if (pPixels[i].m_c[3] < 255)
		{
			handle_alpha_block(pBlock, pPixels, pComp_params, &params);
			return BC7ENC16_TRUE;
		}
	}
	handle_opaque_block(pBlock, pPixels, pComp_params, &params);
	return BC7ENC16_FALSE;
}

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright(c) 2018 Richard Geldreich, Jr.
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain(www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non - commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain.We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors.We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
