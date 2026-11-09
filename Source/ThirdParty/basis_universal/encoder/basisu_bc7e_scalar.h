// bc7e_scalar.h - Pure scalar C++17 port of bc7e.ispc (no ISPC, no SIMD).
// Drop-in alternative BC7 encoder exposing the same surface as the ISPC build,
// but in the bc7e_scalar:: namespace so it can run side-by-side with ispc:: for
// quality comparison. See bc7e.ispc for the original; this is de-SIMD'd to
// encode one 4x4 block at a time.
#pragma once

#include <stdint.h>

namespace bc7e_scalar
{
	// Mirrors ispc::bc7e_compress_block_params (identical layout/semantics).
	struct bc7e_compress_block_params
	{
		uint32_t m_max_partitions_mode[8];

		uint32_t m_weights[4];

		uint32_t m_uber_level;
		uint32_t m_refinement_passes;

		uint32_t m_mode4_rotation_mask;
		uint32_t m_mode4_index_mask;
		uint32_t m_mode5_rotation_mask;
		uint32_t m_uber1_mask;

		bool m_perceptual;
		bool m_pbit_search;
		bool m_mode6_only;
		// When false, color_cell_compression() is NOT allowed to use the precomputed
		// "one color" optimal-endpoint lookup tables (the solid/allSame and average-
		// color paths). Disabling them avoids the extreme-endpoint "trap" blocks that
		// decode badly under lossy weight coding. Default TRUE (matches bc7e's original
		// behavior); callers disable it when needed (e.g. XBC7 below lossless Q).
		// (Repurposed from the former m_unused0 padding bool.)
		bool m_use_luts;

		struct
		{
			uint32_t m_max_mode13_partitions_to_try;
			uint32_t m_max_mode0_partitions_to_try;
			uint32_t m_max_mode2_partitions_to_try;
			bool m_use_mode[7];
			bool m_unused1;
		} m_opaque_settings;

		struct
		{
			uint32_t m_max_mode7_partitions_to_try;
			uint32_t m_mode67_error_weight_mul[4];

			bool m_use_mode4;
			bool m_use_mode5;
			bool m_use_mode6;
			bool m_use_mode7;

			bool m_use_mode4_rotation;
			bool m_use_mode5_rotation;
			bool m_unused2;
			bool m_unused3;
		} m_alpha_settings;
	};

	void bc7e_compress_block_init();

	void bc7e_compress_block_params_init(bc7e_compress_block_params* p, bool perceptual);
	void bc7e_compress_block_params_init_basic(bc7e_compress_block_params* p, bool perceptual);
	void bc7e_compress_block_params_init_fast(bc7e_compress_block_params* p, bool perceptual);
	void bc7e_compress_block_params_init_slow(bc7e_compress_block_params* p, bool perceptual);
	void bc7e_compress_block_params_init_slowest(bc7e_compress_block_params* p, bool perceptual);
	void bc7e_compress_block_params_init_ultrafast(bc7e_compress_block_params* p, bool perceptual);
	void bc7e_compress_block_params_init_veryfast(bc7e_compress_block_params* p, bool perceptual);
	void bc7e_compress_block_params_init_veryslow(bc7e_compress_block_params* p, bool perceptual);

	// pUsed_lut (optional, one byte per block): set to 1 if the WINNING encoding of that
	// block used the precomputed "one color" optimal-endpoint lookup tables on any subset
	// (solid/allSame or average-color path) -- a hint that the block placed endpoints at
	// extreme positions and may be "weird"/fragile under lossy weight recoding. 0 otherwise.
	void bc7e_compress_blocks(uint32_t num_blocks, uint64_t* pBlocks, const uint32_t* pPixelsRGBA, const bc7e_compress_block_params* pComp_params, uint8_t* pUsed_lut = nullptr);

	// Compress a single 4x4 RGBA block (16 pixels) to ONE specified BC7 mode only, using
	// the given settings. Writes the 16-byte block to pBlock and returns its encoding error.
	// Additive testing API - does not affect bc7e_compress_blocks().
	//   mode           : 0..7
	//   partition      : modes 0,1,2,3,7 -> partition pattern index; -1 = auto-select optimal
	//                    (existing logic). Ignored for modes 4,5,6.
	//   rotation       : modes 4,5 -> dual-plane component rotation [0..3] (0 = none). A non-zero
	//                    rotation forces linear (non-perceptual) error internally. Ignored otherwise.
	//   index_selector : mode 4 -> 0 or 1 index-set selector. Ignored otherwise.
	// Note: forcing an opaque mode (0-3) on a block with alpha drops alpha (A becomes 255).
	uint64_t bc7e_compress_block_single_mode(uint64_t* pBlock, const uint32_t* pPixelsRGBA, const bc7e_compress_block_params* pComp_params,
		uint32_t mode, int partition = -1, uint32_t rotation = 0, uint32_t index_selector = 0);

} // namespace bc7e_scalar
