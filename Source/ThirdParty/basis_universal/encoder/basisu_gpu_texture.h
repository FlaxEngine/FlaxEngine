// basisu_gpu_texture.h
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
#include "../transcoder/basisu.h"
#include "../transcoder/basisu_astc_helpers.h"
#include "basisu_etc.h"

// Forward declarations for transcode_ktx2_to_dds() below. The full transcoder
// header is only included by basisu_gpu_texture.cpp, keeping this header light.
// (transcoder_texture_format is a plain int-backed enum class, so an opaque
// forward declaration is valid here.)
namespace basist
{
	class ktx2_transcoder;
	enum class transcoder_texture_format;
	enum class dds_format;
}

namespace basisu
{
	// GPU texture "image"
	class gpu_image
	{
	public:
		enum { cMaxBlockSize = 12 };

		gpu_image()
		{
			clear();
		}

		gpu_image(texture_format fmt, uint32_t width, uint32_t height)
		{
			init(fmt, width, height);
		}

		void clear()
		{
			m_fmt = texture_format::cInvalidTextureFormat;
			m_width = 0;
			m_height = 0;
			m_block_width = 0;
			m_block_height = 0;
			m_blocks_x = 0;
			m_blocks_y = 0;
			m_qwords_per_block = 0;
			m_blocks.clear();
		}

		inline texture_format get_format() const { return m_fmt; }
		inline bool is_hdr() const { return is_hdr_texture_format(m_fmt); }
		inline bool is_ldr() const { return !is_hdr_texture_format(m_fmt); }
		
		// Width/height in pixels
		inline uint32_t get_pixel_width() const { return m_width; }
		inline uint32_t get_pixel_height() const { return m_height; }
		
		// Width/height in blocks, row pitch is assumed to be m_blocks_x.
		inline uint32_t get_blocks_x() const { return m_blocks_x; }
		inline uint32_t get_blocks_y() const { return m_blocks_y; }

		// Size of each block in pixels
		inline uint32_t get_block_width() const { return m_block_width; }
		inline uint32_t get_block_height() const { return m_block_height; }

		inline uint32_t get_qwords_per_block() const { return m_qwords_per_block; }
		inline uint32_t get_total_blocks() const { return m_blocks_x * m_blocks_y; }
		inline uint32_t get_bytes_per_block() const { return get_qwords_per_block() * sizeof(uint64_t); }
		inline uint32_t get_row_pitch_in_bytes() const { return get_bytes_per_block() * get_blocks_x(); }

		inline const uint64_vec &get_blocks() const { return m_blocks; }
		
		inline const uint64_t *get_ptr() const { return &m_blocks[0]; }
		inline uint64_t *get_ptr() { return &m_blocks[0]; }

		inline uint32_t get_size_in_bytes() const { return get_total_blocks() * get_qwords_per_block() * sizeof(uint64_t); }

		inline const void *get_block_ptr(uint32_t block_x, uint32_t block_y, uint32_t element_index = 0) const
		{
			assert(block_x < m_blocks_x && block_y < m_blocks_y);
			return &m_blocks[(block_x + block_y * m_blocks_x) * m_qwords_per_block + element_index];
		}

		inline void *get_block_ptr(uint32_t block_x, uint32_t block_y, uint32_t element_index = 0)
		{
			assert(block_x < m_blocks_x && block_y < m_blocks_y && element_index < m_qwords_per_block);
			return &m_blocks[(block_x + block_y * m_blocks_x) * m_qwords_per_block + element_index];
		}

		void init(texture_format fmt, uint32_t width, uint32_t height)
		{
			m_fmt = fmt;
			m_width = width;
			m_height = height;
			m_block_width = basisu::get_block_width(m_fmt);
			m_block_height = basisu::get_block_height(m_fmt);
			m_blocks_x = (m_width + m_block_width - 1) / m_block_width;
			m_blocks_y = (m_height + m_block_height - 1) / m_block_height;
			m_qwords_per_block = basisu::get_qwords_per_block(m_fmt);

			m_blocks.resize(0);
			m_blocks.resize(m_blocks_x * m_blocks_y * m_qwords_per_block);
		}

		// Unpacks LDR textures only. Asserts and returns false otherwise.
		// astc_srgb: true to use the ASTC sRGB decode profile, false for linear. 
		// For XUASTC LDR, this should match what was used during encoding. For ETC1S/UASTC LDR 4x4, this should be false.
		bool unpack(image& img, bool astc_srgb) const;

		// Unpacks HDR textures only. Asserts and returns false otherwise.
		bool unpack_hdr(imagef& img) const;
		
		inline void override_dimensions(uint32_t w, uint32_t h)
		{
			m_width = w;
			m_height = h;
		}

	private:
		texture_format m_fmt;
		uint32_t m_width, m_height, m_blocks_x, m_blocks_y, m_block_width, m_block_height, m_qwords_per_block;
		uint64_vec m_blocks;
	};

	typedef basisu::vector<gpu_image> gpu_image_vec;

	// A mip chain (or single image) of uncompressed RGBA images. Used by
	// write_uncompressed_rgba32_dds() as one array slice / cubemap face.
	typedef basisu::vector<image> image_vec;

	// KTX1 file writing - compatible with ARM's astcenc tool, and some other tools.
	// Note astc_linear_flag used to be always effectively true in older code. It's ignored for ASTC HDR formats.
	bool create_ktx_texture_file(uint8_vec &ktx_data, const basisu::vector<gpu_image_vec>& gpu_images, bool cubemap_flag, bool astc_srgb_flag);
	
	bool does_dds_support_format(texture_format fmt);

	// Returns true if create_ktx_texture_file() can write this (compressed) texture
	// format. Mirrors that writer's supported set, minus the formats we don't expose
	// for KTX export right now (ATC, FXT1, and all uncompressed formats).
	bool does_ktx_support_format(texture_format fmt);
	bool write_dds_file(uint8_vec& dds_data, const basisu::vector<gpu_image_vec>& gpu_images, bool cubemap_flag, bool use_srgb_format);
	bool write_dds_file(const char* pFilename, const basisu::vector<gpu_image_vec>& gpu_images, bool cubemap_flag, bool use_srgb_format);

	// Writes uncompressed 32-bit RGBA image data to an in-memory .DDS blob.
	// images is indexed [array-slice][mip level]. For cubemaps and cubemap arrays
	// the outer slice count must be a multiple of 6, ordered layer-major then the
	// six faces, and cubemap_flag must be true; for 2D textures and 2D arrays
	// cubemap_flag is false and the outer count is the array size (1 for a plain
	// 2D texture). Every slice must have the same mip count and the same level-0
	// dimensions, and each mip level must be floor(prev/2) (min 1). Supports 2D,
	// 2D arrays, cubemaps, and cubemap arrays, with or without a mip chain.
	// Validates all of the above; returns false (and clears dds_data) on any
	// inconsistency or write failure. Reusable on its own (a sibling to
	// write_dds_file for the uncompressed RGBA case).
	bool write_uncompressed_rgba32_dds(uint8_vec& dds_data, const basisu::vector<image_vec>& images, bool cubemap_flag, bool use_srgb_format);

	// Transcodes the entire contents of an init()'d ktx2_transcoder (every mip
	// level, array layer, and cubemap face) to fmt and serializes a Microsoft
	// .DDS (DirectDraw Surface) blob into dds_data. fmt MUST be one of the DirectX
	// BC formats writable to DDS (cTFBC1_RGB, cTFBC3_RGBA, cTFBC4_R, cTFBC5_RG,
	// cTFBC6H, cTFBC7_RGBA) or uncompressed cTFRGBA32; any other format fails.
	// Supports 2D, 2D arrays, cubemaps, and cubemap arrays (with or without mips).
	// The transcoder need only have been init()'d -- this calls start_transcoding()
	// itself. Returns false (and clears dds_data) on any error.
	// srgb_mode selects whether the sRGB DDS format variants are used: -1 = auto
	// (follow the KTX2's transfer function via transcoder.is_srgb()), 0 = force
	// linear/UNORM, 1 = force sRGB. Ignored for formats that have no sRGB variant
	// (BC4/BC5/BC6H/etc.). decode_flags is passed straight to
	// ktx2_transcoder::transcode_image_level() (a cDecodeFlags* bitmask), letting the
	// caller control the transcode -- e.g. force/disable deblocking, high quality, etc.
	bool transcode_ktx2_to_dds(basist::ktx2_transcoder& transcoder, basist::transcoder_texture_format fmt, uint8_vec& dds_data, int srgb_mode = -1, uint32_t decode_flags = 0);

	// Like transcode_ktx2_to_dds(), but serializes a KTX1 (.ktx) file instead, via
	// create_ktx_texture_file(). COMPRESSED formats only -- BC1-7, ETC1/ETC2,
	// ETC2 EAC R11/RG11, PVRTC1, PVRTC2 (RGBA), ASTC LDR/HDR, UASTC; uncompressed
	// (RGBA32/half/float) is not supported here yet. fmt must pass
	// does_ktx_support_format(). Supports 2D, 2D arrays, cubemaps, and cubemap
	// arrays (with or without mips). srgb_mode is as above; it selects the sRGB GL
	// enum variants for the formats that have them (BC1/BC3/BC7, ETC2, PVRTC1, ASTC LDR)
	// and is ignored for formats with no sRGB variant (BC4/BC5/BC6H/ETC1/PVRTC2). decode_flags is
	// passed straight to transcode_image_level() (a cDecodeFlags* bitmask). The
	// transcoder need only have been init()'d. Returns false (and clears ktx_data)
	// on any error.
	bool transcode_ktx2_to_ktx(basist::ktx2_transcoder& transcoder, basist::transcoder_texture_format fmt, uint8_vec& ktx_data, int srgb_mode = -1, uint32_t decode_flags = 0);

	// Currently reads 2D 32bpp RGBA, 16-bit HALF RGBA, or 32-bit FLOAT RGBA, with or without mipmaps. No tex arrays or cubemaps, yet.
	bool read_uncompressed_dds_file(const char* pFilename, basisu::vector<image>& ldr_mips, basisu::vector<imagef>& hdr_mips);

	// Cracks open a .DDS file with tinydds (header only -- no pixel decode) and
	// prints its high-level info to stdout: texture type (2D / 2D array / cubemap /
	// cubemap array / 3D), dimensions, mip level count, array slice count, and the
	// format (a friendly name for BC1-7 and the common LDR/HDR uncompressed formats,
	// otherwise the raw hex value). Intended as a quick development sanity check.
	bool print_dds_info(const char* pFilename);

	// Reads a KTX1 (.ktx) file's 64-byte header (endian-swapping the fields if the
	// file's endianness marker says so) and prints them to stdout: GL type/format/
	// internalFormat (+ a friendly name for the common compressed formats),
	// dimensions, array elements, faces, mip levels, and key-value-data size. Header
	// only -- it does not parse the key/value data or image data. Development aid.
	bool print_ktx_info(const char* pFilename);

	// Supports DDS and KTX
	bool write_compressed_texture_file(const char *pFilename, const basisu::vector<gpu_image_vec>& g, bool cubemap_flag, bool use_srgb_format);
	bool write_compressed_texture_file(const char* pFilename, const gpu_image_vec& g, bool use_srgb_format);
	bool write_compressed_texture_file(const char *pFilename, const gpu_image &g, bool use_srgb_format);
	
	bool write_3dfx_out_file(const char* pFilename, const gpu_image& gi);

	// Returns the ASCII name of a texture_format enum value, e.g. "BC7", "ASTC_LDR_4x4", "ETC1".
	const char* get_texture_format_name(texture_format fmt);

	// Human-readable name for the exact physical format stored in a .DDS file (basist::dds_transcoder::get_dds_format()).
	const char* get_dds_format_string(basist::dds_format fmt);

	// GPU texture block unpacking
	// For ETC1, use in basisu_etc.h: bool unpack_etc1(const etc_block& block, color_rgba *pDst, bool preserve_alpha)
	void unpack_etc2_eac(const void *pBlock_bits, color_rgba *pPixels);

	bool unpack_bc6h(const void* pSrc_block, void* pDst_block, bool is_signed, uint32_t dest_pitch_in_halfs = 4 * 3); // full format, outputs HALF values, RGB texels only (not RGBA)
	void unpack_atc(const void* pBlock_bits, color_rgba* pPixels);
	// We only support CC_MIXED non-alpha blocks here because that's the only mode the transcoder uses at the moment.
	bool unpack_fxt1(const void* p, color_rgba* pPixels);
	// PVRTC2 is currently limited to only what our transcoder outputs (non-interpolated, hard_flag=1 modulation=0). In this mode, PVRTC2 looks much like BC1/ATC.
	bool unpack_pvrtc2(const void* p, color_rgba* pPixels);
	void unpack_etc2_eac_r(const void *p, color_rgba* pPixels, uint32_t c);
	void unpack_etc2_eac_rg(const void* p, color_rgba* pPixels);
	
	// unpack_block() is primarily intended to unpack texture data created by the transcoder.
	// For some texture formats (like ETC2 RGB, PVRTC2, FXT1) it's not yet a complete implementation.
	// Unpacks LDR texture formats only.
	bool unpack_block(texture_format fmt, const void *pBlock, color_rgba *pPixels, bool astc_srgb);

	// Unpacks HDR texture formats only.
	bool unpack_block_hdr(texture_format fmt, const void* pBlock, vec4F* pPixels);
	
	bool read_astc_file(const uint8_t* pImage_data, size_t image_data_size, vector2D<astc_helpers::astc_block>& blocks, uint32_t& block_width, uint32_t& block_height, uint32_t& width, uint32_t& height);
	bool read_astc_file(const char* pFilename, vector2D<astc_helpers::astc_block>& blocks, uint32_t& block_width, uint32_t& block_height, uint32_t& width, uint32_t& height);
	bool write_astc_file(const char* pFilename, const void* pBlocks, uint32_t block_width, uint32_t block_height, uint32_t dim_x, uint32_t dim_y);
							
} // namespace basisu

