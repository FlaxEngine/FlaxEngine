// basisu_bc15_spmd.h
// Standalone opaque 4-color BC1 (DXT1) encoder for the Basis Universal encoder library.
//
// Two entry points share the SAME algorithm (omatch-solid fast path -> PCA endpoint seed -> 2x integer-threshold
// least-squares -> low-variance avg-solid 2nd seed (keep-better) -> integer-threshold selectors):
//   - encode_bc1_scalar : portable scalar reference ("oracle"), no SIMD.
//   - encode_bc1_spmd   : SSE4.1 cppspmd kernel, 4 blocks per vector; falls back to the scalar path when SSE4.1
//                         is unavailable (e.g. WASM without SIMD).
// Benchmarked (vs basist::encode_bc1): ~3.4-4.7x faster on photos, ~+0.06 dB average, and it beats encode_bc1
// on low-dynamic-range / gradient blocks. Opaque 4-color only (no 3-color / punchthrough).
#pragma once

#include "basisu_enc.h" // basisu::color_rgba

namespace basisu
{
	namespace bc_spmd
	{
		// Build the solid-color omatch tables. Call once before encoding (idempotent). Not safe to call
		// concurrently with itself or the encoders on first use -- call it once at startup (e.g. right after
		// basisu_encoder_init(), which also performs the CPU-feature detection encode_bc1_spmd dispatches on).
		void init();

		// Encode num_blocks opaque BC1 blocks.
		//   pSrc_pixels  : num_blocks * 16 color_rgba, block-contiguous; texel index within a block = y*4 + x.
		//   pBlocks      : output; BC1 block b is written at (uint8_t*)pBlocks + b * 8 * block_stride.
		//   num_blocks   : any count; counts not divisible by 4 are handled internally (no caller padding needed).
		//   block_stride : output spacing in units of 8-byte BC1 blocks. 1 = contiguous BC1. 2 = write into every
		//                  other 8-byte slot, e.g. the color half of 16-byte BC3/BC5 blocks (pass pBlocks already
		//                  offset to that half). Input layout is unaffected.
		// The caller is responsible for extracting the 16 pixels per block from its source image.
		void encode_bc1_scalar(void* pBlocks, const color_rgba* pSrc_pixels, uint32_t num_blocks, uint32_t block_stride = 1);
		void encode_bc1_spmd(void* pBlocks, const color_rgba* pSrc_pixels, uint32_t num_blocks, uint32_t block_stride = 1);

		// Encode num_blocks single-channel BC4 (RGTC1) blocks. Clone of basist::encode_bc4's algorithm: raw bbox
		// min/max endpoints + exact nearest-of-8 integer-threshold selector, ALWAYS 8-value mode (red0 > red1) --
		// including a forced-8-value solid-block path so we NEVER emit a 6-value block (safe for BC3/BC5). Quality
		// matches encode_bc4 (bit-exact on non-solid blocks; identical decode on solid). (Least-squares endpoint
		// refinement may be layered on later.)
		//   pSrc_pixels  : num_blocks * 16 uint8_t, block-contiguous; texel index within a block = y*4 + x. The
		//                  caller extracts the 16 single-channel values per block itself (e.g. one channel of RGBA).
		//   pBlocks      : output; BC4 block b is written at (uint8_t*)pBlocks + b * 8 * block_stride.
		//   num_blocks   : any count; the <4 tail is handled internally.
		//   block_stride : output spacing in units of 8-byte BC4 blocks. 1 = contiguous BC4. 2 = write into every
		//                  other 8-byte slot, e.g. the alpha half of a 16-byte BC3 block or one half of a BC5 pair.
		//   high_quality : false (default) = fast bbox encoder, matches encode_bc4 quality, ~1.4x faster. true = add a
		//                  1-D least-squares endpoint refit (keep-best, monotonic): ~+0.8 dB on photos but ~0.6x speed.
		//   src_stride   : BYTE stride between consecutive texel values in pSrc_pixels. 1 (default) = tightly packed
		//                  single channel. 4 = one channel of color_rgba (point pSrc_pixels at the desired channel
		//                  byte, e.g. &rgba[0].g) -- lets callers encode a channel in place with no extraction copy.
		void encode_bc4_scalar(void* pBlocks, const uint8_t* pSrc_pixels, uint32_t num_blocks, uint32_t block_stride = 1, bool high_quality = false, uint32_t src_stride = 1);
		void encode_bc4_spmd(void* pBlocks, const uint8_t* pSrc_pixels, uint32_t num_blocks, uint32_t block_stride = 1, bool high_quality = false, uint32_t src_stride = 1);

		// ---------------- High-level format helpers (RGBA in -> complete GPU blocks out) ----------------
		// Convenience wrappers over the low-level encoders. Each takes pPixels = num_blocks * 16 color_rgba,
		// block-contiguous (texel index = y*4 + x), and writes complete blocks. num_blocks may be ANY count >= 1
		// (not necessarily a multiple of 4); the <4 tail is handled internally with NO output overrun.
		//   use_spmd     : true = SSE4.1 SPMD path (auto-falls back to scalar when SSE4.1 is unavailable); false = scalar.
		//   high_quality : enables the BC4 channel least-squares refit for the alpha/red/green block(s) (BC3/BC4/BC5).
		// Output block sizes / layout (matches the transcoder's bcu unpackers):
		//   BC1: 8 bytes/block  -- RGB, opaque 4-color.
		//   BC2: 16 bytes/block -- explicit 4-bit alpha [0..7] (scalar) + BC1 color [8..15].
		//   BC3: 16 bytes/block -- BC4 alpha block [0..7] + BC1 color [8..15].
		//   BC4: 8 bytes/block  -- single channel = R.
		//   BC5: 16 bytes/block -- BC4 of R [0..7] + BC4 of G [8..15].
		void encode_bc1(void* pBlocks, const color_rgba* pPixels, uint32_t num_blocks, bool use_spmd = true);
		void encode_bc2(void* pBlocks, const color_rgba* pPixels, uint32_t num_blocks, bool use_spmd = true);
		void encode_bc3(void* pBlocks, const color_rgba* pPixels, uint32_t num_blocks, bool use_spmd = true, bool high_quality = false);
		void encode_bc4(void* pBlocks, const color_rgba* pPixels, uint32_t num_blocks, bool use_spmd = true, bool high_quality = false);
		void encode_bc5(void* pBlocks, const color_rgba* pPixels, uint32_t num_blocks, bool use_spmd = true, bool high_quality = false);

	} // namespace bc_spmd
} // namespace basisu
