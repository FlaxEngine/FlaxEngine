// File: basisu_xbc7_decoder.h
// XBC7 decoder: shared definitions + decoder API. Lives in the transcoder so the
// decode path (XUBC7 -> BC7/etc.) can use it; the xbc7 encoder also includes this
// header for the shared DCT/symbol/enum definitions. The implementation is in
// basisu_xbc7_decoder.inl (included at the end of basisu_transcoder.cpp). Depends
// only on transcoder headers.
#pragma once
#include "basisu.h"
#include "basisu_containers.h"
#include "basisu_transcoder_internal.h"

namespace basist {
namespace xbc7 {
	using basist::fixed16_16;
	using basisu::uint8_vec;          // these basisu containers/util are used unqualified below
	using basisu::vector2D;
	using basisu::fmt_debug_printf;
	using basisu::minimum;
	using basisu::maximum;
	using basisu::clamp;
	using basisu::iabs;

// dct2fx: 2D orthonormal DCT-II (forward) / DCT-III (inverse) on fixed16_16.
// Port of the float dct2f class. Pure integer end to end, INCLUDING table
// generation at init: libm cosf/sqrtf are not bit-identical across platforms,
// which would silently break cross-platform determinism. Tables are built
// with an integer Q30 cosine (range reduction + nested Taylor, error < 1e-8)
// and the deterministic integer sqrt, then quantized once to Q15.16.
//
// Differences from the float original (all intentional):
//  - The alpha(u)/alpha(v) scale factors are FOLDED into the cos tables at
//    init (entry = alpha(u)*cos(...)). One multiply and one rounding fewer
//    per output; the m_a_col/m_a_row members are gone.
//  - Dot products accumulate raw 64-bit products (mul_wide) and round ONCE
//    per output (from_sum) -- the fast pattern, and in fixed point it makes
//    inverse() and inverse_check() bit-identical (int64 addition commutes,
//    unlike float).
//  - The inverse zero-skip is pSrc[...].v == 0 -- no type-punning needed.
//  - The stale BASISU_NOTE_UNUSED(src_stride/dst_stride) lines are gone:
//    the strides were actually used right below them.
//
// Range guidance (Q15.16, asserts catch violations in debug): worst-case
// gain of the 2D transform is sqrt(rows*cols) <= 12, so inputs bounded by
// ~2700 in magnitude can never overflow any intermediate or output.
//
// Precision (measured, 12x12 worst case, inputs +-2000): forward abs error
// vs a double reference <= ~0.5, round-trip error <= ~0.2. Dominated by the
// 2^-17 quantization of the Q15.16 tables (error scales ~ |input|*N*2^-17
// per pass, x sqrt(N) through the second pass) -- the same effect the float
// original has at 2^-24 scale.

	typedef basisu::vector<fixed16_16> fxvec;

	namespace dct_detail
	{
		// cos(pi * k / n) in Q30, pure integer, deterministic. n in [1, 48].
		// Error < 1e-8 (Taylor truncation ~2e-11, arithmetic ~few * 2^-30),
		// far below the Q15.16 table quantization of 2^-17.
		constexpr int64_t cos_pi_frac_q30(uint32_t k, uint32_t n)
		{
			const int64_t Q30 = int64_t(1) << 30;
			// range-reduce: period 2n, fold to [0, n], then to [0, n/2] + sign
			uint32_t m = k % (2u * n);
			if (m > n) m = 2u * n - m;                      // cos(2pi - t) =  cos t
			bool neg = false;
			if (2u * m > n) { m = n - m; neg = true; }      // cos(pi  - t) = -cos t
			// theta = pi*m/n in Q30, theta <= pi/2
			const int64_t PI_Q30 = 3373259426ll;            // round(pi * 2^30)
			const int64_t th = (PI_Q30 * int64_t(m)) / int64_t(n);
			const int64_t x2 = (th * th) >> 30;             // theta^2, Q30
			// cos t = 1 - x2/2*(1 - x2/12*(1 - x2/30*(1 - x2/56*(1 - x2/90*
			//         (1 - x2/132*(1 - x2/182))))))        (nested Taylor)
			const int dens[7] = { 182, 132, 90, 56, 30, 12, 2 };
			int64_t r = Q30;
			for (int i = 0; i < 7; i++)
				r = Q30 - ((x2 * r) >> 30) / dens[i];
			return neg ? -r : r;
		}

		// sqrt(1/n) and sqrt(2/n) in Q30, via the exact integer sqrt
		constexpr int64_t alpha0_q30(uint32_t n)
		{
			return int64_t(basist::fixed_detail::isqrt_floor((uint64_t(1) << 60) / n));
		}
		constexpr int64_t alpha_q30(uint32_t n)
		{
			return int64_t(basist::fixed_detail::isqrt_floor((uint64_t(1) << 61) / n));
		}

		// Q30 * Q30 -> Q15.16, rounded half away from zero
		constexpr int32_t q60_to_q16(int64_t p)
		{
			const int64_t h = int64_t(1) << 43;
			return int32_t(p >= 0 ? ((p + h) >> 44) : -(((-p) + h) >> 44));
		}
	}

	// Fixed-point 4-sample orthonormal DCT-II / IDCT-III (radix-2 butterfly).
	// Overloads for fixed16_16 (the float template needs T(double), which fixed
	// deliberately lacks). Constants are the EXACT Q15.16 quantizations of the
	// alpha*cos table entries the general dct2fx path uses, and partial sums are
	// kept wide (int64) with ONE rounding per output -- int64 sums commute, so
	// these butterflies are bit-identical to the general matrix product. That
	// lets dct2fx dispatch 4x4 to them with zero behavioral change.
	namespace dct4
	{
		namespace fxk
		{
			typedef fixed16_16 fx;
			// alpha(k)*cos(pi*(2n+1)k/8) quantized exactly as dct2fx's tables
			constexpr int64_t A0 = dct_detail::alpha0_q30(4);    // 1/2
			constexpr int64_t A = dct_detail::alpha_q30(4);     // 1/sqrt(2)
			constexpr fx HALF = fx::from_raw(dct_detail::q60_to_q16(A0 * dct_detail::cos_pi_frac_q30(0, 8)));
			constexpr fx C1 = fx::from_raw(dct_detail::q60_to_q16(A * dct_detail::cos_pi_frac_q30(1, 8)));
			constexpr fx C3 = fx::from_raw(dct_detail::q60_to_q16(A * dct_detail::cos_pi_frac_q30(3, 8)));
			static_assert(HALF.v == 32768, "");                  // 0.5 exact
			static_assert(C1.v == 42813, "");                    // cos(pi/8) /sqrt(2) ~ 0.653281
			static_assert(C3.v == 17734, "");                    // cos(3pi/8)/sqrt(2) ~ 0.270598
		}

		inline void forward_ortho(const fixed16_16 x[4], fixed16_16 y[4])
		{
			using namespace fxk;
			const fx a0 = x[0] + x[3];
			const fx a1 = x[1] + x[2];
			const fx a2 = x[1] - x[2];
			const fx a3 = x[0] - x[3];
			y[0] = fx::from_sum(a0.mul_wide(HALF) + a1.mul_wide(HALF));
			y[1] = fx::from_sum(a3.mul_wide(C1) + a2.mul_wide(C3));
			y[2] = fx::from_sum(a0.mul_wide(HALF) - a1.mul_wide(HALF));
			y[3] = fx::from_sum(a3.mul_wide(C3) - a2.mul_wide(C1));
		}

		inline void inverse_ortho(const fixed16_16 y[4], fixed16_16 x[4])
		{
			using namespace fxk;
			// shared partial sums stay wide; each output rounded once
			const int64_t b0 = y[0].mul_wide(HALF) + y[2].mul_wide(HALF);
			const int64_t b1 = y[0].mul_wide(HALF) - y[2].mul_wide(HALF);
			const int64_t t0 = y[1].mul_wide(C1) + y[3].mul_wide(C3);
			const int64_t t1 = y[1].mul_wide(C3) - y[3].mul_wide(C1);
			x[0] = fx::from_sum(b0 + t0);
			x[3] = fx::from_sum(b0 - t0);
			x[1] = fx::from_sum(b1 + t1);
			x[2] = fx::from_sum(b1 - t1);
		}

		inline void forward_ortho_inplace(fixed16_16 x[4])
		{
			fixed16_16 y[4];
			forward_ortho(x, y);
			x[0] = y[0]; x[1] = y[1]; x[2] = y[2]; x[3] = y[3];
		}

		inline void inverse_ortho_inplace(fixed16_16 x[4])
		{
			fixed16_16 y[4];
			inverse_ortho(x, y);
			x[0] = y[0]; x[1] = y[1]; x[2] = y[2]; x[3] = y[3];
		}
	}

	class dct2fx
	{
		enum { cMaxSize = 12 };

	public:
		typedef fixed16_16 fx;

		dct2fx() : m_rows(0u), m_cols(0u) {}

		// call with grid_height/grid_width (INVERTED)
		bool init(uint32_t rows, uint32_t cols)
		{
			if ((rows < 2u) || (rows > cMaxSize) ||
				(cols < 2u) || (cols > cMaxSize))
			{
				assert(0);
				return false;
			}

			m_rows = rows;
			m_cols = cols;

			m_c_col.assign(m_rows * m_rows, fx());
			m_c_row.assign(m_cols * m_cols, fx());

			// tables with alpha folded in: entry = alpha(u) * cos(pi*(2x+1)*u / (2*rows))
			for (uint32_t u = 0; u < m_rows; ++u)
				for (uint32_t x = 0; x < m_rows; ++x)
					m_c_col[u * m_rows + x] = table_entry(u, x, m_rows);

			for (uint32_t v = 0; v < m_cols; ++v)
				for (uint32_t y = 0; y < m_cols; ++y)
					m_c_row[v * m_cols + y] = table_entry(v, y, m_cols);

#ifndef NDEBUG
			// one-time sanity check (debug builds): hash the table entries of ALL
			// legal sizes 2..12 against a golden constant. Any platform/compiler
			// generating different bits trips this assert immediately.
			static const bool s_tables_ok = check_tables();
			assert(s_tables_ok && "dct2fx: table generation differs from golden hash");
#endif

			return true;
		}

		uint32_t rows() const { return m_rows; }
		uint32_t cols() const { return m_cols; }

		void forward(const fx* pSrc, fx* pDst, fxvec& work) const
		{
			forward(pSrc, m_cols, pDst, m_cols, work);
		}

		void inverse(const fx* pSrc, fx* pDst, fxvec& work) const
		{
			inverse(pSrc, m_cols, pDst, m_cols, work);
		}

		void inverse_check(const fx* pSrc, fx* pDst, fxvec& work) const
		{
			inverse_check(pSrc, m_cols, pDst, m_cols, work);
		}

		void forward(const fx* pSrc, uint32_t src_stride,
			fx* pDst, uint32_t dst_stride, fxvec& work) const
		{
			assert(m_rows && m_cols);
			work.resize(m_rows * m_cols);
			forward(pSrc, src_stride, pDst, dst_stride, &work[0]);
		}

		void forward(const fx* pSrc, uint32_t src_stride,
			fx* pDst, uint32_t dst_stride, fx* pWork) const
		{
			assert(m_rows && m_cols);

			if ((m_rows == 4u) && (m_cols == 4u))
			{
				forward_4x4(pSrc, src_stride, pDst, dst_stride);
				return;
			}

			const uint32_t m = m_rows, n = m_cols;

			// horizontal
			for (uint32_t x = 0; x < m; ++x)
			{
				const fx* pRowIn = pSrc + x * src_stride;
				fx* pRowT = pWork + x * n;
				for (uint32_t v = 0; v < n; ++v)
				{
					const fx* pCv = &m_c_row[v * n];
					int64_t acc = 0;
					for (uint32_t y = 0; y < n; ++y)
						acc += pRowIn[y].mul_wide(pCv[y]);
					pRowT[v] = fx::from_sum(acc);          // alpha already folded in
				}
			}

			// vertical
			for (uint32_t v = 0; v < n; ++v)
			{
				for (uint32_t u = 0; u < m; ++u)
				{
					const fx* pCu = &m_c_col[u * m];
					int64_t acc = 0;
					for (uint32_t x = 0; x < m; ++x)
						acc += pWork[x * n + v].mul_wide(pCu[x]);
					pDst[u * dst_stride + v] = fx::from_sum(acc);
				}
			}
		}

		void inverse(const fx* pSrc, uint32_t src_stride,
			fx* pDst, uint32_t dst_stride, fxvec& work) const
		{
			assert(m_rows && m_cols);

			if ((m_rows == 4u) && (m_cols == 4u))
			{
				inverse_4x4(pSrc, src_stride, pDst, dst_stride);
				return;
			}

			work.resize(m_rows * m_cols);

			const uint32_t m = m_rows, n = m_cols;
			fx* pWork = &work[0];

			// vertical
			for (uint32_t v = 0; v < n; ++v) // cols
			{
				int64_t sums[cMaxSize] = { 0 };

				for (uint32_t u = 0; u < m; ++u) // rows
				{
					const fx yU = pSrc[u * src_stride + v];
					if (yU.v == 0)               // most coeffs will be 0
						continue;

					const fx* pCu = &m_c_col[u * m];
					for (uint32_t x = 0; x < m; ++x)
						sums[x] += yU.mul_wide(pCu[x]);
				} // u

				for (uint32_t x = 0; x < m; ++x)
					pWork[x * n + v] = fx::from_sum(sums[x]);
			} // v

			// horizontal
			for (uint32_t x = 0; x < m; ++x)  // rows
			{
				const fx* pRowT = pWork + x * n;
				fx* pRowOut = pDst + x * dst_stride;

				for (uint32_t y = 0; y < n; ++y) // cols
				{
					int64_t acc = 0;
					for (uint32_t v = 0; v < n; ++v)  // cols
						acc += pRowT[v].mul_wide(m_c_row[v * n + y]);
					pRowOut[y] = fx::from_sum(acc);
				}
			}
		}

		void inverse_check(const fx* pSrc, uint32_t src_stride,
			fx* pDst, uint32_t dst_stride, fxvec& work) const
		{
			assert(m_rows && m_cols);
			work.resize(m_rows * m_cols);

			const uint32_t m = m_rows, n = m_cols;
			fx* pWork = &work[0];

			// vertical
			for (uint32_t v = 0; v < n; ++v)
			{
				for (uint32_t x = 0; x < m; ++x)
				{
					int64_t acc = 0;
					for (uint32_t u = 0; u < m; ++u)
						acc += pSrc[u * src_stride + v].mul_wide(m_c_col[u * m + x]);
					pWork[x * n + v] = fx::from_sum(acc);
				}
			}

			// horizontal
			for (uint32_t x = 0; x < m; ++x)  // rows
			{
				const fx* pRowT = pWork + x * n;
				fx* pRowOut = pDst + x * dst_stride;

				for (uint32_t y = 0; y < n; ++y) // cols
				{
					int64_t acc = 0;
					for (uint32_t v = 0; v < n; ++v)  // cols
						acc += pRowT[v].mul_wide(m_c_row[v * n + y]);
					pRowOut[y] = fx::from_sum(acc);
				}
			}
		}

	private:
		// Specialized 4x4 path via the dct4 butterflies. Bit-identical to the
		// general matrix path (same quantized constants, same wide sums, one
		// rounding per output), just ~2.5x fewer multiplies. inverse_check is
		// deliberately NOT dispatched: it stays the independent reference.
		void forward_4x4(const fx* pSrc, uint32_t src_stride,
			fx* pDst, uint32_t dst_stride) const
		{
			fx t[16];
			for (uint32_t x = 0; x < 4; ++x)          // horizontal
				dct4::forward_ortho(pSrc + x * src_stride, t + x * 4);
			for (uint32_t v = 0; v < 4; ++v)          // vertical
			{
				const fx col[4] = { t[v], t[4 + v], t[8 + v], t[12 + v] };
				fx out[4];
				dct4::forward_ortho(col, out);
				for (uint32_t u = 0; u < 4; ++u)
					pDst[u * dst_stride + v] = out[u];
			}
		}

		void inverse_4x4(const fx* pSrc, uint32_t src_stride,
			fx* pDst, uint32_t dst_stride) const
		{
			fx t[16];
			for (uint32_t v = 0; v < 4; ++v)          // vertical
			{
				const fx col[4] = { pSrc[v], pSrc[src_stride + v],
									pSrc[2 * src_stride + v], pSrc[3 * src_stride + v] };
				fx out[4];
				dct4::inverse_ortho(col, out);
				for (uint32_t x = 0; x < 4; ++x)
					t[x * 4 + v] = out[x];
			}
			for (uint32_t x = 0; x < 4; ++x)          // horizontal
				dct4::inverse_ortho(t + x * 4, pDst + x * dst_stride);
		}

		static fx table_entry(uint32_t u, uint32_t x, uint32_t n)
		{
			const int64_t a = u ? dct_detail::alpha_q30(n) : dct_detail::alpha0_q30(n);
			const int64_t c = dct_detail::cos_pi_frac_q30((2u * x + 1u) * u, 2u * n);
			return fx::from_raw(dct_detail::q60_to_q16(a * c));
		}

#ifndef NDEBUG
		static bool check_tables()   // FNV-1a, same constant the offline test bakes
		{
			uint64_t h = 1469598103934665603ull;
			for (uint32_t r = 2; r <= cMaxSize; r++)
				for (uint32_t c = 2; c <= cMaxSize; c++)
					for (int pass = 0; pass < 2; pass++) {
						const uint32_t n = pass ? c : r;
						for (uint32_t u = 0; u < n; ++u)
							for (uint32_t x = 0; x < n; ++x) {
								const uint32_t raw = uint32_t(table_entry(u, x, n).v);
								for (int b = 0; b < 4; b++) { h ^= uint8_t(raw >> (b * 8)); h *= 1099511628211ull; }
							}
					}
			return h == 0x013A49075AF22067ull;
		}
#endif

		uint32_t m_rows, m_cols;
		fxvec m_c_col;   // alpha(u) * cos, [u*m_rows + x]
		fxvec m_c_row;   // alpha(v) * cos, [v*m_cols + y]
	};

	inline constexpr uint8_t g_zigzag4x4_xy[16][2] = // [index][X,Y]
	{
		{ 0, 0 },
		{ 1, 0 },
		{ 0, 1 },
		{ 0, 2 },
		{ 1, 1 },
		{ 2, 0 },
		{ 3, 0 },
		{ 2, 1 },
		{ 1, 2 },
		{ 0, 3 },
		{ 1, 3 },
		{ 2, 2 },
		{ 3, 1 },
		{ 3, 2 },
		{ 2, 3 },
		{ 3, 3 }
	};
		
	inline constexpr fixed16_16 g_base_4x4_quant[16] =
	{
		fixed16_16::from_float_and_raw(1.0f, 65536),     fixed16_16::from_float_and_raw(3.5f, 229376),    fixed16_16::from_float_and_raw(24.0f, 1572864),  fixed16_16::from_float_and_raw(51.0f, 3342336),
		fixed16_16::from_float_and_raw(3.5f, 229376),    fixed16_16::from_float_and_raw(12.0f, 786432),   fixed16_16::from_float_and_raw(40.0f, 2621440),  fixed16_16::from_float_and_raw(78.0f, 5111808),
		fixed16_16::from_float_and_raw(24.0f, 1572864),  fixed16_16::from_float_and_raw(40.0f, 2621440),  fixed16_16::from_float_and_raw(68.0f, 4456448),  fixed16_16::from_float_and_raw(103.0f, 6750208),
		fixed16_16::from_float_and_raw(51.0f, 3342336),  fixed16_16::from_float_and_raw(78.0f, 5111808),  fixed16_16::from_float_and_raw(103.0f, 6750208), fixed16_16::from_float_and_raw(120.0f, 7864320)
	};

	static inline void compute_quant_table_fixed(fixed16_16 q, fixed16_16 level_scale, int* dct_quant_tab)
	{
		const uint32_t grid_width = 4, grid_height = 4;

		assert(q > fixed16_16());

		dct_quant_tab[0] = 1;

		if (q >= fixed16_16::from_int(100))
		{
			for (uint32_t y = 0; y < grid_height; y++)
			{
				for (uint32_t x = 0; x < grid_width; x++)
				{
					if (x || y)
					{
						dct_quant_tab[x + y * grid_width] = 1;
					}
				}
			}
			return;
		}

		for (uint32_t y = 0; y < grid_height; y++)
		{
			for (uint32_t x = y ? y : 1; x < grid_width; x++)
			{
				assert(x || y);

				fixed16_16 base = g_base_4x4_quant[x + y * 4];

				//int quant_scale = (base * level_scale).round_to_int();
				
				int quant_scale = base.mul_round_to_int(level_scale);

				quant_scale = basisu::maximum<int>(1, quant_scale);

				if ((x + y) == 1)
				{
					const int MAX_QUANT_SCALE_AC_1_1 = 73; // 73
					quant_scale = minimum(quant_scale, MAX_QUANT_SCALE_AC_1_1);
				}

				dct_quant_tab[x + y * grid_width] = quant_scale;
				dct_quant_tab[y + x * grid_width] = quant_scale;
			} // x

		} // y
	}

	struct coeff
	{
		int16_t m_num_zeros; // number of zero AC coefficients before this one
		int16_t m_coeff; // both sign and mag, [-255,255], or INT16_MAX if last

		void clear()
		{
			m_num_zeros = 0;
			m_coeff = 0;
		}
	};

	typedef basisu::vector<coeff> coeff_vec;

	struct dct_syms
	{
		int16_t m_dc;		// [-255,255]
				
		coeff_vec m_ac_vals;

		void clear()
		{
			m_dc = 0;
			m_ac_vals.resize(0);
		}
	};

	// ---- standalone (de)serialization of dct_syms <-> a flat 4x4 quantized-coeff
	// grid, used ONLY by the optional AC-truncation RDO. forward()/inverse() are
	// left untouched; these mirror their exact run-length format so a re-packed
	// array is always canonical & correctly terminated. Natural index = x + y*4.

	// Unpack a (valid) dct_syms AC run-length list into flat[16] (DC at [0]).
	static inline void xbc7_syms_to_flat(const dct_syms& syms, int flat[16])
	{
		for (uint32_t i = 0; i < 16; i++)
			flat[i] = 0;

		flat[0] = syms.m_dc;

		uint32_t zig_idx = 1;
		for (uint32_t i = 0; i < syms.m_ac_vals.size(); i++)
		{
			zig_idx += (uint32_t)syms.m_ac_vals[i].m_num_zeros;
			if (zig_idx >= 16)
				break; // EOB / end: remaining slots stay zero
			if (syms.m_ac_vals[i].m_coeff == INT16_MAX)
				break; // defensive (shouldn't occur with zig_idx < 16)

			flat[g_zigzag4x4_xy[zig_idx][0] + g_zigzag4x4_xy[zig_idx][1] * 4] = syms.m_ac_vals[i].m_coeff;
			zig_idx++;
		}
	}

	// Re-pack flat[16] into a canonical dct_syms (DC + RLE ACs + trailing EOB),
	// identical in form to forward()'s emission.
	static inline void xbc7_flat_to_syms(const int flat[16], dct_syms& syms)
	{
		syms.clear();
		syms.m_dc = basisu::safe_cast_int16(flat[0]);

		int total_zeros = 0;
		for (uint32_t i = 1; i < 16; i++)
		{
			const int ac = flat[g_zigzag4x4_xy[i][0] + g_zigzag4x4_xy[i][1] * 4];
			if (!ac)
			{
				total_zeros++;
				continue;
			}

			coeff cf;
			cf.m_num_zeros = basisu::safe_cast_int16(total_zeros);
			cf.m_coeff = basisu::safe_cast_int16(ac);
			syms.m_ac_vals.push_back(cf);
			total_zeros = 0;
		}

		if (total_zeros)
		{
			coeff cf;
			cf.m_num_zeros = basisu::safe_cast_int16(total_zeros);
			cf.m_coeff = INT16_MAX;
			syms.m_ac_vals.push_back(cf);
		}
	}

	inline constexpr fixed16_16 DEADZONE_ALPHA_FIXED = fixed16_16::from_float_and_raw(0.5f, 32768);

	inline constexpr fixed16_16 g_scale_quant_steps_fixed[3] =
	{
		fixed16_16::from_float_and_raw(1.35588217f, 88859), // 4 (2-bits)
		fixed16_16::from_float_and_raw(1.24573100f, 81640), // 8 (3-bits)
		fixed16_16::from_float_and_raw(1.15431654f, 75649), // 16 (4-bits)
	};

	static inline uint32_t get_weight_size_index_from_bits(uint32_t num_weight_bits)
	{
		switch (num_weight_bits)
		{
		case 2: return 0;
		case 3: return 1;
		case 4: return 2;
		default:
			assert(0);
			return 0;
		}
	}

	// When true, weight-grid DC coefficients are uniformly quantized by
	// XBC7_DC_QUANT (6-bit magnitude + sign instead of 8). The orthonormal
	// 4x4 DC spans [-256, 256] but the weights themselves only span [0, 64],
	// so a step of 4 costs at most +-2 DC == +-0.5 of one weight step spread
	// across the whole block -- visually negligible, while the DC stream is
	// one of the largest in the file. NOTE: format-affecting and not (yet)
	// signalled in the stream: encoder and decoder must be built alike.
	inline bool g_xbc7_quantize_dc = true;
	inline constexpr int XBC7_DC_QUANT = 4;

	// When ALSO true, the DC precision scales with the plane's weight depth:
	// (weight_bits + 2) magnitude bits, i.e. quant step 2^(6 - weight_bits)
	// (2-bit: 16, 3-bit: 8, 4-bit: 4). Rationale: an n-bit plane's
	// reconstruction snaps to a 2^n-level grid (step ~64/(2^n - 1) in [0,64]
	// space), so coarse planes tolerate proportionally coarser DC. Same
	// build-alike caveat as above.
	static bool g_xbc7_dc_quant_per_weight_bits = true;

	static inline int get_xbc7_dc_quant(uint32_t num_weight_bits)
	{
		if (!g_xbc7_dc_quant_per_weight_bits)
			return XBC7_DC_QUANT;

		assert((num_weight_bits >= 2) && (num_weight_bits <= 4));
		return 1 << (6 - num_weight_bits);
	}

	class xbc7_weight_grid_dct_fixed
	{
	public:
		typedef basist::fixed16_16 fx;

		xbc7_weight_grid_dct_fixed()
		{}

		void init()
		{
			m_dct.init(BLOCK_HEIGHT, BLOCK_WIDTH);
		}

		void forward(
			fx global_q, uint32_t plane_index,
			const int* pWeight_predictions, // may be nullptr
			const basist::bc7u::log_bc7_block& log_blk,
			dct_syms& syms,
			fxvec& dct_work)
		{
			syms.clear();

			fx orig_weights[16];
			for (uint32_t i = 0; i < 16; i++)
			{
				const int predicted_weight = pWeight_predictions ? pWeight_predictions[i] : 0;
				assert((predicted_weight >= 0) && (predicted_weight <= 64));

				orig_weights[i] = fx::from_int(
					basist::bc7u::dequant_weight(log_blk.m_weights[plane_index][i], log_blk.m_weight_bits[plane_index]) - predicted_weight);
			}

			fx dct_weights[16];
			m_dct.forward(orig_weights, dct_weights, dct_work);

			const fx span_len = get_max_span_len(log_blk, plane_index);
			const fx level_scale = compute_level_scale(global_q, span_len, log_blk.m_weight_bits[plane_index]);

			int dct_quant_tab[16];
			compute_quant_table_fixed(global_q, level_scale, dct_quant_tab);

			int dct_coeffs[16];

			for (uint32_t y = 0; y < 4; y++)
			{
				for (uint32_t x = 0; x < 4; x++)
				{
					if (!x && !y)
					{
						int dc = basisu::clamp<int>(dct_weights[0].round_to_int(), -255, 255);

						if (g_xbc7_quantize_dc)
						{
							// plain uniform quantizer (no deadzone), round half
							// away from zero, mirrored by inverse(). The full
							// quantized range [-256/q, 256/q] fits a magnitude
							// byte at every weight depth, so no clipping at the
							// extremes (the old -1 was a byte-range vestige).
							const int q = get_xbc7_dc_quant(log_blk.m_weight_bits[plane_index]);
							const int max_mag = 256 / q;

							dc = (dc >= 0) ? ((dc + (q / 2)) / q) : -(((-dc) + (q / 2)) / q);
							dc = basisu::clamp<int>(dc, -max_mag, max_mag);
						}

						dct_coeffs[0] = dc;
						continue;
					}

					const int levels = dct_quant_tab[x + y * 4];

					const fx d = dct_weights[x + y * 4];

					const int id = quantize_deadzone(d, levels, DEADZONE_ALPHA_FIXED, x, y);

					dct_coeffs[x + y * 4] = basisu::clamp<int>(id, -255, 255); // clamping to [-255,255] not 256

				} // x

			}  // y

			syms.m_dc = basisu::safe_cast_int16(dct_coeffs[0]);

			syms.m_ac_vals.reserve(17);

			int total_zeros = 0;
			for (uint32_t i = 1; i < 16; i++)
			{
				const uint32_t dct_idx = g_zigzag4x4_xy[i][0] + (g_zigzag4x4_xy[i][1] * 4);
				assert(dct_idx);

				int ac_coeff = dct_coeffs[dct_idx];
				if (!ac_coeff)
				{
					total_zeros++;
					continue;
				}

				coeff cf;
				cf.m_num_zeros = basisu::safe_cast_int16(total_zeros);
				cf.m_coeff = basisu::safe_cast_int16(ac_coeff);

				syms.m_ac_vals.push_back(cf);

				total_zeros = 0;
			}

			if (total_zeros)
			{
				coeff cf;
				cf.m_num_zeros = basisu::safe_cast_int16(total_zeros);
				cf.m_coeff = INT16_MAX;
				syms.m_ac_vals.push_back(cf);
			}
		}

		bool inverse(
			fx global_q, uint32_t plane_index,
			const int* pWeight_predictions, // may be nullptr
			const dct_syms& syms,
			basist::bc7u::log_bc7_block& log_blk,
			fxvec& dct_work)
		{
			const fx span_len = get_max_span_len(log_blk, plane_index);
			const fx level_scale = compute_level_scale(global_q, span_len, log_blk.m_weight_bits[plane_index]);

			int dct_quant_tab[16];
			compute_quant_table_fixed(global_q, level_scale, dct_quant_tab);

			fx dct_weights[16];
			for (uint32_t i = 0; i < 16; i++)
				dct_weights[i] = fx();

			// hostile streams can carry any byte here; *16 worst case (255*16 =
			// 4080) is still inside the 4x4 IDCT's safe input range (~8191 at
			// gain 4), so the decoder remains total
			dct_weights[0] = fx::from_int(g_xbc7_quantize_dc ?
				((int)syms.m_dc * get_xbc7_dc_quant(log_blk.m_weight_bits[plane_index])) : (int)syms.m_dc);

			uint32_t zig_idx = 1;
			uint32_t coeff_ofs = 0;
			while (coeff_ofs < syms.m_ac_vals.size())
			{
				const uint32_t run_len = syms.m_ac_vals[coeff_ofs].m_num_zeros;
				const int coeff = syms.m_ac_vals[coeff_ofs].m_coeff;
				coeff_ofs++;

				if ((run_len + zig_idx) > 16)
					return false;

				zig_idx += run_len;

				if (zig_idx >= 16)
					break;

				// INT16_MAX is impossible in a valid stream. The float class
				// asserts here; a deterministic decoder must instead behave
				// IDENTICALLY on every input in every build (debug included),
				// so malformed streams are rejected, never trapped.
				if (coeff == INT16_MAX)
					return false;

				const int x = g_zigzag4x4_xy[zig_idx][0];
				const int y = g_zigzag4x4_xy[zig_idx][1];
				const int dct_idx = x + (y * 4);

				const int quant = dct_quant_tab[dct_idx];

				dct_weights[dct_idx] = dequant_deadzone(coeff, quant, DEADZONE_ALPHA_FIXED, x, y);

				zig_idx++;
			}

			fx idct_weights[16];
			m_dct.inverse(dct_weights, idct_weights, dct_work);

			for (uint32_t i = 0; i < 16; i++)
			{
				const int pred = pWeight_predictions ? pWeight_predictions[i] : 0;
				log_blk.m_weights[plane_index][i] = basist::bc7u::quant_weight(
					basisu::clamp<int>((idct_weights[i] + fx::from_int(pred)).round_to_int(), 0, 64),
					log_blk.m_weight_bits[plane_index]);
			}

			return true;
		}

	private:
		static const uint32_t BLOCK_WIDTH = 4;
		static const uint32_t BLOCK_HEIGHT = 4;

		dct2fx m_dct;

		// sqrt of a plain non-negative integer, result Q15.16, round-to-nearest.
		// Bypasses fixed::sqrt because the input (sum of squares, <= 260100)
		// exceeds the Q15.16 VALUE range; only the result (<= 510) must fit.
		static fx isqrt_to_fixed(uint32_t ssq)
		{
			const uint64_t x = uint64_t(ssq) << 32;
			uint64_t f = basist::fixed_detail::isqrt_floor(x);
			f += (x - f * f > f);              // round to nearest
			return fx::from_raw((int32_t)f);
		}

		// Needed by AQ. Endpoint values are 8-bit ints: accumulate the sum of
		// squares EXACTLY in integer math, one deterministic sqrt at the end.
		fx get_max_span_len(const basist::bc7u::log_bc7_block& log_blk, uint32_t plane_index) const
		{
			uint32_t max_ssq = 0;

			if (log_blk.is_dual_plane())
			{
				basist::color_rgba ep[2];

				basist::bc7u::unpack_endpoints(log_blk, ep, 0);

				const basist::color_rgba& l = ep[0];
				const basist::color_rgba& h = ep[1];

				for (uint32_t c = 0; c < 4; c++)
				{
					// get the weight plane used by this endpoint channel (NOT the decoded
					// pixel channel, which is after any mode 4/5 channel swapping/rotation)
					const uint32_t endpoint_chan_plane = log_blk.get_endpoint_channel_weight_plane(c);

					if (endpoint_chan_plane == plane_index)
					{
						const int d = (int)h[c] - (int)l[c];
						max_ssq += (uint32_t)(d * d);
					}
				}
			}
			else
			{
				assert(!plane_index);

				for (uint32_t i = 0; i < log_blk.m_num_partitions; i++)
				{
					basist::color_rgba ep[2];

					basist::bc7u::unpack_endpoints(log_blk, ep, i);

					const basist::color_rgba& l = ep[0];
					const basist::color_rgba& h = ep[1];

					uint32_t ssq = 0;
					for (uint32_t c = 0; c < 4; c++)
					{
						const int d = (int)h[c] - (int)l[c];
						ssq += (uint32_t)(d * d);
					}

					// sqrt is monotonic: max of the roots == root of the max
					max_ssq = basisu::maximum(max_ssq, ssq);
				}
			}

			return isqrt_to_fixed(max_ssq);
		}

		// Adaptive quantization (all-integer port of the float version; the
		// comments there still apply)
		fx compute_level_scale(fx q, fx span_len, uint32_t num_weight_bits) const
		{
			const uint32_t weight_size_index = get_weight_size_index_from_bits(num_weight_bits);

			// Standard JPEG quality factor calcs
			q = basisu::clamp(q, fx::from_int(1), fx::from_int(100));

			fx level_scale = (q < fx::from_int(50))
				? fx::from_int(5000) / q
				: fx::from_int(200) - q * 2;

			level_scale = level_scale / 100; // because JPEG's quant table is scaled by 100

			const fx span_floor = fx::from_int(14);
			fx adaptive_factor = fx::from_int(64) / basisu::maximum(span_len, span_floor);

			adaptive_factor = adaptive_factor * g_scale_quant_steps_fixed[weight_size_index];

			return level_scale * adaptive_factor;
		}

		int quantize_deadzone(fx d, int L, fx alpha, uint32_t x, uint32_t y) const
		{
			assert((x < BLOCK_WIDTH) && (y < BLOCK_HEIGHT));

			if (((x == 1) && (y == 0)) ||
				((x == 0) && (y == 1)))
			{
				return (d / L).round_to_int();
			}

			if (L <= 0)
				return 0;

			const fx s = d.abs();
			const fx tau = alpha * L;                     // half-width of the zero band

			if (s <= tau)
				return 0;                                 // inside dead-zone

			// s > tau, so the quotient is positive: round_to_int (half away)
			// equals the float version's floor(qf + 0.5) here
			const int q = ((s - tau) / L).round_to_int();
			return (d < fx()) ? -q : q;
		}

		// int64 + saturation: hostile/corrupt syms (huge |q|) times a low-quality
		// L can exceed Q15.16, and a DECODER must be total -- no trap, wrap, or
		// assert on ANY input, debug builds included. Valid encoder streams can
		// never produce a dequantized coefficient beyond ~768 (a nonzero coeff
		// requires |d| > tau, bounding tau + |q|*L by ~|d| + L/2 <= ~512), so
		// saturating at +-2048 is invisible to legal bitstreams while keeping
		// every IDCT intermediate (gain <= 4) safely inside Q15.16.
		static fx sat_raw(int64_t raw)
		{
			const int64_t lim = int64_t(2048) * fx::ONE;
			return fx::from_raw((int32_t)basisu::clamp<int64_t>(raw, -lim, lim));
		}

		fx dequant_deadzone(int q, int L, fx alpha, uint32_t x, uint32_t y) const
		{
			assert((x < BLOCK_WIDTH) && (y < BLOCK_HEIGHT));

			if (((x == 1) && (y == 0)) ||
				((x == 0) && (y == 1)))
			{
				return sat_raw((int64_t)q * L * fx::ONE);
			}

			if (q == 0 || L <= 0)
				return fx();

			const int64_t aq = (q < 0) ? -(int64_t)q : (int64_t)q;
			// center of the (nonzero) bin: tau + |q|*L, computed wide
			const int64_t mag_raw = (int64_t)alpha.v * L + aq * L * fx::ONE;
			return (q < 0) ? sat_raw(-mag_raw) : sat_raw(mag_raw);
		}
	};

// KISS tagged-blob container for the XBC7 Zstd profile.
//
// Encoder: blob_stream_writer -- append bytes to blobs by ID, then
// serialize() everything to one uint8_vec. Each blob is Zstd compressed,
// UNLESS that doesn't shrink it, in which case it's stored raw (this handles
// "AC sign bits are noise" automatically -- no caller flags needed).
//
// Decoder: blob_stream_reader -- point it at the serialized bytes, init()
// scans the directory, validates everything, and decompresses all compressed
// blobs into ONE arena allocation (raw blobs are zero-copy pointers into the
// input). Queries are then O(1) table lookups. Total decoder allocations:
// exactly one (zero if nothing was compressed).
//
// Serialized format (deterministic across platforms; sizes are LEB128
// varints -- 7 bits per byte, high bit = continue, max 5 bytes for uint32):
//   [uint8 0xB7]                        (begin marker)
//   [uint8 num_blobs]                   (only non-empty blobs are stored)
//   repeated num_blobs times:
//     [uint8 id_and_flag]               (low 7 bits = blob id, so ids must be
//                                        < 128; high bit set == Zstd
//                                        compressed, clear == stored RAW)
//     if RAW:
//       [varint size]                   (never 0; size raw bytes follow)
//     if COMPRESSED:
//       [varint uncompressed_size]      (never 0)
//       [varint stored_size]            (never 0, strictly < uncompressed;
//                                        stored_size bytes follow)
//     [blob data]
//   [uint8 0x6A]                        (end marker; must land exactly at the
//                                        final byte -- trailing garbage and
//                                        truncation both fail validation)
// Per-blob overhead: typically 3 bytes raw / 5 bytes compressed (sizes under
// 16KB take 2 varint bytes). ~20 blobs -> ~80 bytes per mipmap level.
//
// IMPORTANT (decoder): raw blobs alias the input buffer. The serialized data
// passed to init() must outlive the reader.
		
	inline constexpr uint32_t BLOB_STREAM_MAX_IDS = 128; // low 7 bits of the entry byte
	inline constexpr uint8_t BLOB_STREAM_MAGIC_BEGIN = 0xB7;
	inline constexpr uint8_t BLOB_STREAM_MAGIC_END = 0x6A;

static inline uint32_t index_from_xy(uint32_t x, uint32_t y) { assert((x < 4) && (y < 4));  return x + y * 4; }

// XBC7 weight predictor candidates. The old plain-copy candidates
// (left/up/left-diag/right-diag) are subsumed by the generic XY-delta block
// references at the end of the enum.
enum xbc7_cand_t : uint32_t
{
	cCandAbsolute = 0,			// no prediction (residual == signal)

	// synthetic predictors
	cCandLeftEdge,				// left block's right edge replicated
	cCandUpperEdge,				// upper block's bottom edge replicated
	cCandLUBlend,				// left+upper edge distance blend
	cCandReflectLeft,			// left block mirrored about the shared edge
	cCandReflectUpper,			// upper block mirrored about the shared edge
	cCandLUAvg,					// left+upper edge simple average
	cCandLUBlendStrong,			// left+upper edge squared-distance blend
	cCandGradient,				// L + U - C plane gradient
	cCandGradientDamped,		// gradient blended with cCandLUBlend
	cCandDiagAvg,				// upper-left/upper-right block average
	cCandDiagEdgeBlend,			// upper-left right edge <-> upper-right left edge
	cCandUpperDiagEdgeBlend,	// upper edge blended with diagonal lateral structure
	cCandMED,					// JPEG-LS median edge detector
	cCandGAB,					// gradient-adaptive blend (CALIC-spirit)
	cCandPlaneFit,				// LS plane fit through left+upper edges
	cCandDDL,					// 45-degree diagonal-down-left propagation
	cCandDDR,					// 45-degree diagonal-down-right propagation

	// generic causal block references (copies); amp codes apply as usual
	cCandFirstXYDelta,
	cCandLastXYDelta = cCandFirstXYDelta + 31,

	cTotalCandidates
};

// Causal block reference deltas (same layout as astc_hdr_6x6::g_reuse_xy_deltas).
// All entries are causal by construction: dy < 0, or dy == 0 and dx < 0.
struct xbc7_xy_delta { int8_t m_dx, m_dy; };

inline constexpr uint32_t NUM_XY_DELTAS = 32;

inline constexpr xbc7_xy_delta g_xbc7_xy_deltas[NUM_XY_DELTAS] =
{
	{ -1, 0 }, { -2, 0 }, { -3, 0 }, { -4, 0 },
	{ 3, -1 }, { 2, -1 }, { 1, -1 }, { 0, -1 }, { -1, -1 }, { -2, -1 }, { -3, -1 }, { -4, -1 },
	{ 3, -2 }, { 2, -2 }, { 1, -2 }, { 0, -2 }, { -1, -2 }, { -2, -2 }, { -3, -2 }, { -4, -2 },
	{ 3, -3 }, { 2, -3 }, { 1, -3 }, { 0, -3 }, { -1, -3 }, { -2, -3 }, { -3, -3 }, { -4, -3 },
	{ 3, -4 }, { 2, -4 }, { 1, -4 }, { 0, -4 }
};

	inline constexpr uint32_t XBC7_FLAG_HAS_ALPHA = 1;

#pragma pack(push, 1)
	struct xbc7_header
	{
		basisu::packed_uint<2> m_width_in_texels;
		basisu::packed_uint<2> m_height_in_texels;
		uint8_t m_dct_q;
		uint8_t m_flags;

		// Encoder stripe count (>= 1). The decoder needs it because the
		// solid-block prediction is IMPLICIT (derived from neighbors on both
		// sides), so its upper-neighbor clamp at stripe seams must mirror the
		// encoder's. All EXPLICIT references remain valid anywhere causal.
		uint8_t m_num_stripes;
	};
#pragma pack(pop)

	enum xbc7_blob_id : uint8_t
	{
		// File-level metadata: dims, version, global Q, flags. Always first
		// logically; readers locate it by ID, not position.
		cBlobHeader = 0,

		// One command byte per block, raster order. Drives all other streams.
		cBlobCommands = 1,

		// Config bytes (CMD = new-config only): mode in bits 0-2, mode 4/5
		// component rotation in bits 3-4, mode 4 index selector in bit 5,
		// bits 6-7 reserved (writer zeros; decoder rejects nonzero).
		cBlobBC7BlockConfig = 2,

		// Partition indices, one byte each, split by subset count because the
		// two tables are disjoint vocabularies (same byte value = unrelated
		// geometry). Present only when the just-parsed mode is partitioned.
		cBlobPartition2 = 3,	// modes 1, 3, 7 (64 patterns)
		cBlobPartition3 = 4,	// modes 0, 2   (mode 0: index < 16, else reject)

		// Joint (candidate, amp code) predictor byte, one per full-block
		// command (CMD = new-config/reuse-config), consumed by BOTH weight
		// modes (DPCM and DCT).
		// value = cand_index + amp_code * cTotalCandidates; >= 200 rejects.
		cBlobWeightPredictors = 5,

		// DC values, one per coded plane of every WT = DCT block (dual-plane
		// = two). Lattice-coded magnitude; sign is conditional (absolute-
		// predictor planes are unsigned by construction -- no sign emitted).
		cBlobDCCoeffsSmall = 6, // 2/3-bit weight modes

		cBlobDCCoeffsLarge = 7, // 4-bit weight modes

		cBlobACCoeffs = 8,

		// Bit-packed raw sign bits: AC signs, plus DC signs where present.
		cBlobCoeffSigns = 9,

		// Bit-packed endpoint p-bit RESIDUALS for the DPCM endpoint modes
		// (1 or 2 per subset per the mode's pbit shape), stored raw. The
		// EP = raw escape path's p-bits travel in cBlobEPRaw instead. Own
		// stream so the accounting can price them.
		cBlobPBits = 10,

		cBlobEPDeltaFineR = 11, // >= 6 bits
		cBlobEPDeltaFineG = 12,	
		cBlobEPDeltaFineB = 13,	
		cBlobEPDeltaFineA = 14,

		cBlobEPDeltaCoarseR = 15, // < 6 bits
		cBlobEPDeltaCoarseG = 16,
		cBlobEPDeltaCoarseB = 17,
		cBlobEPDeltaCoarseA = 18,

		// Raw endpoints (EP = raw escape)
		cBlobEPRaw = 19,

		// EP = indexed-DPCM block references: one byte per reference holding
		// the 5-bit delta-table index (top 3 bits reserved-zero).
		cBlobEPBlockIndex = 20,

		// WT = DPCM with the absolute predictor: the plane's quantized weight
		// indices, byte-packed (2-bit: 4 per byte LSB-first; 3-bit: expanded to
		// nibbles, 2 per byte; 4-bit: 2 per byte). Each plane is a whole number
		// of bytes, so planes never straddle a byte.
		cBlobRawWeightBits = 21,

		// CMD = solid: per-solid-block DPCM residual vs the neighbor edge
		// prediction, one wrapped byte per channel in R,G,B,A order (3 bytes
		// when the file has no alpha, 4 with alpha) in 8-bit PIXEL space --
		// distinct domain and predictor from the endpoint deltas, so its own
		// stream and FSE context. Interleaved rather than planar; revisit if
		// UI-class corpora make this blob dominant.
		cBlobSolidRGBADeltas = 22,

		// WT = DPCM with a real predictor: wrapped n-bit weight index residuals,
		// byte-packed exactly like cBlobRawWeightBits, split by bit width
		// (disjoint vocabularies -- same byte value, unrelated statistics).
		cBlobDPCMWeightResid2 = 23,
		cBlobDPCMWeightResid3 = 24,
		cBlobDPCMWeightResid4 = 25,

		// Per-stripe seek table (present only when num_stripes > 1): for each
		// stripe, the start offset of its data in every per-stripe stream id
		// 1..25 -- a BYTE offset for byte blobs, a BIT offset for the three
		// bit blobs (coeff_signs, pbits, ep_raw). Lets the decoder seek each
		// stripe directly and decode them independently (in parallel). Stored
		// as little-endian packed_uint<4>, stripe-major, as DELTAS from the
		// previous stripe's start (stripe 0's delta is always 0) -- the small
		// non-monotonic values compress far better than absolute offsets. The
		// decoder reconstructs absolute offsets with a running prefix sum.
		cBlobStripeSeekTable = 26,

		// 27..127 reserved for future streams (P-frame motion, temporal
		// references, optional tables). IDs >= 128 are invalid (the blob
		// container uses bit 7 as its compression flag).
		//
		// NOTE: IDs freeze permanently at the FIRST golden mint, not before.
		cBlobFirstUnused = 27
	};

	enum class xbc7_command_id : uint8_t
	{
		cCmdRepeatLast = 0,
		cCmdRepeatUpper = 1,
		cCmdSolidDPCM = 2,
		cCmdNewConfig = 3,
		cCmdReuseConfigLeft = 4,
		cCmdReuseConfigUpper = 5,
		cCmdReuseConfigLeftDiagonal = 6,
		cCmdReuseConfigRightDiagonal = 7
	};

	enum class xbc7_command_endpoint_mode : uint8_t
	{
		cCmdEndpointRaw = 0,
		cCmdEndpointDPCMLeft = 1,
		cCmdEndpointDPCMUp = 2,
		cCmdEndpointDPCMLeftDiagonal = 3,
		cCmdEndpointDPCMRightDiagonal = 4,
		cCmdEndpointDPCMBlockIndex = 5,

		// like Left/Up but predicting from the neighbor's SECOND subset --
		// useful when a partitioned neighbor's other half matches better.
		// The decoder REJECTS these when the referenced block has fewer than
		// 2 subsets.
		cCmdEndpointDPCMLeftSubset1 = 6,
		cCmdEndpointDPCMUpSubset1 = 7
	};
		
	enum class xbc7_command_weight_mode : uint8_t
	{
		cCmdWeightRaw = 0,
		cCmdWeightDCT = 1
	};

	inline constexpr uint32_t XBC7_COMMAND_ENDPOINT_MODE_SHIFT = 3;
	inline constexpr uint32_t XBC7_COMMAND_WEIGHT_MODE_SHIFT = 6;

	// Format-level max stripe count. The decoder REJECTS stripe counts above
	// this, and the encoder clamps to it, so it's shared by both sides; raising
	// it later is a format-affecting change. (The encoder-only sizing thresholds
	// XBC7_MIN_IMAGE_TEXEL_ROWS_TO_STRIPE / XBC7_MIN_STRIPE_BLOCK_ROWS stay in
	// basisu_xbc7_encode.cpp.)
	inline constexpr uint32_t XBC7_MAX_ENCODER_STRIPES = 16;

	struct stripe_range
	{
		uint32_t m_first_block_row = 0;
		uint32_t m_num_block_rows = 0;
	};

	// Inclusive 2D bounding box (in BC7 logical block coords) that a coding
	// unit may reference. Generalizes the stripe row-clamp: EVERY causal
	// predictor access -- neighbor/diagonal blocks, the XY-delta block
	// references, and the weight predictor bank -- is gated through
	// contains(), so the encoder can never read a block outside its tile.
	// Initially each tile is a full-width stripe { 0, first_row,
	// num_blocks_x-1, last_row }, so the AABB test is identical to the old
	// row clamp and the emitted bytes don't change; later, narrower tiles
	// enable 2D-parallel encode. The decoder passes a whole-image tile, so
	// it stays fully permissive and decoding is unaffected for now.
	struct tile_bounds
	{
		int m_bx0 = 0, m_by0 = 0, m_bx1 = 0, m_by1 = 0; // inclusive

		bool contains(int bx, int by) const
		{
			return (bx >= m_bx0) && (bx <= m_bx1) && (by >= m_by0) && (by <= m_by1);
		}
	};

	// Splits num_blocks_y rows as evenly as possible into num_stripes
	// contiguous ranges (the first num_blocks_y % num_stripes stripes carry
	// one extra row). Shared by the encoder AND the decoder: the decoder
	// rebuilds the same geometry from the header's stripe count, because the
	// solid-block prediction is implicit and must clamp identically on both
	// sides.
	[[maybe_unused]] static void compute_stripe_ranges(uint32_t num_blocks_y, uint32_t num_stripes, basisu::vector<stripe_range>& stripes)
	{
		assert((num_stripes >= 1) && (num_stripes <= num_blocks_y));

		stripes.resize(num_stripes);

		const uint32_t base_rows = num_blocks_y / num_stripes;
		const uint32_t extra_rows = num_blocks_y % num_stripes;

		uint32_t cur_row = 0;
		for (uint32_t i = 0; i < num_stripes; i++)
		{
			stripes[i].m_first_block_row = cur_row;
			stripes[i].m_num_block_rows = base_rows + ((i < extra_rows) ? 1 : 0);
			cur_row += stripes[i].m_num_block_rows;
		}

		assert(cur_row == num_blocks_y);
	}


	// eval_weight_predictor: reconstruct the 16 weight predictions for predictor
	// (cand_index, amp_code) at block (bx,by). SHARED by the encoder (predictor
	// search) and the decoder (reconstruction); DEFINED in
	// basisu_xbc7_decode.cpp so both sides link a single copy. Returns false for
	// an invalid / out-of-tile candidate.
	bool eval_weight_predictor(
		uint32_t cand_index, uint32_t amp_code,
		uint32_t bx, uint32_t by, uint32_t num_blocks_x,
		const tile_bounds& tile,
		const vector2D<basist::bc7u::log_bc7_block>& log_blks,
		uint32_t p, int pOut_preds[16]);

	// ----------------------------- decoder API -----------------------------
	// Bounds-checked read-only byte view that the decoder takes as input (the
	// decoder no longer accepts a uint8_vec -- it's a low-level API over a
	// pointer+size). Unlike std::span it TRAPS on any access outside [0,size):
	// assert() in debug, and a safe sentinel (the first byte, or 0 if empty) in
	// all builds -- so a malformed stream or a decoder bug degrades to a
	// controlled, reproducible value instead of reading bogus memory or crashing.
	// The viewed buffer must outlive every decode call that uses the span.
	struct byte_span
	{
		const uint8_t* m_p = nullptr;
		size_t         m_size = 0;

		byte_span() = default;
		byte_span(const uint8_t* p, size_t size) : m_p(p), m_size(size) {}
		// convenience for callers that hold a uint8_vec (e.g. basisu_tool)
		byte_span(const uint8_vec& v) : m_p(v.data()), m_size(v.size()) {}

		const uint8_t* data() const { return m_p; }
		size_t size() const { return m_size; }
		bool empty() const { return m_size == 0; }
		uint8_t first() const { return m_size ? m_p[0] : 0; } // safe sentinel

		// Checked single-byte read. Out of range -> assert + first byte.
		uint8_t operator[](size_t i) const
		{
			if (i < m_size)
				return m_p[i];
			assert(!"byte_span: index out of range");
			return first();
		}

		// Checked pointer to the [offset, offset+len) region, so a caller can
		// read a whole run directly (no per-byte overhead). Out of range (either
		// end) -> assert + m_p clamped to the start, so the read stays inside the
		// buffer rather than walking off it. (offset==m_size, len==0 is valid.)
		const uint8_t* checked_ptr(size_t offset, size_t len) const
		{
			if ((offset <= m_size) && (len <= m_size - offset)) // len<=m_size-offset avoids overflow
				return m_p + offset;
			assert(!"byte_span: region out of bounds");
			return m_p;
		}
	};

	// Callback-streaming decoder (same shape as the transcoder's XUASTC LDR
	// path). The decoder owns NO output image: it hands each decoded LOGICAL
	// BC7 block to the caller, who decides what to do -- pack to physical BC7,
	// store the logical block, compare vs a reference, transcode, etc. Context
	// flows through the opaque pData pointer (so captureless lambdas work as
	// callbacks with zero allocation).
	//
	// init: fired ONCE, after the header is parsed/validated and before any
	// block, so the caller can validate geometry and allocate. (block dims are
	// always 4x4 for BC7.) Return false to abort.
	typedef bool (*decode_init_callback_ptr)(
		uint32_t num_blocks_x, uint32_t num_blocks_y,
		uint32_t width_in_texels, uint32_t height_in_texels,
		uint32_t dct_q, bool has_alpha, void* pData);

	// block: fired once per decoded block. In unpack_image_threaded() it may be
	// invoked CONCURRENTLY from multiple worker threads, but always for DISTINCT
	// (bx,by) blocks (each stripe is a disjoint block-row range), and never in
	// global raster order. Return false to abort the decode.
	typedef bool (*decode_block_callback_ptr)(
		uint32_t bx, uint32_t by, const basist::bc7u::log_bc7_block& log_blk, void* pData);
	// ------------------------------ decoder internals ------------------------------
	// Tagged-blob reader (decoder side). Trivial queries are inline; the heavy
	// init_internal() (Zstd) is defined in basisu_xbc7_decoder.inl.
	class blob_stream_reader
	{
	public:
		blob_stream_reader() { clear(); }
		void clear() { memset(m_ptrs, 0, sizeof(m_ptrs)); memset(m_sizes, 0, sizeof(m_sizes)); m_arena.clear(); }
		bool init(const void* pData, size_t data_size, uint64_t max_total_uncomp = 1ULL << 30)
		{
			if (!init_internal(pData, data_size, max_total_uncomp)) { clear(); return false; }
			return true;
		}
		inline bool has(uint32_t id) const { return (id < BLOB_STREAM_MAX_IDS) && (m_sizes[id] != 0); }
		inline uint32_t get_size(uint32_t id) const { return (id < BLOB_STREAM_MAX_IDS) ? m_sizes[id] : 0; }
		inline const uint8_t* get_ptr(uint32_t id) const { return (id < BLOB_STREAM_MAX_IDS) ? m_ptrs[id] : nullptr; }
	private:
		bool init_internal(const void* pData, size_t data_size, uint64_t max_total_uncomp); // basisu_xbc7_decoder.inl
		const uint8_t* m_ptrs[BLOB_STREAM_MAX_IDS];
		uint32_t m_sizes[BLOB_STREAM_MAX_IDS];
		uint8_vec m_arena; // the single decoder allocation
		// bounds-checked LEB128; rejects encodings past 5 bytes / 32 bits
		static inline bool read_varint(const uint8_t* pBytes, size_t data_size, uint64_t& ofs, uint32_t& result)
		{
			uint32_t v = 0;
			for (uint32_t shift = 0; shift < 35; shift += 7)
			{
				if (ofs >= data_size) return false;
				const uint8_t b = pBytes[ofs++];
				if ((shift == 28) && (b > 0x0Fu)) return false;
				v |= (uint32_t)(b & 0x7Fu) << shift;
				if (!(b & 0x80u)) { result = v; return true; }
			}
			return false;
		}
	};

	// Stateful XBC7 image decoder. init() does the one-time prep; each stripe is
	// decoded by decode_stripe() (self-contained, safe to run concurrently across
	// distinct stripes). Impl in basisu_xbc7_decoder.inl.
	class image_unpacker
	{
	public:
		bool init(const byte_span& comp,
			decode_init_callback_ptr pInit_callback, void* pInit_callback_data,
			decode_block_callback_ptr pBlock_callback, void* pBlock_callback_data);
		uint32_t get_num_stripes() const { return m_num_stripes; }
		bool decode_stripe(uint32_t stripe_index);
		bool decode_all();
	private:
		bool init_tiny_mip(const byte_span& comp, bool has_alpha,
			decode_init_callback_ptr pInit_callback, void* pInit_callback_data);
		bool decode_tiny_mip();

		bool m_initialized = false;
		blob_stream_reader m_rdr;
		uint32_t m_width = 0, m_height = 0, m_global_q = 0;
		bool m_has_alpha = false;
		uint32_t m_num_blocks_x = 0, m_num_blocks_y = 0, m_num_stripes = 0;
		basisu::vector<stripe_range> m_stripes;
		basisu::vector2D<uint64_t> m_seek;
		vector2D<basist::bc7u::log_bc7_block> m_log_blks;
		decode_block_callback_ptr m_block_cb = nullptr;
		void* m_block_data = nullptr;
		bool m_tiny_mip = false;
		const uint8_t* m_tiny_blocks = nullptr;
	};

	// Caller-provided job spawner: replaces the encoder's job_pool so the threaded
	// decode can live in the transcoder. spawn_job() schedules
	// dec.decode_stripe(stripe_index) on the caller's own threads and must NOT
	// block; the caller waits for all spawned jobs itself, then inspects results.
	struct job_spawner
	{
		virtual ~job_spawner() {}
		virtual void spawn_job(image_unpacker& dec, uint32_t stripe_index) = 0;
	};

	// ------------------------------- decoder API -------------------------------
	// Single-threaded one-shot. Returns false on any malformed stream (total over
	// hostile input). Either callback may be null. Requires zstd.
	bool unpack_image(const byte_span& comp,
		decode_init_callback_ptr pInit_callback, void* pInit_callback_data,
		decode_block_callback_ptr pBlock_callback, void* pBlock_callback_data);

	// Threaded: caller owns `dec` (must outlive the spawned jobs). init + spawn one
	// job per stripe via the spawner; returns false only on init failure and does
	// NOT wait. Caller waits on its own pool, then inspects results.
	bool unpack_image_threaded(image_unpacker& dec, const byte_span& comp, job_spawner& spawner,
		decode_init_callback_ptr pInit_callback, void* pInit_callback_data,
		decode_block_callback_ptr pBlock_callback, void* pBlock_callback_data);

} // namespace xbc7
} // namespace basist
