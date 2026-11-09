// basisu_dds_export.h
// Copyright (C) 2019-2025 Binomial LLC. All Rights Reserved.
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
// Basic DX10-style .DDS writer for basisu_tool's -dds command. It consumes the prepared source image
// slices from a basis_compressor that has been run in process_source_images() mode (so all the image
// loading, mipmap generation, and cubemap/array layout is done by the encoder), then packs each slice
// into the requested output format and writes a modern DX10 (DDS_HEADER_DXT10) .dds file.
//
// Supported -dds_format tokens (see parse_dds_output_format):
//   Block compressed: bc1 bc2 bc3 bc4 bc5 bc7
//   Uncompressed:     a8r8g8b8 r8 r8g8 r5g6b5 a1r5g5b5 a4r4g4b4
//
// The sRGB-vs-UNORM DXGI variant is selected from the compressor's transfer function flag
// (m_ktx2_and_basis_srgb_transfer_function, set by -srgb/-photo/-linear); only bc1/bc2/bc3/bc7/a8r8g8b8/a8b8g8r8
// have an sRGB DXGI variant, the rest are always written as UNORM. No transfer-function conversion is
// ever applied to the texel data - we only pick the format tag and pack the bytes as-is.
#pragma once

#include <string>
#include "../transcoder/basisu.h"		// for basisu::uint8_vec

namespace basisu
{
	class basis_compressor;

	// The output formats supported by the -dds command.
	enum dds_output_format
	{
		cDDSFmtInvalid = -1,

		// Block compressed (packed with the real-time BC encoders).
		cDDSFmtBC1,
		cDDSFmtBC2,
		cDDSFmtBC3,
		cDDSFmtBC4,
		cDDSFmtBC5,
		cDDSFmtBC7,

		// Uncompressed (simple pixel format conversion).
		cDDSFmtA8R8G8B8,	// DXGI B8G8R8A8 (32bpp, BGRA in memory), has sRGB variant
		cDDSFmtA8B8G8R8,	// DXGI R8G8B8A8 (32bpp, RGBA in memory), has sRGB variant
		cDDSFmtR8,			// DXGI R8 (8bpp)
		cDDSFmtR8G8,		// DXGI R8G8 (16bpp), source R->R, source G->G (like BC5)
		cDDSFmtR5G6B5,		// DXGI B5G6R5 (16bpp)
		cDDSFmtA1R5G5B5,	// DXGI B5G5R5A1 (16bpp)
		cDDSFmtA4R4G4B4,	// DXGI B4G4R4A4 (16bpp)

		cDDSFmtTotal
	};

	// Parses a -dds_format token (case-insensitive) into a dds_output_format. Returns false (and sets
	// fmt to cDDSFmtInvalid) on an unrecognized token.
	bool parse_dds_output_format(const char* pToken, dds_output_format& fmt);

	// Returns the canonical token string for a format (for messages), or "?" if invalid.
	const char* get_dds_output_format_string(dds_output_format fmt);

	// True if the format has a distinct sRGB DXGI variant (bc1/bc3/bc7/a8r8g8b8); false for the formats
	// that are always written UNORM. Lets a caller report the actually-emitted variant accurately.
	bool dds_output_format_has_srgb_variant(dds_output_format fmt);

	// Which BC7 base packer to use for bc7 output. (Eventually a third encoder may be added.)
	enum dds_bc7_encoder
	{
		cDDSBC7Encoder_Default = -1,		// "not specified" sentinel (CLI use); resolves to the dds_export_params default
		cDDSBC7Encoder_BC7F = 0,			// built-in fast real-time packer (basist::bc7f), default
		cDDSBC7Encoder_BC7E_Scalar = 1		// higher-quality (slower) scalar bc7e encoder
	};

	// bc7f quality level -> which analytical mode flags the bc7f packer uses (higher = slower/better).
	enum dds_bc7f_level
	{
		cDDSBC7FLevel_Analytical = 0,			// cPackBC7FlagDefault
		cDDSBC7FLevel_PartiallyAnalytical = 1,	// cPackBC7FlagDefaultPartiallyAnalytical (default)
		cDDSBC7FLevel_NonAnalytical = 2			// cPackBC7FlagDefaultNonAnalytical (slowest/best)
	};

	// Input parameters for build_dds(). Intended to grow as more -dds options are added (BC1 quality, etc.).
	struct dds_export_params
	{
		dds_output_format m_format;

		// BC7-only knobs (ignored for non-bc7 formats):
		dds_bc7_encoder m_bc7_encoder;		// which BC7 base packer
		int m_bc7f_level;					// dds_bc7f_level [0,2]: analytical / partially / non analytical (bc7f only)
		int m_bc7e_scalar_level;			// bc7e_scalar quality level [0,6], 0=ultrafast..6=slowest (bc7e_scalar only)

		dds_export_params()
			: m_format(cDDSFmtInvalid),
			  m_bc7_encoder(cDDSBC7Encoder_BC7F),
			  m_bc7f_level(cDDSBC7FLevel_PartiallyAnalytical),
			  m_bc7e_scalar_level(2)
		{ }
		explicit dds_export_params(dds_output_format format)
			: m_format(format),
			  m_bc7_encoder(cDDSBC7Encoder_BC7F),
			  m_bc7f_level(cDDSBC7FLevel_PartiallyAnalytical),
			  m_bc7e_scalar_level(2)
		{ }
	};

	// Builds an in-memory DX10 .dds file from the prepared slices of comp (which must have had a successful
	// process_source_images() call) into dds_data (which is resize(0)'d first, then appended to). The caller
	// is responsible for writing dds_data out (or using it however it likes). Picks the UNORM vs sRGB DXGI
	// variant from comp's transfer function flag. On failure returns false and sets error_msg. On success,
	// the optional out_* values report the texture's dimensions/structure (for a caller summary).
	bool build_dds(uint8_vec& dds_data, const basis_compressor& comp, const dds_export_params& params,
		std::string& error_msg,
		uint32_t* pOut_width = nullptr, uint32_t* pOut_height = nullptr,
		uint32_t* pOut_levels = nullptr, uint32_t* pOut_layers = nullptr, uint32_t* pOut_faces = nullptr);

} // namespace basisu
