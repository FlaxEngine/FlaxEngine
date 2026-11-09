// basisu_transcoder.h
// Copyright (C) 2019-2026 Binomial LLC. All Rights Reserved.
// Important: If compiling with gcc, be sure strict aliasing is disabled: -fno-strict-aliasing
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Also see basis_tex_format in basisu_file_headers.h (TODO: Perhaps move key definitions into here.)
#pragma once

// By default KTX2 support is enabled to simplify compilation. This implies the need for the Zstandard library (which we distribute as a single source file in the "zstd" directory) by default.
// Set BASISD_SUPPORT_KTX2 to 0 to completely disable KTX2 support as well as Zstd/miniz usage which is only required for UASTC supercompression in KTX2 files.
// Also see BASISD_SUPPORT_KTX2_ZSTD in basisu_transcoder.cpp, which individually disables Zstd usage.
#ifndef BASISD_SUPPORT_KTX2
	#define BASISD_SUPPORT_KTX2 1
#endif

// Set BASISD_SUPPORT_KTX2_ZSTD to 0 to disable Zstd usage and KTX2 UASTC Zstd supercompression support 
#ifndef BASISD_SUPPORT_KTX2_ZSTD
	#define BASISD_SUPPORT_KTX2_ZSTD 0
#endif

#include "basisu_transcoder_internal.h"
#include "basisu_transcoder_uastc.h"
#include "basisu_file_headers.h"

namespace basist
{
	const uint32_t BASISU_MAX_SUPPORTED_TEXTURE_DIMENSION = 16384;
	const uint32_t BASISU_DEBLOCKING_BLOCK_SIZE_THRESHOLD = 80; // in pixels/texels, 10x8 or larger

	// High-level composite texture formats supported by the transcoder.
	// Each of these texture formats directly correspond to OpenGL/D3D/Vulkan etc. texture formats.
	// Notes:
	// - If you specify a texture format that supports alpha, but the .basis file doesn't have alpha, the transcoder will automatically output a 
	// fully opaque (255) alpha channel.
	// - The PVRTC1 texture formats only support power of 2 dimension .basis files, but this may be relaxed in a future version.
	// - The PVRTC1 transcoders are real-time encoders, so don't expect the highest quality. We may add a slower encoder with improved quality.
	// - These enums must be kept in sync with Javascript code that calls the transcoder.
	enum class transcoder_texture_format
	{
		// Compressed formats

		// ETC1-2
		cTFETC1_RGB = 0,							// Opaque only, returns RGB or alpha data if cDecodeFlagsTranscodeAlphaDataToOpaqueFormats flag is specified
		cTFETC2_RGBA = 1,							// Opaque+alpha, ETC2_EAC_A8 block followed by a ETC1 block, alpha channel will be opaque for opaque .basis files

		// BC1-5, BC7 (desktop, some mobile devices)
		cTFBC1_RGB = 2,								// Opaque only, no punchthrough alpha support yet, transcodes alpha slice if cDecodeFlagsTranscodeAlphaDataToOpaqueFormats flag is specified
		cTFBC3_RGBA = 3, 							// Opaque+alpha, BC4 followed by a BC1 block, alpha channel will be opaque for opaque .basis files
		cTFBC4_R = 4,								// Red only, alpha slice is transcoded to output if cDecodeFlagsTranscodeAlphaDataToOpaqueFormats flag is specified
		cTFBC5_RG = 5,								// XY: Two BC4 blocks, X=R and Y=Alpha, .basis file should have alpha data (if not Y will be all 255's)
		cTFBC7_RGBA = 6,							// RGB or RGBA, mode 5 for ETC1S, modes (1,2,3,5,6,7) for UASTC

		// PVRTC1 4bpp (mobile, PowerVR devices)
		cTFPVRTC1_4_RGB = 8,						// Opaque only, RGB or alpha if cDecodeFlagsTranscodeAlphaDataToOpaqueFormats flag is specified, nearly lowest quality of any texture format.
		cTFPVRTC1_4_RGBA = 9,						// Opaque+alpha, most useful for simple opacity maps. If .basis file doesn't have alpha cTFPVRTC1_4_RGB will be used instead. Lowest quality of any supported texture format.

		// ASTC (mobile, some Intel CPU's, hopefully all desktop GPU's one day)
		cTFASTC_LDR_4x4_RGBA = 10,					// LDR. Opaque+alpha, ASTC 4x4, alpha channel will be opaque for opaque .basis files. 
													// LDR: Transcoder uses RGB/RGBA/L/LA modes, void extent, and up to two ([0,47] and [0,255]) endpoint precisions.

		// ATC (mobile, Adreno devices, this is a niche format)
		cTFATC_RGB = 11,							// Opaque, RGB or alpha if cDecodeFlagsTranscodeAlphaDataToOpaqueFormats flag is specified. ATI ATC (GL_ATC_RGB_AMD)
		cTFATC_RGBA = 12,							// Opaque+alpha, alpha channel will be opaque for opaque .basis files. ATI ATC (GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD) 

		// FXT1 (desktop, Intel devices, this is a super obscure format)
		cTFFXT1_RGB = 17,							// Opaque only, uses exclusively CC_MIXED blocks. Notable for having a 8x4 block size. GL_3DFX_texture_compression_FXT1 is supported on Intel integrated GPU's (such as HD 630).
													// Punch-through alpha is relatively easy to support, but full alpha is harder. This format is only here for completeness so opaque-only is fine for now.
													// See the BASISU_USE_ORIGINAL_3DFX_FXT1_ENCODING macro in basisu_transcoder_internal.h.

		cTFPVRTC2_4_RGB = 18,						// Opaque-only, almost BC1 quality, much faster to transcode and supports arbitrary texture dimensions (unlike PVRTC1 RGB).
		cTFPVRTC2_4_RGBA = 19,						// Opaque+alpha, slower to encode than cTFPVRTC2_4_RGB. Premultiplied alpha is highly recommended, otherwise the color channel can leak into the alpha channel on transparent blocks.

		cTFETC2_EAC_R11 = 20,						// R only (ETC2 EAC R11 unsigned)
		cTFETC2_EAC_RG11 = 21,						// RG only (ETC2 EAC RG11 unsigned), R=opaque.r, G=alpha - for tangent space normal maps

		cTFBC6H = 22,								// HDR, RGB only, unsigned
		cTFASTC_HDR_4x4_RGBA = 23,					// HDR, RGBA (currently UASTC HDR 4x4 encoders are only RGB), unsigned

		// Uncompressed (raw pixel) formats
		// Note these uncompressed formats (RGBA32, 565, and 4444) can only be transcoded to from LDR input files (ETC1S or UASTC LDR).
		cTFRGBA32 = 13,								// 32bpp RGBA image stored in raster (not block) order in memory, R is first byte, A is last byte.
		cTFRGB565 = 14,								// 16bpp RGB image stored in raster (not block) order in memory, R at bit position 11
		cTFBGR565 = 15,								// 16bpp RGB image stored in raster (not block) order in memory, R at bit position 0
		cTFRGBA4444 = 16,							// 16bpp RGBA image stored in raster (not block) order in memory, R at bit position 12, A at bit position 0
		
		// Note these uncompressed formats (HALF and 9E5) can only be transcoded to from HDR input files (UASTC HDR 4x4 or ASTC HDR 6x6).
		cTFRGB_HALF = 24,							// 48bpp RGB half (16-bits/component, 3 components)
		cTFRGBA_HALF = 25,							// 64bpp RGBA half (16-bits/component, 4 components) (A will always currently 1.0, UASTC_HDR doesn't support alpha)
		cTFRGB_9E5 = 26,							// 32bpp RGB 9E5 (shared exponent, positive only, see GL_EXT_texture_shared_exponent)

		cTFASTC_HDR_6x6_RGBA = 27,					// HDR, RGBA (currently our ASTC HDR 6x6 encodes are only RGB), unsigned

		
		// The remaining LDR ASTC block sizes, excluding 4x4 (which is above). There are 14 total valid ASTC LDR/HDR block sizes.
		cTFASTC_LDR_5x4_RGBA = 28,
		cTFASTC_LDR_5x5_RGBA = 29,
		cTFASTC_LDR_6x5_RGBA = 30,
		cTFASTC_LDR_6x6_RGBA = 31,
		cTFASTC_LDR_8x5_RGBA = 32,
		cTFASTC_LDR_8x6_RGBA = 33,
		cTFASTC_LDR_10x5_RGBA = 34,
		cTFASTC_LDR_10x6_RGBA = 35,
		cTFASTC_LDR_8x8_RGBA = 36,
		cTFASTC_LDR_10x8_RGBA = 37,
		cTFASTC_LDR_10x10_RGBA = 38,
		cTFASTC_LDR_12x10_RGBA = 39,
		cTFASTC_LDR_12x12_RGBA = 40,
				
		cTFTotalTextureFormats = 41,

		// ----- The following are old/legacy enums for compatibility with code compiled against previous versions
		cTFETC1 = cTFETC1_RGB,
		cTFETC2 = cTFETC2_RGBA,
		cTFBC1 = cTFBC1_RGB,
		cTFBC3 = cTFBC3_RGBA,
		cTFBC4 = cTFBC4_R,
		cTFBC5 = cTFBC5_RG,

		// Previously, the caller had some control over which BC7 mode the transcoder output. We've simplified this due to UASTC LDR 4x4, which supports numerous modes.
		cTFBC7_M6_RGB = cTFBC7_RGBA,				// Opaque only, RGB or alpha if cDecodeFlagsTranscodeAlphaDataToOpaqueFormats flag is specified. Highest quality of all the non-ETC1 formats.
		cTFBC7_M5_RGBA = cTFBC7_RGBA,				// Opaque+alpha, alpha channel will be opaque for opaque .basis files
		cTFBC7_M6_OPAQUE_ONLY = cTFBC7_RGBA,
		cTFBC7_M5 = cTFBC7_RGBA,
		cTFBC7_ALT = 7,

		cTFASTC_4x4 = cTFASTC_LDR_4x4_RGBA,

		cTFATC_RGBA_INTERPOLATED_ALPHA = cTFATC_RGBA,

		cTFASTC_4x4_RGBA = cTFASTC_LDR_4x4_RGBA
	};

	// For compressed texture formats, this returns the # of bytes per block. For uncompressed, it returns the # of bytes per pixel.
	// NOTE: Previously, this function was called basis_get_bytes_per_block(), and it always returned 16*bytes_per_pixel for uncompressed formats which was confusing.
	uint32_t basis_get_bytes_per_block_or_pixel(transcoder_texture_format fmt);

	// Returns the transcoder texture format's name in ASCII
	const char* basis_get_format_name(transcoder_texture_format fmt);

	// Returns basis texture format name in ASCII
	const char* basis_get_tex_format_name(basis_tex_format fmt);

	// Returns block format name in ASCII
	const char* basis_get_block_format_name(block_format fmt);

	// Returns true if the format supports an alpha channel.
	bool basis_transcoder_format_has_alpha(transcoder_texture_format fmt);

	// Returns true if the format is HDR.
	bool basis_transcoder_format_is_hdr(transcoder_texture_format fmt);

	// Returns true if the format is LDR.
	inline bool basis_transcoder_format_is_ldr(transcoder_texture_format fmt) { return !basis_transcoder_format_is_hdr(fmt); }

	// Returns true if the format is an LDR or HDR ASTC format.
	bool basis_is_transcoder_texture_format_astc(transcoder_texture_format fmt);

	// Returns the basisu::texture_format corresponding to the specified transcoder_texture_format.
	basisu::texture_format basis_get_basisu_texture_format(transcoder_texture_format fmt);

	// Returns the texture type's name in ASCII.
	const char* basis_get_texture_type_name(basis_texture_type tex_type);

	// Returns true if the transcoder texture type is an uncompressed (raw pixel) format.
	bool basis_transcoder_format_is_uncompressed(transcoder_texture_format tex_type);

	// Returns the # of bytes per pixel for uncompressed formats, or 0 for block texture formats.
	uint32_t basis_get_uncompressed_bytes_per_pixel(transcoder_texture_format fmt);

	// Returns the block width for the specified texture format, which is currently either 4 or 8 for FXT1.
	uint32_t basis_get_block_width(transcoder_texture_format fmt);

	// Returns the block height for the specified texture format, which is currently always 4.
	uint32_t basis_get_block_height(transcoder_texture_format fmt);
		
	// ASTC/XUASTC LDR formats only: Given a basis_tex_format (mode or codec), return the corresponding ASTC basisu::texture_format with the proper block size from 4x4-12x12.
	basisu::texture_format basis_get_texture_format_from_xuastc_or_astc_ldr_basis_tex_format(basis_tex_format fmt);
		
	// For any given basis_tex_format (mode or codec), return the LDR/HDR ASTC transcoder texture format with the proper block size.
	transcoder_texture_format basis_get_transcoder_texture_format_from_basis_tex_format(basis_tex_format fmt);
	// basis_get_transcoder_texture_format_from_xuastc_or_astc_ldr_basis_tex_format: same as basis_get_transcoder_texture_format_from_basis_tex_format (TODO: remove)
	transcoder_texture_format basis_get_transcoder_texture_format_from_xuastc_or_astc_ldr_basis_tex_format(basis_tex_format fmt);

	// Returns true if the specified format was enabled at compile time, and is supported for the specific basis/ktx2 texture format (ETC1S, UASTC, or UASTC HDR, or XUASTC LDR 4x4-12x12).
	// For XUASTC the ASTC block size must match the transcoder_texture_format's ASTC block size.
	bool basis_is_format_supported(transcoder_texture_format tex_type, basis_tex_format fmt = basis_tex_format::cETC1S);

	// Returns the block width/height for the specified basis texture file format.
	uint32_t basis_tex_format_get_block_width(basis_tex_format fmt);
	uint32_t basis_tex_format_get_block_height(basis_tex_format fmt);
		
	bool basis_tex_format_is_hdr(basis_tex_format fmt);
	inline bool basis_tex_format_is_ldr(basis_tex_format fmt) { return !basis_tex_format_is_hdr(fmt); }
		
	// Validates that the output buffer is large enough to hold the entire transcoded texture.
	// For uncompressed texture formats, most input parameters are in pixels, not blocks.
	bool basis_validate_output_buffer_size(transcoder_texture_format target_format,
		uint32_t output_blocks_buf_size_in_blocks_or_pixels,
		uint32_t orig_width, uint32_t orig_height,
		uint32_t output_row_pitch_in_blocks_or_pixels,
		uint32_t output_rows_in_pixels);

	// Computes the size in bytes of a transcoded image or texture, taking into account the format's block width/height and any minimum size PVRTC1 requirements required by OpenGL.
	// Note the returned value is not necessarily the # of bytes a transcoder could write to the output buffer due to these minimum PVRTC1 requirements.
	// (These PVRTC1 requirements are not ours, but OpenGL's.)
	uint32_t basis_compute_transcoded_image_size_in_bytes(transcoder_texture_format target_format, uint32_t orig_width, uint32_t orig_height);

	class basisu_transcoder;

	// This struct holds all state used during transcoding. For video, it needs to persist between image transcodes (it holds the previous frame).
	// For threading you can use one state per thread.
	struct basisu_transcoder_state
	{
		struct block_preds
		{
			uint16_t m_endpoint_index;
			uint8_t m_pred_bits;
		};

		basisu::vector<block_preds> m_block_endpoint_preds[2];

		enum { cMaxPrevFrameLevels = 16 };
		basisu::vector<uint32_t> m_prev_frame_indices[2][cMaxPrevFrameLevels]; // [alpha_flag][level_index] 

		void clear()
		{
			for (uint32_t i = 0; i < 2; i++)
			{
				m_block_endpoint_preds[i].clear();

				for (uint32_t j = 0; j < cMaxPrevFrameLevels; j++)
					m_prev_frame_indices[i][j].clear();
			}
		}
	};

	// Low-level helper classes that do the actual transcoding.
	enum basisu_decode_flags
	{
		// PVRTC1: decode non-pow2 ETC1S texture level to the next larger power of 2 (not implemented yet, but we're going to support it). Ignored if the slice's dimensions are already a power of 2.
		cDecodeFlagsPVRTCDecodeToNextPow2 = 2,

		// When decoding to an opaque texture format, if the basis file has alpha, decode the alpha slice instead of the color slice to the output texture format.
		// This is primarily to allow decoding of textures with alpha to multiple ETC1 textures (one for color, another for alpha).
		cDecodeFlagsTranscodeAlphaDataToOpaqueFormats = 4,

		// Forbid usage of BC1 3 color blocks (we don't support BC1 punchthrough alpha yet).
		// This flag is used internally when decoding to BC3.
		cDecodeFlagsBC1ForbidThreeColorBlocks = 8,

		// The output buffer contains alpha endpoint/selector indices. 
		// Used internally when decoding formats like ASTC that require both color and alpha data to be available when transcoding to the output format.
		cDecodeFlagsOutputHasAlphaIndices = 16,

		// Enable slower, but higher quality transcoding for some formats.
		// For ASTC/XUASTC->BC7, this enables partially analytical encoding vs. fully analytical.
		cDecodeFlagsHighQuality = 32,

		// Disable ETC1S->BC7 adaptive chroma filtering, for much faster transcoding to BC7.
		cDecodeFlagsNoETC1SChromaFiltering = 64,

		// Disable deblock filtering for XUASTC LDR/ASTC LDR transcoding to non-ASTC formats.
		// For block sizes smaller than 10x8 (block area < BASISU_DEBLOCKING_BLOCK_SIZE_THRESHOLD texels), deblocking is disabled by default during encoding, but it can be enabled via compressor parameters.
		// Note: The encoder writes a "DeblockFilterID" key value field into KTX2 files. By default (i.e. when neither this flag nor cDecodeFlagsForceDeblockFiltering
		// is specified), the KTX2 transcoder uses that field to decide if CPU deblocking should occur during transcoding. Specifying this flag, or
		// cDecodeFlagsForceDeblockFiltering, causes the transcoder to IGNORE the file's DeblockFilterID field and use the requested behavior instead.
		cDecodeFlagsNoDeblockFiltering = 128,

		// Always apply deblocking, even for smaller ASTC block sizes (smaller than 10x8). Overrides the KTX2 file's DeblockFilterID field (see above).
		cDecodeFlagsForceDeblockFiltering = 512,

		// By default XUASTC LDR 4x4, 6x6 and 8x6 are directly transcoded to BC7 without always requiring a full ASTC block unpack and analytical BC7 encode. This is 1.4x up to 3x faster in WASM.
		// This trade offs some quality. The largest transcoding speed gain is achieved when the source XUASTC data isn't dual plane and only uses 1 subset. Otherwise the actual perf. gain is variable.
		// To disable this optimization for all XUASTC block sizes and always use the fallback encoder, specify cDecodeFlagXUASTCLDRDisableFastBC7Transcoding.
		cDecodeFlagXUASTCLDRDisableFastBC7Transcoding = 1024
	};
	
	// ETC1S
	class basisu_lowlevel_etc1s_transcoder
	{
		friend class basisu_transcoder;

	public:
		basisu_lowlevel_etc1s_transcoder();

		void set_global_codebooks(const basisu_lowlevel_etc1s_transcoder* pGlobal_codebook) { m_pGlobal_codebook = pGlobal_codebook; }
		const basisu_lowlevel_etc1s_transcoder* get_global_codebooks() const { return m_pGlobal_codebook; }

		bool decode_palettes(
			uint32_t num_endpoints, const uint8_t* pEndpoints_data, uint32_t endpoints_data_size,
			uint32_t num_selectors, const uint8_t* pSelectors_data, uint32_t selectors_data_size);

		bool decode_tables(const uint8_t* pTable_data, uint32_t table_data_size);

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const bool is_video, const bool is_alpha_slice, const uint32_t level_index, const uint32_t orig_width, const uint32_t orig_height, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, bool astc_transcode_alpha = false, void* pAlpha_blocks = nullptr, uint32_t output_rows_in_pixels = 0, uint32_t decode_flags = 0);

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const basis_file_header& header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, bool astc_transcode_alpha = false, void* pAlpha_blocks = nullptr, uint32_t output_rows_in_pixels = 0, uint32_t decode_flags = 0)
		{
			return transcode_slice(pDst_blocks, num_blocks_x, num_blocks_y, pImage_data, image_data_size, fmt, output_block_or_pixel_stride_in_bytes, bc1_allow_threecolor_blocks,
				header.m_tex_type == cBASISTexTypeVideoFrames, (slice_desc.m_flags & cSliceDescFlagsHasAlpha) != 0, slice_desc.m_level_index,
				slice_desc.m_orig_width, slice_desc.m_orig_height, output_row_pitch_in_blocks_or_pixels, pState,
				astc_transcode_alpha,
				pAlpha_blocks,
				output_rows_in_pixels, decode_flags);
		}

		// Container independent transcoding
		bool transcode_image(
			transcoder_texture_format target_format,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			const uint8_t* pCompressed_data, uint32_t compressed_data_length,
			uint32_t num_blocks_x, uint32_t num_blocks_y, uint32_t orig_width, uint32_t orig_height, uint32_t level_index,
			uint64_t rgb_offset, uint32_t rgb_length, uint64_t alpha_offset, uint32_t alpha_length,
			uint32_t decode_flags = 0,
			bool basis_file_has_alpha_slices = false,
			bool is_video = false,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr,
			uint32_t output_rows_in_pixels = 0);

		void clear()
		{
			m_local_endpoints.clear();
			m_local_selectors.clear();
			m_endpoint_pred_model.clear();
			m_delta_endpoint_model.clear();
			m_selector_model.clear();
			m_selector_history_buf_rle_model.clear();
			m_selector_history_buf_size = 0;
		}

		// Low-level methods
		typedef basisu::vector<endpoint> endpoint_vec;
		const endpoint_vec& get_endpoints() const { return m_local_endpoints; }

		typedef basisu::vector<selector> selector_vec;
		const selector_vec& get_selectors() const { return m_local_selectors; }
				
	private:
		const basisu_lowlevel_etc1s_transcoder* m_pGlobal_codebook;

		endpoint_vec m_local_endpoints;
		selector_vec m_local_selectors;
				
		huffman_decoding_table m_endpoint_pred_model, m_delta_endpoint_model, m_selector_model, m_selector_history_buf_rle_model;

		uint32_t m_selector_history_buf_size;

		basisu_transcoder_state m_def_state;
	};
		
	// UASTC LDR 4x4
	class basisu_lowlevel_uastc_ldr_4x4_transcoder
	{
		friend class basisu_transcoder;

	public:
		basisu_lowlevel_uastc_ldr_4x4_transcoder();

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, bool has_alpha, const uint32_t orig_width, const uint32_t orig_height, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0);

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const basis_file_header& header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0)
		{
			return transcode_slice(pDst_blocks, num_blocks_x, num_blocks_y, pImage_data, image_data_size, fmt,
				output_block_or_pixel_stride_in_bytes, bc1_allow_threecolor_blocks, (header.m_flags & cBASISHeaderFlagHasAlphaSlices) != 0, slice_desc.m_orig_width, slice_desc.m_orig_height, output_row_pitch_in_blocks_or_pixels,
				pState, output_rows_in_pixels, channel0, channel1, decode_flags);
		}

		// Container independent transcoding
		bool transcode_image(
			transcoder_texture_format target_format,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			const uint8_t* pCompressed_data, uint32_t compressed_data_length,
			uint32_t num_blocks_x, uint32_t num_blocks_y, uint32_t orig_width, uint32_t orig_height, uint32_t level_index,
			uint64_t slice_offset, uint32_t slice_length,
			uint32_t decode_flags = 0,
			bool has_alpha = false,
			bool is_video = false,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr,
			uint32_t output_rows_in_pixels = 0,
			int channel0 = -1, int channel1 = -1);
	};

#if BASISD_SUPPORT_XUASTC
	// XUASTC LDR 4x4-12x12 or ASTC LDR 4x4-12x12
	struct xuastc_decoded_image
	{
		uint32_t m_actual_block_width = 0, m_actual_block_height = 0, m_actual_width = 0, m_actual_height = 0;
		bool m_actual_has_alpha = false, m_uses_srgb_astc_decode_mode = false;
				
		bool decode(const uint8_t* pImage_data, uint32_t image_data_size, 
			astc_ldr_t::xuastc_decomp_image_init_callback_ptr pInit_callback, void* pInit_callback_data,
			astc_ldr_t::xuastc_decomp_image_block_callback_ptr pBlock_callback, void* pBlock_callback_data)
		{
			const bool decomp_flag = astc_ldr_t::xuastc_ldr_decompress_image(pImage_data, image_data_size, 
				m_actual_block_width, m_actual_block_height,
				m_actual_width, m_actual_height,
				m_actual_has_alpha, m_uses_srgb_astc_decode_mode, basisu::g_debug_printf, 
				pInit_callback, pInit_callback_data, 
				pBlock_callback, pBlock_callback_data);

			return decomp_flag;
		}
				
		void clear()
		{
			m_actual_block_width = 0;
			m_actual_block_height = 0;
			m_actual_width = 0;
			m_actual_height = 0;
			m_actual_has_alpha = false;
			m_uses_srgb_astc_decode_mode = false;
		}
	};
#endif

	// This is both ASTC LDR 4x4-12x12 and XUASTC LDR 4x4-12x12.
	class basisu_lowlevel_xuastc_ldr_transcoder
	{
		friend class basisu_transcoder;

	public:
		basisu_lowlevel_xuastc_ldr_transcoder();

		bool transcode_slice(basis_tex_format src_format, bool use_astc_srgb_decode_profile, void* pDst_blocks, uint32_t src_num_blocks_x, uint32_t src_num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, bool has_alpha, const uint32_t orig_width, const uint32_t orig_height, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0);

		bool transcode_slice(basis_tex_format src_format, bool use_astc_srgb_decode_profile, void* pDst_blocks, uint32_t src_num_blocks_x, uint32_t src_num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const basis_file_header& header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0)
		{
			return transcode_slice(src_format, use_astc_srgb_decode_profile, pDst_blocks, src_num_blocks_x, src_num_blocks_y, pImage_data, image_data_size, fmt,
				output_block_or_pixel_stride_in_bytes, bc1_allow_threecolor_blocks, (header.m_flags & cBASISHeaderFlagHasAlphaSlices) != 0, slice_desc.m_orig_width, slice_desc.m_orig_height, output_row_pitch_in_blocks_or_pixels,
				pState, output_rows_in_pixels, channel0, channel1, decode_flags);
		}

		// Container independent transcoding
		bool transcode_image(
			basis_tex_format src_format, bool use_astc_srgb_decode_profile,
			transcoder_texture_format target_format,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			const uint8_t* pCompressed_data, uint32_t compressed_data_length,
			uint32_t src_num_blocks_x, uint32_t src_num_blocks_y, uint32_t orig_width, uint32_t orig_height, uint32_t level_index,
			uint64_t slice_offset, uint32_t slice_length,
			uint32_t decode_flags = 0,
			bool has_alpha = false,
			bool is_video = false,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr,
			uint32_t output_rows_in_pixels = 0,
			int channel0 = -1, int channel1 = -1);
	};

	class basisu_lowlevel_xubc7_transcoder
	{
		friend class basisu_transcoder;

	public:
		basisu_lowlevel_xubc7_transcoder();

		bool transcode_slice(basis_tex_format src_tex_format, void* pDst_blocks, uint32_t src_num_blocks_x, uint32_t src_num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, bool has_alpha, const uint32_t orig_width, const uint32_t orig_height, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0);

		bool transcode_slice(basis_tex_format src_tex_format, void* pDst_blocks, uint32_t src_num_blocks_x, uint32_t src_num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const basis_file_header& header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0)
		{
			return transcode_slice(src_tex_format, pDst_blocks, src_num_blocks_x, src_num_blocks_y, pImage_data, image_data_size, fmt,
				output_block_or_pixel_stride_in_bytes, bc1_allow_threecolor_blocks, (header.m_flags & cBASISHeaderFlagHasAlphaSlices) != 0, slice_desc.m_orig_width, slice_desc.m_orig_height, output_row_pitch_in_blocks_or_pixels,
				pState, output_rows_in_pixels, channel0, channel1, decode_flags);
		}

		// Container independent transcoding
		bool transcode_image(
			basis_tex_format src_tex_format, 
			transcoder_texture_format target_format,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			const uint8_t* pCompressed_data, uint32_t compressed_data_length,
			uint32_t src_num_blocks_x, uint32_t src_num_blocks_y, uint32_t orig_width, uint32_t orig_height, uint32_t level_index,
			uint64_t slice_offset, uint32_t slice_length,
			uint32_t decode_flags = 0,
			bool has_alpha = false,
			bool is_video = false,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr,
			uint32_t output_rows_in_pixels = 0,
			int channel0 = -1, int channel1 = -1);
	};

	// UASTC HDR 4x4
	class basisu_lowlevel_uastc_hdr_4x4_transcoder
	{
		friend class basisu_transcoder;

	public:
		basisu_lowlevel_uastc_hdr_4x4_transcoder();

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, bool has_alpha, const uint32_t orig_width, const uint32_t orig_height, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0);

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const basis_file_header& header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0)
		{
			return transcode_slice(pDst_blocks, num_blocks_x, num_blocks_y, pImage_data, image_data_size, fmt,
				output_block_or_pixel_stride_in_bytes, bc1_allow_threecolor_blocks, (header.m_flags & cBASISHeaderFlagHasAlphaSlices) != 0, slice_desc.m_orig_width, slice_desc.m_orig_height, output_row_pitch_in_blocks_or_pixels,
				pState, output_rows_in_pixels, channel0, channel1, decode_flags);
		}

		// Container independent transcoding
		bool transcode_image(
			transcoder_texture_format target_format,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			const uint8_t* pCompressed_data, uint32_t compressed_data_length,
			uint32_t num_blocks_x, uint32_t num_blocks_y, uint32_t orig_width, uint32_t orig_height, uint32_t level_index,
			uint64_t slice_offset, uint32_t slice_length,
			uint32_t decode_flags = 0,
			bool has_alpha = false,
			bool is_video = false,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr,
			uint32_t output_rows_in_pixels = 0,
			int channel0 = -1, int channel1 = -1);
	};

	// ASTC HDR 6x6
	class basisu_lowlevel_astc_hdr_6x6_transcoder
	{
		friend class basisu_transcoder;

	public:
		basisu_lowlevel_astc_hdr_6x6_transcoder();

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, bool has_alpha, const uint32_t orig_width, const uint32_t orig_height, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0);

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const basis_file_header& header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0)
		{
			return transcode_slice(pDst_blocks, num_blocks_x, num_blocks_y, pImage_data, image_data_size, fmt,
				output_block_or_pixel_stride_in_bytes, bc1_allow_threecolor_blocks, (header.m_flags & cBASISHeaderFlagHasAlphaSlices) != 0, slice_desc.m_orig_width, slice_desc.m_orig_height, output_row_pitch_in_blocks_or_pixels,
				pState, output_rows_in_pixels, channel0, channel1, decode_flags);
		}

		// Container independent transcoding
		bool transcode_image(
			transcoder_texture_format target_format,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			const uint8_t* pCompressed_data, uint32_t compressed_data_length,
			uint32_t num_blocks_x, uint32_t num_blocks_y, uint32_t orig_width, uint32_t orig_height, uint32_t level_index,
			uint64_t slice_offset, uint32_t slice_length,
			uint32_t decode_flags = 0,
			bool has_alpha = false,
			bool is_video = false,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr,
			uint32_t output_rows_in_pixels = 0,
			int channel0 = -1, int channel1 = -1);
	};

	// UASTC HDR 6x6 intermediate
	class basisu_lowlevel_uastc_hdr_6x6_intermediate_transcoder
	{
		friend class basisu_transcoder;

	public:
		basisu_lowlevel_uastc_hdr_6x6_intermediate_transcoder();

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, bool has_alpha, const uint32_t orig_width, const uint32_t orig_height, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0);

		bool transcode_slice(void* pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t* pImage_data, uint32_t image_data_size, block_format fmt,
			uint32_t output_block_or_pixel_stride_in_bytes, bool bc1_allow_threecolor_blocks, const basis_file_header& header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1, uint32_t decode_flags = 0)
		{
			return transcode_slice(pDst_blocks, num_blocks_x, num_blocks_y, pImage_data, image_data_size, fmt,
				output_block_or_pixel_stride_in_bytes, bc1_allow_threecolor_blocks, (header.m_flags & cBASISHeaderFlagHasAlphaSlices) != 0, slice_desc.m_orig_width, slice_desc.m_orig_height, output_row_pitch_in_blocks_or_pixels,
				pState, output_rows_in_pixels, channel0, channel1, decode_flags);
		}

		// Container independent transcoding
		bool transcode_image(
			transcoder_texture_format target_format,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			const uint8_t* pCompressed_data, uint32_t compressed_data_length,
			uint32_t num_blocks_x, uint32_t num_blocks_y, uint32_t orig_width, uint32_t orig_height, uint32_t level_index,
			uint64_t slice_offset, uint32_t slice_length,
			uint32_t decode_flags = 0,
			bool has_alpha = false,
			bool is_video = false,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			basisu_transcoder_state* pState = nullptr,
			uint32_t output_rows_in_pixels = 0,
			int channel0 = -1, int channel1 = -1);
	};

	struct basisu_slice_info
	{
		// The image's ACTUAL dimensions in texels.
		uint32_t m_orig_width;
		uint32_t m_orig_height;

		// The texture's dimensions in texels - always a multiple of the texture's underlying block size (4x4-12x12).
		uint32_t m_width;
		uint32_t m_height;

		uint32_t m_num_blocks_x;
		uint32_t m_num_blocks_y;
		uint32_t m_total_blocks;

		uint32_t m_block_width;
		uint32_t m_block_height;

		uint32_t m_compressed_size;

		uint32_t m_slice_index;	// the slice index in the .basis file
		uint32_t m_image_index;	// the source image index originally provided to the encoder
		uint32_t m_level_index;	// the mipmap level within this image

		uint32_t m_unpacked_slice_crc16;

		bool m_alpha_flag;		// true if the slice has alpha data
		bool m_iframe_flag;		// true if the slice is an I-Frame
	};

	typedef basisu::vector<basisu_slice_info> basisu_slice_info_vec;

	struct basisu_image_info
	{
		uint32_t m_image_index;
		uint32_t m_total_levels;

		// The image's actual/unpadded dimensions in texels.
		uint32_t m_orig_width;
		uint32_t m_orig_height;
				
		// The texture's physical/padded dimensions in texels - always a multiple of the texture's underlying block size (4x4-12x12).
		uint32_t m_width;
		uint32_t m_height;

		uint32_t m_block_width;
		uint32_t m_block_height;

		uint32_t m_num_blocks_x;
		uint32_t m_num_blocks_y;
		uint32_t m_total_blocks;

		uint32_t m_first_slice_index;

		bool m_alpha_flag;		// true if the image has alpha data
		bool m_iframe_flag;		// true if the image is an I-Frame
	};

	struct basisu_image_level_info
	{
		uint32_t m_image_index;
		uint32_t m_level_index;

		// The image's actual/unpadded dimensions in texels.
		uint32_t m_orig_width;
		uint32_t m_orig_height;

		// The texture's physical/padded dimensions in texels - always a multiple of the texture's underlying block size (4x4-12x12).
		uint32_t m_width;
		uint32_t m_height;

		uint32_t m_block_width;
		uint32_t m_block_height;

		uint32_t m_num_blocks_x;
		uint32_t m_num_blocks_y;
		uint32_t m_total_blocks;

		uint32_t m_first_slice_index;

		uint32_t m_rgb_file_ofs;
		uint32_t m_rgb_file_len;
		uint32_t m_alpha_file_ofs;
		uint32_t m_alpha_file_len;

		bool m_alpha_flag;		// true if the image has alpha data
		bool m_iframe_flag;		// true if the image is an I-Frame
	};

	struct basisu_file_info
	{
		uint32_t m_version;
		uint32_t m_total_header_size;

		uint32_t m_total_selectors;
		// will be 0 for UASTC or if the file uses global codebooks
		uint32_t m_selector_codebook_ofs;
		uint32_t m_selector_codebook_size;

		uint32_t m_total_endpoints;
		// will be 0 for UASTC or if the file uses global codebooks
		uint32_t m_endpoint_codebook_ofs;
		uint32_t m_endpoint_codebook_size;

		uint32_t m_tables_ofs;
		uint32_t m_tables_size;

		uint32_t m_slices_size;

		basis_texture_type m_tex_type;
		uint32_t m_us_per_frame;

		// Low-level slice information (1 slice per image for color-only basis files, 2 for alpha basis files)
		basisu_slice_info_vec m_slice_info;

		uint32_t m_total_images;	 // total # of images
		basisu::vector<uint32_t> m_image_mipmap_levels; // the # of mipmap levels for each image

		uint32_t m_userdata0;
		uint32_t m_userdata1;

		basis_tex_format m_tex_format; // ETC1S, UASTC, etc.

		uint32_t m_block_width;
		uint32_t m_block_height;

		bool m_y_flipped;				// true if the image was Y flipped
		bool m_srgb;					// true if the image is sRGB, false if linear
		bool m_etc1s;					// true if the file is ETC1S
		bool m_has_alpha_slices;		// true if the texture has alpha slices (for ETC1S: even slices RGB, odd slices alpha)
	};

// "x.xx" ASCII string value - always written to output .basis file
#define BASISU_LIB_VERSION_KEY_NAME "BasisULibVers"

// ASCII string numeric value - only for HDR (matches what we've used for KTX2 for a while)
#define BASISU_LDR_UPCONVERSION_SCALE_KEY_NAME "LDRUpconversionMultiplier"

// ASCII string numeric value - only for HDR (matches what we've used for KTX2 for a while)
#define BASISU_LDR_UPCONVERSION_SRGB_TO_LIN_KEY_NAME "LDRUpconversionSRGBToLinear"

// filter ID index is an ASCII string containing a single decimal integer, currently only "1" is supported
#define BASISU_DEBLOCK_FILTER_ID_NAME "DeblockFilterID"

// 8 byte struct - only for HDR - exactly matches what KTX2 uses.
#define BASISU_HDR_MAP_RANGE_KEY_NAME "KTXmapRange"
	struct basisu_map_range
	{
		basisu::packed_uint<4> m_scale;
		basisu::packed_uint<4> m_offset;
	};

	// Key value field data.
	struct key_value
	{
		// The key field is UTF8 and always zero terminated. 
		// In memory we always append a zero terminator to the key.
		basisu::uint8_vec m_key;

		// The value may be empty. In the KTX2 file it consists of raw bytes which may or may not be zero terminated. 
		// In memory we always append a zero terminator to the value.
		basisu::uint8_vec m_value;

		bool operator< (const key_value& rhs) const { return strcmp((const char*)m_key.data(), (const char*)rhs.m_key.data()) < 0; }
	};
	typedef basisu::vector<key_value> key_value_vec;

	// Replaces if the key already exists
	inline void add_key_value(key_value_vec& key_values, const std::string& key, const std::string& val)
	{
		assert(key.size());

		basist::key_value* p = nullptr;

		// Try to find an existing key
		for (size_t i = 0; i < key_values.size(); i++)
		{
			if (strcmp((const char*)key_values[i].m_key.data(), key.c_str()) == 0)
			{
				p = &key_values[i];
				break;
			}
		}

		if (!p)
			p = key_values.enlarge(1);

		p->m_key.resize(0);
		p->m_value.resize(0);

		p->m_key.resize(key.size() + 1);
		memcpy(p->m_key.data(), key.c_str(), key.size());

		p->m_value.resize(val.size() + 1);
		if (val.size())
			memcpy(p->m_value.data(), val.c_str(), val.size());
	}

	// Replaces if the key already exists
	inline void add_key_value(key_value_vec& key_values, const std::string& key, const basisu::uint8_vec &val)
	{
		assert(key.size());

		basist::key_value* p = nullptr;

		// Try to find an existing key
		for (size_t i = 0; i < key_values.size(); i++)
		{
			if (strcmp((const char*)key_values[i].m_key.data(), key.c_str()) == 0)
			{
				p = &key_values[i];
				break;
			}
		}

		if (!p)
			p = key_values.enlarge(1);

		p->m_key.resize(0);
	
		p->m_key.resize(key.size() + 1);
		memcpy(p->m_key.data(), key.c_str(), key.size());

		p->m_value = val;
	}

	// Tries to find key_name in key_values (checks all keys). Returns nullptr or pointer to key_value struct in the array.
	inline const key_value* find_key_value(const key_value_vec& key_values, const std::string& key_name)
	{
		for (uint32_t i = 0; i < key_values.size(); i++)
			if (strcmp((const char*)key_values[i].m_key.data(), key_name.c_str()) == 0)
				return &key_values[i];

		return nullptr;
	}

	// High-level transcoder class which accepts .basis file data and allows the caller to query information about the file and transcode image levels to various texture formats.
	// If you're just starting out this is the class you care about (or see the KTX2 transcoder below).
	class basisu_transcoder
	{
		basisu_transcoder(basisu_transcoder&);
		basisu_transcoder& operator= (const basisu_transcoder&);

	public:
		basisu_transcoder();

		// Validates the .basis file. This computes a crc16 over the entire file, so it's slow.
		bool validate_file_checksums(const void* pData, uint32_t data_size, bool full_validation) const;

		// Quick header validation - no crc16 checks.
		bool validate_header(const void* pData, uint32_t data_size) const;

		basis_texture_type get_texture_type(const void* pData, uint32_t data_size) const;
		bool get_userdata(const void* pData, uint32_t data_size, uint32_t& userdata0, uint32_t& userdata1) const;

		// Returns the total number of images in the basis file (always 1 or more).
		// Note that the number of mipmap levels for each image may differ, and that images may have different resolutions.
		uint32_t get_total_images(const void* pData, uint32_t data_size) const;

		basis_tex_format get_basis_tex_format(const void* pData, uint32_t data_size) const;

		// Returns the number of mipmap levels in an image.
		uint32_t get_total_image_levels(const void* pData, uint32_t data_size, uint32_t image_index) const;

		// Returns basic information about an image. Note that orig_width/orig_height may not be a multiple of 4.
		bool get_image_level_desc(const void* pData, uint32_t data_size, uint32_t image_index, uint32_t level_index, uint32_t& orig_width, uint32_t& orig_height, uint32_t& total_blocks) const;

		// Returns information about the specified image.
		bool get_image_info(const void* pData, uint32_t data_size, basisu_image_info& image_info, uint32_t image_index) const;

		// Returns information about the specified image's mipmap level.
		bool get_image_level_info(const void* pData, uint32_t data_size, basisu_image_level_info& level_info, uint32_t image_index, uint32_t level_index) const;

		// Get a description of the basis file and low-level information about each slice.
		bool get_file_info(const void* pData, uint32_t data_size, basisu_file_info& file_info) const;
				
		// Retrieves key-value data from basis file. Key-value support was added to v2.20.
		bool get_key_values(const void* pData, uint32_t data_size, key_value_vec& key_values, bool crc_checking = true) const;

		// start_transcoding() must be called before calling transcode_slice() or transcode_image_level().
		// For ETC1S files, this call decompresses the selector/endpoint codebooks, so ideally you would only call this once per .basis file (not each image/mipmap level).
		bool start_transcoding(const void* pData, uint32_t data_size);

		bool stop_transcoding();

		// Returns true if start_transcoding() has been called.
		bool get_ready_to_transcode() const { return m_ready_to_transcode; }

		// transcode_image_level() decodes a single mipmap level from the .basis file to any of the supported output texture formats.
		// It'll first find the slice(s) to transcode, then call transcode_slice() one or two times to decode both the color and alpha texture data (or RG texture data from two slices for BC5).
		// If the .basis file doesn't have alpha slices, the output alpha blocks will be set to fully opaque (all 255's).
		// Currently, to decode to PVRTC1 the basis texture's dimensions in pixels must be a power of 2, due to PVRTC1 format requirements. 
		// output_blocks_buf_size_in_blocks_or_pixels should be at least the image level's total_blocks (num_blocks_x * num_blocks_y), or the total number of output pixels if fmt==cTFRGBA32 etc.
		// output_row_pitch_in_blocks_or_pixels: Number of blocks or pixels per row. If 0, the transcoder uses the slice's num_blocks_x or orig_width (NOT num_blocks_x * 4). Ignored for PVRTC1 (due to texture swizzling).
		// output_rows_in_pixels: Ignored unless fmt is uncompressed (cRGBA32, etc.). The total number of output rows in the output buffer. If 0, the transcoder assumes the slice's orig_height (NOT num_blocks_y * 4).
		// Notes: 
		// - basisu_transcoder_init() must have been called first to initialize the transcoder lookup tables before calling this function.
		// - This method assumes the output texture buffer is readable. In some cases to handle alpha, the transcoder will write temporary data to the output texture in
		// a first pass, which will be read in a second pass.
		bool transcode_image_level(
			const void* pData, uint32_t data_size,
			uint32_t image_index, uint32_t level_index,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			transcoder_texture_format fmt,
			uint32_t decode_flags = 0, uint32_t output_row_pitch_in_blocks_or_pixels = 0, basisu_transcoder_state* pState = nullptr, uint32_t output_rows_in_pixels = 0) const;

		// Finds the basis slice corresponding to the specified image/level/alpha params, or -1 if the slice can't be found.
		int find_slice(const void* pData, uint32_t data_size, uint32_t image_index, uint32_t level_index, bool alpha_data) const;

		// transcode_slice() decodes a single slice from the .basis file. It's a low-level API - most likely you want to use transcode_image_level().
		// This is a low-level API, and will be needed to be called multiple times to decode some texture formats (like BC3, BC5, or ETC2).
		// output_blocks_buf_size_in_blocks_or_pixels is just used for verification to make sure the output buffer is large enough.
		// output_blocks_buf_size_in_blocks_or_pixels should be at least the image level's total_blocks (num_blocks_x * num_blocks_y), or the total number of output pixels if fmt==cTFRGBA32.
		// output_block_stride_in_bytes: Number of bytes between each output block.
		// output_row_pitch_in_blocks_or_pixels: Number of blocks or pixels per row. If 0, the transcoder uses the slice's num_blocks_x or orig_width (NOT num_blocks_x * 4). Ignored for PVRTC1 (due to texture swizzling).
		// output_rows_in_pixels: Ignored unless fmt is cRGBA32. The total number of output rows in the output buffer. If 0, the transcoder assumes the slice's orig_height (NOT num_blocks_y * 4).
		// Notes:
		// - basisu_transcoder_init() must have been called first to initialize the transcoder lookup tables before calling this function.
		bool transcode_slice(const void* pData, uint32_t data_size, uint32_t slice_index,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			block_format fmt, uint32_t output_block_stride_in_bytes, uint32_t decode_flags = 0, uint32_t output_row_pitch_in_blocks_or_pixels = 0, basisu_transcoder_state* pState = nullptr, void* pAlpha_blocks = nullptr,
			uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1) const;

		static void write_opaque_alpha_blocks(
			uint32_t num_blocks_x, uint32_t num_blocks_y,
			void* pOutput_blocks, block_format fmt,
			uint32_t block_stride_in_bytes, uint32_t output_row_pitch_in_blocks_or_pixels);

		void set_global_codebooks(const basisu_lowlevel_etc1s_transcoder* pGlobal_codebook) { m_lowlevel_etc1s_decoder.set_global_codebooks(pGlobal_codebook); }
		const basisu_lowlevel_etc1s_transcoder* get_global_codebooks() const { return m_lowlevel_etc1s_decoder.get_global_codebooks(); }

		const basisu_lowlevel_etc1s_transcoder& get_lowlevel_etc1s_decoder() const { return m_lowlevel_etc1s_decoder; }
		basisu_lowlevel_etc1s_transcoder& get_lowlevel_etc1s_decoder() { return m_lowlevel_etc1s_decoder; }

		const basisu_lowlevel_uastc_ldr_4x4_transcoder& get_lowlevel_uastc_decoder() const { return m_lowlevel_uastc_ldr_4x4_decoder; }
		basisu_lowlevel_uastc_ldr_4x4_transcoder& get_lowlevel_uastc_decoder() { return m_lowlevel_uastc_ldr_4x4_decoder; }

	private:
		mutable basisu_lowlevel_etc1s_transcoder m_lowlevel_etc1s_decoder;
		mutable basisu_lowlevel_uastc_ldr_4x4_transcoder m_lowlevel_uastc_ldr_4x4_decoder;
		mutable basisu_lowlevel_xuastc_ldr_transcoder m_lowlevel_xuastc_ldr_decoder;
		mutable basisu_lowlevel_xubc7_transcoder m_lowlevel_xubc7_decoder;
		mutable basisu_lowlevel_uastc_hdr_4x4_transcoder m_lowlevel_uastc_4x4_hdr_decoder;
		mutable basisu_lowlevel_astc_hdr_6x6_transcoder m_lowlevel_astc_6x6_hdr_decoder;
		mutable basisu_lowlevel_uastc_hdr_6x6_intermediate_transcoder m_lowlevel_astc_6x6_hdr_intermediate_decoder;

		bool m_ready_to_transcode;

		int find_first_slice_index(const void* pData, uint32_t data_size, uint32_t image_index, uint32_t level_index) const;

		bool validate_header_quick(const void* pData, uint32_t data_size) const;
	};

	// basisu_transcoder_init() MUST be called before a .basis file can be transcoded.
	void basisu_transcoder_init();
		
	enum debug_flags_t
	{
		cDebugFlagVisCRs = 1,
		cDebugFlagVisBC1Sels = 2,
		cDebugFlagVisBC1Endpoints = 4
	};
	uint32_t get_debug_flags();
	void set_debug_flags(uint32_t f);

	// Information about a single 2D texture "image" in a KTX2 file. (Also used by the DDS transcoder, so it is defined unconditionally, i.e. even when BASISD_SUPPORT_KTX2 is 0.)
	struct ktx2_image_level_info
	{
		// The mipmap level index (0=largest), texture array layer index, and cubemap face index of the image.
		uint32_t m_level_index;
		uint32_t m_layer_index;
		uint32_t m_face_index;

		// The image's ACTUAL (or the original source image's) width/height in pixels, which may not be divisible by the block size (4-12 pixels).
		uint32_t m_orig_width;
		uint32_t m_orig_height;

		// The image's physical width/height, which will always be divisible by the format's block size (4-12 pixels).
		uint32_t m_width;
		uint32_t m_height;

		// The texture's dimensions in 4x4-12x12 texel blocks.
		uint32_t m_num_blocks_x;
		uint32_t m_num_blocks_y;

		// The format's block width/height (4-12).
		uint32_t m_block_width;
		uint32_t m_block_height;

		// The total number of blocks
		uint32_t m_total_blocks;

		// true if the image has alpha data
		bool m_alpha_flag;

		// true if the image is an I-Frame. Currently, for ETC1S textures, the first frame will always be an I-Frame, and subsequent frames will always be P-Frames.
		bool m_iframe_flag;
	};

	// ------------------------------------------------------------------------------------------------------
	// Optional .KTX2 file format support
	// KTX2 reading optionally requires miniz or Zstd decompressors for supercompressed UASTC files.
	// ------------------------------------------------------------------------------------------------------ 
#if BASISD_SUPPORT_KTX2
#pragma pack(push)
#pragma pack(1)
	struct ktx2_header
	{
		uint8_t m_identifier[12];
		basisu::packed_uint<4> m_vk_format;
		basisu::packed_uint<4> m_type_size;
		basisu::packed_uint<4> m_pixel_width;
		basisu::packed_uint<4> m_pixel_height;
		basisu::packed_uint<4> m_pixel_depth;
		basisu::packed_uint<4> m_layer_count;
		basisu::packed_uint<4> m_face_count;
		basisu::packed_uint<4> m_level_count;
		basisu::packed_uint<4> m_supercompression_scheme;
		basisu::packed_uint<4> m_dfd_byte_offset;
		basisu::packed_uint<4> m_dfd_byte_length;
		basisu::packed_uint<4> m_kvd_byte_offset;
		basisu::packed_uint<4> m_kvd_byte_length;
		basisu::packed_uint<8> m_sgd_byte_offset;
		basisu::packed_uint<8> m_sgd_byte_length;
	};

	struct ktx2_level_index
	{
		basisu::packed_uint<8> m_byte_offset;
		basisu::packed_uint<8> m_byte_length;
		basisu::packed_uint<8> m_uncompressed_byte_length;
	};

	struct ktx2_etc1s_global_data_header
	{
		basisu::packed_uint<2> m_endpoint_count;
		basisu::packed_uint<2> m_selector_count;
		basisu::packed_uint<4> m_endpoints_byte_length;
		basisu::packed_uint<4> m_selectors_byte_length;
		basisu::packed_uint<4> m_tables_byte_length;
		basisu::packed_uint<4> m_extended_byte_length;
	};

	struct ktx2_etc1s_image_desc
	{
		basisu::packed_uint<4> m_image_flags;
		basisu::packed_uint<4> m_rgb_slice_byte_offset;
		basisu::packed_uint<4> m_rgb_slice_byte_length;
		basisu::packed_uint<4> m_alpha_slice_byte_offset;
		basisu::packed_uint<4> m_alpha_slice_byte_length;
	};

	// The initial v1.6 release (for backwards compatibility only with our older .KTX2 files)
	struct ktx2_slice_offset_len_desc_orig
	{
		basisu::packed_uint<4> m_slice_byte_offset; // byte offset relative to the KTX2 mipmap level
		basisu::packed_uint<4> m_slice_byte_length;
	};

	// The Khronos KTX2 spec standard
	struct ktx2_slice_offset_len_desc_std
	{
		basisu::packed_uint<4> m_slice_byte_offset; // byte offset relative to the KTX2 mipmap level
		basisu::packed_uint<4> m_slice_byte_length;
		basisu::packed_uint<4> m_profile; // codec specific
	};

	struct ktx2_animdata
	{
		basisu::packed_uint<4> m_duration;
		basisu::packed_uint<4> m_timescale;
		basisu::packed_uint<4> m_loopcount;
	};
#pragma pack(pop)

	const uint32_t KTX2_VK_FORMAT_UNDEFINED = 0;
	
	// These are standard Vulkan texture VkFormat ID's, see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
	const uint32_t KTX2_FORMAT_ASTC_4x4_SFLOAT_BLOCK = 1000066000;
	const uint32_t KTX2_FORMAT_ASTC_5x4_SFLOAT_BLOCK = 1000066001;
	const uint32_t KTX2_FORMAT_ASTC_5x5_SFLOAT_BLOCK = 1000066002;
	const uint32_t KTX2_FORMAT_ASTC_6x5_SFLOAT_BLOCK = 1000066003;
	const uint32_t KTX2_FORMAT_ASTC_6x6_SFLOAT_BLOCK = 1000066004;
	const uint32_t KTX2_FORMAT_ASTC_8x5_SFLOAT_BLOCK = 1000066005;
	const uint32_t KTX2_FORMAT_ASTC_8x6_SFLOAT_BLOCK = 1000066006;

	const uint32_t KTX2_FORMAT_ASTC_4x4_UNORM_BLOCK = 157, KTX2_FORMAT_ASTC_4x4_SRGB_BLOCK = 158;
	const uint32_t KTX2_FORMAT_ASTC_5x4_UNORM_BLOCK = 159, KTX2_FORMAT_ASTC_5x4_SRGB_BLOCK = 160;
	const uint32_t KTX2_FORMAT_ASTC_5x5_UNORM_BLOCK = 161, KTX2_FORMAT_ASTC_5x5_SRGB_BLOCK = 162;
	const uint32_t KTX2_FORMAT_ASTC_6x5_UNORM_BLOCK = 163, KTX2_FORMAT_ASTC_6x5_SRGB_BLOCK = 164;
	const uint32_t KTX2_FORMAT_ASTC_6x6_UNORM_BLOCK = 165, KTX2_FORMAT_ASTC_6x6_SRGB_BLOCK = 166;
	const uint32_t KTX2_FORMAT_ASTC_8x5_UNORM_BLOCK = 167, KTX2_FORMAT_ASTC_8x5_SRGB_BLOCK = 168;
	const uint32_t KTX2_FORMAT_ASTC_8x6_UNORM_BLOCK = 169, KTX2_FORMAT_ASTC_8x6_SRGB_BLOCK = 170;
	const uint32_t KTX2_FORMAT_ASTC_10x5_UNORM_BLOCK = 173, KTX2_FORMAT_ASTC_10x5_SRGB_BLOCK = 174;
	const uint32_t KTX2_FORMAT_ASTC_10x6_UNORM_BLOCK = 175, KTX2_FORMAT_ASTC_10x6_SRGB_BLOCK = 176;
	const uint32_t KTX2_FORMAT_ASTC_8x8_UNORM_BLOCK = 171, KTX2_FORMAT_ASTC_8x8_SRGB_BLOCK = 172;  // note the ASTC block size order is off in the vkFormat definitions
	const uint32_t KTX2_FORMAT_ASTC_10x8_UNORM_BLOCK = 177, KTX2_FORMAT_ASTC_10x8_SRGB_BLOCK = 178;
	const uint32_t KTX2_FORMAT_ASTC_10x10_UNORM_BLOCK = 179, KTX2_FORMAT_ASTC_10x10_SRGB_BLOCK = 180;
	const uint32_t KTX2_FORMAT_ASTC_12x10_UNORM_BLOCK = 181, KTX2_FORMAT_ASTC_12x10_SRGB_BLOCK = 182;
	const uint32_t KTX2_FORMAT_ASTC_12x12_UNORM_BLOCK = 183, KTX2_FORMAT_ASTC_12x12_SRGB_BLOCK = 184;

	const uint32_t KTX2_KDF_DF_MODEL_ASTC = 162; // 0xA2
	const uint32_t KTX2_KDF_DF_MODEL_ETC1S = 163; // 0xA3
	const uint32_t KTX2_KDF_DF_MODEL_UASTC_LDR_4X4 = 166; // 0xA6
	const uint32_t KTX2_KDF_DF_MODEL_UASTC_HDR_4X4 = 167; // 0xA7
	const uint32_t KTX2_KDF_DF_MODEL_UASTC_HDR_6X6_INTERMEDIATE = 168; // 0xA8, TODO - coordinate with Khronos on this
	const uint32_t KTX2_KDF_DF_MODEL_XUASTC_LDR_INTERMEDIATE = 169; // 0xA9, TODO - coordinate with Khronos on this
	const uint32_t KTX2_KDF_DF_MODEL_XUBC7 = 170; // 0xAA, TODO - coordinate with Khronos on this
	
	const uint32_t KTX2_IMAGE_IS_P_FRAME = 2;
	const uint32_t KTX2_UASTC_BLOCK_SIZE = 16; // also the block size for UASTC_HDR
	const uint32_t KTX2_MAX_SUPPORTED_LEVEL_COUNT = 16; // this is an implementation specific constraint and can be increased
	const uint32_t KTX2_MAX_SUPPORTED_LAYER_COUNT = 65535; // this is an implementation specific constraint and can be increased

	// The KTX2 transfer functions supported by KTX2
	const uint32_t KTX2_KHR_DF_TRANSFER_LINEAR = 1;
	const uint32_t KTX2_KHR_DF_TRANSFER_SRGB = 2;

	enum ktx2_supercompression
	{
		KTX2_SS_NONE = 0,
		KTX2_SS_BASISLZ = 1, // actually ETC1S
		KTX2_SS_ZSTANDARD = 2,
		KTX2_SS_DEFLATE = 3, // currently unsupported by us
		KTX2_SS_UASTC_HDR_6x6I = 4, // UASTC HDR 6x6i (picked by Khronos, in KTX-Software as of 2/19/2026)
		KTX2_SS_XUASTC_LDR = 5, // XUASTC LDR 4x4-12x12 (coordinate with Khronos, not in KTX-Software yet as of 2/19/2026)
		KTX2_SS_XUBC7 = 6 // XUBC7 (coordinate with Khronos, not in KTX-Software yet as of 2/19/2026)
	};

	extern const uint8_t g_ktx2_file_identifier[12];

	enum ktx2_df_channel_id
	{
		KTX2_DF_CHANNEL_ETC1S_RGB = 0U,
		KTX2_DF_CHANNEL_ETC1S_RRR = 3U,
		KTX2_DF_CHANNEL_ETC1S_GGG = 4U,
		KTX2_DF_CHANNEL_ETC1S_AAA = 15U,

		KTX2_DF_CHANNEL_UASTC_DATA = 0U,
		KTX2_DF_CHANNEL_UASTC_RGB = 0U,
		KTX2_DF_CHANNEL_UASTC_RGBA = 3U,
		KTX2_DF_CHANNEL_UASTC_RRR = 4U,
		KTX2_DF_CHANNEL_UASTC_RRRG = 5U,
		KTX2_DF_CHANNEL_UASTC_RG = 6U,
	};

	inline const char* ktx2_get_etc1s_df_channel_id_str(ktx2_df_channel_id id)
	{
		switch (id)
		{
		case KTX2_DF_CHANNEL_ETC1S_RGB: return "RGB";
		case KTX2_DF_CHANNEL_ETC1S_RRR: return "RRR";
		case KTX2_DF_CHANNEL_ETC1S_GGG: return "GGG";
		case KTX2_DF_CHANNEL_ETC1S_AAA: return "AAA";
		default: break;
		}
		return "?";
	}

	inline const char* ktx2_get_uastc_df_channel_id_str(ktx2_df_channel_id id)
	{
		switch (id)
		{
		case KTX2_DF_CHANNEL_UASTC_RGB: return "RGB";
		case KTX2_DF_CHANNEL_UASTC_RGBA: return "RGBA";
		case KTX2_DF_CHANNEL_UASTC_RRR: return "RRR";
		case KTX2_DF_CHANNEL_UASTC_RRRG: return "RRRG";
		case KTX2_DF_CHANNEL_UASTC_RG: return "RG";
		default: break;
		}
		return "?";
	}

	enum ktx2_df_color_primaries
	{
		KTX2_DF_PRIMARIES_UNSPECIFIED = 0,
		KTX2_DF_PRIMARIES_BT709 = 1,
		KTX2_DF_PRIMARIES_SRGB = 1,
		KTX2_DF_PRIMARIES_BT601_EBU = 2,
		KTX2_DF_PRIMARIES_BT601_SMPTE = 3,
		KTX2_DF_PRIMARIES_BT2020 = 4,
		KTX2_DF_PRIMARIES_CIEXYZ = 5,
		KTX2_DF_PRIMARIES_ACES = 6,
		KTX2_DF_PRIMARIES_ACESCC = 7,
		KTX2_DF_PRIMARIES_NTSC1953 = 8,
		KTX2_DF_PRIMARIES_PAL525 = 9,
		KTX2_DF_PRIMARIES_DISPLAYP3 = 10,
		KTX2_DF_PRIMARIES_ADOBERGB = 11
	};

	inline const char* ktx2_get_df_color_primaries_str(ktx2_df_color_primaries p)
	{
		switch (p)
		{
		case KTX2_DF_PRIMARIES_UNSPECIFIED: return "UNSPECIFIED";
		case KTX2_DF_PRIMARIES_BT709: return "BT709";
		case KTX2_DF_PRIMARIES_BT601_EBU: return "EBU"; 
		case KTX2_DF_PRIMARIES_BT601_SMPTE: return "SMPTE";
		case KTX2_DF_PRIMARIES_BT2020: return "BT2020";
		case KTX2_DF_PRIMARIES_CIEXYZ: return "CIEXYZ";
		case KTX2_DF_PRIMARIES_ACES: return "ACES";
		case KTX2_DF_PRIMARIES_ACESCC: return "ACESCC"; 
		case KTX2_DF_PRIMARIES_NTSC1953: return "NTSC1953";
		case KTX2_DF_PRIMARIES_PAL525: return "PAL525";
		case KTX2_DF_PRIMARIES_DISPLAYP3: return "DISPLAYP3";
		case KTX2_DF_PRIMARIES_ADOBERGB: return "ADOBERGB";
		default: break;
		}
		return "?";
	}	

	// ktx2_image_level_info is defined above, before the BASISD_SUPPORT_KTX2 block (unconditionally, since the DDS transcoder also uses it).
		
	// Thread-specific ETC1S/supercompressed UASTC transcoder state. (If you're not doing multithreading transcoding you can ignore this.)
	struct ktx2_transcoder_state
	{
		basist::basisu_transcoder_state m_transcoder_state;
		basisu::uint8_vec m_level_uncomp_data;
		int m_uncomp_data_level_index;

		void clear()
		{
			m_transcoder_state.clear();
			m_level_uncomp_data.clear();
			m_uncomp_data_level_index = -1;
		}
	};
		
	// This class is quite similar to basisu_transcoder. It treats KTX2 files as a simple container for ETC1S/UASTC texture data.
	// It does not support 1D or 3D textures.
	// It only supports 2D and cubemap textures, with or without mipmaps, texture arrays of 2D/cubemap textures, and texture video files. 
	// It only supports our codec formats: ETC1S, UASTC LDR 4x4, UASTC HDR 4x4, etc.
	// DFD (Data Format Descriptor) parsing is purposely as simple as possible. 
	// If you need to know how to interpret the texture channels you'll need to parse the DFD yourself after calling get_dfd().
	class ktx2_transcoder
	{
	public:
		ktx2_transcoder();

		// Frees all allocations, resets object.
		void clear();

		// init() parses the KTX2 header, level index array, DFD, and key values, but nothing else.
		// Importantly, it does not parse or decompress the ETC1S global supercompressed data, so some things (like which frames are I/P-Frames) won't be available until start_transcoding() is called.
		// This method holds a pointer to the file data until clear() is called.
		bool init(const void* pData, uint32_t data_size);

		// Returns the data/size passed to init().
		const uint8_t* get_data() const { return m_pData; }
		uint32_t get_data_size() const { return m_data_size; }

		// Returns the KTX2 header. Valid after init().
		const ktx2_header& get_header() const { return m_header; }

		// Returns the KTX2 level index array. There will be one entry for each mipmap level. Valid after init().
		const basisu::vector<ktx2_level_index>& get_level_index() const { return m_levels; }

		// Returns the texture's width in texels. Always non-zero, might not be divisible by the block size. Valid after init().
		uint32_t get_width() const { return m_header.m_pixel_width; }

		// Returns the texture's height in texels. Always non-zero, might not be divisible by the block size. Valid after init().
		uint32_t get_height() const { return m_header.m_pixel_height; }

		// Returns the texture's number of mipmap levels. Always returns 1 or higher. Valid after init().
		uint32_t get_levels() const { return m_header.m_level_count; }

		// Returns the number of faces. Returns 1 for 2D textures and or 6 for cubemaps. Valid after init().
		uint32_t get_faces() const { return m_header.m_face_count; }

		// Returns 0 or the number of layers in the texture array or texture video. Valid after init().
		uint32_t get_layers() const { return m_header.m_layer_count; }

		// Returns cETC1S, cUASTC4x4, cUASTC_HDR_4x4, cASTC_HDR_6x6, cUASTC_HDR_6x6_INTERMEDIATE, etc. Valid after init().
		basist::basis_tex_format get_basis_tex_format() const { return m_format; }

		// ETC1S LDR 4x4
		bool is_etc1s() const { return get_basis_tex_format() == basist::basis_tex_format::cETC1S; }

		// UASTC LDR 4x4 (only)
		bool is_uastc() const { return get_basis_tex_format() == basist::basis_tex_format::cUASTC_LDR_4x4; }
				
		// Is ASTC HDR 4x4 or 6x6
		bool is_hdr() const
		{
			return basis_tex_format_is_hdr(get_basis_tex_format());
		}

		bool is_ldr() const
		{
			return !is_hdr();
		}

		// is UASTC HDR 4x4 (which is also standard ASTC HDR 4x4 data)
		bool is_hdr_4x4() const
		{
			return (get_basis_tex_format() == basist::basis_tex_format::cUASTC_HDR_4x4);
		}

		// is ASTC HDR 6x6 or UASTC HDR 6x6 intermediate (only)
		bool is_hdr_6x6() const
		{
			return (get_basis_tex_format() == basist::basis_tex_format::cASTC_HDR_6x6) || (get_basis_tex_format() == basist::basis_tex_format::cUASTC_HDR_6x6_INTERMEDIATE);
		}
				
		// is ASTC LDR 4x4-12x12 (only)
		bool is_astc_ldr() const { return basis_tex_format_is_astc_ldr(get_basis_tex_format()); }

		// is XUASTC LDR 4x4-12x12 (only)
		bool is_xuastc_ldr() const { return basis_tex_format_is_xuastc_ldr(get_basis_tex_format()); }

		bool is_xubc7() const { return basis_tex_format_is_xubc7(get_basis_tex_format()); }

		uint32_t get_block_width() const { return basis_tex_format_get_block_width(get_basis_tex_format()); }
		uint32_t get_block_height() const { return basis_tex_format_get_block_height(get_basis_tex_format()); }

		// Returns true if the ETC1S file has two planes (typically RGBA, or RRRG), or true if the UASTC file has alpha data. Valid after init().
		uint32_t get_has_alpha() const { return m_has_alpha; }

		// Returns the entire Data Format Descriptor (DFD) from the KTX2 file. Valid after init().
		// See https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#_the_khronos_data_format_descriptor_overview
		const basisu::uint8_vec& get_dfd() const { return m_dfd; }

		// Some basic DFD accessors. Valid after init().
		uint32_t get_dfd_color_model() const { return m_dfd_color_model; }

		// Returns the DFD color primary.
		// We do not validate the color primaries, so the returned value may not be in the ktx2_df_color_primaries enum.
		ktx2_df_color_primaries get_dfd_color_primaries() const { return m_dfd_color_prims; }
		
		// Returns KTX2_KHR_DF_TRANSFER_LINEAR or KTX2_KHR_DF_TRANSFER_SRGB.
		uint32_t get_dfd_transfer_func() const { return m_dfd_transfer_func; }

		bool is_srgb() const { return (get_dfd_transfer_func() == KTX2_KHR_DF_TRANSFER_SRGB); }

		uint32_t get_dfd_flags() const { return m_dfd_flags; }

		// Returns 1 (ETC1S/UASTC) or 2 (ETC1S with an internal alpha channel).
		uint32_t get_dfd_total_samples() const { return m_dfd_samples;	}
		
		// Returns the channel mapping for each DFD "sample". UASTC always has 1 sample, ETC1S can have one or two. 
		// Note the returned value SHOULD be one of the ktx2_df_channel_id enums, but we don't validate that. 
		// It's up to the caller to decide what to do if the value isn't in the enum.
		ktx2_df_channel_id get_dfd_channel_id0() const { return m_dfd_chan0; }
		ktx2_df_channel_id get_dfd_channel_id1() const { return m_dfd_chan1; }
				
		// Returns the array of key-value entries. This may be empty. Valid after init().
		// The order of key values fields in this array exactly matches the order they were stored in the file. The keys are supposed to be sorted by their Unicode code points.
		const key_value_vec& get_key_values() const { return m_key_values; }

		const basisu::uint8_vec *find_key(const std::string& key_name) const;

		// Low-level ETC1S specific accessors

		// Returns the ETC1S global supercompression data header, which is only valid after start_transcoding() is called.
		const ktx2_etc1s_global_data_header& get_etc1s_header() const { return m_etc1s_header; }

		// Returns the array of ETC1S image descriptors, which is only valid after get_etc1s_image_descs() is called.
		const basisu::vector<ktx2_etc1s_image_desc>& get_etc1s_image_descs() const { return m_etc1s_image_descs; }

		const basisu::vector<ktx2_slice_offset_len_desc_orig>& get_slice_offset_len_descs() const { return m_slice_offset_len_descs; }

		// Must have called startTranscoding() first
		uint32_t get_etc1s_image_descs_image_flags(uint32_t level_index, uint32_t layer_index, uint32_t face_index) const;

		// is_video() is only valid after start_transcoding() is called.
		// For ETC1S data, if this returns true you must currently transcode the file from first to last frame, in order, without skipping any frames.
		bool is_video() const { return m_is_video; }
		
		// Defaults to 0, only non-zero if the key existed in the source KTX2 file.
		float get_ldr_hdr_upconversion_nit_multiplier() const { return m_ldr_hdr_upconversion_nit_multiplier; }

		// Returns the value of the deblocking filter key-index value (BASISU_DEBLOCK_FILTER_ID_NAME), or 0 if the key didn't exist.
		uint32_t get_deblocking_filter_index() const { return m_deblocking_filter_index; }
				
		// start_transcoding() MUST be called before calling transcode_image_level().
		// This method decompresses the ETC1S global endpoint/selector codebooks, which is not free, so try to avoid calling it excessively.
		bool start_transcoding();
								
		// get_image_level_info() be called after init(), but the m_iframe_flag's won't be valid until start_transcoding() is called.
		// You can call this method before calling transcode_image_level() to retrieve basic information about the mipmap level's dimensions, etc.
		bool get_image_level_info(ktx2_image_level_info& level_info, uint32_t level_index, uint32_t layer_index, uint32_t face_index) const;

		// transcode_image_level() transcodes a single 2D texture or cubemap face from the KTX2 file.
		// Internally it uses the same low-level transcode API's as basisu_transcoder::transcode_image_level().
		// If the file is UASTC and is supercompressed with Zstandard, and the file is a texture array or cubemap, it's highly recommended that each mipmap level is 
		// completely transcoded before switching to another level. Every time the mipmap level is changed all supercompressed level data must be decompressed using Zstandard as a single unit.
		// Currently ETC1S videos must always be transcoded from first to last frame (or KTX2 "layer"), in order, with no skipping of frames.
		// By default this method is not thread safe unless you specify a pointer to a user allocated thread-specific transcoder_state struct.
		bool transcode_image_level(
			uint32_t level_index, uint32_t layer_index, uint32_t face_index,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			basist::transcoder_texture_format fmt,
			uint32_t decode_flags = 0, uint32_t output_row_pitch_in_blocks_or_pixels = 0, uint32_t output_rows_in_pixels = 0, int channel0 = -1, int channel1 = -1,
			ktx2_transcoder_state *pState = nullptr);
				
	private:
		const uint8_t* m_pData;
		uint32_t m_data_size;

		ktx2_header m_header;
		basisu::vector<ktx2_level_index> m_levels;
		basisu::uint8_vec m_dfd;
		key_value_vec m_key_values;
		
		ktx2_etc1s_global_data_header m_etc1s_header;
		basisu::vector<ktx2_etc1s_image_desc> m_etc1s_image_descs;
		basisu::vector<ktx2_slice_offset_len_desc_orig> m_slice_offset_len_descs;

		basist::basis_tex_format m_format;
					
		uint32_t m_dfd_color_model;
		ktx2_df_color_primaries m_dfd_color_prims;
		
		// KTX2_KHR_DF_TRANSFER_LINEAR vs. KTX2_KHR_DF_TRANSFER_SRGB (for XUASTC LDR: which profile was used during encoding)
		uint32_t m_dfd_transfer_func; 

		uint32_t m_dfd_flags;
		uint32_t m_dfd_samples;
		ktx2_df_channel_id m_dfd_chan0, m_dfd_chan1;

		uint32_t m_deblocking_filter_index;
								
		basist::basisu_lowlevel_etc1s_transcoder m_etc1s_transcoder;
		basist::basisu_lowlevel_uastc_ldr_4x4_transcoder m_uastc_ldr_transcoder;
		basist::basisu_lowlevel_xuastc_ldr_transcoder m_xuastc_ldr_transcoder;
		basist::basisu_lowlevel_xubc7_transcoder m_xubc7_transcoder;
		basist::basisu_lowlevel_uastc_hdr_4x4_transcoder m_uastc_hdr_transcoder;
		basist::basisu_lowlevel_astc_hdr_6x6_transcoder m_astc_hdr_6x6_transcoder;
		basist::basisu_lowlevel_uastc_hdr_6x6_intermediate_transcoder m_astc_hdr_6x6_intermediate_transcoder;
				
		ktx2_transcoder_state m_def_transcoder_state;

		bool m_has_alpha;
		bool m_is_video;
		float m_ldr_hdr_upconversion_nit_multiplier;

		bool decompress_level_data(uint32_t level_index, basisu::uint8_vec& uncomp_data);
		bool read_slice_offset_len_global_data(bool read_std_structs);
		bool decompress_etc1s_global_data();
		bool read_key_values();
	};
#endif // BASISD_SUPPORT_KTX2

	// Returns true if the transcoder was compiled with KTX2 support.
	bool basisu_transcoder_supports_ktx2();

	// Returns true if the transcoder was compiled with Zstandard support.
	bool basisu_transcoder_supports_ktx2_zstd();

	// ==============================================================================================
	// Plain BC1/BC3/BC4/BC5 (DXT1/DXT5/BC4/BC5) block UNPACKERS ("bcu" == BC Unpack) and the DDS
	// reader/transcoder (dds_transcoder). Both are implemented in basisu_dds_transcoder.inl, which is
	// included at the end of basisu_transcoder.cpp.
	// ==============================================================================================

	class color_rgba; // fully defined in basisu_transcoder_internal.h

	// Plain, vendor-neutral block unpackers (used by dds_transcoder, and by the encoder's unpack_block()).
	// These let the transcoder decode plain BC1-5 source blocks with no dependency on the encoder library.
	namespace bcu
	{
		// Returns true if the block used 3-color punchthrough alpha mode (BC1 only). Pass force_4color=true for a
		// BC2/BC3 color block: those are ALWAYS decoded in 4-color mode (the color0<=color1 punchthrough switch is a
		// BC1-only feature, per the D3D/S3TC specs & DirectXTex/bcdec/Mesa).
		bool unpack_bc1(const void* pBlock_bits, color_rgba* pPixels, bool set_alpha, bool force_4color = false);
		void unpack_bc4(const void* pBlock_bits, uint8_t* pPixels, uint32_t stride);
		// BC3 = a BC4 alpha block + a BC1-layout color block decoded ALWAYS in 4-color mode (never punchthrough).
		// Always succeeds (no punchthrough/failure path), hence void.
		void unpack_bc3(const void* pBlock_bits, color_rgba* pPixels);
		// BC2 = 8 bytes explicit 4-bit-per-texel alpha + a BC1-layout color block (always 4-color, like BC3).
		void unpack_bc2(const void* pBlock_bits, color_rgba* pPixels);
		void unpack_bc5(const void* pBlock_bits, color_rgba* pPixels); // writes R,G
	} // namespace bcu

	// Standalone Microsoft DDS (DirectDraw Surface) DX9/DX10 reader + KTX2-style transcoder.
	//
	// Supported SOURCE formats (DX9 FourCC and DX10 DXGI):
	//   BC1 (DXT1), BC2 (DXT2/DXT3), BC3 (DXT4/DXT5), BC4 (ATI1/BC4U), BC5 (ATI2/DXN), BC7 (DX10 only).
	//   Uncompressed (decoded via a generic channel-mask / byte-swizzle decoder):
	//     16-bit: 565, 1555, 4444 (and the matching DX10 B5G6R5 / B5G5R5A1 / B4G4R4A4);
	//     24-bit: R8G8B8 / B8G8R8;
	//     32-bit: R8G8B8A8 / A8B8G8R8, A8R8G8B8 (BGRA, swizzled), X8R8G8B8 (BGRX -> opaque);
	//     8/16-bit byte: R8 -> (R,0,0,255), R8G8 -> (R,G,0,255), A8 -> (0,0,0,A), L8 -> (L,L,L,255), A8L8 -> (L,L,L,A).
	// Supported transcode TARGET formats (transcode_image_level / is_transcode_format_supported):
	//   ETC1, ETC2_RGBA, EAC R11/RG11, BC1, BC3, BC4, BC5, BC7, ASTC LDR 4x4, PVRTC1 4bpp (RGB/RGBA,
	//   power-of-2 only), and uncompressed RGBA32 / RGB565 / RGBA4444. (If target == the contained
	//   format and the byte layout matches, it's a passthrough copy; otherwise decode->repack.)
	//   NOTE on BC1: DECODING a BC1 source is fully supported, with or without punchthrough alpha (and a stored-BC1
	//   source passes through to a BC1 target verbatim). The limitation is ONLY real-time ENCODING to BC1: the
	//   current encoder emits opaque (4-color) blocks only -- no punchthrough alpha yet. (Planned for a future release.)
	// Supported texture types: 2D, 2D+mips, cubemap(+mips), texture array(+mips), cubemap array(+mips).
	// Rejected (clean failure at init): 1D, volume/3D, BC6H, float/other DXGI/D3DFMT, partial cubemaps.
	//   NOTE on BC2 (DXT2/DXT3): DECODE-ONLY compatibility kludge. A BC2 source decodes correctly (explicit 4-bit
	//   alpha + 4-color BC1 color), but there is no cTFBC2 in transcoder_texture_format, so get_format() reports
	//   cTFBC3_RGBA as a closest-match hint and a BC2 source NEVER passes through verbatim (BC2 alpha != BC3 alpha) --
	//   it always decode->repacks. get_dds_format() still reports the exact cBC2. No real-time encoding TO BC2.
	//
	// Note on sRGB: is_srgb() is a best-effort colorspace guess (DDS only signals sRGB on DX10/DXGI files,
	// via the _UNORM vs _UNORM_SRGB format variants; DX9 carries no signaling). Policy: assume sRGB (true)
	// UNLESS the format is known linear -> false. Known-linear = a DX10 color format explicitly stored as
	// the _UNORM (non-_SRGB) variant, or BC4/BC5 (single/two-channel data, always linear). Everything without
	// a reliable signal (all DX9 sources; DX10 16-bit color formats with no _SRGB variant) defaults to true,
	// the safer guess for the common sRGB-albedo case. Note this means the same BC format can report different
	// is_srgb() per container (DX10 BC1_UNORM=false vs DX9 DXT1=true) -- by design. See the full rationale in
	// basisu_dds_transcoder.inl (search "sRGB flag policy"). is_srgb() never affects the decoded pixel values
	// produced here -- it's a reported hint only (though callers like basisu's -unpack may propagate it into
	// the output file's transfer-function metadata).
	//
	// The API mirrors basist::ktx2_transcoder: init() -> start_transcoding() -> get_*() ->
	// get_image_level_info() -> transcode_image_level(). init() parses the header(s), detects the
	// format, and pre-computes & validates every (layer, face, level) byte offset/size against the
	// file size. The parser is hardened against corrupt/malformed/truncated input (all reads bounds
	// checked; any inconsistency -> init() returns false, no UB).

	// The EXACT low-level pixel format physically stored in a .DDS file, as reported by
	// dds_transcoder::get_dds_format(). Unlike get_format() (the closest-matching transcoder_texture_format
	// used for transcoding), this names the actual stored layout. DX9 (FourCC/D3DFMT) and DX10 (DXGI) sources
	// that store identical bytes map to the same value here. Use basisu::get_dds_format_string() for a name.
	// (Namespace-scope rather than nested in dds_transcoder so the light encoder header basisu_gpu_texture.h
	// can forward-declare it without pulling in the full transcoder header.)
	enum class dds_format
	{
		cInvalid = 0,
		// Compressed 4x4 block formats
		cBC1,			// DXT1
		cBC2,			// DXT2 / DXT3 (explicit 4-bit alpha)
		cBC3,			// DXT4 / DXT5
		cBC4,			// ATI1 / BC4U
		cBC5,			// ATI2 / BC5U / DXN / 3Dc
		cBC7,
		// Uncompressed 16-bit
		cR5G6B5,
		cA1R5G5B5,
		cX1R5G5B5,
		cA4R4G4B4,
		cX4R4G4B4,
		// Uncompressed 24-bit
		cR8G8B8,		// D3DFMT_R8G8B8 (B,G,R in memory)
		cB8G8R8,		// R,G,B in memory
		// Uncompressed 32-bit
		cA8R8G8B8,		// BGRA in memory
		cX8R8G8B8,		// BGRX in memory (opaque)
		cA8B8G8R8,		// RGBA in memory
		cX8B8G8R8,		// RGBX in memory (opaque)
		// Byte-oriented single/dual-channel + luminance/alpha. Decoded to RGBA8 with the GPU/D3D convention:
		cR8,			// DXGI R8_UNORM        -> (R,0,0,255)   (single red, like BC4)
		cR8G8,			// DXGI R8G8_UNORM      -> (R,G,0,255)   (two-channel red-green, like BC5)
		cA8,			// DXGI A8_UNORM / D3DFMT_A8 -> (0,0,0,A) (alpha only)
		cL8,			// D3DFMT_L8 (DDPF_LUMINANCE)      -> (L,L,L,255)
		cA8L8,			// D3DFMT_A8L8 (DDPF_LUMINANCE|ALPHA) -> (L,L,L,A)
		cTotalDDSFormats
	};

	// Returns true ONLY for the uncompressed dds_format values (the 16/24/32-bit channel-mask layouts).
	// Returns false for the compressed block formats (BC1/3/4/5/7), cInvalid, and the count sentinel.
	inline bool basis_is_dds_format_uncompressed(dds_format fmt)
	{
		switch (fmt)
		{
		case dds_format::cR5G6B5:
		case dds_format::cA1R5G5B5:
		case dds_format::cX1R5G5B5:
		case dds_format::cA4R4G4B4:
		case dds_format::cX4R4G4B4:
		case dds_format::cR8G8B8:
		case dds_format::cB8G8R8:
		case dds_format::cA8R8G8B8:
		case dds_format::cX8R8G8B8:
		case dds_format::cA8B8G8R8:
		case dds_format::cX8B8G8R8:
		case dds_format::cR8:
		case dds_format::cR8G8:
		case dds_format::cA8:
		case dds_format::cL8:
		case dds_format::cA8L8:
			return true;
		default:
			return false;
		}
	}

	// Per-channel decode parameters for the uncompressed path, precomputed once in dds_transcoder::init() so the per-pixel
	// decode doesn't recompute anything from the mask. m_shift is the mask's trailing-zero count and m_channel_bits is its
	// pop_count (masks are contiguous, enforced by the accepted-layout whitelist); m_channel_bits == 0 means the channel is
	// absent and decodes to opaque 255 (the generic mask path uses these two). m_byte_offset drives the byte-aligned fast
	// path: >= 0 = byte index of a clean 8-bit channel within each pixel; -1 = not byte-aligned (fall back to the generic
	// mask path); -2 = channel absent (alpha -> opaque). A 0 mask yields m_channel_bits 0 and m_byte_offset -2.
	struct dds_uncompressed_channel
	{
		uint32_t m_mask;
		uint32_t m_shift;
		uint32_t m_channel_bits;
		int m_byte_offset;
	};

	class dds_transcoder
	{
	public:
		dds_transcoder();

		void clear();

		// Parse the DDS header(s), detect/validate the format and layout, and pre-compute every
		// (layer,face,level) slice offset+size. Returns false on any malformed/unsupported input.
		// pData must remain valid for the lifetime of transcode calls (we borrow it, like ktx2_transcoder).
		bool init(const void* pData, uint32_t data_size);

		// Mirrors ktx2_transcoder::start_transcoding(). For DDS there are no global tables to unpack,
		// so this just verifies init() succeeded. Safe to call repeatedly.
		bool start_transcoding();

		bool is_valid() const { return m_init_succeeded; }

		// --- KTX2-style geometry/format accessors (valid after init) ---
		uint32_t get_width() const { return m_width; }
		uint32_t get_height() const { return m_height; }
		uint32_t get_levels() const { return m_levels; }		// mipmap levels (>=1)
		uint32_t get_layers() const { return m_layers; }		// array elements; 0 == not an array (ktx2 convention)
		uint32_t get_faces() const { return m_faces; }			// 6 == cubemap, else 1
		bool get_is_cubemap() const { return m_faces == 6; }

		// Format-level alpha presence (this is NOT a scan of the pixel data -- init() never reads texels).
		// BC2, BC3, BC7, BC1/DXT1, and any uncompressed layout with a nonzero alpha mask report 1; BC4, BC5 and the
		// opaque X8.../X.R...-style layouts report 0. NOTE on BC1/DXT1: it reports 1 because BC1 can carry per-block
		// 1-bit "punchthrough" alpha (we decode it, and DXGI treats BC1_UNORM as a 4-component format). That alpha is
		// per-block and not signaled in the header, so we report it conservatively rather than scanning every block --
		// an opaque BC1 simply decodes alpha = 255. (Return type is uint32_t to mirror ktx2_transcoder.)
		uint32_t get_has_alpha() const { return m_has_alpha; }
		bool is_srgb() const { return m_is_srgb; }

		// The format physically contained in the DDS, expressed as a transcoder_texture_format
		// (e.g. cTFBC7_RGBA, cTFBC1_RGB, cTFRGBA32). This is what a passthrough transcode emits.
		transcoder_texture_format get_format() const { return m_format; }

		// What's physically stored in the file (compressed block kind, or generic uncompressed).
		enum class source_kind
		{
			cInvalid,
			cBC1, cBC2, cBC3, cBC4, cBC5, cBC7,	// compressed 4x4 blocks
			cUncompressed					// 16- or 32-bpp, decoded via the channel masks
		};
		source_kind get_source_kind() const { return m_src_kind; }

		// The exact physical format stored in the file (more specific than get_format() / get_source_kind();
		// e.g. distinguishes A8R8G8B8 vs X8R8G8B8 vs A8B8G8R8, R5G6B5 vs A1R5G5B5, etc). Valid after init().
		dds_format get_dds_format() const { return m_dds_format; }

		// Per-(level,layer,face) info, same struct ktx2_transcoder uses.
		bool get_image_level_info(ktx2_image_level_info& level_info, uint32_t level_index, uint32_t layer_index, uint32_t face_index) const;

		// True if this DDS's contents can be transcoded to fmt (passthrough, decode-to-uncompressed,
		// or decode+repack via transcode_4x4_block).
		bool is_transcode_format_supported(transcoder_texture_format fmt) const;

		// Transcode one image (level,layer,face) to fmt. If fmt == the contained format it's a straight
		// block copy (passthrough); otherwise each 4x4 block is unpacked to RGBA and repacked to fmt.
		// pOutput_blocks / sizes / pitch semantics match ktx2_transcoder::transcode_image_level().
		// NOTE: the non-passthrough (decode->repack) path requires BASISD_SUPPORT_XUASTC; in a build with it
		// disabled, only passthrough (fmt == contained format) succeeds and every other target returns false.
		bool transcode_image_level(
			uint32_t level_index, uint32_t layer_index, uint32_t face_index,
			void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks_or_pixels,
			transcoder_texture_format fmt,
			uint32_t decode_flags = 0,
			uint32_t output_row_pitch_in_blocks_or_pixels = 0,
			uint32_t output_rows_in_pixels = 0,
			int channel0 = -1, int channel1 = -1);

		// ---- Advanced: direct access to the raw stored slice data ----
		// For callers that want to read/process the stored bytes themselves instead of going through
		// transcode_image_level(). One descriptor per physical image (level,layer,face): its byte range within the
		// file data passed to init() (also available via get_data()), plus the slice geometry.
		struct slice_desc
		{
			uint64_t m_ofs;			// byte offset into the init() data (see get_data())
			uint32_t m_size;		// byte size of this slice's stored data
			uint32_t m_width, m_height;				// original (unpadded) texel dimensions of this mip
			uint32_t m_num_blocks_x, m_num_blocks_y;	// 4x4-block grid = ceil(dim/4) (relevant for compressed sources)
			uint32_t m_row_pitch;	// uncompressed: bytes per row (>= width*bpp; honors DDSD_PITCH/DWORD padding). 0/unused for compressed.
		};

		// The borrowed file data passed to init() (valid for the transcoder's lifetime). A slice's raw bytes are
		// [get_data() + desc.m_ofs, get_data() + desc.m_ofs + desc.m_size).
		const uint8_t* get_data() const { return m_pData; }
		uint32_t get_data_size() const { return m_data_size; }

		// Number of physical image slices = (layers?:1) * faces * levels.
		uint32_t get_total_slices() const { return m_slices.size_u32(); }

		// Fetch the descriptor for one (level,layer,face). Returns false (out untouched) if not inited or any index
		// is out of range. Lets a caller locate and handle a slice's stored data directly.
		bool get_slice_desc(slice_desc& out, uint32_t level_index, uint32_t layer_index, uint32_t face_index) const;

	private:
		const uint8_t* m_pData;			// borrowed file bytes
		uint32_t m_data_size;

		bool m_init_succeeded;

		uint32_t m_width, m_height;
		uint32_t m_levels;				// mip levels, >=1
		uint32_t m_layers;				// array element count; 0 if not an array
		uint32_t m_faces;				// 6 if cubemap else 1
		bool m_has_alpha;
		bool m_is_srgb;

		transcoder_texture_format m_format;		// contained format, as a transcoder_texture_format
		uint32_t m_block_width, m_block_height;	// 4,4 for BC*, else 1,1
		uint32_t m_bytes_per_block_or_pixel;	// 8/16 for BC*; uncompressed bytes/pixel: 1 (R8/A8/L8), 2 (16bpp/R8G8), 3 (24bpp RGB), 4 (32bpp)

		source_kind m_src_kind;
		dds_format m_dds_format;		// exact physical format (set by init())

		// Uncompressed source description (valid when m_src_kind == cUncompressed). Every uncompressed
		// DX9 and DX10 format is reduced to a bit count + per-channel masks and decoded generically
		// (mask -> shift/width -> scale to 8 bits; a missing alpha mask => opaque). DX9 supplies these
		// directly; DX10 DXGI formats synthesize them.
		uint32_t m_rgb_bit_count;				// bits/pixel of the uncompressed source: 8, 16, 24, or 32
		// Per-channel mask + precomputed shift/bit-width + fast-path byte offset for the uncompressed decode, indexed
		// [0]=R [1]=G [2]=B [3]=A. All four fields are set once in init() (masks are image-constant). See dds_uncompressed_channel.
		dds_uncompressed_channel m_uncomp_channels[4];
		// True only when the uncompressed layout is exactly R8G8B8A8 in memory, i.e. a passthrough to
		// cTFRGBA32 is a straight memcpy. Any other uncompressed layout (BGRA, X8, all 16-bpp) must
		// decode->repack even to its "own" format, since there is no matching BGRA/16bpp transcoder format.
		bool m_uncompressed_is_canonical_rgba8;
		// True when the uncompressed layout is 24/32-bit with EVERY channel on a clean byte boundary (alpha may be
		// absent) -- the decode fast path reads each channel as a direct byte. Any sub-byte channel (565/1555/4444,
		// or an odd mask) clears this and forces the generic per-pixel mask decode. Computed once in init().
		bool m_uncompressed_byte_aligned;

		// Byte-swizzle decode tables: one unified model for EVERY byte-aligned uncompressed layout -- RGBA/BGRA/RGBX/
		// BGRX, 24-bit RGB/BGR, and the single/dual-channel + luminance/alpha formats R8/R8G8/A8/L8/A8L8. Each output
		// channel is built as  out[c] = (src_pixel_byte[m_swizzle[c]] & m_and_mask[c]) | m_or_mask[c]  -- i.e. a
		// passthrough source byte (and=0xFF, or=0), a forced 0 (and=0, or=0), or a forced 255 (and=0, or=0xFF); the
		// swizzle also expresses BGRA reordering and luminance replication (R=G=B all index the same byte). Built once
		// in init(); used only when m_uncompressed_byte_aligned. Sub-byte layouts (565/1555/4444) use the mask path.
		uint8_t m_swizzle[4], m_and_mask[4], m_or_mask[4];

		// One descriptor per physical image (layer*face*level). slice_desc is defined in the public section above
		// (exposed for advanced direct data access via get_slice_desc()).
		basisu::vector<slice_desc> m_slices;	// indexed by slice_index(level,layer,face)

		uint32_t slice_index(uint32_t level, uint32_t layer, uint32_t face) const
		{
			// Disk order: array element (layer) major, then face, then mip (matches the Microsoft DDS layout).
			// The public accessors validate these against the bounds before calling; the asserts catch internal misuse.
			const uint32_t eff_layers = m_layers ? m_layers : 1;
			(void)eff_layers;
			assert(level < m_levels);
			assert(layer < eff_layers);
			assert(face < m_faces);
			const uint32_t idx = (layer * m_faces + face) * m_levels + level;
			assert(idx < m_slices.size());
			return idx;
		}

		// Decodes source 4x4 block (bx,by) of a slice into 16 RGBA texels (uncompressed fast/generic paths + BC1/3/4/5/7),
		// reading m_src_kind / m_bytes_per_block_or_pixel / m_uncomp_channels from the instance. Only defined (and only
		// called) when BASISD_SUPPORT_XUASTC is enabled -- the decode->repack path that uses it requires that decoder set.
		void decode_source_block(const uint8_t* pSrc, uint32_t slice_w, uint32_t slice_h, uint32_t nbx, uint32_t row_pitch,
			uint32_t bx, uint32_t by, color32* pTexels) const;
	};

} // namespace basist

