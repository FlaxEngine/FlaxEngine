// File: bc7enc16.h - Richard Geldreich, Jr. - MIT license or public domain (see end of bc7enc16.c)
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BC7ENC16_BLOCK_SIZE (16)
#define BC7ENC16_MAX_PARTITIONS1 (64)
#define BC7ENC16_MAX_UBER_LEVEL (4)

typedef uint8_t bc7enc16_bool;
#define BC7ENC16_TRUE (1)
#define BC7ENC16_FALSE (0)

typedef struct
{
	// m_max_partitions_mode1 may range from 0 (disables mode 1) to BC7ENC16_MAX_PARTITIONS1. The higher this value, the slower the compressor, but the higher the quality.
	uint32_t m_max_partitions_mode1;
	
	// Relative RGBA or YCbCrA weights.
	uint32_t m_weights[4];
	
	// m_uber_level may range from 0 to BC7ENC16_MAX_UBER_LEVEL. The higher this value, the slower the compressor, but the higher the quality.
	uint32_t m_uber_level;

	// If m_perceptual is true, colorspace error is computed in YCbCr space, otherwise RGB.
	bc7enc16_bool m_perceptual;

	// Set m_try_least_squares to false for slightly faster/lower quality compression.
	bc7enc16_bool m_try_least_squares;
	
	// When m_mode1_partition_estimation_filterbank, the mode1 partition estimator skips lesser used partition patterns unless they are strongly predicted to be potentially useful.
	// There's a slight loss in quality with this enabled (around .08 dB RGB PSNR or .05 dB Y PSNR), but up to a 11% gain in speed depending on the other settings.
	bc7enc16_bool m_mode1_partition_estimation_filterbank;

} bc7enc16_compress_block_params;

inline void bc7enc16_compress_block_params_init_linear_weights(bc7enc16_compress_block_params *p)
{
	p->m_perceptual = BC7ENC16_FALSE;
	p->m_weights[0] = 1;
	p->m_weights[1] = 1;
	p->m_weights[2] = 1;
	p->m_weights[3] = 1;
}

inline void bc7enc16_compress_block_params_init_perceptual_weights(bc7enc16_compress_block_params *p)
{
	p->m_perceptual = BC7ENC16_TRUE;
	p->m_weights[0] = 128;
	p->m_weights[1] = 64;
	p->m_weights[2] = 16;
	p->m_weights[3] = 32;
}

inline void bc7enc16_compress_block_params_init(bc7enc16_compress_block_params *p)
{
	p->m_max_partitions_mode1 = BC7ENC16_MAX_PARTITIONS1;
	p->m_try_least_squares = BC7ENC16_TRUE;
	p->m_mode1_partition_estimation_filterbank = BC7ENC16_TRUE;
	p->m_uber_level = 0;
	bc7enc16_compress_block_params_init_perceptual_weights(p);
}

// bc7enc16_compress_block_init() MUST be called before calling bc7enc16_compress_block() (or you'll get artifacts).
void bc7enc16_compress_block_init();

// Packs a single block of 16x16 RGBA pixels (R first in memory) to 128-bit BC7 block pBlock, using either mode 1 and/or 6.
// Alpha blocks will always use mode 6, and by default opaque blocks will use either modes 1 or 6.
// Returns BC7ENC16_TRUE if the block had any pixels with alpha < 255, otherwise it return BC7ENC16_FALSE. (This is not an error code - a block is always encoded.)
bc7enc16_bool bc7enc16_compress_block(void *pBlock, const void *pPixelsRGBA, const bc7enc16_compress_block_params *pComp_params);

#ifdef __cplusplus
}
#endif
