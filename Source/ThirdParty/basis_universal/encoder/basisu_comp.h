// basisu_comp.h
// Copyright (C) 2019-2026 Binomial LLC. All Rights Reserved.
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
#pragma once
#include "basisu_frontend.h"
#include "basisu_backend.h"
#include "basisu_basis_file.h"
#include "../transcoder/basisu_transcoder.h"
#include "basisu_uastc_enc.h"
#include "basisu_uastc_hdr_4x4_enc.h"
#include "basisu_astc_hdr_6x6_enc.h"
#include "basisu_astc_ldr_encode.h"
#include "basisu_xbc7_encode.h"

#define BASISU_LIB_VERSION 250
#define BASISU_LIB_VERSION_STRING "2.50"

#ifndef BASISD_SUPPORT_KTX2
	#error BASISD_SUPPORT_KTX2 is undefined
#endif
#ifndef BASISD_SUPPORT_KTX2_ZSTD
	#error BASISD_SUPPORT_KTX2_ZSTD is undefined
#endif

#if !BASISD_SUPPORT_KTX2
	#error BASISD_SUPPORT_KTX2 must be enabled when building the encoder. To reduce code size if KTX2 support is not needed, set BASISD_SUPPORT_KTX2_ZSTD to 0
#endif

namespace basisu
{
	struct opencl_context;
	typedef opencl_context* opencl_context_ptr;
		
	// Allow block's color distance to increase by 1.5 while searching for an alternative nearby endpoint.
	const float BASISU_DEFAULT_ENDPOINT_RDO_THRESH = 1.5f; 
	
	// Allow block's color distance to increase by 1.25 while searching the selector history buffer for a close enough match.
	const float BASISU_DEFAULT_SELECTOR_RDO_THRESH = 1.25f; 

	const int BASISU_DEFAULT_QUALITY = 128;
	const float BASISU_DEFAULT_HYBRID_SEL_CB_QUALITY_THRESH = 2.0f;

	const uint32_t BASISU_MAX_IMAGE_DIMENSION = 16384;
	
	// The original ETC1S specific (non-unified) quality level
	const uint32_t BASISU_QUALITY_MIN = 1; // note 0 is also technically valid in the code/API for ETC1S; the difference in quality is tiny (both result in very small codebooks)
	const uint32_t BASISU_QUALITY_MAX = 255;
		
	const uint32_t BASISU_MAX_ENDPOINT_CLUSTERS = basisu_frontend::cMaxEndpointClusters;
	const uint32_t BASISU_MAX_SELECTOR_CLUSTERS = basisu_frontend::cMaxSelectorClusters;

	// [1,100] are also the valid unified quality levels
	const uint32_t BASISU_XUASTC_QUALITY_MIN = 1;
	const uint32_t BASISU_XUASTC_QUALITY_MAX = 100;

	const uint32_t BASISU_MAX_SLICES = 0xFFFFFF;

	const int BASISU_RDO_UASTC_DICT_SIZE_DEFAULT = 4096; // 32768;
	const int BASISU_RDO_UASTC_DICT_SIZE_MIN = 64;
	const int BASISU_RDO_UASTC_DICT_SIZE_MAX = 65536;

	const float BASISU_XUASTC_LDR_DEFAULT_SHARPEN_AMOUNT = 1.1f;

	struct image_stats
	{
		image_stats()
		{
			clear();
		}

		void clear()
		{
			m_filename.clear();
			m_width = 0;
			m_height = 0;

			m_tex_format = texture_format::cInvalidTextureFormat;

			m_basis_rgb_avg_psnr = 0.0f;
			m_basis_rgba_avg_psnr = 0.0f;
			m_basis_a_avg_psnr = 0.0f;
			m_basis_luma_709_psnr = 0.0f;
			m_basis_luma_601_psnr = 0.0f;
			m_basis_luma_709_ssim = 0.0f;

			m_basis_rgb_avg_astc_hdr_log2_psnr = 0.0f;
			m_basis_rgb_avg_bc6h_psnr = 0.0f;
			m_basis_rgb_avg_bc6h_log2_psnr = 0.0f;

			m_bc7_rgb_avg_psnr = 0.0f;
			m_bc7_rgba_avg_psnr = 0.0f;
			m_bc7_a_avg_psnr = 0.0f;
			m_bc7_luma_709_psnr = 0.0f;
			m_bc7_luma_601_psnr = 0.0f;
			m_bc7_luma_709_ssim = 0.0f;
						
			m_best_etc1s_rgb_avg_psnr = 0.0f;
			m_best_etc1s_luma_709_psnr = 0.0f;
			m_best_etc1s_luma_601_psnr = 0.0f;
			m_best_etc1s_luma_709_ssim = 0.0f;

			m_hvs_metrics.clear();
			m_hvs_metrics_bc7.clear();

			m_opencl_failed = false;
		}

		std::string m_filename;
		uint32_t m_width;
		uint32_t m_height;

		// .basis/.ktx2 metrics
		
		// The texture_format the m_basis_* stats below were computed against: the transcoded ASTC HDR format for HDR,
		// or the transcoded ETC1S/UASTC/ASTC LDR format for LDR. (For XUBC7 this is the ASTC LDR transcode format, not
		// native BC7 - see the m_bc7_* stats for native BC7.) cInvalidTextureFormat if stats weren't computed.
		texture_format m_tex_format;

		// LDR formats: Transcoded ETC1S, UASTC LDR 4x4, or ASTC LDR 4x4-12x12 statistics
		// HDR formats: Transcoded ASTC HDR statistics
		// All ASTC based - so for XUBC7, these are transcoded statistics (not native BC7, see below)
		float m_basis_rgb_avg_psnr;   // LDR/HDR
		float m_basis_rgba_avg_psnr;  // not valid in HDR
		float m_basis_a_avg_psnr;     // not valid in HDR
		float m_basis_luma_709_psnr;  // not valid in HDR
		float m_basis_luma_601_psnr;  // not valid in HDR
		float m_basis_luma_709_ssim;  // not valid in HDR
		
		// HDR formats only: Transcoded ASTC HDR and BC6H statistics
		float m_basis_rgb_avg_astc_hdr_log2_psnr;	
		float m_basis_rgb_avg_bc6h_psnr;			
		float m_basis_rgb_avg_bc6h_log2_psnr;		

		// LDR formats: Transcoded BC7 statistics (native BC7 for XUBC7)
		float m_bc7_rgb_avg_psnr;
		float m_bc7_rgba_avg_psnr;
		float m_bc7_a_avg_psnr;
		float m_bc7_luma_709_psnr;
		float m_bc7_luma_601_psnr;
		float m_bc7_luma_709_ssim;

		psnr_hvs_metrics m_hvs_metrics;
		psnr_hvs_metrics m_hvs_metrics_bc7;
		
		// ETC1S only: Highest achievable quality ETC1S statistics, for development
		float m_best_etc1s_rgb_avg_psnr;
		float m_best_etc1s_luma_709_psnr;
		float m_best_etc1s_luma_601_psnr;
		float m_best_etc1s_luma_709_ssim;

		// true if OpenCL failed during compression
		bool m_opencl_failed;
	};

	enum class hdr_modes
	{
		// standard but constrained ASTC HDR 4x4 tex data that can be rapidly transcoded to BC6H
		cUASTC_HDR_4X4, 
		// standard RDO optimized or non-RDO (highest quality) ASTC HDR 6x6 tex data that can be rapidly re-encoded to BC6H
		cASTC_HDR_6X6,
		// a custom intermediate format based off ASTC HDR that can be rapidly decoded straight to ASTC HDR or re-encoded to BC6H
		cUASTC_HDR_6X6_INTERMEDIATE,
		cTotal
	};

	enum class xuastc_ldr_sharpen_mode
	{
		cDisabled = 0, // never sharpen mipmap levels before compression
		cOnlyLargestBlocks = 1, // only sharpen 10x8 or larger block sizes
		cAllBlockSizes = 2, // sharpen all block sizes

		cTotal
	};

	enum class xuastc_ldr_deblocking_mode
	{
		cDisabled = 0,				// no SCD, no deblocking filter, write no deblocking filter ID to ktx2/.basis file
		
		cUseSCDAndFilteringOnlyLargestBlocks = 1,		// SCD+deblocking filter on 10x8 or larger block sizes (default)
		
		cUseSCDAndFilteringAllBlockSizes = 2,			// SCD+deblocking filter on all block sizes (not recommended, will overblur smaller blocks)

		cUseSCDNoFiltering = 3, // SCD enabled, but no filtering, all block sizes

		cNoSCDButEnableFilteringOnLargestBlocks = 4, // SCD disabled, but filtering enabled on larger block sizes, write a deblocking filter ID to ktx2/.basis file

		cNoSCDButEnableFilteringOnAllBlocks = 5, // SCD disabled, but filtering enabled on all block sizes, write a deblocking filter ID to ktx2/.basis file

		cTotal
	};

	enum class xuastc_ldr_astc_comp_selection
	{
		cAuto = 0,				// automatically select between cASTCF or merging cASTCF with cBasisU depending on the effort level
		cBasisU,				// always available
		cASTCENC,				// only if compiled in
		cASTCF,					// new ASTC encoder
		cBasisU_and_ASTCENC,	// merge basisu+ASTCENC when enabled
		cBasisU_and_ASTCF,		// merge basisu+astcf when possible/enabled
		cUseAll,				// merge all available encoders whenever possible/enabled
		cTotal
	};

	template<bool def>
	struct bool_param
	{
		bool_param() :
			m_value(def),
			m_changed(false)
		{
		}

		void clear()
		{
			m_value = def;
			m_changed = false;
		}

		operator bool() const
		{
			return m_value;
		}

		bool operator= (bool v)
		{
			m_value = v;
			m_changed = true;
			return m_value;
		}

		bool was_changed() const { return m_changed; }
		void set_changed(bool flag) { m_changed = flag; }

		bool m_value;
		bool m_changed;
	};

	template<typename T>
	struct param
	{
		param(T def, T min_v, T max_v) :
			m_value(def),
			m_def(def),
			m_min(min_v),
			m_max(max_v),
			m_changed(false)
		{
		}

		void clear()
		{
			m_value = m_def;
			m_changed = false;
		}

		operator T() const
		{
			return m_value;
		}

		T operator= (T v)
		{
			m_value = clamp<T>(v, m_min, m_max);
			m_changed = true;
			return m_value;
		}

		T operator *= (T v)
		{
			m_value *= v;
			m_changed = true;
			return m_value;
		}

		bool was_changed() const { return m_changed; }
		void set_changed(bool flag) { m_changed = flag; }

		T m_value;
		T m_def;
		T m_min;
		T m_max;
		bool m_changed;
	};

	// Low-level direct compressor parameters. 
	// Also see basis_compress() below for a simplified C-style interface.
	struct basis_compressor_params
	{
		friend class basis_compressor;

		basis_compressor_params() :
			m_xuastc_or_astc_ldr_basis_tex_format(-1, -1, INT_MAX),
			// Note the ETC1S default compression/effort level is 2, not the command line default of 1.
			m_etc1s_compression_level((int)BASISU_DEFAULT_ETC1S_COMPRESSION_LEVEL, 0, (int)BASISU_MAX_ETC1S_COMPRESSION_LEVEL),
			m_selector_rdo_thresh(BASISU_DEFAULT_SELECTOR_RDO_THRESH, 0.0f, 1e+10f),
			m_endpoint_rdo_thresh(BASISU_DEFAULT_ENDPOINT_RDO_THRESH, 0.0f, 1e+10f),
			m_mip_scale(1.0f, .000125f, 4.0f),
			m_mip_smallest_dimension(1, 1, 16384),
			m_etc1s_max_endpoint_clusters(0),
			m_etc1s_max_selector_clusters(0),
			m_quality_level(-1),
			m_pack_uastc_ldr_4x4_flags(cPackUASTCLevelDefault),
			m_rdo_uastc_ldr_4x4_quality_scalar(1.0f, 0.001f, 50.0f),
			m_rdo_uastc_ldr_4x4_dict_size(BASISU_RDO_UASTC_DICT_SIZE_DEFAULT, BASISU_RDO_UASTC_DICT_SIZE_MIN, BASISU_RDO_UASTC_DICT_SIZE_MAX),
			m_rdo_uastc_ldr_4x4_max_smooth_block_error_scale(UASTC_RDO_DEFAULT_SMOOTH_BLOCK_MAX_ERROR_SCALE, 1.0f, 300.0f),
			m_rdo_uastc_ldr_4x4_smooth_block_max_std_dev(UASTC_RDO_DEFAULT_MAX_SMOOTH_BLOCK_STD_DEV, .01f, 65536.0f),
			m_rdo_uastc_ldr_4x4_max_allowed_rms_increase_ratio(UASTC_RDO_DEFAULT_MAX_ALLOWED_RMS_INCREASE_RATIO, .01f, 100.0f),
			m_rdo_uastc_ldr_4x4_skip_block_rms_thresh(UASTC_RDO_DEFAULT_SKIP_BLOCK_RMS_THRESH, .01f, 100.0f),
			m_resample_width(0, 1, 16384),
			m_resample_height(0, 1, 16384),
			m_resample_factor(0.0f, .00125f, 100.0f),
			m_ktx2_uastc_supercompression(basist::KTX2_SS_NONE),
			m_ktx2_zstd_supercompression_level(6, INT_MIN, INT_MAX),
			m_transcode_flags(0, 0, UINT32_MAX),
			m_ldr_hdr_upconversion_nit_multiplier(0.0f, 0.0f, basist::MAX_HALF_FLOAT),
			m_ldr_hdr_upconversion_black_bias(0.0f, 0.0f, 1.0f),
			m_xuastc_ldr_effort_level(astc_ldr::EFFORT_LEVEL_DEF, astc_ldr::EFFORT_LEVEL_MIN, astc_ldr::EFFORT_LEVEL_MAX),
			m_xuastc_ldr_syntax((int)basist::astc_ldr_t::xuastc_ldr_syntax::cFullZStd, (int)basist::astc_ldr_t::xuastc_ldr_syntax::cFullArith, (int)basist::astc_ldr_t::xuastc_ldr_syntax::cFullZStd),
			m_xuastc_ldr_deblocking_mode((int)xuastc_ldr_deblocking_mode::cUseSCDAndFilteringOnlyLargestBlocks, (int)xuastc_ldr_deblocking_mode::cDisabled, (int)xuastc_ldr_deblocking_mode::cTotal - 1),
			m_xuastc_ldr_num_deblocking_passes(256, 2, 256), // 256=automatic depending on XUASTC LDR effort level
			m_xuastc_ldr_sharpen_mode((int)xuastc_ldr_sharpen_mode::cDisabled, (int)xuastc_ldr_sharpen_mode::cDisabled, (int)xuastc_ldr_sharpen_mode::cAllBlockSizes),
			m_xuastc_ldr_sharpen_amount(BASISU_XUASTC_LDR_DEFAULT_SHARPEN_AMOUNT, 0.0f, 10.0f),
			m_ls_min_psnr(35.0f, 0.0f, 100.0f), m_ls_min_alpha_psnr(38.0f, 0.0f, 100.0f),
			m_ls_thresh_psnr(1.5f, 0.0f, 100.0f), m_ls_thresh_alpha_psnr(0.75f, 0.0f, 100.0f),
			m_ls_thresh_edge_psnr(1.0f, 0.0f, 100.00f), m_ls_thresh_edge_alpha_psnr(0.5f, 0.0f, 100.00f),
			m_xuastc_ldr_debug_block_x(-1, INT_MIN, INT_MAX), m_xuastc_ldr_debug_block_y(-1, INT_MIN, INT_MAX),
			m_xuastc_ldr_astc_comp_selection((int)xuastc_ldr_astc_comp_selection::cAuto, (int)xuastc_ldr_astc_comp_selection::cAuto, (int)xuastc_ldr_astc_comp_selection::cTotal - 1),
			m_xubc7_effort_level(xbc7::DEFAULT_EFFORT_LEVEL, 0, 10),
			m_xubc7_rdo_level(0, 0, 100),
			m_xubc7_num_stripes(8, 1, 16),
			m_xubc7_encoder((int)xbc7::bc7_encoder_type::cBC7F, (int)xbc7::bc7_encoder_type::cBC7F, (int)xbc7::bc7_encoder_type::cBC7E_Scalar),
			m_xubc7_bc7e_scalar_level(xbc7::DEFAULT_BC7E_SCALAR_LEVEL, xbc7::BC7E_SCALAR_MIN_LEVEL, xbc7::BC7E_SCALAR_MAX_LEVEL),
			m_pJob_pool(nullptr)
		{
			clear();
		}

		void clear()
		{
			m_format_mode = basist::basis_tex_format::cETC1S;

			m_uastc.clear();
			m_hdr.clear();
			m_hdr_mode = hdr_modes::cUASTC_HDR_4X4;
			m_xuastc_or_astc_ldr_basis_tex_format = -1;

			m_use_opencl.clear();
			m_status_output.clear();

			m_source_filenames.clear();
			m_source_alpha_filenames.clear();

			m_source_images.clear();
			m_source_mipmap_images.clear();

			m_out_filename.clear();

			m_y_flip.clear();
			m_debug.clear();
			m_validate_etc1s.clear();
			m_debug_images.clear();
			m_perceptual.clear();
			m_no_selector_rdo.clear();
			m_selector_rdo_thresh.clear();
			m_read_source_images.clear();
			m_write_output_basis_or_ktx2_files.clear();
			m_etc1s_compression_level.clear();
			m_compute_stats.clear();
			m_psnr_hvs_m_stats.clear();
			m_print_stats.clear();
			m_check_for_alpha.clear();
			m_force_alpha.clear();
			m_multithreading.clear();
			m_swizzle[0] = 0;
			m_swizzle[1] = 1;
			m_swizzle[2] = 2;
			m_swizzle[3] = 3;
			m_renormalize.clear();
			m_disable_hierarchical_endpoint_codebooks.clear();

			m_no_endpoint_rdo.clear();
			m_endpoint_rdo_thresh.clear();
						
			m_mip_gen.clear();
			m_mip_scale.clear();
			m_mip_filter = "kaiser";
			m_mip_scale = 1.0f;
			m_mip_srgb.clear();
			m_mip_premultiplied.clear();
			m_mip_renormalize.clear();
			m_mip_wrapping.clear();
			m_mip_fast.clear();
			m_mip_smallest_dimension.clear();

			m_etc1s_max_endpoint_clusters = 0;
			m_etc1s_max_selector_clusters = 0;
			m_quality_level = -1;

			m_tex_type = basist::cBASISTexType2D;
			m_userdata0 = 0;
			m_userdata1 = 0;
			m_us_per_frame = 0;

			m_pack_uastc_ldr_4x4_flags = cPackUASTCLevelDefault;
			m_rdo_uastc_ldr_4x4.clear();
			m_rdo_uastc_ldr_4x4_quality_scalar.clear();
			m_rdo_uastc_ldr_4x4_max_smooth_block_error_scale.clear();
			m_rdo_uastc_ldr_4x4_smooth_block_max_std_dev.clear();
			m_rdo_uastc_ldr_4x4_max_allowed_rms_increase_ratio.clear();
			m_rdo_uastc_ldr_4x4_skip_block_rms_thresh.clear();
			m_rdo_uastc_ldr_4x4_favor_simpler_modes_in_rdo_mode.clear();
			m_rdo_uastc_ldr_4x4_multithreading.clear();

			m_resample_width.clear();
			m_resample_height.clear();
			m_resample_factor.clear();

			m_pGlobal_codebooks = nullptr;

			m_create_ktx2_file.clear();
			m_ktx2_uastc_supercompression = basist::KTX2_SS_NONE;
			m_key_values.clear();
			m_ktx2_zstd_supercompression_level.clear();
			m_ktx2_and_basis_srgb_transfer_function.clear();

			m_validate_output_data.clear();
			m_transcode_flags.clear();

			m_ldr_hdr_upconversion_srgb_to_linear.clear();

			m_hdr_favor_astc.clear();
			
			m_uastc_hdr_4x4_options.init();
			m_astc_hdr_6x6_options.clear();

			m_ldr_hdr_upconversion_nit_multiplier.clear();
			m_ldr_hdr_upconversion_black_bias.clear();

			m_xuastc_ldr_effort_level.clear();
			m_xuastc_ldr_use_dct.clear();
			m_xuastc_ldr_use_lossy_supercompression.clear();
			m_xuastc_ldr_force_disable_subsets.clear();
			m_xuastc_ldr_force_disable_rgb_dual_plane.clear();
			m_xuastc_ldr_syntax.clear();
			
			m_ls_min_psnr.clear();
			m_ls_min_alpha_psnr.clear();
			m_ls_thresh_psnr.clear();
			m_ls_thresh_alpha_psnr.clear();
			m_ls_thresh_edge_psnr.clear();
			m_ls_thresh_edge_alpha_psnr.clear();

			m_xuastc_ldr_debug_block_x.clear();
			m_xuastc_ldr_debug_block_y.clear();
			
			// 4/26/2026: The default ASTC/XUASTC/XUBC7/DDS creation LDR channel weights are now 9,11,1 - this looks a lot better for photos vs. 1,1,1.
			// The challenge with this new default: developers compressing normal maps or non-sRGB content must now override these channel weights to 1,1,1,1.
			set_ldr_srgb_channel_weights(true);

			m_xuastc_ldr_blurring.clear();
			m_xuastc_ldr_astc_comp_selection.clear();
			m_xuastc_ldr_sharpen_mode.clear();
			m_xuastc_ldr_sharpen_amount.clear();

			m_xuastc_ldr_deblocking_mode.clear();
			m_xuastc_ldr_num_deblocking_passes.clear();
			m_xuastc_ldr_heavy_subset_usage.clear();

			m_xubc7_effort_level.clear();
			m_xubc7_rdo_level.clear();
			m_xubc7_num_stripes.clear();
			m_xubc7_encoder.clear();
			m_xubc7_bc7e_scalar_level.clear();
									
			m_pJob_pool = nullptr;
		}
				
		// Configures the compressor's mode by setting the proper parameters (which were preserved for backwards compatibility with old code).
		// This is the preferred way of controlling which codec mode the compressor will select.
		void set_format_mode(basist::basis_tex_format mode)
		{
			m_format_mode = mode;

			switch (mode)
			{
			case basist::basis_tex_format::cETC1S:
			{
				// ETC1S
				m_xuastc_or_astc_ldr_basis_tex_format = -1;
				m_hdr = false;
				m_uastc = false;
				m_hdr_mode = hdr_modes::cUASTC_HDR_4X4; // doesn't matter
				break;
			}
			case basist::basis_tex_format::cUASTC_LDR_4x4:
			{
				// UASTC LDR 4x4
				m_xuastc_or_astc_ldr_basis_tex_format = -1;
				m_hdr = false;
				m_uastc = true;
				m_hdr_mode = hdr_modes::cUASTC_HDR_4X4; // doesn't matter
				break;
			}
			case basist::basis_tex_format::cUASTC_HDR_4x4:
			{
				// UASTC HDR 4x4
				m_xuastc_or_astc_ldr_basis_tex_format = -1;
				m_hdr = true;
				m_uastc = true;
				m_hdr_mode = hdr_modes::cUASTC_HDR_4X4;
				break;
			}
			case basist::basis_tex_format::cASTC_HDR_6x6:
			{
				// ASTC HDR 6x6
				m_xuastc_or_astc_ldr_basis_tex_format = -1;
				m_hdr = true;
				m_uastc = true;
				m_hdr_mode = hdr_modes::cASTC_HDR_6X6;
				break;
			}
			case basist::basis_tex_format::cUASTC_HDR_6x6_INTERMEDIATE:
			{
				// UASTC HDR 6x6
				m_xuastc_or_astc_ldr_basis_tex_format = -1;
				m_hdr = true;
				m_uastc = true;
				m_hdr_mode = hdr_modes::cUASTC_HDR_6X6_INTERMEDIATE;
				break;
			}
			case basist::basis_tex_format::cXUASTC_LDR_4x4:
			case basist::basis_tex_format::cXUASTC_LDR_5x4:
			case basist::basis_tex_format::cXUASTC_LDR_5x5:
			case basist::basis_tex_format::cXUASTC_LDR_6x5:
			case basist::basis_tex_format::cXUASTC_LDR_6x6:
			case basist::basis_tex_format::cXUASTC_LDR_8x5:
			case basist::basis_tex_format::cXUASTC_LDR_8x6:
			case basist::basis_tex_format::cXUASTC_LDR_10x5:
			case basist::basis_tex_format::cXUASTC_LDR_10x6:
			case basist::basis_tex_format::cXUASTC_LDR_8x8:
			case basist::basis_tex_format::cXUASTC_LDR_10x8:
			case basist::basis_tex_format::cXUASTC_LDR_10x10:
			case basist::basis_tex_format::cXUASTC_LDR_12x10:
			case basist::basis_tex_format::cXUASTC_LDR_12x12:
			case basist::basis_tex_format::cASTC_LDR_4x4:
			case basist::basis_tex_format::cASTC_LDR_5x4:
			case basist::basis_tex_format::cASTC_LDR_5x5:
			case basist::basis_tex_format::cASTC_LDR_6x5:
			case basist::basis_tex_format::cASTC_LDR_6x6:
			case basist::basis_tex_format::cASTC_LDR_8x5:
			case basist::basis_tex_format::cASTC_LDR_8x6:
			case basist::basis_tex_format::cASTC_LDR_10x5:
			case basist::basis_tex_format::cASTC_LDR_10x6:
			case basist::basis_tex_format::cASTC_LDR_8x8:
			case basist::basis_tex_format::cASTC_LDR_10x8:
			case basist::basis_tex_format::cASTC_LDR_10x10:
			case basist::basis_tex_format::cASTC_LDR_12x10:
			case basist::basis_tex_format::cASTC_LDR_12x12:
			{
				// ASTC LDR 4x4-12x12 or XUASTC LDR 4x4-12x12
				m_xuastc_or_astc_ldr_basis_tex_format = (int)mode;
				m_hdr = false;
				m_uastc = true;
				m_hdr_mode = hdr_modes::cUASTC_HDR_4X4; // doesn't matter
				break;
			}
			case basist::basis_tex_format::cXUBC7:
			{
				// XUBC7
				m_xuastc_or_astc_ldr_basis_tex_format = -1;
				m_hdr = false;
				m_uastc = true;
				m_hdr_mode = hdr_modes::cUASTC_HDR_4X4; // doesn't matter
				break;
			}
			default:
				assert(0);
				break;
			}
		}

		// Like set_format_mode() but also sets the effort and quality parameters appropriately for the selected mode.
		// "Effort" (perf. vs. highest achievable quality) and "quality" (quality vs. bitrate) parameters are now mode dependent.
		// Effort ranges from [0,10] and quality ranges from [1,100], unless they are -1 in which case you get the codec's default settings.
		bool set_format_mode_and_effort(basist::basis_tex_format mode, int effort = -1, bool set_defaults = true);
		bool set_format_mode_and_quality_effort(basist::basis_tex_format mode, int quality = -1, int effort = -1, bool set_defaults = true);

		// Sets all the sRGB-related options (m_perceptual, m_mip_srgb, m_ktx2_and_basis_srgb_transfer_function) to the specified value.
		// If m_perceptual is true, the encoder assumes the input is sRGB photographic-like data and optimizes for perceptual quality. If false, the encoder assumes the input is linear data and optimizes for PSNR.
		// For ASTC/XUASTC LDR and XUBC7, also see the channel weights below (m_ldr_channel_weights) and the set_ldr_srgb_channel_weights(bool srgb_flag) helper method. The channel weights default to 9,11,1,11 (perceptual).
		void set_srgb_options(bool srgb_flag)
		{
			m_perceptual = srgb_flag;
			m_mip_srgb = srgb_flag;
			m_ktx2_and_basis_srgb_transfer_function = srgb_flag;
		}
		
		// Simpler helpers - I wish this was easier, but backwards API compat is also valuable.
		bool is_etc1s() const
		{
			return !m_uastc;
		}

		bool is_uastc_ldr_4x4() const
		{
			return m_uastc && !m_hdr && (m_xuastc_or_astc_ldr_basis_tex_format == -1);
		}

		bool is_uastc_hdr_4x4() const
		{
			return m_uastc && m_hdr && (m_hdr_mode == hdr_modes::cUASTC_HDR_4X4);
		}

		bool is_xubc7() const
		{
			return m_uastc && (m_format_mode == basist::basis_tex_format::cXUBC7);
		}
												
		// By default we generate LDR ETC1S data. 
		// Ideally call set_format_mode() above instead of directly manipulating the below fields. These individual parameters are for backwards API compatibility. 
		//   - If m_uastc is false you get ETC1S (the default).
		//   - If m_format_mode==cXUBC7, we generate XUBC7 data (lossy or lossless supercompressed 8bpp)
		//   - If m_uastc is true, and m_hdr is not true, and m_xuastc_or_astc_ldr_basis_tex_format==-1, we generate UASTC 4x4 LDR data (8bpp with or without RDO). 
		//   - If m_uastc is true, and m_hdr is not true, and m_xuastc_or_astc_ldr_basis_tex_format!=-1, we generate XUASTC 4x4-12x12 or ASTC 4x4-12x12 LDR data, controlled by m_xuastc_or_astc_ldr_basis_tex_format.
		//   - If m_uastc is true and m_hdr is true, we generate 4x4 or 6x6 HDR data, controlled by m_hdr_mode.
				
		// True to generate UASTC .basis/.KTX2 file data, otherwise ETC1S.
		// Should be true for any non-ETC1S format (UASTC 4x4 LDR, UASTC 4x4 HDR, RDO ASTC 6x6 HDR, UASTC 6x6 HDR, or ASTC/XUASTC LDR 4x4-12x12).
		// Note: Ideally call set_format_mode() or set_format_mode_and_quality_effort() above instead. 
		// Many of these individual parameters are for backwards API compatibility.
		bool_param<false> m_uastc;

		// Set m_hdr to true to switch to UASTC HDR mode. m_hdr_mode then controls which format is output.
		// m_hdr_mode then controls which format is output (4x4, 6x6, or 6x6 intermediate).
		// Note: Ideally call set_format_mode() instead. This is for backwards API compatibility.
		bool_param<false> m_hdr;
				
		// If m_hdr is true, this specifies which mode we operate in (currently UASTC 4x4 HDR or ASTC 6x6 HDR). Defaults to UASTC 4x4 HDR for backwards compatibility.
		// Note: Ideally call set_format_mode() instead. This is for backwards API compatibility.
		hdr_modes m_hdr_mode;

		// If not -1: Generate XUASTC or ASTC LDR 4x4-12x12 files in the specified basis_tex_format (which also sets the ASTC block size). If -1 (the default), don't generate XUASTC/ASTC LDR files.
		// m_uastc must also be set to true if this is not -1.
		// Note: Ideally call set_format_mode() instead.
		param<int> m_xuastc_or_astc_ldr_basis_tex_format; // enum basis_tex_format
		
		// True to enable OpenCL if it's available. The compressor will fall back to CPU encoding if something goes wrong.
		bool_param<false> m_use_opencl;

		// If m_read_source_images is true, m_source_filenames (and optionally m_source_alpha_filenames) contains the filenames of PNG etc. images to read. 
		// Otherwise, the compressor processes the images in m_source_images or m_source_images_hdr.
		basisu::vector<std::string> m_source_filenames;
		basisu::vector<std::string> m_source_alpha_filenames;
		
		// An array of 2D LDR/SDR source images.
		basisu::vector<image> m_source_images;
		
		// An array of 2D HDR source images.
		basisu::vector<imagef> m_source_images_hdr;
				
		// Stores mipmaps starting from level 1. Level 0 is still stored in m_source_images, as usual.
		// If m_source_mipmaps isn't empty, automatic mipmap generation isn't done. m_source_mipmaps.size() MUST equal m_source_images.size() or the compressor returns an error.
		// The compressor applies the user-provided swizzling (in m_swizzle) to these images.
		basisu::vector< basisu::vector<image> > m_source_mipmap_images;

		basisu::vector< basisu::vector<imagef> > m_source_mipmap_images_hdr;
						
		// Filename of the output basis/ktx2 file
		std::string m_out_filename;

		// The params are done this way so we can detect when the user has explictly changed them.

		// Flip images across Y axis
		bool_param<false> m_y_flip;

		// If true, the compressor will print basis status to stdout during compression.
		bool_param<true> m_status_output;
		
		// Output debug information during compression
		bool_param<false> m_debug;
		
		// Low-level ETC1S data validation during encoding (slower/development).
		bool_param<false> m_validate_etc1s;
		
		// m_debug_images is pretty slow
		bool_param<false> m_debug_images;

		// ETC1S compression effort level, from 0 to BASISU_MAX_ETC1S_COMPRESSION_LEVEL (higher is slower). 
		// This parameter controls numerous internal encoding speed vs. compression efficiency/performance tradeoffs.
		// Note this is NOT the same as the ETC1S quality level, and most users shouldn't change this.
		param<int> m_etc1s_compression_level;
						
		// Use perceptual sRGB colorspace metrics instead of linear.
		// Note: You probably also want to set m_ktx2_srgb_transfer_func to match.
		// Prefer calling set_srgb_options() instead, so all the sRGB-related/perceptual encoding related options are set together and consistently.
		bool_param<true> m_perceptual;

		// Disable selector RDO, for faster compression but larger files
		bool_param<false> m_no_selector_rdo;
		param<float> m_selector_rdo_thresh;

		bool_param<false> m_no_endpoint_rdo;
		param<float> m_endpoint_rdo_thresh;

		// Read source images from m_source_filenames/m_source_alpha_filenames
		bool_param<false> m_read_source_images;

		// Write the output basis/ktx2 file to disk using m_out_filename
		bool_param<false> m_write_output_basis_or_ktx2_files;
								
		// Compute and display image metrics 
		bool_param<false> m_compute_stats;
		bool_param<false> m_psnr_hvs_m_stats;

		// Print stats to stdout, if m_compute_stats is true.
		bool_param<true> m_print_stats;
		
		// Check to see if any input image has an alpha channel, if so then the output basis/ktx2 file will have alpha channels. If false: all image source alpha is slammed to 255.
		bool_param<true> m_check_for_alpha;
		
		// Always put alpha slices in the output basis/ktx2 file, even when the input doesn't have alpha
		bool_param<false> m_force_alpha; 

		// True to enable multithreading in various compressors. 
		// Note currently, some compressors (like ASTC/XUASTC LDR) will utilize threading anyway if the job pool is more than one thread.
		bool_param<true> m_multithreading;
		
		// Split the R channel to RGB and the G channel to alpha, then write a basis/ktx2 file with alpha channels
		uint8_t m_swizzle[4];

		// Renormalize normal map normals after loading image
		bool_param<false> m_renormalize;

		// If true the front end will not use 2 level endpoint codebook searching, for slightly higher quality but much slower execution.
		// Note some m_etc1s_compression_level's disable this automatically.
		bool_param<false> m_disable_hierarchical_endpoint_codebooks;
						
		// mipmap generation parameters
		bool_param<false> m_mip_gen;
		param<float> m_mip_scale;
		std::string m_mip_filter;
		bool_param<true> m_mip_srgb;			// 4/26/2026: defaulting this to true to match the defaults for m_perceptual and m_ktx2_and_basis_srgb_transfer_function
		bool_param<true> m_mip_premultiplied;	// not currently supported
		bool_param<false> m_mip_renormalize; 
		bool_param<true> m_mip_wrapping;
		bool_param<true> m_mip_fast;
		param<int> m_mip_smallest_dimension;
						
		// ETC1S codebook size (quality) control. 
		// If m_quality_level (previously named m_etc1s_quality_level) != -1, it controls the quality level. It ranges from [1,255] or [BASISU_QUALITY_MIN, BASISU_QUALITY_MAX].
		// Otherwise m_max_endpoint_clusters/m_max_selector_clusters controls the codebook sizes directly.
		uint32_t m_etc1s_max_endpoint_clusters;
		uint32_t m_etc1s_max_selector_clusters;

		// Quality level (bitrate vs. distortion tradeoff) control for ETC1S or XUASTC LDR 4x4-12x12. 
		// ETC1S: Must set to [1,255] or [BASISU_QUALITY_MIN, BASISU_QUALITY_MAX] to control quality vs. bitrate. If -1 (the default!), quality is controlled by m_etc1s_max_endpoint_clusters and m_etc1s_max_selector_clusters directly.
		// XUASTC LDR: Must not be -1 for DCT.
		// XUBC7: -1 or 100=no DCT, [1,99]=DCT
		int m_quality_level; 
		
		// m_tex_type, m_userdata0, m_userdata1, m_framerate - These fields go directly into the .basis file header.
		basist::basis_texture_type m_tex_type;
		uint32_t m_userdata0;
		uint32_t m_userdata1;
		uint32_t m_us_per_frame;

		// UASTC LDR 4x4 parameters
		// cPackUASTCLevelDefault, etc.
		uint32_t m_pack_uastc_ldr_4x4_flags;
		bool_param<false> m_rdo_uastc_ldr_4x4;
		param<float> m_rdo_uastc_ldr_4x4_quality_scalar; // RDO lambda for UASTC 4x4 LDR
		param<int> m_rdo_uastc_ldr_4x4_dict_size;
		param<float> m_rdo_uastc_ldr_4x4_max_smooth_block_error_scale;
		param<float> m_rdo_uastc_ldr_4x4_smooth_block_max_std_dev;
		param<float> m_rdo_uastc_ldr_4x4_max_allowed_rms_increase_ratio;
		param<float> m_rdo_uastc_ldr_4x4_skip_block_rms_thresh;
		bool_param<true> m_rdo_uastc_ldr_4x4_favor_simpler_modes_in_rdo_mode;
		bool_param<true> m_rdo_uastc_ldr_4x4_multithreading;

		// Resample input texture after loading
		param<int> m_resample_width;
		param<int> m_resample_height;
		param<float> m_resample_factor;

		// ETC1S global codebook control
		const basist::basisu_lowlevel_etc1s_transcoder *m_pGlobal_codebooks;

		// .basis or .KTX2 key value fields.
		basist::key_value_vec m_key_values;

		// KTX2 specific parameters.
		// Internally, the compressor always creates a .basis file then it converts that losslessly to KTX2.
		bool_param<false> m_create_ktx2_file;
		basist::ktx2_supercompression m_ktx2_uastc_supercompression;
		param<int> m_ktx2_zstd_supercompression_level;
		
		// Note: The default for this parameter (which used to be "m_ktx2_srgb_transfer_func") used to be false, now setting this to true and renaming to m_ktx2_and_basis_srgb_transfer_function.
		// Also see m_perceptual and m_mip_srgb, which should in most uses be the same.
		// This also controls the XUASTC LDR ASTC decode profile (linear vs. sRGB) in the simulated decoder block. 
		// For XUASTC LDR, it's also still used when generating .basis files vs. .KTX2.
		bool_param<true> m_ktx2_and_basis_srgb_transfer_function; // false = linear transfer function, true = sRGB transfer function

		// HDR codec specific options
		uastc_hdr_4x4_codec_options m_uastc_hdr_4x4_options;
		astc_6x6_hdr::astc_hdr_6x6_global_config m_astc_hdr_6x6_options; // also UASTC HDR 6x6i

		// True to try transcoding the generated output after compression to a few formats.
		bool_param<false> m_validate_output_data;
		
		// The flags to use while transcoding if m_validate_output_data
		param<uint32_t> m_transcode_flags;

		// LDR->HDR upconversion parameters.
		// 
		// If true, LDR images (such as PNG) will be converted to normalized [0,1] linear light (via a sRGB->Linear conversion), or absolute luminance (nits or candelas per meter squared), and then processed as HDR. 
		// Otherwise, LDR images are assumed to already be in linear light (i.e. they don't use the sRGB transfer function).
		bool_param<true> m_ldr_hdr_upconversion_srgb_to_linear;
		
		// m_ldr_hdr_upconversion_nit_multiplier is only used when loading SDR/LDR images and compressing to an HDR output format.
		// By default m_ldr_hdr_upconversion_nit_multiplier is 0. It's an override for the default, which is now 100.0 nits (LDR_TO_HDR_NITS).
		// UASTC HDR 4x4: The default multiplier of 1.0 was previously used in this codec's original release. Note this encoder isn't dependent on absolute nits, unlike the ASTC 6x6 HDR encoder.
		// RDO ASTC HDR 6x6/UASTC HDR 6x6i: These encoders expect inputs in absolute nits, so the LDR upconversion luminance multiplier default will be 100 nits. (Most SDR monitors were/are 80-100 nits or so.)
		param<float> m_ldr_hdr_upconversion_nit_multiplier;

		// The optional sRGB space bias to use during LDR->HDR upconversion. Should be between [0,.49] or so. Only applied on black (0.0) color components.
		// Defaults to no bias (0.0f).
		param<float> m_ldr_hdr_upconversion_black_bias;

		// If true, ASTC HDR quality is favored more than BC6H quality by the dual target encoder. Otherwise it's a rough balance.
		// UASTC HDR 4x4
		bool_param<false> m_hdr_favor_astc;

		// XUASTC LDR 4x4-12x12 specific options
		
		// XUASTC LDR specific effort level. Prefer calling set_format_mode_and_effort() or set_format_mode_and_quality_effort() instead of setting this directly.
		param<int> m_xuastc_ldr_effort_level;
						
		// Enable or disable Weight Grid DCT. Set the DCT quality above using m_quality_level: [1,100]
		// Prefer calling set_format_mode_and_effort() or set_format_mode_and_quality_effort() instead of setting this directly.
		bool_param<false> m_xuastc_ldr_use_dct; 
		
		// Allow the compressor to introduce a bounded amount of distortion if doing so would make smaller files (actually ASTC or XUASTC).
		// Prefer calling set_format_mode_and_effort() or set_format_mode_and_quality_effort() instead of setting this directly.
		bool_param<false> m_xuastc_ldr_use_lossy_supercompression; 
		
		// Disable 2-3 subset usage in all effort levels, faster encoding, faster transcoding to BC7, but lower quality).
		bool_param<false> m_xuastc_ldr_force_disable_subsets; 
		
		// Disable RGB dual plane usage (still can use dual plane on alpha blocks), for faster transcoding to BC7 but lower quality.
		bool_param<false> m_xuastc_ldr_force_disable_rgb_dual_plane; 

		// Entropy coding syntax: Default is basist::astc_ldr_t::xuastc_ldr_syntax::cFullZStd (fastest transcoding but lower ratio).
		param<int> m_xuastc_ldr_syntax; 

		// ASTC/XUASTC/XUBC7 weights - these per-channel weights are also used when encoding .DDS files
		// Each component channel weight must be >= 1 (no 0 weights allowed). 
		// Important: Default channel weights are 9,11,1,11. 
		// For best photo quality, especially on the largest block sizes, the RGB weights should be set to roughly 9,11,1, and alpha set to ~G, so 11.
		// For non-photographic, non-sRGB or linear (normal map) content, this should be set to 1,1,1,1.
		uint32_t m_ldr_channel_weights[4]; 

		// Set ASTC/XUASTC/XUBC7 LDR linear or Rec 709-like channel weights. On larger ASTC/XUASTC LDR block sizes, 709-like weights make a noticeable difference in quality.
		void set_ldr_srgb_channel_weights(bool srgb_flag)
		{
			if (srgb_flag)
			{
				// 9,11,1,11
				m_ldr_channel_weights[0] = 9;
				m_ldr_channel_weights[1] = 11;
				m_ldr_channel_weights[2] = 1;
				m_ldr_channel_weights[3] = 11;
			}
			else
			{
				m_ldr_channel_weights[0] = 1;
				m_ldr_channel_weights[1] = 1;
				m_ldr_channel_weights[2] = 1;
				m_ldr_channel_weights[3] = 1;
			}
		}
		
		// Previous method to set the channel weights for ASTC/XUASTC - but now these channel weights drive other codecs like XUBC7, too.
		void set_xuastc_ldr_srgb_channel_weights(bool srgb_flag)
		{
			set_ldr_srgb_channel_weights(srgb_flag);
		}

		// Enable prefiltering (slight H and/or V axis blurring) during encoding: much slower, but higher quality especially on larger block sizes with DCT enabled.
		bool_param<false> m_xuastc_ldr_blurring; 
				
		// See xuastc_ldr_deblocking_mode - controls SCD (stochastic coordinate descent) and whether or not a deblocking filter is applied during encoding
		param<int> m_xuastc_ldr_deblocking_mode;
				
		// Controls the # of SCD passes [2,256], 256=automatic depending on effort (the default)
		param<uint32_t> m_xuastc_ldr_num_deblocking_passes;

		// If true, low DCT quality factor quality is greatly improved - at the cost of slower encoding and higher bitrate. Experimental.
		bool_param<false> m_xuastc_ldr_heavy_subset_usage;
				
		// Sharpening mode and amount (somewhat experimental). Sharpening uses a Difference of Gaussians based unsharp masking approach to ALL levels including mip 0 before any compression.
		// Note the encoder's internal metrics are computed against the post-sharpened images, but the top-level compressor stats (m_compute_stats) compare against the original unsharpened source images, so reported PSNR will read lower when sharpening is enabled.
		// By default sharpening is disabled.
		param<int> m_xuastc_ldr_sharpen_mode; // enum class xuastc_ldr_sharpen_mode, default is cDisabled
		param<float> m_xuastc_ldr_sharpen_amount; // defaults to BASISU_XUASTC_LDR_DEFAULT_SHARPEN_AMOUNT, or 1.1 - the higher, the more sharpening, but also the more likely to introduce artifacts.
						
		// XUASTC LDR: Lossy supercompression PSNR threshold parameters
		param<float> m_ls_min_psnr, m_ls_min_alpha_psnr;
		param<float> m_ls_thresh_psnr, m_ls_thresh_alpha_psnr;
		param<float> m_ls_thresh_edge_psnr, m_ls_thresh_edge_alpha_psnr;

		param<int> m_xuastc_ldr_debug_block_x, m_xuastc_ldr_debug_block_y;

		// Selects which ASTC LDR block encoder(s) to use (basisu, astcf, or merged combinations - all fully supported).
		// ASTCENC options: we have an experimental fork of astcenc that we optionally support as a candidate generator, but it's not released yet.
		// Not usable unless the lib is compiled in/enabled via BASISU_SUPPORT_ASTCENC. ASTCENC has known quality issues with alpha blocks as of 4/24/2026.
		param<int> m_xuastc_ldr_astc_comp_selection; // enum class xuastc_ldr_astc_comp_selection

		// XUBC7
		param<int> m_xubc7_effort_level; // [0,10]
		param<int> m_xubc7_rdo_level; // [0,100], 0=no RDO
		param<int> m_xubc7_num_stripes; // [1,16], desired # of encode stripes (decode parallelism vs size)
		param<int> m_xubc7_encoder; // xbc7::bc7_encoder_type: 0=bc7f (default), 1=bc7e_scalar
		param<int> m_xubc7_bc7e_scalar_level; // bc7e_scalar quality level, clamped to [BC7E_SCALAR_MIN_LEVEL, BC7E_SCALAR_MAX_LEVEL]
																		
		// Job pool, MUST not be nullptr;
		job_pool *m_pJob_pool;

		// Returns the current format mode as set by set_format_mode() above.
		// Because of backwards API compatibility we don't use this directly yet, it's just here to aid the transition to the new API.
		basist::basis_tex_format get_format_mode() const { return m_format_mode; }

	private:
		// This is set by set_format_mode() above. For backwards API compat we don't use it directly, it's just here to aid the transition to the new API.
		basist::basis_tex_format m_format_mode;
	};

	// Important: basisu_encoder_init() MUST be called first before using this class.
	class basis_compressor
	{
		BASISU_NO_EQUALS_OR_COPY_CONSTRUCT(basis_compressor);

	public:
		basis_compressor();
		~basis_compressor();

		// Note it *should* be possible to call init() multiple times with different inputs, but this scenario isn't well tested. Ideally, create 1 object, compress, then delete it.
		bool init(const basis_compressor_params &params);
		
		enum error_code
		{
			cECSuccess = 0,
			cECFailedInitializing,
			cECFailedReadingSourceImages,
			cECFailedValidating,
			cECFailedEncodeUASTC,
			cECFailedFrontEnd,
			cECFailedFrontendExtract,
			cECFailedBackend,
			cECFailedCreateBasisFile,
			cECFailedWritingOutput,
			cECFailedUASTCRDOPostProcess,
			cECFailedCreateKTX2File,
			cECFailedInvalidParameters
		};

		error_code process();

		// Special-purpose alternative to process(): runs ONLY the source image loading/preparation prefix of the
		// pipeline (read DDS sources, pick the format mode, read/prepare source images incl. mipmap generation and
		// texture array/cubemap layout, then validate), and STOPS before any block extraction, encoding, or file output.
		// init() MUST have been called first (identical contract to process()). To control LDR vs HDR preparation, set
		// the format mode in the params BEFORE init() via set_format_mode(): use set_format_mode(cXUBC7) for the LDR path
		// or set_format_mode(cUASTC_HDR_4x4) for the HDR path, so the existing format-driven loading/preparation code
		// does something sane on this alternative path. After success, inspect the prepared slices/metadata via the
		// getters below (get_slice_images()/get_slice_images_hdr()/get_slice_descs()/get_params()/etc.).
		// NOTE: get_params() reflects the params AFTER pick_format_mode() has resolved/normalized them (e.g. m_hdr,
		// m_uastc, the picked format), not necessarily what was passed to init(); use get_fmt_mode() for the resolved format.
		// Like process(), this may only be called once per init() (see m_has_been_processed); to retry after a failure, call init() again.
		error_code process_source_images();

		// The output .basis file will always be valid of process() succeeded.
		const uint8_vec &get_output_basis_file() const { return m_output_basis_file; }
		
		// The output .ktx2 file will only be valid if m_create_ktx2_file was true and process() succeeded.
		const uint8_vec& get_output_ktx2_file() const { return m_output_ktx2_file; }

		const basisu::vector<image_stats> &get_stats() const { return m_stats; }

		// Sum of all slice orig pixels. Intended for statistics display.
		uint64_t get_total_slice_orig_texels() const { return m_total_slice_orig_texels; }

		uint64_t get_basis_file_size() const { return m_basis_file_size; }
		double get_basis_bits_per_texel() const { return m_basis_bits_per_texel; }

		uint64_t get_ktx2_file_size() const { return m_ktx2_file_size; }
		double get_ktx2_bits_per_texel() const { return m_ktx2_bits_per_texel; }
		
		bool get_any_source_image_has_alpha() const { return m_any_source_image_has_alpha; }

		bool get_opencl_failed() const { return m_opencl_failed; }

		// Accessors for the prepared source image data. These are valid after a successful process_source_images()
		// (or process()) call, and remain valid for the lifetime of this object. The slice images are expanded if
		// necessary (duplicating cols/rows) to account for block dimensions.
		const basisu::vector<image> &get_slice_images() const { return m_slice_images; }
		const basisu::vector<imagef> &get_slice_images_hdr() const { return m_slice_images_hdr; }
		const basisu_backend_slice_desc_vec &get_slice_descs() const { return m_slice_descs; }
		const basis_compressor_params &get_params() const { return m_params; }
		// Finalized during HDR source image reading (only changed from the default 1.0f when the HDR input had to be
		// scaled down to fit into half floats); stays 1.0f on the LDR path.
		float get_hdr_image_scale() const { return m_hdr_image_scale; }
		float get_ldr_to_hdr_upconversion_nit_multiplier() const { return m_ldr_to_hdr_upconversion_nit_multiplier; }
		bool get_upconverted_any_ldr_images() const { return m_upconverted_any_ldr_images; }

		// The resolved output format mode and its block dimensions, as chosen by pick_format_mode() (these reflect the
		// actual codec/format that the source images were prepared for, which may differ from get_params().get_format_mode()).
		basist::basis_tex_format get_fmt_mode() const { return m_fmt_mode; }
		uint32_t get_fmt_mode_block_width() const { return m_fmt_mode_block_width; }
		uint32_t get_fmt_mode_block_height() const { return m_fmt_mode_block_height; }

		// Total number of (block-dimensioned) blocks across all prepared slices.
		uint32_t get_total_blocks() const { return m_total_blocks; }

	private:
		basis_compressor_params m_params;
				
		opencl_context_ptr m_pOpenCL_context;

		// the output mode/codec
		basist::basis_tex_format m_fmt_mode; 
		
		// the output mode/codec's block width/height
		uint32_t m_fmt_mode_block_width; 
		uint32_t m_fmt_mode_block_height;
		
		// Note these images are expanded if necessary (duplicating cols/rows) to account for block dimensions.
		basisu::vector<image> m_slice_images;
		basisu::vector<imagef> m_slice_images_hdr;

		basisu::vector<image_stats> m_stats;

		uint64_t m_total_slice_orig_texels;

		uint64_t m_basis_file_size;
		double m_basis_bits_per_texel;

		uint64_t m_ktx2_file_size;
		double m_ktx2_bits_per_texel;
						
		basisu_backend_slice_desc_vec m_slice_descs;

		uint32_t m_total_blocks;
		
		basisu_frontend m_frontend;

		// These are 4x4 blocks.
		pixel_block_vec m_source_blocks;
		pixel_block_hdr_vec m_source_blocks_hdr;

		basisu::vector<gpu_image> m_frontend_output_textures;

		basisu::vector<gpu_image> m_best_etc1s_images;
		basisu::vector<image> m_best_etc1s_images_unpacked;

		basisu_backend m_backend;

		basisu_file m_basis_file;

		basisu::vector<gpu_image> m_decoded_output_textures;			// BC6H in HDR mode
		basisu::vector<image> m_decoded_output_textures_unpacked;
		
		basisu::vector<gpu_image> m_decoded_output_textures_bc7;
		basisu::vector<image> m_decoded_output_textures_unpacked_bc7;

		basisu::vector<imagef> m_decoded_output_textures_bc6h_hdr_unpacked;	// BC6H in HDR mode

		basisu::vector<gpu_image> m_decoded_output_textures_astc_hdr;
		basisu::vector<imagef> m_decoded_output_textures_astc_hdr_unpacked;

		uint8_vec m_output_basis_file;
		uint8_vec m_output_ktx2_file;
		
		basisu::vector<gpu_image> m_uastc_slice_textures;
		basisu_backend_output m_uastc_backend_output;

		// The amount the HDR input has to be scaled up in case it had to be rescaled to fit into half floats.
		float m_hdr_image_scale; 
		
		// The upconversion multiplier used to load LDR images in HDR mode.
		float m_ldr_to_hdr_upconversion_nit_multiplier;
				
		// True if any loaded source images were LDR and upconverted to HDR.
		bool m_upconverted_any_ldr_images;

		bool m_any_source_image_has_alpha;

		bool m_opencl_failed;

		// True once process() or process_source_images() has been called for the current init(). Guards against
		// calling either more than once per init() (which is unsupported and would re-run/corrupt the front pipeline).
		// Reset by init().
		bool m_has_been_processed;

		void check_for_hdr_inputs();
		bool sanity_check_input_params();
		void clean_hdr_image(imagef& src_img);
		bool read_dds_source_images();
		bool read_source_images();
		bool extract_source_blocks();
		bool process_frontend();
		bool extract_frontend_texture_data();
		bool process_backend();
		bool create_basis_file_and_transcode();
		bool write_hdr_debug_images(const char* pBasename, const imagef& img, uint32_t width, uint32_t height);
		bool write_output_files_and_compute_stats();
		error_code encode_slices_to_astc_6x6_hdr();
		error_code encode_slices_to_uastc_4x4_hdr();
		error_code encode_slices_to_xubc7();
		error_code encode_slices_to_uastc_4x4_ldr();
		error_code encode_slices_to_xuastc_or_astc_ldr();
		bool generate_mipmaps(const imagef& img, basisu::vector<imagef>& mips, bool has_alpha);
		bool generate_mipmaps(const image &img, basisu::vector<image> &mips, bool has_alpha);
		bool validate_texture_type_constraints();
		bool validate_ktx2_constraints();
		bool get_dfd(uint8_vec& dfd, const basist::ktx2_header& hdr);
		bool create_ktx2_file();
		bool pick_format_mode();

		uint32_t get_block_width() const { return m_fmt_mode_block_width; }
		uint32_t get_block_height() const { return m_fmt_mode_block_height; }
	};
				
	// Alternative simple C-style wrapper API around the basis_compressor class. 
	// This doesn't expose every encoder feature, but it's enough to get going.
	// Important: basisu_encoder_init() MUST be called first before calling these functions.
	//
	// Input parameters:
	//   source_images: Array of "image" objects, one per mipmap level, largest mipmap level first.
	// OR
	//   pImageRGBA: pointer to a 32-bpp RGBx or RGBA raster image, R first in memory, A last. Top scanline first in memory.
	//   width/height/pitch_in_pixels: dimensions of pImageRGBA
	//   
	// flags_and_quality: Combination of the above flags logically OR'd with the ETC1S or UASTC quality or effort level.
	//    Note: basis_compress2() variants below accept the new-style "quality_level" (0-100) and "effort_level" (0-10) parameters instead of packing them into flags_and_quality.
	//	  In ETC1S mode, the lower 8-bits are the ETC1S quality level which ranges from [1,255] (higher=better quality/larger files)
	//	  In UASTC LDR 4x4 mode, the lower 8-bits are the UASTC LDR/HDR pack or effort level (see cPackUASTCLevelFastest to cPackUASTCLevelVerySlow). Fastest/lowest quality is 0, so be sure to set it correctly. Valid values are [0,4] for both LDR/HDR.
	//    In UASTC HDR 4x4 mode, the lower 8-bits are the codec's effort level. Valid range is [uastc_hdr_4x4_codec_options::cMinLevel, uastc_hdr_4x4_codec_options::cMaxLevel]. Higher=better quality, but slower.
	//    In RDO ASTC HDR 6x6/UASTC HDR 6x6 mode, the lower 8-bits are the codec's effort level. Valid range is [0,astc_6x6_hdr::ASTC_HDR_6X6_MAX_USER_COMP_LEVEL]. Higher levels=better quality, but slower.
	//    In XUASTC/ASTC LDR 4x4-12x12/XUBC7 mode, the lower 8-bits are the compressor's effort level from [0,10] (astc_ldr_t::EFFORT_LEVEL_MIN, astc_ldr_t::EFFORT_LEVEL_MAX). If you don't set it, you'll get lowest effort (lowest quality, worst ratio).
	// 
	// float uastc_rdo_or_dct_quality:
	//    UASTC LDR 4x4 RDO quality level: RDO lambda setting - 0=no change/highest quality. Higher values lower quality but increase compressibility, initially try .5-1.5.
	//    RDO ASTC 6x6 HDR/UASTC 6x6 HDR: RDO lambda setting. 0=no change/highest quality. Higher values lower quality but increase compressibility, initially try 250-2000 (HDR) or 1000-10000 (LDR/SDR inputs upconverted to HDR). 
	//	  In XUASTC/ASTC/XUBC7 LDR 4x4-12x12 mode, this is the [1,100] weight grid DCT quality level.
	// 
	// pSize: Returns the output data's compressed size in bytes
	// 
	// Return value is the compressed .basis or .ktx2 file data, or nullptr on failure. Must call basis_free() to free it.
	enum
	{
		cFlagUseOpenCL = 1 << 8,		// use OpenCL if available
		cFlagThreaded = 1 << 9,			// use multiple threads for compression
		cFlagDebug = 1 << 10,			// enable debug output

		cFlagKTX2 = 1 << 11,			// generate a KTX2 file
		cFlagKTX2UASTCSuperCompression = 1 << 12, // use KTX2 Zstd supercompression on non-supercompressed formats that support it.

		cFlagSRGB = 1 << 13,			// input texture is sRGB, use perceptual colorspace metrics, also use sRGB filtering during mipmap gen, set 9,11,1,11 channel ASTC/XUASTC weights, and also sets KTX2/.basis output transfer func to sRGB. Otherwise assume linear input.
		cFlagGenMipsClamp = 1 << 14,	// generate mipmaps with clamp addressing
		cFlagGenMipsWrap = 1 << 15,		// generate mipmaps with wrap addressing
		
		cFlagYFlip = 1 << 16,			// flip source image on Y axis before compression
		
		// Note 11/18/2025: cFlagUASTCRDO flag is now ignored. Now if uastc_rdo_or_dct_quality>0 in UASTC LDR 4x4 mode, you automatically get RDO.
		//cFlagUASTCRDO = 1 << 17,		// use RDO postprocessing when generating UASTC LDR 4x4 files (must set uastc_rdo_or_dct_quality to the quality scalar) 
		
		cFlagPrintStats = 1 << 18,		// print image stats to stdout
		cFlagPrintStatus = 1 << 19,		// print status to stdout
		
		cFlagDebugImages = 1 << 20,		// enable debug image generation (for development, slower)

		cFlagREC2020 = 1 << 21,			// treat input as REC 2020 vs. the default 709 (for codecs that support this, currently UASTC HDR and ASTC 6x6), bit is always placed into KTX2 DFD
		
		cFlagValidateOutput = 1 << 22,	// transcode the output after encoding for testing
				
		// XUASTC LDR profile: full arith, hybrid or full zstd (see basist::astc_ldr_t::xuastc_ldr_syntax)
		cFlagXUASTCLDRSyntaxFullArith	= 0 << 23,
		cFlagXUASTCLDRSyntaxHybrid		= 1 << 23,
		cFlagXUASTCLDRSyntaxFullZStd	= 2 << 23,
		
		cFlagXUASTCLDRSyntaxShift		= 23,
		cFlagXUASTCLDRSyntaxMask		= 3,
		
		// Texture Type: 2D, 2D Array, Cubemap Array, or Texture Video (see enum basis_texture_type). Defaults to plain 2D.
		cFlagTextureType2D				= 0 << 25,
		cFlagTextureType2DArray			= 1 << 25,
		cFlagTextureTypeCubemapArray	= 2 << 25,
		cFlagTextureTypeVideoFrames		= 3 << 25,

		cFlagTextureTypeShift			= 25,
		cFlagTextureTypeMask			= 3,

		cFlagDisableDeblocking			= 1 << 27, // ASTC/XUASTC LDR: by default 10x8 block sizes or larger (>= 80 texels/block) get deblocked/deblocking aware encoding. The deblocking ID will be written to the output file. This disables all deblocking related features.
		cFlagForceDeblocking			= 1 << 28, // ASTC/XUASTC LDR: force deblocking aware encoding on all block sizes, write deblocking ID to the output file

		// XUBC7 only: selects the BC7 base encoder and (for bc7e_scalar) its quality level, packed into a 3-bit field
		// in the (otherwise unused) high bits. Field value 0 = bc7f (the default fast real-time packer; no level).
		// Values 1-7 = bc7e_scalar (slower, higher quality) at level 0-6 (i.e. bc7e_scalar level = field value - 1).
		// Packed as a single field because only 3 flag bits remain and bc7f needs no level; build the bc7e_scalar
		// value as ((level + 1) << cFlagXUBC7BaseEncoderShift). Adding new bits here doesn't change the API signature.
		cFlagXUBC7BaseEncoderShift		= 29,
		cFlagXUBC7BaseEncoderMask		= 7
	};

	void* basis_compress_internal(
		basist::basis_tex_format mode,
		const basisu::vector<image>* pSource_images,
		const basisu::vector<imagef>* pSource_images_hdr,
		uint32_t flags_and_quality, float uastc_rdo_or_dct_quality,
		size_t* pSize,
		image_stats* pStats,
		int quality_level = -1, int effort_level = -1);

	// This function accepts an array of source images. 
	// If more than one image is provided, it's assumed the images form a mipmap pyramid and automatic mipmap generation is disabled.
	// Returns a pointer to the compressed .basis or .ktx2 file data. *pSize is the size of the compressed data. 
	// Important: The returned block MUST be manually freed using basis_free_data().
	// basisu_encoder_init() MUST be called first!
	// LDR version.
	void* basis_compress(
		basist::basis_tex_format mode,
		const basisu::vector<image> &source_images,
		uint32_t flags_and_quality, float uastc_rdo_or_dct_quality,
		size_t* pSize,
		image_stats* pStats = nullptr);

	// HDR-only version.
	// Important: The returned block MUST be manually freed using basis_free_data().
	void* basis_compress(
		basist::basis_tex_format mode,
		const basisu::vector<imagef>& source_images_hdr,
		uint32_t flags_and_quality, float uastc_rdo_or_dct_quality,
		size_t* pSize,
		image_stats* pStats = nullptr);

	// This function only accepts a single LDR source image. It's just a wrapper for basis_compress() above.
	// Important: The returned block MUST be manually freed using basis_free_data().
	void* basis_compress(
		basist::basis_tex_format mode,
		const uint8_t* pImageRGBA, uint32_t width, uint32_t height, uint32_t pitch_in_pixels,
		uint32_t flags_and_quality, float uastc_rdo_or_dct_quality,
		size_t* pSize,
		image_stats* pStats = nullptr);

	// basis_compress2() variants accept the new unified quality_level and effort_level parameters instead of the old flags/float uastc_rdo_or_dct_quality parameter.
	// quality_level must be [0,100], effort_level [0,10].
	void* basis_compress2(
		basist::basis_tex_format mode,
		const basisu::vector<image>& source_images,
		uint32_t flags_and_quality, int quality_level, int effort_level,
		size_t* pSize,
		image_stats* pStats = nullptr);

	void* basis_compress2(
		basist::basis_tex_format mode,
		const basisu::vector<imagef>& source_images_hdr,
		uint32_t flags_and_quality, int quality_level, int effort_level,
		size_t* pSize,
		image_stats* pStats = nullptr);

	void* basis_compress2(
		basist::basis_tex_format mode,
		const uint8_t* pImageRGBA, uint32_t width, uint32_t height, uint32_t pitch_in_pixels,
		uint32_t flags_and_quality, int quality_level, int effort_level,
		size_t* pSize,
		image_stats* pStats = nullptr);

	// Frees the dynamically allocated file data returned by basis_compress().
	// This MUST be called on the pointer returned by basis_compress() when you're done with it.
	void basis_free_data(void* p);

	// Runs a short benchmark using synthetic image data to time OpenCL encoding vs. CPU encoding, with multithreading enabled.
	// Returns true if opencl is worth using on this system, otherwise false.
	// If pOpenCL_failed is not null, it will be set to true if OpenCL encoding failed *on this particular machine/driver/BasisU version* and the encoder falled back to CPU encoding.
	// basisu_encoder_init() MUST be called first. If OpenCL support wasn't enabled this always returns false.
	bool basis_benchmark_etc1s_opencl(bool *pOpenCL_failed = nullptr);

	// Parallel compression API
	struct parallel_results
	{
		double m_total_time;
		basis_compressor::error_code m_error_code;
		uint8_vec m_basis_file;
		uint8_vec m_ktx2_file;
		basisu::vector<image_stats> m_stats;
		double m_basis_bits_per_texel;
		bool m_any_source_image_has_alpha;

		parallel_results() 
		{
			clear();
		}

		void clear()
		{
			m_total_time = 0.0f;
			m_error_code = basis_compressor::cECFailedInitializing;
			m_basis_file.clear();
			m_ktx2_file.clear();
			m_stats.clear();
			m_basis_bits_per_texel = 0.0f;
			m_any_source_image_has_alpha = false;
		}
	};
		
	// Compresses an array of input textures across total_threads threads using the basis_compressor class.
	// Compressing multiple textures at a time is substantially more efficient than just compressing one at a time.
	// total_threads must be >= 1.
	bool basis_parallel_compress(
		uint32_t total_threads,
		const basisu::vector<basis_compressor_params> &params_vec,
		basisu::vector< parallel_results > &results_vec);
		
} // namespace basisu

