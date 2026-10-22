// File: basisu_xbc7_encode.h
// XBC7: experimental supercompressed-BC7 codec (prototype moved out of basisu_tool.cpp).
#pragma once
#include "basisu_enc.h"
#include "../transcoder/basisu_transcoder_internal.h"

namespace basisu {
namespace xbc7 {

	const uint32_t DEFAULT_EFFORT_LEVEL = 3;

	// Default quality "level" for the optional bc7e_scalar BC7 base encoder.
	const uint32_t DEFAULT_BC7E_SCALAR_LEVEL = 2;

	// Supported bc7e_scalar quality level range (higher = slower/better). Maps to the
	// encoder's ultrafast..slowest presets where it's invoked; clamp to this range.
	const uint32_t BC7E_SCALAR_MIN_LEVEL = 0;
	const uint32_t BC7E_SCALAR_MAX_LEVEL = 6;

	// Which BC7 base encoder XBC7 uses to pack the initial BC7 blocks before
	// supercompression. cBC7F is the built-in fast real-time packer (the default);
	// cBC7E_Scalar is the higher-quality (slower) scalar bc7e encoder.
	enum class bc7_encoder_type
	{
		cBC7F = 0,
		cBC7E_Scalar = 1
	};

	struct pack_options
	{
		uint32_t m_dct_q = 100; // [1,100]; 100 == lossless mode (weights always residual DPCM)

		// Per-channel error weights. ALWAYS used by the XBC7 encoder when evaluating block candidates vs. the
		// original pixels during supercompression, regardless of the BC7 base encoder. (The base encoders differ:
		// bc7f supports neither weights nor perceptual; bc7e_scalar supports either - see m_perceptual below.)
		uint32_t m_weights[4] = { 1, 1, 1, 1 };

		// Encoder effort/speed knob [0,10]: 0 == fastest, 10 == slowest/best
		// (9 is the usual practical max; 10 exists for very high core counts).
		// Currently controls how many weight-grid predictors the per-block DCT and
		// DPCM sweeps evaluate: at 10 the full candidate set is searched (identical
		// output to before this knob existed); as effort drops the sweep is pruned
		// down to a small core of empirically high-value predictors (~6 at effort
		// 0), trading a little ratio for a large drop in encode time -- the DCT
		// predictor search (Q < 100) is the dominant cost. Default
		// DEFAULT_EFFORT_LEVEL (a balanced speed/quality point, NOT the full
		// search); pass 10 to reproduce the pre-knob output exactly.
		uint32_t m_effort_level = DEFAULT_EFFORT_LEVEL;

		// Optimize weights after encoding with bc7f. bc7f is a fast real-time BC7
		// packer whose per-texel weights aren't optimal; when true, every block's
		// weights are exhaustively re-derived for its (unchanged) config+endpoints
		// right after the base pack. Slower (but threaded with the base pack);
		// endpoints/config are untouched, so quality only holds or improves.
		// Default off.
		bool m_optimize_weights_after_bc7f = false;

		// Flags for the underlying real-time BC7 base packer
		// (basist::bc7f::fast_pack_bc7_auto_rgba). See bc7f::cPackBC7Flag* /
		// cPackBC7FlagDefault* in basisu_transcoder_internal.h -- controls the
		// quality/speed of the BC7 blocks XBC7 then supercompresses.
		uint32_t m_bc7_pack_flags = basist::bc7f::cPackBC7FlagDefaultPartiallyAnalytical;

		// Selects which BC7 base encoder packs the initial blocks. Default cBC7F
		// (the existing fast built-in packer); cBC7E_Scalar selects the higher-quality
		// scalar bc7e encoder at m_bc7e_scalar_level. Affects only the primary base
		// pack -- the optional alt-pack RDO path still uses bc7f for now.
		bc7_encoder_type m_bc7_encoder = bc7_encoder_type::cBC7F;

		// Quality level for the bc7e_scalar encoder (only used when m_bc7_encoder ==
		// cBC7E_Scalar). Higher = slower/better. Default DEFAULT_BC7E_SCALAR_LEVEL (2).
		uint32_t m_bc7e_scalar_level = DEFAULT_BC7E_SCALAR_LEVEL;

		// True if the source is perceptual (sRGB) data. Currently only consulted by
		// the bc7e_scalar base encoder: when true it runs in its built-in perceptual
		// error mode (and ignores m_weights); when false it runs linear and honors
		// m_weights. Mirror basis_compressor_params::m_perceptual here.
		bool m_perceptual = false;

		// When true (the default), the encoder decodes the stream it just produced
		// (via the transcoder) and verifies every logical BC7 block round-trips
		// exactly to the coded blocks; a failure fails the encode. Guarantees every
		// emitted stream is valid/decodable. Disable only to skip the extra decode.
		bool m_self_validate = true;

		// Optional "poor man's RDO" on the BC7 base pack. When enabled, every
		// block is ALSO packed with m_bc7_pack_flags_alt (typically a cheaper,
		// e.g. mode-6-only set). Both candidates are unpacked and their RGBA PSNR
		// vs the source measured; the cheaper alternate is KEPT as long as it's no
		// more than m_bc7_alt_max_psnr_drop dB worse than the primary. Trades a
		// little quality for fewer command/config bits downstream. Default off.
		bool m_bc7_alt_pack_enabled = false;
		uint32_t m_bc7_pack_flags_alt = basist::bc7f::cPackBC7FlagPBitOpt | basist::bc7f::cPackBC7FlagUseTrivialMode6;
		float m_bc7_alt_max_psnr_drop = 0.5f; // dB the alternate may be worse than the primary and still be kept

		// Optional block-reuse RDO pre-pass (runs after the BC7 base pack, BEFORE
		// the endpoint RDO -- once a whole block is reused there's no point trying
		// cheaper endpoints). For each non-solid block it tries replacing it with:
		//  - REPEAT: an exact copy of its left/upper causal neighbor (the cheapest
		//    command -- one byte, no config/endpoints/weights), kept if within
		//    m_repeat_rdo_max_psnr_drop dB; and/or
		//  - SOLID: its mean color (cheap SolidDPCM command), kept if within
		//    m_solid_rdo_max_psnr_drop dB.
		// Repeat is preferred over Solid (cheaper). Each independently enabled; off
		// by default.
		bool m_repeat_rdo_enabled = false;
		float m_repeat_rdo_max_psnr_drop = 0.5f; // dB a block may drop to become a Repeat of a neighbor
		bool m_solid_rdo_enabled = false;
		float m_solid_rdo_max_psnr_drop = 0.5f;  // dB a block may drop to become a solid-color block

		// Optional endpoint-prediction RDO pre-pass (runs after the BC7 base pack,
		// before stripe coding). For each block it tries forcing the endpoints to
		// each valid causal neighbor's prediction (left/upper/left-diag/right-diag)
		// via a zero-residual endpoint DPCM and keeps the best whose weighted RGBA
		// PSNR drops by no more than m_endpoint_rdo_max_psnr_drop dB -- those zero
		// residuals then cost almost nothing downstream. Default off.
		bool m_endpoint_rdo_enabled = false;
		float m_endpoint_rdo_max_psnr_drop = 0.5f; // dB the block PSNR may drop to adopt a neighbor's endpoints

		// Optional weight-DCT AC-truncation RDO (DCT-coded blocks only). After a
		// block is DCT-coded, its highest-frequency weight-DCT AC coefficients are
		// zeroed one at a time in reverse zigzag order (the 2x2 low-freq corner DC,
		// (1,0),(0,1),(1,1) is protected) while the decoded PSNR stays within this
		// dB drop -- fewer coded ACs. 0 == disabled.
		float m_ac_trunc_rdo_max_psnr_drop = 0.0f;

		// Quality floor shared by ALL RDO passes (block-reuse + endpoint + alt-pack):
		// a candidate whose absolute weighted RGBA PSNR is below this is rejected
		// outright, even if its drop is within tolerance -- prevents accepting
		// excessive distortion in already-degraded blocks.
		float m_rdo_min_block_psnr = 33.0f; // dB

		// Master switch for ALL development/debug console output (printed via
		// fmt_debug_printf, so it also respects the global enable_debug_printf()
		// and stays silent in normal builds). Default false. Nothing prints
		// unless this is true.
		bool m_debug_output = false;

		// Print the detailed pack statistics dashboard. Requires m_debug_output
		// to also be set (m_debug_output gates everything).
		bool m_print_stats = false;

		// When true, the encoder writes visualization PNG(s) -- one pixel block
		// per BC7 block -- to (m_debug_file_prefix + "<name>.png"). Encode-time
		// only; never affects the compressed output. Default off.
		bool m_debug_images = false;
		std::string m_debug_file_prefix;

		// Weight DCT-vs-DPCM decision: choose lossless DPCM when
		// dpcm_cost * 100 <= dct_cost * alpha. Both costs are measured-rate
		// estimates (see g_dpcm_resid_cost_obits + the DCT byte cost
		// constants), so 100 is the neutral operating point; raising alpha
		// flips more blocks to lossless DPCM (quality up, size up).
		uint32_t m_wt_dpcm_alpha_pct = 100;

		// REQUIRED, never null: the caller's job pool (total threads INCLUDES
		// the caller, basisu convention). The encoder always uses it; a pool of 1
		// just runs serially. Pool size affects scheduling only, never the emitted
		// bytes (the stripe count is independent of it).
		job_pool* m_pJob_pool = nullptr;

		// Stripe count for the main coding pass: 0 == auto (derived from image
		// dimensions); else force exactly this many, clamped to
		// [1, min(block_rows, XBC7_MAX_ENCODER_STRIPES)]. 1 == single-stripe (max
		// ratio: no seam cost, no seek table). More stripes == more decode
		// parallelism but a slightly larger file (per-seam prediction reset +
		// seek-table cost); more stripes than pool threads simply queue. Prefer
		// set_num_stripes_for_image() over assigning this directly.
		uint32_t m_num_stripes = 0;

		// Validate a desired stripe count against an image and store it in
		// m_num_stripes. Clamps so every stripe holds at least the minimum
		// efficient number of block rows (never tiny or empty stripes) and never
		// exceeds the format max; images too short to stripe usefully collapse to
		// a single stripe. Returns the actual count stored (>= 1).
		uint32_t set_num_stripes_for_image(const image& img, uint32_t desired_num_stripes);

		// Configure ALL "poor man's RDO" knobs from a single level [0,100].
		//  0       -> RDO fully OFF: every enable flag false, every PSNR-drop 0.
		//  [1,100] -> enables the RDO passes and linearly ramps their tolerated
		//             per-block PSNR drop with the level: the general passes
		//             (BC7 alt-pack, endpoint-DPCM, weight-DCT AC-truncation) up to
		//             10 dB at 100, and the cheaper block-reuse passes (repeat,
		//             solid) up to 4 dB at 100.
		// The absolute quality floor (m_rdo_min_block_psnr) is left untouched -- it
		// still gates every candidate regardless of level.
		void set_rdo_level(uint32_t rdo_level);
	};

	// coded_log_blocks (REQUIRED, by reference): receives the final coded logical
	// BC7 blocks (resized + filled by the encoder). It's mandatory because the
	// encoder always decodes the stream it just produced and validates every block
	// round-trips to these -- see the self-validation at the end of pack_image().
	bool pack_image(
		const image& orig_img,
		const pack_options& opts,
		uint8_vec& comp_bytes,
		vector2D<basist::bc7u::log_bc7_block>& coded_log_blocks);

	// NOTE: the decoder API (unpack_image / unpack_image_threaded) now lives in
	// basisu_xbc7_decode.h.

} // namespace xbc7
} // namespace basisu
