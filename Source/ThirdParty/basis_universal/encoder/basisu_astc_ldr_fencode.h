// basisu_astc_ldr_fencode.h
#pragma once
#include "../transcoder/basisu.h"
#include "../transcoder/basisu_transcoder_internal.h"
#include "basisu_astc_ldr_common.h"

namespace basisu
{

namespace astc_ldrf
{

BASISU_FORCE_INLINE int popcount64(uint64_t x)
{
#if defined(__cplusplus) && (__cplusplus >= 202002L) && defined(__cpp_lib_bitops)
	return static_cast<int>(std::popcount(x));

#elif defined(__EMSCRIPTEN__) || defined(__clang__) || defined(__GNUC__)
	return __builtin_popcountll(static_cast<unsigned long long>(x));

#elif defined(_MSC_VER) && defined(_M_X64)
	return static_cast<int>(__popcnt64(x));

#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_ARM64) || defined(_M_ARM))
	return static_cast<int>(
		__popcnt(static_cast<uint32_t>(x)) +
		__popcnt(static_cast<uint32_t>(x >> 32))
		);

#else
	int count = 0;
	while (x)
	{
		x &= (x - 1);
		++count;
	}
	return count;
#endif
}

struct bitmask192
{
	uint64_t m_a, m_b, m_c; // a=lowest qword (bits 0-63), b=middle (64-127), c=highest (128-191)

	inline bitmask192() {}

	constexpr inline bitmask192(const bitmask192& o) noexcept
		: m_a(o.m_a), m_b(o.m_b), m_c(o.m_c)
	{ 
	}

	inline bitmask192& operator=(const bitmask192& o) noexcept
	{
		m_a = o.m_a;
		m_b = o.m_b;
		m_c = o.m_c;
		return *this;
	}

	constexpr inline bitmask192(uint64_t a, uint64_t b, uint64_t c) noexcept
		: m_a(a), m_b(b), m_c(c)
	{
	}

	static constexpr inline bitmask192 zero() noexcept
	{
		return bitmask192(0, 0, 0);
	}

	static constexpr inline bitmask192 all() noexcept
	{
		return bitmask192(UINT64_MAX, UINT64_MAX, UINT64_MAX);
	}

	inline void clear() noexcept
	{
		m_a = 0;
		m_b = 0;
		m_c = 0;
	}

	inline void set_all() noexcept
	{
		m_a = UINT64_MAX;
		m_b = UINT64_MAX;
		m_c = UINT64_MAX;
	}

	inline bool is_zero() const noexcept
	{
		return (m_a | m_b | m_c) == 0;
	}

	inline bool any() const noexcept
	{
		return (m_a | m_b | m_c) != 0;
	}

	inline bool none() const noexcept
	{
		return !any();
	}

	inline bool all_set() const noexcept
	{
		return (m_a == UINT64_MAX) && (m_b == UINT64_MAX) && (m_c == UINT64_MAX);
	}

	inline uint32_t popcount() const noexcept
	{
		return popcount64(m_a) + popcount64(m_b) + popcount64(m_c);
	}

	constexpr inline bitmask192 operator~() const noexcept
	{
		return bitmask192(~m_a, ~m_b, ~m_c);
	}

	constexpr inline bitmask192 operator&(const bitmask192& rhs) const noexcept
	{
		return bitmask192(m_a & rhs.m_a, m_b & rhs.m_b, m_c & rhs.m_c);
	}

	constexpr inline bitmask192 operator|(const bitmask192& rhs) const noexcept
	{
		return bitmask192(m_a | rhs.m_a, m_b | rhs.m_b, m_c | rhs.m_c);
	}

	constexpr inline bitmask192 operator^(const bitmask192& rhs) const noexcept
	{
		return bitmask192(m_a ^ rhs.m_a, m_b ^ rhs.m_b, m_c ^ rhs.m_c);
	}

	inline bitmask192& operator&=(const bitmask192& rhs) noexcept
	{
		m_a &= rhs.m_a;
		m_b &= rhs.m_b;
		m_c &= rhs.m_c;
		return *this;
	}

	inline bitmask192& operator|=(const bitmask192& rhs) noexcept
	{
		m_a |= rhs.m_a;
		m_b |= rhs.m_b;
		m_c |= rhs.m_c;
		return *this;
	}

	inline bitmask192& operator^=(const bitmask192& rhs) noexcept
	{
		m_a ^= rhs.m_a;
		m_b ^= rhs.m_b;
		m_c ^= rhs.m_c;
		return *this;
	}

	constexpr inline bool operator==(const bitmask192& rhs) const noexcept
	{
		return (m_a == rhs.m_a) && (m_b == rhs.m_b) && (m_c == rhs.m_c);
	}

	constexpr inline bool operator!=(const bitmask192& rhs) const noexcept
	{
		return !(*this == rhs);
	}

	// m_a is the lowest qword, m_b highest
	constexpr inline bool operator<(const bitmask192& rhs) const noexcept
	{
		if (m_c != rhs.m_c) return m_c < rhs.m_c;
		if (m_b != rhs.m_b) return m_b < rhs.m_b;
		return m_a < rhs.m_a;
	}

	constexpr inline bool operator>(const bitmask192& rhs) const noexcept
	{
		return rhs < *this;
	}

	constexpr inline bool operator<=(const bitmask192& rhs) const noexcept
	{
		return !(rhs < *this);
	}

	constexpr inline bool operator>=(const bitmask192& rhs) const noexcept
	{
		return !(*this < rhs);
	}

	explicit constexpr inline operator bool() const noexcept
	{
		return (m_a | m_b | m_c) != 0;
	}

	inline bool intersects(const bitmask192& rhs) const noexcept
	{
		return ((m_a & rhs.m_a) | (m_b & rhs.m_b) | (m_c & rhs.m_c)) != 0;
	}

	inline bool disjoint(const bitmask192& rhs) const noexcept
	{
		return !intersects(rhs);
	}

	constexpr inline bool contains_all(const bitmask192& rhs) const noexcept
	{
		return ((m_a & rhs.m_a) == rhs.m_a) &&
			((m_b & rhs.m_b) == rhs.m_b) &&
			((m_c & rhs.m_c) == rhs.m_c);
	}

	constexpr inline bool is_subset_of(const bitmask192& rhs) const noexcept
	{
		return rhs.contains_all(*this);
	}

	inline bool contains_any(const bitmask192& rhs) const noexcept
	{
		return intersects(rhs);
	}

	constexpr inline uint64_t low64() const noexcept
	{
		return m_a;
	}

	constexpr inline uint64_t mid64() const noexcept
	{
		return m_b;
	}

	constexpr inline uint64_t high64() const noexcept
	{
		return m_c;
	}

	inline void set_bit(uint32_t index) noexcept
	{
		assert(index < 192);

		if (index < 64)
			m_a |= uint64_t(1) << index;
		else if (index < 128)
			m_b |= uint64_t(1) << (index - 64);
		else
			m_c |= uint64_t(1) << (index - 128);
	}

	inline void set_bit(uint32_t index, uint32_t bit_val) noexcept
	{
		assert(index < 192);
		assert(bit_val <= 1);
				
		if (index < 64)
		{
			m_a &= ~(uint64_t(1) << index);
			m_a |= uint64_t(bit_val) << index;
		}
		else if (index < 128)
		{
			m_b &= ~(uint64_t(1) << (index - 64));
			m_b |= uint64_t(bit_val) << (index - 64);
		}
		else
		{
			m_c &= ~(uint64_t(1) << (index - 128));
			m_c |= uint64_t(bit_val) << (index - 128);
		}
	}

	inline bool is_bit_set(uint32_t index) const noexcept
	{
		assert(index < 192);

		if (index < 64)
			return ((m_a >> index) & 1) != 0;
		else if (index < 128)
			return ((m_b >> (index - 64)) & 1) != 0;
		else
			return ((m_c >> (index - 128)) & 1) != 0;
	}

	static inline bitmask192 lsb_mask(uint32_t num_bits) noexcept
	{
		assert(num_bits <= 192);

		if (num_bits == 0)
			return bitmask192(0, 0, 0);

		if (num_bits < 64)
			return bitmask192((uint64_t(1) << num_bits) - 1, 0, 0);

		if (num_bits == 64)
			return bitmask192(UINT64_MAX, 0, 0);

		if (num_bits < 128)
		{
			return bitmask192(
				UINT64_MAX,
				(uint64_t(1) << (num_bits - 64)) - 1,
				0);
		}

		if (num_bits == 128)
			return bitmask192(UINT64_MAX, UINT64_MAX, 0);

		if (num_bits < 192)
		{
			return bitmask192(
				UINT64_MAX,
				UINT64_MAX,
				(uint64_t(1) << (num_bits - 128)) - 1);
		}

		return bitmask192(UINT64_MAX, UINT64_MAX, UINT64_MAX);
	}
};

inline uint32_t popcount192(const bitmask192& a) { return a.popcount(); }

const uint32_t MAX_CANDIDATES = 512;

struct rgba32_image
{
    const uint8_t* m_pPixels;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_row_pitch_in_texels; // pitch in pixels/texels, not bytes
};

struct single_subset_enc_context
{
    uint32_t m_block_width, m_block_height;
    uint32_t m_block_size_index; 
    uint32_t m_total_block_pixels;
    
    uint32_t m_max_candidates;
    uint32_t m_num_ls_iterations;
	
    uint32_t m_chan_weights[4];

    basist::astc_ldr_t::dct2f m_dct;

    astc_helpers::decode_mode m_astc_decode_mode;
    bool m_disable_dual_plane;
	bool m_weight_polishing;
    
    bool m_has_alpha;

	bool m_try_base_ofs;
	bool m_higher_effort_bc;
};

const uint32_t MAX_UNIQUE_2SUBSET_PATS = 838;
const uint32_t MAX_UNIQUE_3SUBSET_PATS = 626;

typedef basisu::vector<astc_helpers::log_astc_block> astc_lblock_vec;

// source pEndpoints[] = ASTC direct order: LR HR LG HG LB HB LA HA
bool cem_encode(uint32_t cem_index, const float pEndpoints[8], uint32_t endpoint_ise_range, uint8_t* pCEM_values, bool allow_bc = true, bool high_effort = true);

bool init_single_subset_context(
    single_subset_enc_context& ctx,
    uint32_t block_width, uint32_t block_height,
    astc_helpers::decode_mode astc_decode_mode,
    const uint32_t chan_weights[4],
    uint32_t max_candidates, uint32_t num_ls_iterations, bool disable_dual_plane, bool has_alpha, bool weight_polishing);

// best_lblock will be in ise space (see is_lblock_ise() below), elements in array pointed to by pAll_candidates (which may be null) will be in rank space
double compress_single_subset(
	single_subset_enc_context& ctx,
	const uint8_t* pBlock_pixels,
	astc_helpers::log_astc_block& best_lblock,
	astc_lblock_vec* pAll_candidates,
	bool always_compute_error);

static const uint32_t NUM_DOT_THRESH_FRACTS = 6;

struct subset_enc_context : single_subset_enc_context
{
	subset_enc_context() {}

	uint32_t m_max_subsets;

	uint32_t m_num_carrier_candidates;
	uint32_t m_num_pattern_candidates;
	float m_two_subset_var_thresh;
	float m_three_subset_var_thresh;
	uint32_t m_two_subset_dot_thresh_fract_index;

	bitmask192 m_two_subset_pat_bitmask[MAX_UNIQUE_2SUBSET_PATS];

	uint32_t m_num_unique_two_subset_pats;
	const uint16_t* m_pUnique_two_subset_pats;

	bool m_use_method1;
	bool m_use_method2;

	astc_ldr::partitions_data* m_pPart_data_p2;
	astc_ldr::partitions_data* m_pPart_data_p3;
};

// must have first called init_single_subset_context() on ctx
bool init_multi_subset_context(
	subset_enc_context& ctx,
	uint32_t max_subsets,
	uint32_t num_carrier_candidates, uint32_t num_pattern_candidates,
	float two_subset_var_thresh, uint32_t two_subset_dot_thresh_fract_index,
	float three_subset_var_thresh,
	astc_ldr::partitions_data* pPart_data_p2, astc_ldr::partitions_data* pPart_data_p3);

struct subset_enc_thread_context
{
	astc_ldr::partition_pattern_vec m_pat_vec;
};

double compress_block_subsets(
	const subset_enc_context& enc_context,
	subset_enc_thread_context& enc_thread_context,
	const uint8_t* pBlock_pixels,
	astc_helpers::log_astc_block& best_lblock, astc_lblock_vec* pAll_candidates);

enum
{
    cUserModeISEValues = 0,
    cUserModeRankValues = 1
};

static inline bool is_lblock_ise(const astc_helpers::log_astc_block& log_blk)
{
    // the default user mode value == ISE, 1 = ranks (which is the exceptional case for astc_helpers)
    return log_blk.m_user_mode == cUserModeISEValues;
}

void convert_rank_lblock_to_ise(astc_helpers::log_astc_block& log_blk);
void convert_ise_lblock_to_rank(astc_helpers::log_astc_block& log_blk);

} // namespace astc_ldr_f
} // namespace basisu
