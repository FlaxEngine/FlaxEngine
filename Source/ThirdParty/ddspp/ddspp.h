#pragma once

// Sources
// https://learn.microsoft.com/en-us/windows/uwp/gaming/complete-code-for-ddstextureloader
// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10
// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat

#if (__cpp_constexpr >= 201304) || (_MSC_VER > 1900)
#define ddspp_constexpr constexpr
#else
#define ddspp_constexpr const
#endif

namespace ddspp
{
	namespace internal
	{
		static ddspp_constexpr unsigned int DDS_MAGIC      = 0x20534444;

		static ddspp_constexpr unsigned int DDS_ALPHAPIXELS = 0x00000001;
		static ddspp_constexpr unsigned int DDS_ALPHA       = 0x00000002; // DDPF_ALPHA
		static ddspp_constexpr unsigned int DDS_FOURCC      = 0x00000004; // DDPF_FOURCC
		static ddspp_constexpr unsigned int DDS_RGB         = 0x00000040; // DDPF_RGB
		static ddspp_constexpr unsigned int DDS_RGBA        = 0x00000041; // DDPF_RGB | DDPF_ALPHAPIXELS
		static ddspp_constexpr unsigned int DDS_YUV         = 0x00000200; // DDPF_YUV
		static ddspp_constexpr unsigned int DDS_LUMINANCE   = 0x00020000; // DDPF_LUMINANCE
		static ddspp_constexpr unsigned int DDS_LUMINANCEA  = 0x00020001; // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
		
		static ddspp_constexpr unsigned int DDS_PAL8       = 0x00000020; // DDPF_PALETTEINDEXED8
		static ddspp_constexpr unsigned int DDS_PAL8A      = 0x00000020; // DDPF_PALETTEINDEXED8 | DDPF_ALPHAPIXELS
		static ddspp_constexpr unsigned int DDS_BUMPDUDV   = 0x00080000; // DDPF_BUMPDUDV

		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_CAPS        = 0x00000001; // DDSD_CAPS
		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_HEIGHT      = 0x00000002; // DDSD_HEIGHT
		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_WIDTH       = 0x00000004; // DDSD_WIDTH
		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_PITCH       = 0x00000008; // DDSD_PITCH
		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_PIXELFORMAT = 0x00001000; // DDSD_PIXELFORMAT
		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_MIPMAP      = 0x00020000; // DDSD_MIPMAPCOUNT
		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_LINEARSIZE  = 0x00080000; // DDSD_LINEARSIZE
		static ddspp_constexpr unsigned int DDS_HEADER_FLAGS_VOLUME      = 0x00800000; // DDSD_DEPTH

		static ddspp_constexpr unsigned int DDS_HEADER_CAPS_COMPLEX      = 0x00000008; // DDSCAPS_COMPLEX
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS_MIPMAP       = 0x00400000; // DDSCAPS_MIPMAP
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS_TEXTURE      = 0x00001000; // DDSCAPS_TEXTURE

		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP           = 0x00000200; // DDSCAPS2_CUBEMAP
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP_POSITIVEX = 0x00000600; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEX = 0x00000a00; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP_POSITIVEY = 0x00001200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEY = 0x00002200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP_POSITIVEZ = 0x00004200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEZ = 0x00008200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_VOLUME            = 0x00200000; // DDSCAPS2_VOLUME
		static ddspp_constexpr unsigned int DDS_HEADER_CAPS2_CUBEMAP_ALLFACES  = 
			DDS_HEADER_CAPS2_CUBEMAP_POSITIVEX | DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEX | DDS_HEADER_CAPS2_CUBEMAP_POSITIVEY | 
			DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEY | DDS_HEADER_CAPS2_CUBEMAP_POSITIVEZ | DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEZ;

		static ddspp_constexpr unsigned int DXGI_MISC_FLAG_CUBEMAP = 0x4; // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_resource_misc_flag
		static ddspp_constexpr unsigned int DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7;

		enum DXGIAlphaMode : unsigned int
		{
			DDS_ALPHA_MODE_UNKNOWN       = 0,
			DDS_ALPHA_MODE_STRAIGHT      = 1,
			DDS_ALPHA_MODE_PREMULTIPLIED = 2,
			DDS_ALPHA_MODE_OPAQUE        = 3,
			DDS_ALPHA_MODE_CUSTOM        = 4
		};

		#define ddspp_make_fourcc(a, b, c, d) ((a) + ((b) << 8) + ((c) << 16) + ((d) << 24))

		// FOURCC constants
		static ddspp_constexpr unsigned int FOURCC_DXT1 = ddspp_make_fourcc('D', 'X', 'T', '1'); // BC1_UNORM
		static ddspp_constexpr unsigned int FOURCC_DXT2 = ddspp_make_fourcc('D', 'X', 'T', '2'); // BC2_UNORM
		static ddspp_constexpr unsigned int FOURCC_DXT3 = ddspp_make_fourcc('D', 'X', 'T', '3'); // BC2_UNORM
		static ddspp_constexpr unsigned int FOURCC_DXT4 = ddspp_make_fourcc('D', 'X', 'T', '4'); // BC3_UNORM
		static ddspp_constexpr unsigned int FOURCC_DXT5 = ddspp_make_fourcc('D', 'X', 'T', '5'); // BC3_UNORM
		static ddspp_constexpr unsigned int FOURCC_ATI1 = ddspp_make_fourcc('A', 'T', 'I', '1'); // BC4_UNORM
		static ddspp_constexpr unsigned int FOURCC_BC4U = ddspp_make_fourcc('B', 'C', '4', 'U'); // BC4_UNORM
		static ddspp_constexpr unsigned int FOURCC_BC4S = ddspp_make_fourcc('B', 'C', '4', 'S'); // BC4_SNORM
		static ddspp_constexpr unsigned int FOURCC_ATI2 = ddspp_make_fourcc('A', 'T', 'I', '2'); // BC5_UNORM
		static ddspp_constexpr unsigned int FOURCC_BC5U = ddspp_make_fourcc('B', 'C', '5', 'U'); // BC5_UNORM
		static ddspp_constexpr unsigned int FOURCC_BC5S = ddspp_make_fourcc('B', 'C', '5', 'S'); // BC5_SNORM
		static ddspp_constexpr unsigned int FOURCC_RGBG = ddspp_make_fourcc('R', 'G', 'B', 'G'); // R8G8_B8G8_UNORM
		static ddspp_constexpr unsigned int FOURCC_GRBG = ddspp_make_fourcc('G', 'R', 'G', 'B'); // G8R8_G8B8_UNORM
		static ddspp_constexpr unsigned int FOURCC_YUY2 = ddspp_make_fourcc('Y', 'U', 'Y', '2'); // YUY2
		static ddspp_constexpr unsigned int FOURCC_UYVY = ddspp_make_fourcc('U', 'Y', 'V', 'Y'); // UYVY
		static ddspp_constexpr unsigned int FOURCC_DXT10 = ddspp_make_fourcc('D', 'X', '1', '0'); // DDS extension header

		// These values come from the original D3D9 D3DFORMAT values https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dformat
		static ddspp_constexpr unsigned int FOURCC_RGB8        =  20;
		static ddspp_constexpr unsigned int FOURCC_A8R8G8B8    =  21;
		static ddspp_constexpr unsigned int FOURCC_X8R8G8B8    =  22;
		static ddspp_constexpr unsigned int FOURCC_R5G6B5      =  23; // B5G6R5_UNORM   (needs swizzling)
		static ddspp_constexpr unsigned int FOURCC_X1R5G5B5    =  24;
		static ddspp_constexpr unsigned int FOURCC_RGB5A1      =  25; // B5G5R5A1_UNORM (needs swizzling)
		static ddspp_constexpr unsigned int FOURCC_RGBA4       =  26; // B4G4R4A4_UNORM (needs swizzling)
		static ddspp_constexpr unsigned int FOURCC_R3G3B2      =  27;
		static ddspp_constexpr unsigned int FOURCC_A8          =  28;
		static ddspp_constexpr unsigned int FOURCC_A8R3G3B2    =  29;
		static ddspp_constexpr unsigned int FOURCC_X4R4G4B4    =  30;
		static ddspp_constexpr unsigned int FOURCC_A2B10G10R10 =  31;
		static ddspp_constexpr unsigned int FOURCC_A8B8G8R8    =  32;
		static ddspp_constexpr unsigned int FOURCC_X8B8G8R8    =  33;
		static ddspp_constexpr unsigned int FOURCC_G16R16      =  34;
		static ddspp_constexpr unsigned int FOURCC_A2R10G10B10 =  35;
		static ddspp_constexpr unsigned int FOURCC_RGBA16U     =  36; // R16G16B16A16_UNORM
		static ddspp_constexpr unsigned int FOURCC_RGBA16S     = 110; // R16G16B16A16_SNORM
		static ddspp_constexpr unsigned int FOURCC_R16F        = 111; // R16_FLOAT
		static ddspp_constexpr unsigned int FOURCC_RG16F       = 112; // R16G16_FLOAT
		static ddspp_constexpr unsigned int FOURCC_RGBA16F     = 113; // R16G16B16A16_FLOAT
		static ddspp_constexpr unsigned int FOURCC_R32F        = 114; // R32_FLOAT
		static ddspp_constexpr unsigned int FOURCC_RG32F       = 115; // R32G32_FLOAT
		static ddspp_constexpr unsigned int FOURCC_RGBA32F     = 116; // R32G32B32A32_FLOAT

		struct PixelFormat
		{
			unsigned int size;
			unsigned int flags;
			unsigned int fourCC;
			unsigned int RGBBitCount;
			unsigned int RBitMask;
			unsigned int GBitMask;
			unsigned int BBitMask;
			unsigned int ABitMask;
		};

		static_assert(sizeof(PixelFormat) == 32, "PixelFormat size mismatch");

		inline ddspp_constexpr bool is_rgba_mask(const PixelFormat& ddspf, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask)
		{
			return (ddspf.RBitMask == rmask) && (ddspf.GBitMask == gmask) && (ddspf.BBitMask == bmask) && (ddspf.ABitMask == amask);
		}

		inline ddspp_constexpr bool is_rgb_mask(const PixelFormat& ddspf, unsigned int rmask, unsigned int gmask, unsigned int bmask)
		{
			return (ddspf.RBitMask == rmask) && (ddspf.GBitMask == gmask) && (ddspf.BBitMask == bmask);
		}
	}

	using namespace internal;

	// https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ne-d3d11-d3d11_resource_dimension
	enum DXGIResourceDimension : unsigned int
	{
		DXGI_Unknown   = 0,
		DXGI_Buffer    = 1,
		DXGI_Texture1D = 2,
		DXGI_Texture2D = 3,
		DXGI_Texture3D = 4
	};

	// Matches DXGI_FORMAT https://docs.microsoft.com/en-us/windows/desktop/api/dxgiformat/ne-dxgiformat-dxgi_format
	enum DXGIFormat : unsigned int
	{
		UNKNOWN	                    = 0,
		R32G32B32A32_TYPELESS       = 1,
		R32G32B32A32_FLOAT          = 2,
		R32G32B32A32_UINT           = 3,
		R32G32B32A32_SINT           = 4,
		R32G32B32_TYPELESS          = 5,
		R32G32B32_FLOAT             = 6,
		R32G32B32_UINT              = 7,
		R32G32B32_SINT              = 8,
		R16G16B16A16_TYPELESS       = 9,
		R16G16B16A16_FLOAT          = 10,
		R16G16B16A16_UNORM          = 11,
		R16G16B16A16_UINT           = 12,
		R16G16B16A16_SNORM          = 13,
		R16G16B16A16_SINT           = 14,
		R32G32_TYPELESS             = 15,
		R32G32_FLOAT                = 16,
		R32G32_UINT                 = 17,
		R32G32_SINT                 = 18,
		R32G8X24_TYPELESS           = 19,
		D32_FLOAT_S8X24_UINT        = 20,
		R32_FLOAT_X8X24_TYPELESS    = 21,
		X32_TYPELESS_G8X24_UINT     = 22,
		R10G10B10A2_TYPELESS        = 23,
		R10G10B10A2_UNORM           = 24,
		R10G10B10A2_UINT            = 25,
		R11G11B10_FLOAT             = 26,
		R8G8B8A8_TYPELESS           = 27,
		R8G8B8A8_UNORM              = 28,
		R8G8B8A8_UNORM_SRGB         = 29,
		R8G8B8A8_UINT               = 30,
		R8G8B8A8_SNORM              = 31,
		R8G8B8A8_SINT               = 32,
		R16G16_TYPELESS             = 33,
		R16G16_FLOAT                = 34,
		R16G16_UNORM                = 35,
		R16G16_UINT                 = 36,
		R16G16_SNORM                = 37,
		R16G16_SINT                 = 38,
		R32_TYPELESS                = 39,
		D32_FLOAT                   = 40,
		R32_FLOAT                   = 41,
		R32_UINT                    = 42,
		R32_SINT                    = 43,
		R24G8_TYPELESS              = 44,
		D24_UNORM_S8_UINT           = 45,
		R24_UNORM_X8_TYPELESS       = 46,
		X24_TYPELESS_G8_UINT        = 47,
		R8G8_TYPELESS               = 48,
		R8G8_UNORM                  = 49,
		R8G8_UINT                   = 50,
		R8G8_SNORM                  = 51,
		R8G8_SINT                   = 52,
		R16_TYPELESS                = 53,
		R16_FLOAT                   = 54,
		D16_UNORM                   = 55,
		R16_UNORM                   = 56,
		R16_UINT                    = 57,
		R16_SNORM                   = 58,
		R16_SINT                    = 59,
		R8_TYPELESS                 = 60,
		R8_UNORM                    = 61,
		R8_UINT                     = 62,
		R8_SNORM                    = 63,
		R8_SINT                     = 64,
		A8_UNORM                    = 65,
		R1_UNORM                    = 66,
		R9G9B9E5_SHAREDEXP          = 67,
		R8G8_B8G8_UNORM             = 68,
		G8R8_G8B8_UNORM             = 69,
		BC1_TYPELESS                = 70,
		BC1_UNORM                   = 71,
		BC1_UNORM_SRGB              = 72,
		BC2_TYPELESS                = 73,
		BC2_UNORM                   = 74,
		BC2_UNORM_SRGB              = 75,
		BC3_TYPELESS                = 76,
		BC3_UNORM                   = 77,
		BC3_UNORM_SRGB              = 78,
		BC4_TYPELESS                = 79,
		BC4_UNORM                   = 80,
		BC4_SNORM                   = 81,
		BC5_TYPELESS                = 82,
		BC5_UNORM                   = 83,
		BC5_SNORM                   = 84,
		B5G6R5_UNORM                = 85,
		B5G5R5A1_UNORM              = 86,
		B8G8R8A8_UNORM              = 87,
		B8G8R8X8_UNORM              = 88,
		R10G10B10_XR_BIAS_A2_UNORM  = 89,
		B8G8R8A8_TYPELESS           = 90,
		B8G8R8A8_UNORM_SRGB         = 91,
		B8G8R8X8_TYPELESS           = 92,
		B8G8R8X8_UNORM_SRGB         = 93,
		BC6H_TYPELESS               = 94,
		BC6H_UF16                   = 95,
		BC6H_SF16                   = 96,
		BC7_TYPELESS                = 97,
		BC7_UNORM                   = 98,
		BC7_UNORM_SRGB              = 99,
		AYUV                        = 100,
		Y410                        = 101,
		Y416                        = 102,
		NV12                        = 103,
		P010                        = 104,
		P016                        = 105,
		OPAQUE_420                  = 106,
		YUY2                        = 107,
		Y210                        = 108,
		Y216                        = 109,
		NV11                        = 110,
		AI44                        = 111,
		IA44                        = 112,
		P8                          = 113,
		A8P8                        = 114,
		B4G4R4A4_UNORM              = 115,

		// Xbox-specific
		R10G10B10_7E3_A2_FLOAT      = 116,
		R10G10B10_6E4_A2_FLOAT      = 117,
		D16_UNORM_S8_UINT           = 118,
		R16_UNORM_X8_TYPELESS       = 119,
		X16_TYPELESS_G8_UINT        = 120,
		
		P208                        = 130,
		V208                        = 131,
		V408                        = 132,
		ASTC_4X4_TYPELESS           = 133,
		ASTC_4X4_UNORM              = 134,
		ASTC_4X4_UNORM_SRGB         = 135,
		ASTC_5X4_TYPELESS           = 137,
		ASTC_5X4_UNORM              = 138,
		ASTC_5X4_UNORM_SRGB         = 139,
		ASTC_5X5_TYPELESS           = 141,
		ASTC_5X5_UNORM              = 142,
		ASTC_5X5_UNORM_SRGB         = 143,

		ASTC_6X5_TYPELESS           = 145,
		ASTC_6X5_UNORM              = 146,
		ASTC_6X5_UNORM_SRGB         = 147,

		ASTC_6X6_TYPELESS           = 149,
		ASTC_6X6_UNORM              = 150,
		ASTC_6X6_UNORM_SRGB         = 151,

		ASTC_8X5_TYPELESS           = 153,
		ASTC_8X5_UNORM              = 154,
		ASTC_8X5_UNORM_SRGB         = 155,

		ASTC_8X6_TYPELESS           = 157,
		ASTC_8X6_UNORM              = 158,
		ASTC_8X6_UNORM_SRGB         = 159,

		ASTC_8X8_TYPELESS           = 161,
		ASTC_8X8_UNORM              = 162,
		ASTC_8X8_UNORM_SRGB         = 163,

		ASTC_10X5_TYPELESS          = 165,
		ASTC_10X5_UNORM             = 166,
		ASTC_10X5_UNORM_SRGB        = 167,

		ASTC_10X6_TYPELESS          = 169,
		ASTC_10X6_UNORM             = 170,
		ASTC_10X6_UNORM_SRGB        = 171,

		ASTC_10X8_TYPELESS          = 173,
		ASTC_10X8_UNORM             = 174,
		ASTC_10X8_UNORM_SRGB        = 175,

		ASTC_10X10_TYPELESS         = 177,
		ASTC_10X10_UNORM            = 178,
		ASTC_10X10_UNORM_SRGB       = 179,

		ASTC_12X10_TYPELESS         = 181,
		ASTC_12X10_UNORM            = 182,
		ASTC_12X10_UNORM_SRGB       = 183,

		ASTC_12X12_TYPELESS         = 185,
		ASTC_12X12_UNORM            = 186,
		ASTC_12X12_UNORM_SRGB       = 187,

		// Xbox-specific
		R10G10B10_SNORM_A2_UNORM    = 189,
		R4G4_UNORM                  = 190,

		A4B4G4R4_UNORM              = 191,

		FORCE_UINT                  = 0xffffffff
	};

	struct Header
	{
		unsigned int size;
		unsigned int flags;
		unsigned int height;
		unsigned int width;
		unsigned int pitchOrLinearSize;
		unsigned int depth;
		unsigned int mipMapCount;
		unsigned int reserved1[11];
		PixelFormat ddspf;
		unsigned int caps;
		unsigned int caps2;
		unsigned int caps3;
		unsigned int caps4;
		unsigned int reserved2;
	};

	static_assert(sizeof(Header) == 124, "DDS Header size mismatch");

	struct HeaderDXT10
	{
		DXGIFormat dxgiFormat;
		DXGIResourceDimension resourceDimension;
		unsigned int miscFlag;
		unsigned int arraySize;
		unsigned int miscFlags2;
	};

	static_assert(sizeof(HeaderDXT10) == 20, "DDS DX10 Extended Header size mismatch");

	// Maximum possible size of header. Use this to read in only the header, decode, seek to the real header size, then read in the rest of the image data
	ddspp_constexpr int MAX_HEADER_SIZE = sizeof(DDS_MAGIC) + sizeof(Header) + sizeof(HeaderDXT10);

	enum Result : unsigned char
	{
		Success,
		Error
	};

	enum TextureType : unsigned char
	{
		Texture1D,
		Texture2D,
		Texture3D,
		Cubemap,
	};

	struct Descriptor
	{
		DXGIFormat   format;
		TextureType  type;
		unsigned int width;
		unsigned int height;
		unsigned int depth;
		unsigned int numMips;
		unsigned int arraySize;
		unsigned int rowPitch; // Row pitch for mip 0
		unsigned int depthPitch; // Size of mip 0
		unsigned int bitsPerPixelOrBlock; // If compressed bits per block, else bits per pixel
		unsigned int blockWidth; // Width of block in pixels (1 if uncompressed)
		unsigned int blockHeight;// Height of block in pixels (1 if uncompressed)
		bool         compressed;
		bool         srgb;
		unsigned int headerSize; // Actual size of header, use this to get to image data
	};

	inline ddspp_constexpr bool is_dxt10(const Header& header)
	{
		const PixelFormat& ddspf = header.ddspf;
		return (ddspf.flags & DDS_FOURCC) && (ddspf.fourCC == FOURCC_DXT10);
	}

	inline ddspp_constexpr bool is_compressed(DXGIFormat format)
	{
		return (format >= BC1_UNORM && format <= BC5_SNORM) || 
		       (format >= BC6H_TYPELESS && format <= BC7_UNORM_SRGB) || 
		       (format >= ASTC_4X4_TYPELESS && format <= ASTC_12X12_UNORM_SRGB);
	}

	inline ddspp_constexpr bool is_srgb(DXGIFormat format)
	{
		switch(format)
		{
			case R8G8B8A8_UNORM_SRGB:
			case BC1_UNORM_SRGB:
			case BC2_UNORM_SRGB:
			case BC3_UNORM_SRGB:
			case B8G8R8A8_UNORM_SRGB:
			case B8G8R8X8_UNORM_SRGB:
			case BC7_UNORM_SRGB:
			case ASTC_4X4_UNORM_SRGB:
			case ASTC_5X4_UNORM_SRGB:
			case ASTC_5X5_UNORM_SRGB:
			case ASTC_6X5_UNORM_SRGB:
			case ASTC_6X6_UNORM_SRGB:
			case ASTC_8X5_UNORM_SRGB:
			case ASTC_8X6_UNORM_SRGB:
			case ASTC_8X8_UNORM_SRGB:
			case ASTC_10X5_UNORM_SRGB:
			case ASTC_10X6_UNORM_SRGB:
			case ASTC_10X8_UNORM_SRGB:
			case ASTC_10X10_UNORM_SRGB:
			case ASTC_12X10_UNORM_SRGB:
			case ASTC_12X12_UNORM_SRGB:
				return true;
			default:
				return false;
		}
	}

	inline ddspp_constexpr unsigned int get_bits_per_pixel_or_block(DXGIFormat format)
	{
		if(format >= ASTC_4X4_TYPELESS && format <= ASTC_12X12_UNORM_SRGB)
		{
			return 128; // All ASTC blocks are the same size
		}

		switch(format)
		{
			case R1_UNORM:
				return 1;
			case R8_TYPELESS:
			case R8_UNORM:
			case R8_UINT:
			case R8_SNORM:
			case R8_SINT:
			case A8_UNORM:
			case AI44:
			case IA44:
			case P8:
			case R4G4_UNORM:
				return 8;
			case NV12:
			case OPAQUE_420:
			case NV11:
				return 12;
			case R8G8_TYPELESS:
			case R8G8_UNORM:
			case R8G8_UINT:
			case R8G8_SNORM:
			case R8G8_SINT:
			case R16_TYPELESS:
			case R16_FLOAT:
			case D16_UNORM:
			case R16_UNORM:
			case R16_UINT:
			case R16_SNORM:
			case R16_SINT:
			case B5G6R5_UNORM:
			case B5G5R5A1_UNORM:
			case A8P8:
			case B4G4R4A4_UNORM:
				return 16;
			case P010:
			case P016:
			case D16_UNORM_S8_UINT:
			case R16_UNORM_X8_TYPELESS:
			case X16_TYPELESS_G8_UINT:
				return 24;
			case BC1_UNORM:
			case BC1_UNORM_SRGB:
			case BC1_TYPELESS:
			case BC4_UNORM:
			case BC4_SNORM:
			case BC4_TYPELESS:
			case R16G16B16A16_TYPELESS:
			case R16G16B16A16_FLOAT:
			case R16G16B16A16_UNORM:
			case R16G16B16A16_UINT:
			case R16G16B16A16_SNORM:
			case R16G16B16A16_SINT:
			case R32G32_TYPELESS:
			case R32G32_FLOAT:
			case R32G32_UINT:
			case R32G32_SINT:
			case R32G8X24_TYPELESS:
			case D32_FLOAT_S8X24_UINT:
			case R32_FLOAT_X8X24_TYPELESS:
			case X32_TYPELESS_G8X24_UINT:
			case Y416:
			case Y210:
			case Y216:
				return 64;
			case R32G32B32_TYPELESS:
			case R32G32B32_FLOAT:
			case R32G32B32_UINT:
			case R32G32B32_SINT:
				return 96;
			case BC2_UNORM:
			case BC2_UNORM_SRGB:
			case BC2_TYPELESS:
			case BC3_UNORM:
			case BC3_UNORM_SRGB:
			case BC3_TYPELESS:
			case BC5_UNORM:
			case BC5_SNORM:
			case BC6H_UF16:
			case BC6H_SF16:
			case BC7_UNORM:
			case BC7_UNORM_SRGB:
			case R32G32B32A32_TYPELESS:
			case R32G32B32A32_FLOAT:
			case R32G32B32A32_UINT:
			case R32G32B32A32_SINT:
				return 128;
			default:
				return 32; // Most formats are 32 bits per pixel
				break;
		}
	}

	inline ddspp_constexpr void get_block_size(DXGIFormat format, unsigned int& blockWidth, unsigned int& blockHeight)
	{
		switch(format)
		{
			case BC1_UNORM:
			case BC1_UNORM_SRGB:
			case BC1_TYPELESS:
			case BC4_UNORM:
			case BC4_SNORM:
			case BC4_TYPELESS:
			case BC2_UNORM:
			case BC2_UNORM_SRGB:
			case BC2_TYPELESS:
			case BC3_UNORM:
			case BC3_UNORM_SRGB:
			case BC3_TYPELESS:
			case BC5_UNORM:
			case BC5_SNORM:
			case BC6H_UF16:
			case BC6H_SF16:
			case BC7_UNORM:
			case BC7_UNORM_SRGB:
			case ASTC_4X4_TYPELESS:
			case ASTC_4X4_UNORM:
			case ASTC_4X4_UNORM_SRGB:
				blockWidth = 4; blockHeight = 4;
				break;
			case ASTC_5X4_TYPELESS:
			case ASTC_5X4_UNORM:
			case ASTC_5X4_UNORM_SRGB:
				blockWidth = 5; blockHeight = 4;
				break;
			case ASTC_5X5_TYPELESS:
			case ASTC_5X5_UNORM:
			case ASTC_5X5_UNORM_SRGB:
				blockWidth = 5; blockHeight = 5;
				break;
			case ASTC_6X5_TYPELESS:
			case ASTC_6X5_UNORM:
			case ASTC_6X5_UNORM_SRGB:
				blockWidth = 6; blockHeight = 5;
			case ASTC_6X6_TYPELESS:
			case ASTC_6X6_UNORM:
			case ASTC_6X6_UNORM_SRGB:
				blockWidth = 6; blockHeight = 6;
			case ASTC_8X5_TYPELESS:
			case ASTC_8X5_UNORM:
			case ASTC_8X5_UNORM_SRGB:
				blockWidth = 8; blockHeight = 5;
			case ASTC_8X6_TYPELESS:
			case ASTC_8X6_UNORM:
			case ASTC_8X6_UNORM_SRGB:
				blockWidth = 8; blockHeight = 6;
			case ASTC_8X8_TYPELESS:
			case ASTC_8X8_UNORM:
			case ASTC_8X8_UNORM_SRGB:
				blockWidth = 8; blockHeight = 8;
			case ASTC_10X5_TYPELESS:
			case ASTC_10X5_UNORM:
			case ASTC_10X5_UNORM_SRGB:
				blockWidth = 10; blockHeight = 5;
			case ASTC_10X6_TYPELESS:
			case ASTC_10X6_UNORM:
			case ASTC_10X6_UNORM_SRGB:
				blockWidth = 10; blockHeight = 6;
			case ASTC_10X8_TYPELESS:
			case ASTC_10X8_UNORM:
			case ASTC_10X8_UNORM_SRGB:
				blockWidth = 10; blockHeight = 8;
			case ASTC_10X10_TYPELESS:
			case ASTC_10X10_UNORM:
			case ASTC_10X10_UNORM_SRGB:
				blockWidth = 10; blockHeight = 10;
			case ASTC_12X10_TYPELESS:
			case ASTC_12X10_UNORM:
			case ASTC_12X10_UNORM_SRGB:
				blockWidth = 12; blockHeight = 10;
			case ASTC_12X12_TYPELESS:
			case ASTC_12X12_UNORM:
			case ASTC_12X12_UNORM_SRGB:
				blockWidth = 12; blockHeight = 12;
			default:
				blockWidth = 1; blockHeight = 1;
				break;
		}

		return;
	}

	bool has_alpha_channel(DXGIFormat format)
	{
		switch(format)
		{
			case R32G32B32A32_TYPELESS:
			case R32G32B32A32_FLOAT:
			case R32G32B32A32_UINT:
			case R32G32B32A32_SINT:
			case R16G16B16A16_TYPELESS:
			case R16G16B16A16_FLOAT:
			case R16G16B16A16_UNORM:
			case R16G16B16A16_UINT:
			case R16G16B16A16_SNORM:
			case R16G16B16A16_SINT:
			case R10G10B10A2_TYPELESS:
			case R10G10B10A2_UNORM:
			case R10G10B10A2_UINT:
			case R8G8B8A8_TYPELESS:
			case R8G8B8A8_UNORM:
			case R8G8B8A8_UNORM_SRGB:
			case R8G8B8A8_UINT:
			case R8G8B8A8_SNORM:
			case R8G8B8A8_SINT:
			case A8_UNORM:
			case BC1_TYPELESS:
			case BC1_UNORM:
			case BC1_UNORM_SRGB:
			case BC2_TYPELESS:
			case BC2_UNORM:
			case BC2_UNORM_SRGB:
			case BC3_TYPELESS:
			case BC3_UNORM:
			case BC3_UNORM_SRGB:
			case B5G5R5A1_UNORM:
			case B8G8R8A8_UNORM:
			case R10G10B10_XR_BIAS_A2_UNORM:
			case B8G8R8A8_TYPELESS:
			case B8G8R8A8_UNORM_SRGB:
			case BC7_TYPELESS:
			case BC7_UNORM:
			case BC7_UNORM_SRGB:
			case AYUV:
			case Y410:
			case Y416:
			case AI44:
			case IA44:
			case A8P8:
			case B4G4R4A4_UNORM:
			case R10G10B10_7E3_A2_FLOAT:
			case R10G10B10_6E4_A2_FLOAT:
			case R10G10B10_SNORM_A2_UNORM:
			case A4B4G4R4_UNORM:
				return true;
			default:
				return false;
		}
	}

	// Returns number of bytes for each row of a given mip. Valid range is [0, desc.numMips)
	inline ddspp_constexpr unsigned int get_row_pitch(unsigned int width, unsigned int bitsPerPixelOrBlock, unsigned int blockWidth, unsigned int mip)
	{
		// Shift width by mipmap index, round to next block size and round to next byte (for the rare less than 1 byte per pixel formats)
		// E.g. width = 119, mip = 3, BC1 compression
		// ((((119 >> 2) + 4 - 1) / 4) * 64) / 8 = 64 bytes
		return ((((((width >> mip) > 1) ? (width >> mip) : 1) + blockWidth - 1) / blockWidth) * bitsPerPixelOrBlock + 7) / 8;
	}

	inline ddspp_constexpr unsigned int get_row_pitch(const Descriptor& desc, unsigned int mip)
	{
		return get_row_pitch(desc.width, desc.bitsPerPixelOrBlock, desc.blockWidth, mip);
	}

	// Return the height for a given mip in either pixels or blocks depending on whether the format is compressed
	inline ddspp_constexpr unsigned int get_height_pixels_blocks(const unsigned int height, const unsigned int blockHeight, const unsigned int mip)
	{
		return ((height / blockHeight) >> mip) ? ((height / blockHeight) >> mip) : 1;
	}

	inline ddspp_constexpr unsigned int get_height_pixels_blocks(const Descriptor& desc, const unsigned int mip)
	{
		return get_height_pixels_blocks(desc.height, desc.blockHeight, mip);
	}

	inline Result decode_header(const unsigned char* sourceData, Descriptor& desc)
	{
		unsigned int magic = *reinterpret_cast<const unsigned int*>(sourceData); // First 4 bytes are the magic DDS number

		if (magic != DDS_MAGIC)
		{
			return ddspp::Error;
		}

		const Header header = *reinterpret_cast<const Header*>(sourceData + sizeof(DDS_MAGIC));
		const PixelFormat& ddspf = header.ddspf;
		bool dxt10Extension = is_dxt10(header);
		const HeaderDXT10 dxt10Header = *reinterpret_cast<const HeaderDXT10*>(sourceData + sizeof(DDS_MAGIC) + sizeof(Header));

		// Read basic data from the header
		desc.width = header.width > 0 ? header.width : 1;
		desc.height = header.height > 0 ? header.height : 1;
		desc.depth = header.depth > 0 ? header.depth : 1;
		desc.numMips = header.mipMapCount > 0 ? header.mipMapCount : 1;

		// Set some sensible defaults
		desc.arraySize = 1;
		desc.srgb = false;
		desc.type = Texture2D;
		desc.format = UNKNOWN;

		if (dxt10Extension)
		{
			desc.format = dxt10Header.dxgiFormat;

			desc.arraySize = dxt10Header.arraySize;

			switch(dxt10Header.resourceDimension)
			{
				case DXGI_Texture1D:
					desc.depth = 1;
					desc.type = Texture1D;
					break;
				case DXGI_Texture2D:
					desc.depth = 1;

					if(dxt10Header.miscFlag & DXGI_MISC_FLAG_CUBEMAP)
					{
						desc.type = Cubemap;
					}
					else
					{
						desc.type = Texture2D;
					}

					break;
				case DXGI_Texture3D:
					desc.type = Texture3D;
					desc.arraySize = 1; // There are no 3D texture arrays
					break;
				default:
					break;
			}
		}
		else
		{
			if(ddspf.flags & DDS_FOURCC)
			{
				unsigned int fourCC = ddspf.fourCC;

				switch(fourCC)
				{
					// Compressed
					case FOURCC_DXT1:    desc.format = BC1_UNORM; break;
					case FOURCC_DXT2:    // fallthrough
					case FOURCC_DXT3:    desc.format = BC2_UNORM; break;
					case FOURCC_DXT4:    // fallthrough
					case FOURCC_DXT5:    desc.format = BC3_UNORM; break;
					case FOURCC_ATI1:    // fallthrough
					case FOURCC_BC4U:    desc.format = BC4_UNORM; break;
					case FOURCC_BC4S:    desc.format = BC4_SNORM; break;
					case FOURCC_ATI2:    // fallthrough
					case FOURCC_BC5U:    desc.format = BC5_UNORM; break;
					case FOURCC_BC5S:    desc.format = BC5_SNORM; break;

						// Video
					case FOURCC_RGBG:    desc.format = R8G8_B8G8_UNORM; break;
					case FOURCC_GRBG:    desc.format = G8R8_G8B8_UNORM; break;
					case FOURCC_YUY2:    desc.format = YUY2; break;

						// Packed
					case FOURCC_R5G6B5:  desc.format = B5G6R5_UNORM; break;
					case FOURCC_RGB5A1:  desc.format = B5G5R5A1_UNORM; break;
					case FOURCC_RGBA4:   desc.format = B4G4R4A4_UNORM; break;

						// Uncompressed
					case FOURCC_A8:          desc.format = R8_UNORM; break;
					case FOURCC_A2B10G10R10: desc.format = R10G10B10A2_UNORM; break;
					case FOURCC_RGBA16U:     desc.format = R16G16B16A16_UNORM; break;
					case FOURCC_RGBA16S:     desc.format = R16G16B16A16_SNORM; break;
					case FOURCC_R16F:        desc.format = R16_FLOAT; break;
					case FOURCC_RG16F:       desc.format = R16G16_FLOAT; break;
					case FOURCC_RGBA16F:     desc.format = R16G16B16A16_FLOAT; break;
					case FOURCC_R32F:        desc.format = R32_FLOAT; break;
					case FOURCC_RG32F:       desc.format = R32G32_FLOAT; break;
					case FOURCC_RGBA32F:     desc.format = R32G32B32A32_FLOAT; break;
					default: break;
				}
			}
			else if(ddspf.flags & DDS_RGB)
			{
				switch(ddspf.RGBBitCount)
				{
					case 32:
						if(is_rgba_mask(ddspf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
						{
							desc.format = R8G8B8A8_UNORM;
						}
						else if(is_rgba_mask(ddspf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
						{
							desc.format = B8G8R8A8_UNORM;
						}
						else if(is_rgba_mask(ddspf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
						{
							desc.format = B8G8R8X8_UNORM;
						}

						// No DXGI format maps to (0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000) aka D3DFMT_X8B8G8R8
						// No DXGI format maps to (0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000) aka D3DFMT_A2R10G10B10

						// Note that many common DDS reader/writers (including D3DX) swap the
						// the RED/BLUE masks for 10:10:10:2 formats. We assume
						// below that the 'backwards' header mask is being used since it is most
						// likely written by D3DX. The more robust solution is to use the 'DX10'
						// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

						// For 'correct' writers, this should be 0x000003ff, 0x000ffc00, 0x3ff00000 for RGB data
						else if (is_rgba_mask(ddspf, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
						{
							desc.format = R10G10B10A2_UNORM;
						}
						else if (is_rgba_mask(ddspf, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
						{
							desc.format = R16G16_UNORM;
						}
						else if (is_rgba_mask(ddspf, 0xffffffff, 0x00000000, 0x00000000, 0x00000000))
						{
							// The only 32-bit color channel format in D3D9 was R32F
							desc.format = R32_FLOAT; // D3DX writes this out as a FourCC of 114
						}
						break;
					case 24:
						if(is_rgb_mask(ddspf, 0x00ff0000, 0x0000ff00, 0x000000ff))
						{
							desc.format = B8G8R8X8_UNORM;
						}
						break;
					case 16:
						if (is_rgba_mask(ddspf, 0x7c00, 0x03e0, 0x001f, 0x8000))
						{
							desc.format = B5G5R5A1_UNORM;
						}
						else if (is_rgba_mask(ddspf, 0xf800, 0x07e0, 0x001f, 0x0000))
						{
							desc.format = B5G6R5_UNORM;
						}
						else if (is_rgba_mask(ddspf, 0x0f00, 0x00f0, 0x000f, 0xf000))
						{
							desc.format = B4G4R4A4_UNORM;
						}
						// No DXGI format maps to (0x7c00, 0x03e0, 0x001f, 0x0000) aka D3DFMT_X1R5G5B5.
						// No DXGI format maps to (0x0f00, 0x00f0, 0x000f, 0x0000) aka D3DFMT_X4R4G4B4.
						// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
						break;
					default:
						break;
				}
			}
			else if (ddspf.flags & DDS_LUMINANCE)
			{
				switch(ddspf.RGBBitCount)
				{
					case 16:
						if (is_rgba_mask(ddspf, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
						{
							desc.format = R16_UNORM; // D3DX10/11 writes this out as DX10 extension.
						}
						if (is_rgba_mask(ddspf, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
						{
							desc.format = R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension.
						}
						break;
					case 8:
						if (is_rgba_mask(ddspf, 0x000000ff, 0x00000000, 0x00000000, 0x00000000))
						{
							desc.format = R8_UNORM; // D3DX10/11 writes this out as DX10 extension
						}

						// No DXGI format maps to (0x0f, 0x00, 0x00, 0xf0) aka D3DFMT_A4L4

						if (is_rgba_mask(ddspf, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
						{
							desc.format = R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
						}
						break;
				}
			}
			else if (ddspf.flags & DDS_ALPHA)
			{
				if (ddspf.RGBBitCount == 8)
				{
					desc.format = A8_UNORM;
				}
			}
			else if (ddspf.flags & DDS_BUMPDUDV)
			{
				if (ddspf.RGBBitCount == 32)
				{
					if (is_rgba_mask(ddspf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
					{
						desc.format = R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
					}
					if (is_rgba_mask(ddspf, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
					{
						desc.format = R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
					}

					// No DXGI format maps to (0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
				}
				else if (ddspf.RGBBitCount == 16)
				{
					if (is_rgba_mask(ddspf, 0x000000ff, 0x0000ff00, 0x00000000, 0x00000000))
					{
						desc.format = R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
					}
				}
			}

			if((header.flags & DDS_HEADER_FLAGS_VOLUME) || (header.caps2 & DDS_HEADER_CAPS2_VOLUME))
			{
				desc.type = Texture3D;
			}
			else if (header.caps2 & DDS_HEADER_CAPS2_CUBEMAP)
			{
				if((header.caps2 & DDS_HEADER_CAPS2_CUBEMAP_ALLFACES) != DDS_HEADER_CAPS2_CUBEMAP_ALLFACES)
				{
					return ddspp::Error;
				}

				desc.type = Cubemap;
				desc.arraySize = 1;
				desc.depth = 1;
			}
			else
			{
				desc.type = Texture2D;
			}
		}

		desc.compressed = is_compressed(desc.format);
		desc.srgb = is_srgb(desc.format);
		desc.bitsPerPixelOrBlock = get_bits_per_pixel_or_block(desc.format);
		get_block_size(desc.format, desc.blockWidth, desc.blockHeight);
		
		desc.rowPitch = get_row_pitch(desc.width, desc.bitsPerPixelOrBlock, desc.blockWidth, 0);
		desc.depthPitch = desc.rowPitch * desc.height / desc.blockHeight;
		desc.headerSize = sizeof(DDS_MAGIC) + sizeof(Header) + (dxt10Extension ? sizeof(HeaderDXT10) : 0);

		return ddspp::Success;
	}

	inline void encode_header(const DXGIFormat dxgiFormat, const unsigned int width, const unsigned int height, const unsigned int depth,
	                          const TextureType type, const unsigned int mipCount, const unsigned int arraySize, 
	                          Header& header, HeaderDXT10& dxt10Header)
	{
		header.size = sizeof(Header);

		// Fill in header flags
		header.flags = DDS_HEADER_FLAGS_CAPS | DDS_HEADER_FLAGS_HEIGHT | DDS_HEADER_FLAGS_WIDTH | DDS_HEADER_FLAGS_PIXELFORMAT;
		header.caps = DDS_HEADER_CAPS_TEXTURE;
		header.caps2 = 0;

		if (mipCount > 1)
		{
			header.flags |= DDS_HEADER_FLAGS_MIPMAP;
			header.caps |= DDS_HEADER_CAPS_COMPLEX | DDS_HEADER_CAPS_MIPMAP;
		}

		unsigned int bitsPerPixelOrBlock = get_bits_per_pixel_or_block(dxgiFormat);

		if (is_compressed(dxgiFormat))
		{
			header.flags |= DDS_HEADER_FLAGS_LINEARSIZE;
			unsigned int blockWidth, blockHeight;
			get_block_size(dxgiFormat, blockWidth, blockHeight);
			header.pitchOrLinearSize = width * height * bitsPerPixelOrBlock / (8 * blockWidth * blockHeight);
		}
		else
		{
			header.flags |= DDS_HEADER_FLAGS_PITCH;
			header.pitchOrLinearSize = width * bitsPerPixelOrBlock / 8;
		}

		header.height = height;
		header.width = width;
		header.depth = depth;
		header.mipMapCount = mipCount;
		header.reserved1[0] = header.reserved1[1] = header.reserved1[2] = 0;
		header.reserved1[3] = header.reserved1[4] = header.reserved1[5] = 0;
		header.reserved1[6] = header.reserved1[7] = header.reserved1[8] = 0;
		header.reserved1[9] = header.reserved1[10] = 0;
		
		// Fill in pixel format
		PixelFormat& ddspf = header.ddspf;
		ddspf.size = sizeof(PixelFormat);
		ddspf.fourCC = FOURCC_DXT10;
		ddspf.flags = 0;
		ddspf.flags |= DDS_FOURCC;

		dxt10Header.dxgiFormat = dxgiFormat;
		dxt10Header.arraySize = arraySize;
		dxt10Header.miscFlag = 0;
		
		if (type == Texture1D)
		{
			dxt10Header.resourceDimension = DXGI_Texture1D;
		}
		else if (type == Texture2D)
		{
			dxt10Header.resourceDimension = DXGI_Texture2D;
		}
		else if (type == Cubemap)
		{
			dxt10Header.resourceDimension = DXGI_Texture2D;
			dxt10Header.miscFlag |= DXGI_MISC_FLAG_CUBEMAP;
			header.caps |= DDS_HEADER_CAPS_COMPLEX;
			header.caps2 |= DDS_HEADER_CAPS2_CUBEMAP | DDS_HEADER_CAPS2_CUBEMAP_ALLFACES;
		}
		else if (type == Texture3D)
		{
			dxt10Header.resourceDimension = DXGI_Texture3D;
			header.flags |= DDS_HEADER_FLAGS_VOLUME;
			header.caps2 |= DDS_HEADER_CAPS2_VOLUME;
		}

		// I don't think this gives us a lot of information unless we can supply it externally. It's metadata related to how the alpha
		// channel has been factored into the texture. The format already gives us a lot to go on; the situations where this is ambiguous
		// are BC1_UNORM (1-bit alpha) and premultiplied. None of them can be deduced from the data we are being supplied currently
		dxt10Header.miscFlags2 = DDS_ALPHA_MODE_UNKNOWN;

		// Unused
		header.caps3 = 0;
		header.caps4 = 0;
		header.reserved2 = 0;
	}

	// Returns the offset from the base pointer to the desired mip and slice
	// Slice is either a texture from an array, a face from a cubemap, or a 2D slice of a volume texture
	inline ddspp_constexpr unsigned int get_offset(const Descriptor& desc, const unsigned int mip, const unsigned int slice)
	{
		// The mip/slice arrangement is different between texture arrays and volume textures
		//
		// Arrays
		//  __________  _____  __  __________  _____  __  __________  _____  __ 
		// |          ||     ||__||          ||     ||__||          ||     ||__|
		// |          ||_____|    |          ||_____|    |          ||_____|
		// |          |           |          |           |          |
		// |__________|           |__________|           |__________|
		//
		// Volume
		//  __________  __________  __________  _____  _____  _____  __  __  __ 
		// |          ||          ||          ||     ||     ||     ||__||__||__|
		// |          ||          ||          ||_____||_____||_____|
		// |          ||          ||          |
		// |__________||__________||__________|
		//

		unsigned long long offset = 0;

		if (desc.type == Texture3D)
		{
			for (unsigned int m = 0; m <= mip; ++m)
			{
				unsigned int mipWidth = (desc.width >> m) > 1 ? (desc.width >> m) : 1;
				unsigned int mipHeight = (desc.height >> m) > 1 ? (desc.height >> m) : 1;
				unsigned int mipBlocksWidth = (mipWidth + desc.blockWidth - 1) / desc.blockWidth;
				unsigned int mipBlocksHeight = (mipHeight + desc.blockHeight - 1) / desc.blockHeight;
				unsigned long long mipSize = mipBlocksWidth * mipBlocksHeight * desc.bitsPerPixelOrBlock;
				offset += mipSize * ((m == mip) ? slice : desc.numMips);
			}
		}
		else
		{
			unsigned long long mipChainSize = 0;

			for (unsigned int m = 0; m < desc.numMips; ++m)
			{
				unsigned int mipWidth = (desc.width >> m) > 1 ? (desc.width >> m) : 1;
				unsigned int mipHeight = (desc.height >> m) > 1 ? (desc.height >> m) : 1;
				unsigned int mipBlocksWidth = (mipWidth + desc.blockWidth - 1) / desc.blockWidth;
				unsigned int mipBlocksHeight = (mipHeight + desc.blockHeight - 1) / desc.blockHeight;
				mipChainSize += mipBlocksWidth * mipBlocksHeight * desc.bitsPerPixelOrBlock;
			}

			offset += mipChainSize * slice;

			for (unsigned int m = 0; m < mip; ++m)
			{
				unsigned int mipWidth = (desc.width >> m) > 1 ? (desc.width >> m) : 1;
				unsigned int mipHeight = (desc.height >> m) > 1 ? (desc.height >> m) : 1;
				unsigned int mipBlocksWidth = (mipWidth + desc.blockWidth - 1) / desc.blockWidth;
				unsigned int mipBlocksHeight = (mipHeight + desc.blockHeight - 1) / desc.blockHeight;
				offset += mipBlocksWidth * mipBlocksHeight * desc.bitsPerPixelOrBlock;
			}
		}

		offset /= 8; // Back to bytes

		return (unsigned int)offset;
	}
}
