/*
bc6h_enc -- https://github.com/0xc0de/bc6h_enc

   Single file library for BC6H compression with no external dependencies.

   The code is based on BC6HBC7.cpp from DirectXTex and localized into a single header library
   with no external dependencies.

   CREDITS:
      Alexander Samusev (0xc0de)

   Do this:
      #define BC6H_ENC_IMPLEMENTATION
   before you include this file in *one* C++ file to create the implementation.

   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define BC6H_ENC_IMPLEMENTATION
   #include "bc6h_enc.h"

   You can define:
     - for debug logging:
        #define BC6H_LOG(s) YourPrint(s)
     - for asserts:
        #define BC6H_ASSERT(expression) YourAssert(expression)
     - to override float<->half packing:
        #define BC6H_HALF_TO_FLOAT(h) YourImpl(h)
        #define BC6H_FLOAT_TO_HALF(f) YourImpl(f)


   Public interface:
      bc6h_enc::DecodeBC6HU(void* pDest, const void* pSrc)
      bc6h_enc::DecodeBC6HS(void* pDest, const void* pSrc)
      bc6h_enc::EncodeBC6HU(void* pDest, const void* pSrc)
      bc6h_enc::EncodeBC6HS(void* pDest, const void* pSrc)

 LICENSE
 -----------------------------------------------------------------------------------
 MIT License

 Copyright (c) 2022 Alexander Samusev

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 -----------------------------------------------------------------------------------
 DirectXTex
 The MIT License (MIT)

 Copyright (c) 2011-2022 Microsoft Corp

 Permission is hereby granted, free of charge, to any person obtaining a copy of this
 software and associated documentation files (the "Software"), to deal in the Software
 without restriction, including without limitation the rights to use, copy, modify,
 merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be included in all copies
 or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-----------------------------------------------------------------------------------
 Branch-free implementation of half-precision (16 bit) floating point
 Copyright 2006 Mike Acton <macton@gmail.com>

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE
 -----------------------------------------------------------------------------------
*/

#ifndef BC6H_ENC_IMPLEMENTATION

#ifndef BC6H_ENC_INCLUDED
#define BC6H_ENC_INCLUDED

namespace bc6h_enc
{
void DecodeBC6HU(void* pDest, const void* pSrc) noexcept;
void DecodeBC6HS(void* pDest, const void* pSrc) noexcept;
void EncodeBC6HU(void* pDest, const void* pSrc) noexcept;
void EncodeBC6HS(void* pDest, const void* pSrc) noexcept;
} // namespace bc6h_enc

#endif

#else

#include <stdint.h>

#    ifdef BC6H_SSE_INTRINSICS
#        include <immintrin.h>
#    endif

#    ifdef BC6H_ARM_NEON_INTRINSICS
#        include <arm_neon.h>
#    endif

#ifndef BC6H_ASSERT
#    define BC6H_ASSERT(expression)
#    define BC6H_ASSERT_UNDEF
#endif

#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38F // max value
#endif

#ifndef FLT_MIN
#define FLT_MIN 1.175494351e-38F // min normalized positive value
#endif

#define BC6H_INLINE inline

namespace bc6h_enc
{

namespace Impl
{

using HALF = uint16_t;

#if !defined(BC6H_HALF_TO_FLOAT)
// Fast half to float conversion based on:
// http://www.fox-toolkit.org/ftp/fasthalffloatconversion.pdf
struct FastHalfToFloat
{
    FastHalfToFloat()
    {
        m_MantissaTable[0] = 0;
        for (int i = 1; i < 1024; i++)
        {
            uint32_t m = i << 13;
            uint32_t e = 0;
            while ((m & 0x00800000) == 0)
            {
                e -= 0x00800000;
                m <<= 1;
            }
            m &= ~0x00800000;
            e += 0x38800000;
            m_MantissaTable[i] = m | e;
        }
        for (int i = 1024; i < 2048; i++)
            m_MantissaTable[i] = (i - 1024) << 13;
        m_ExponentTable[0] = 0;
        for (int i = 1; i < 31; i++)
            m_ExponentTable[i] = 0x38000000 + (i << 23);
        m_ExponentTable[31] = 0x7f800000;
        m_ExponentTable[32] = 0x80000000;
        for (int i = 33; i < 63; i++)
            m_ExponentTable[i] = 0xb8000000 + ((i - 32) << 23);
        m_ExponentTable[63] = 0xff800000;
        m_OffsetTable[0]    = 0;
        for (int i = 1; i < 32; i++)
            m_OffsetTable[i] = 1024;
        m_OffsetTable[32] = 0;
        for (int i = 33; i < 64; i++)
            m_OffsetTable[i] = 1024;
    }
    uint32_t m_MantissaTable[2048];
    uint32_t m_ExponentTable[64];
    uint32_t m_OffsetTable[64];

    BC6H_INLINE uint32_t Convert(uint16_t h) const
    {
        uint32_t exp = h >> 10;
        return m_MantissaTable[m_OffsetTable[exp] + (h & 0x3ff)] + m_ExponentTable[exp];
    }
};
FastHalfToFloat g_FastHalfToFloat;

#    define BC6H_HALF_TO_FLOAT g_FastHalfToFloat.Convert
#    define BC6H_HALF_TO_FLOAT_UNDEF

#endif

#if !defined(BC6H_FLOAT_TO_HALF)

#    ifdef _MSC_VER
#        pragma warning(push)
#        pragma warning(disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#    endif

BC6H_INLINE uint32_t _uint32_li(uint32_t a) { return (a); }
BC6H_INLINE uint32_t _uint32_dec(uint32_t a) { return (a - 1); }
BC6H_INLINE uint32_t _uint32_inc(uint32_t a) { return (a + 1); }
BC6H_INLINE uint32_t _uint32_not(uint32_t a) { return (~a); }
BC6H_INLINE uint32_t _uint32_neg(uint32_t a) { return (-a); }
BC6H_INLINE uint32_t _uint32_ext(uint32_t a) { return (((int32_t)a) >> 31); }
BC6H_INLINE uint32_t _uint32_and(uint32_t a, uint32_t b) { return (a & b); }
BC6H_INLINE uint32_t _uint32_andc(uint32_t a, uint32_t b) { return (a & ~b); }
BC6H_INLINE uint32_t _uint32_or(uint32_t a, uint32_t b) { return (a | b); }
BC6H_INLINE uint32_t _uint32_srl(uint32_t a, int sa) { return (a >> sa); }
BC6H_INLINE uint32_t _uint32_sll(uint32_t a, int sa) { return (a << sa); }
BC6H_INLINE uint32_t _uint32_add(uint32_t a, uint32_t b) { return (a + b); }
BC6H_INLINE uint32_t _uint32_sub(uint32_t a, uint32_t b) { return (a - b); }
BC6H_INLINE uint32_t _uint32_sels(uint32_t test, uint32_t a, uint32_t b)
{
    const uint32_t mask   = _uint32_ext(test);
    const uint32_t sel_a  = _uint32_and(a, mask);
    const uint32_t sel_b  = _uint32_andc(b, mask);
    const uint32_t result = _uint32_or(sel_a, sel_b);
    return (result);
}
BC6H_INLINE uint16_t half_from_float(uint32_t f)
{
    const uint32_t one                       = _uint32_li(0x00000001);
    const uint32_t f_s_mask                  = _uint32_li(0x80000000);
    const uint32_t f_e_mask                  = _uint32_li(0x7f800000);
    const uint32_t f_m_mask                  = _uint32_li(0x007fffff);
    const uint32_t f_m_hidden_bit            = _uint32_li(0x00800000);
    const uint32_t f_m_round_bit             = _uint32_li(0x00001000);
    const uint32_t f_snan_mask               = _uint32_li(0x7fc00000);
    const uint32_t f_e_pos                   = _uint32_li(0x00000017);
    const uint32_t h_e_pos                   = _uint32_li(0x0000000a);
    const uint32_t h_e_mask                  = _uint32_li(0x00007c00);
    const uint32_t h_snan_mask               = _uint32_li(0x00007e00);
    const uint32_t h_e_mask_value            = _uint32_li(0x0000001f);
    const uint32_t f_h_s_pos_offset          = _uint32_li(0x00000010);
    const uint32_t f_h_bias_offset           = _uint32_li(0x00000070);
    const uint32_t f_h_m_pos_offset          = _uint32_li(0x0000000d);
    const uint32_t h_nan_min                 = _uint32_li(0x00007c01);
    const uint32_t f_h_e_biased_flag         = _uint32_li(0x0000008f);
    const uint32_t f_s                       = _uint32_and(f, f_s_mask);
    const uint32_t f_e                       = _uint32_and(f, f_e_mask);
    const uint16_t h_s                       = _uint32_srl(f_s, f_h_s_pos_offset);
    const uint32_t f_m                       = _uint32_and(f, f_m_mask);
    const uint16_t f_e_amount                = _uint32_srl(f_e, f_e_pos);
    const uint32_t f_e_half_bias             = _uint32_sub(f_e_amount, f_h_bias_offset);
    const uint32_t f_snan                    = _uint32_and(f, f_snan_mask);
    const uint32_t f_m_round_mask            = _uint32_and(f_m, f_m_round_bit);
    const uint32_t f_m_round_offset          = _uint32_sll(f_m_round_mask, one);
    const uint32_t f_m_rounded               = _uint32_add(f_m, f_m_round_offset);
    const uint32_t f_m_denorm_sa             = _uint32_sub(one, f_e_half_bias);
    const uint32_t f_m_with_hidden           = _uint32_or(f_m_rounded, f_m_hidden_bit);
    const uint32_t f_m_denorm                = _uint32_srl(f_m_with_hidden, f_m_denorm_sa);
    const uint32_t h_m_denorm                = _uint32_srl(f_m_denorm, f_h_m_pos_offset);
    const uint32_t f_m_rounded_overflow      = _uint32_and(f_m_rounded, f_m_hidden_bit);
    const uint32_t m_nan                     = _uint32_srl(f_m, f_h_m_pos_offset);
    const uint32_t h_em_nan                  = _uint32_or(h_e_mask, m_nan);
    const uint32_t h_e_norm_overflow_offset  = _uint32_inc(f_e_half_bias);
    const uint32_t h_e_norm_overflow         = _uint32_sll(h_e_norm_overflow_offset, h_e_pos);
    const uint32_t h_e_norm                  = _uint32_sll(f_e_half_bias, h_e_pos);
    const uint32_t h_m_norm                  = _uint32_srl(f_m_rounded, f_h_m_pos_offset);
    const uint32_t h_em_norm                 = _uint32_or(h_e_norm, h_m_norm);
    const uint32_t is_h_ndenorm_msb          = _uint32_sub(f_h_bias_offset, f_e_amount);
    const uint32_t is_f_e_flagged_msb        = _uint32_sub(f_h_e_biased_flag, f_e_half_bias);
    const uint32_t is_h_denorm_msb           = _uint32_not(is_h_ndenorm_msb);
    const uint32_t is_f_m_eqz_msb            = _uint32_dec(f_m);
    const uint32_t is_h_nan_eqz_msb          = _uint32_dec(m_nan);
    const uint32_t is_f_inf_msb              = _uint32_and(is_f_e_flagged_msb, is_f_m_eqz_msb);
    const uint32_t is_f_nan_underflow_msb    = _uint32_and(is_f_e_flagged_msb, is_h_nan_eqz_msb);
    const uint32_t is_e_overflow_msb         = _uint32_sub(h_e_mask_value, f_e_half_bias);
    const uint32_t is_h_inf_msb              = _uint32_or(is_e_overflow_msb, is_f_inf_msb);
    const uint32_t is_f_nsnan_msb            = _uint32_sub(f_snan, f_snan_mask);
    const uint32_t is_m_norm_overflow_msb    = _uint32_neg(f_m_rounded_overflow);
    const uint32_t is_f_snan_msb             = _uint32_not(is_f_nsnan_msb);
    const uint32_t h_em_overflow_result      = _uint32_sels(is_m_norm_overflow_msb, h_e_norm_overflow, h_em_norm);
    const uint32_t h_em_nan_result           = _uint32_sels(is_f_e_flagged_msb, h_em_nan, h_em_overflow_result);
    const uint32_t h_em_nan_underflow_result = _uint32_sels(is_f_nan_underflow_msb, h_nan_min, h_em_nan_result);
    const uint32_t h_em_inf_result           = _uint32_sels(is_h_inf_msb, h_e_mask, h_em_nan_underflow_result);
    const uint32_t h_em_denorm_result        = _uint32_sels(is_h_denorm_msb, h_m_denorm, h_em_inf_result);
    const uint32_t h_em_snan_result          = _uint32_sels(is_f_snan_msb, h_snan_mask, h_em_denorm_result);
    const uint32_t h_result                  = _uint32_or(h_s, h_em_snan_result);
    return (uint16_t)(h_result);
}
#    ifdef _MSC_VER
#        pragma warning(pop)
#    endif

#    define BC6H_FLOAT_TO_HALF half_from_float
#    define BC6H_FLOAT_TO_HALF_UNDEF

#endif

#    if defined(BC6H_SSE_INTRINSICS)
using XMVECTOR = __m128;
#    elif defined(BC6H_ARM_NEON_INTRINSICS)
using XMVECTOR = float32x4_t;
#    else
struct XMVECTOR
{
    float x;
    float y;
    float z;
    float w;
};
#    endif

// Fix-up for (1st-3rd) XMVECTOR parameters that are pass-in-register for x86, ARM, ARM64, and vector call; by reference otherwise
#    if defined(BC6H_SSE_INTRINSICS) || defined(BC6H_NEON_INTRINSICS)
typedef const XMVECTOR FXMVECTOR;
#    else
typedef const XMVECTOR& FXMVECTOR;
#    endif

struct XMINT4
{
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t w;
};
struct XMHALF4
{
    HALF x;
    HALF y;
    HALF z;
    HALF w;
};
struct XMFLOAT4
{
    float x;
    float y;
    float z;
    float w;
};
BC6H_INLINE float XMConvertHalfToFloat(HALF h) noexcept
{
#    if defined(BC6H_SSE_INTRINSICS)
    __m128i V1 = _mm_cvtsi32_si128(static_cast<int>(h));
    __m128  V2 = _mm_cvtph_ps(V1);
    return _mm_cvtss_f32(V2);
#    elif defined(BC6H_ARM_NEON_INTRINSICS)
    uint16x4_t  vHalf  = vdup_n_u16(h);
    float32x4_t vFloat = vcvt_f32_f16(vreinterpret_f16_u16(vHalf));
    return vgetq_lane_f32(vFloat, 0);
#    else
    uint32_t f = BC6H_HALF_TO_FLOAT(h);
    return *reinterpret_cast<float*>(&f);
#    endif
}
BC6H_INLINE HALF XMConvertFloatToHalf(float f) noexcept
{
    return BC6H_FLOAT_TO_HALF(*reinterpret_cast<uint32_t*>(&f));
}

 struct alignas(16) XMFLOAT4A : public XMFLOAT4
{
    using XMFLOAT4::XMFLOAT4;
};

 BC6H_INLINE void XMStoreFloat4A(XMFLOAT4A* pDestination, FXMVECTOR V) noexcept
{
    BC6H_ASSERT((reinterpret_cast<uintptr_t>(pDestination) & 0xF) == 0);

#    if defined(BC6H_SSE_INTRINSICS)
    _mm_store_ps(&pDestination->x, V);
#    elif defined(BC6H_ARM_NEON_INTRINSICS)
#        if defined(_MSC_VER) && !defined(__clang__)
    vst1q_f32_ex(reinterpret_cast<float*>(pDestination), V, 128);
#        else
    vst1q_f32(reinterpret_cast<float*>(pDestination), V);
#        endif
#    else
    pDestination->x = V.x;
    pDestination->y = V.y;
    pDestination->z = V.z;
    pDestination->w = V.w;
#    endif
}
BC6H_INLINE void XMStoreHalf4(XMHALF4* pDestination, FXMVECTOR V) noexcept
{
//#    if defined(BC6H_SSE_INTRINSICS /* _XM_F16C_INTRINSICS_*/)// && !defined(_XM_NO_INTRINSICS_)
#    if defined(BC6H_SSE_INTRINSICS)
    __m128i V1 = _mm_cvtps_ph(V, _MM_FROUND_TO_NEAREST_INT);
    _mm_storel_epi64(reinterpret_cast<__m128i*>(pDestination), V1);
#    else
    pDestination->x = XMConvertFloatToHalf(V.x);
    pDestination->y = XMConvertFloatToHalf(V.y);
    pDestination->z = XMConvertFloatToHalf(V.z);
    pDestination->w = XMConvertFloatToHalf(V.w);
#    endif
}
BC6H_INLINE XMVECTOR XMLoadFloat4(const XMFLOAT4* pSource) noexcept
{
#    if defined BC6H_SSE_INTRINSICS
    return _mm_loadu_ps(&pSource->x);
#    elif defined BC6H_ARM_NEON_INTRINSIC
    return vld1q_f32(reinterpret_cast<const float*>(pSource));
#    else
    XMVECTOR V;
    V.x = pSource->x;
    V.y = pSource->y;
    V.z = pSource->z;
    V.w = pSource->w;
    return V;
#    endif
}
BC6H_INLINE XMVECTOR XMVectorSubtract(FXMVECTOR V1, FXMVECTOR V2) noexcept
{
#if defined(BC6H_SSE_INTRINSICS)
    return _mm_sub_ps(V1, V2);
#elif defined(BC6H_ARM_NEON_INTRINSIC)
    return vsubq_f32(V1, V2);
#else
    return {V1.x - V2.x,
            V1.y - V2.y,
            V1.z - V2.z,
            V1.w - V2.w};
#endif
}
struct alignas(16) XMVECTORU32
{
    union
    {
        uint32_t u[4];
        XMVECTOR v;
    };

    BC6H_INLINE operator XMVECTOR() const noexcept { return v; }

#    if defined(BC6H_SSE_INTRINSICS)
    BC6H_INLINE operator __m128i() const noexcept
    {
        return _mm_castps_si128(v);
    }
    BC6H_INLINE operator __m128d() const noexcept { return _mm_castps_pd(v); }
#    elif defined(BC6H_ARM_NEON_INTRINSIC) && defined(__GNUC__)
    BC6H_INLINE operator int32x4_t() const noexcept
    {
        return vreinterpretq_s32_f32(v);
    }
    BC6H_INLINE operator uint32x4_t() const noexcept { return vreinterpretq_u32_f32(v); }
#    endif
};

const XMVECTORU32 g_XMMask3 = {{{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000}}};

#    if defined(BC6H_SSE_INTRINSICS) || defined(BC6H_ARM_NEON_INTRINSIC)

// SSE2
//BC6H_INLINE XMVECTOR XMVector3Dot(FXMVECTOR V1, FXMVECTOR V2)
//{
//    XMVECTOR vTemp = _mm_mul_ps(V1, V2);
//    vTemp          = _mm_and_ps(vTemp, g_XMMask3);
//    vTemp          = _mm_hadd_ps(vTemp, vTemp);
//    return _mm_hadd_ps(vTemp, vTemp);
//}
// SSE3
BC6H_INLINE XMVECTOR XMVector3Dot(FXMVECTOR V1, FXMVECTOR V2)
{
    return _mm_dp_ps(V1, V2, 0x7f);
}

BC6H_INLINE float XMVectorGetX(FXMVECTOR V) noexcept
{
#    if defined(BC6H_SSE_INTRINSICS)
    return _mm_cvtss_f32(V);
#    elif defined(BC6H_ARM_NEON_INTRINSIC)
    return vgetq_lane_f32(V, 0);
#    endif
}
BC6H_INLINE float XMVectorDot(FXMVECTOR a, FXMVECTOR b)
{
    return XMVectorGetX(XMVector3Dot(a, b));
}
#else
BC6H_INLINE float XMVectorDot(FXMVECTOR a, FXMVECTOR b)
{
    // XMVectorGetX(XMVector3Dot(a, b))
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
#endif

BC6H_INLINE XMVECTOR XMLoadSInt4(const XMINT4* pSource) noexcept
{
#    if defined(BC6H_SSE_INTRINSICS)
    __m128i V = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSource));
    return _mm_cvtepi32_ps(V);
#    elif defined(BC6H_ARM_NEON_INTRINSICS_)
    int32x4_t v = vld1q_s32(reinterpret_cast<const int32_t*>(pSource));
    return vcvtq_f32_s32(v);
#    else
    XMVECTOR V;
    V.x = static_cast<float>(pSource->x);
    V.y = static_cast<float>(pSource->y);
    V.z = static_cast<float>(pSource->z);
    V.w = static_cast<float>(pSource->w);
    return V;
#    endif
}

template <typename _Tp, size_t _Nm>
constexpr size_t std__size(const _Tp (&)[_Nm]) noexcept
{
    return _Nm;
}

template <class T>
const T& std__max(const T& a, const T& b) noexcept
{
    return (a < b) ? b : a;
}

template <class T>
const T& std__min(const T& a, const T& b) noexcept
{
    return (b < a) ? b : a;
}

template <class T>
void std__swap(T& a, T& b) noexcept
{
    T temp(a);
    a = b;
    b = temp;
}

class LDRColorA;

class HDRColorA
{
public:
    float r, g, b, a;

public:
    HDRColorA() = default;
    HDRColorA(float _r, float _g, float _b, float _a) noexcept :
        r(_r), g(_g), b(_b), a(_a) {}
    HDRColorA(const HDRColorA& c) noexcept :
        r(c.r), g(c.g), b(c.b), a(c.a) {}

    // binary operators
    HDRColorA operator+(const HDRColorA& c) const noexcept
    {
        return HDRColorA(r + c.r, g + c.g, b + c.b, a + c.a);
    }

    HDRColorA operator-(const HDRColorA& c) const noexcept
    {
        return HDRColorA(r - c.r, g - c.g, b - c.b, a - c.a);
    }

    HDRColorA operator*(float f) const noexcept
    {
        return HDRColorA(r * f, g * f, b * f, a * f);
    }

    HDRColorA operator/(float f) const noexcept
    {
        const float fInv = 1.0f / f;
        return HDRColorA(r * fInv, g * fInv, b * fInv, a * fInv);
    }

    float operator*(const HDRColorA& c) const noexcept
    {
        return r * c.r + g * c.g + b * c.b + a * c.a;
    }

    // assignment operators
    HDRColorA& operator+=(const HDRColorA& c) noexcept
    {
        r += c.r;
        g += c.g;
        b += c.b;
        a += c.a;
        return *this;
    }

    HDRColorA& operator-=(const HDRColorA& c) noexcept
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        a -= c.a;
        return *this;
    }

    HDRColorA& operator*=(float f) noexcept
    {
        r *= f;
        g *= f;
        b *= f;
        a *= f;
        return *this;
    }

    HDRColorA& operator/=(float f) noexcept
    {
        const float fInv = 1.0f / f;
        r *= fInv;
        g *= fInv;
        b *= fInv;
        a *= fInv;
        return *this;
    }

    HDRColorA& Clamp(float fMin, float fMax) noexcept
    {
        r = std__min<float>(fMax, std__max<float>(fMin, r));
        g = std__min<float>(fMax, std__max<float>(fMin, g));
        b = std__min<float>(fMax, std__max<float>(fMin, b));
        a = std__min<float>(fMax, std__max<float>(fMin, a));
        return *this;
    }

    HDRColorA(const LDRColorA& c) noexcept;
    HDRColorA& operator=(const LDRColorA& c) noexcept;
};

//-------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------

constexpr uint16_t F16S_MASK  = 0x8000; // f16 sign mask
constexpr uint16_t F16EM_MASK = 0x7fff; // f16 exp & mantissa mask
constexpr uint16_t F16MAX     = 0x7bff; // MAXFLT bit pattern for XMHALF

constexpr size_t   BC6H_NUM_PIXELS_PER_BLOCK = 16;
constexpr size_t   BC6H_MAX_REGIONS          = 2;
constexpr size_t   BC6H_MAX_INDICES          = 16;
constexpr size_t   BC6H_NUM_CHANNELS         = 3;
constexpr size_t   BC6H_MAX_SHAPES           = 32;
constexpr int32_t  BC6H_WEIGHT_MAX           = 64;
constexpr uint32_t BC6H_WEIGHT_SHIFT         = 6;
constexpr int32_t  BC6H_WEIGHT_ROUND         = 32;

constexpr float fEpsilon = (0.25f / 64.0f) * (0.25f / 64.0f);
constexpr float pC3[]    = {2.0f / 2.0f, 1.0f / 2.0f, 0.0f / 2.0f};
constexpr float pD3[]    = {0.0f / 2.0f, 1.0f / 2.0f, 2.0f / 2.0f};
constexpr float pC4[]    = {3.0f / 3.0f, 2.0f / 3.0f, 1.0f / 3.0f, 0.0f / 3.0f};
constexpr float pD4[]    = {0.0f / 3.0f, 1.0f / 3.0f, 2.0f / 3.0f, 3.0f / 3.0f};

// Partition, Shape, Pixel (index into 4x4 block)
const uint8_t g_aPartitionTable[2][32][16] =
    {
        {
            // 1 Region case has no subsets (all 0)
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
        },

        {
            // BC6H/BC7 Partition Set for 2 Subsets
            {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1}, // Shape 0
            {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}, // Shape 1
            {0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1}, // Shape 2
            {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1}, // Shape 3
            {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1}, // Shape 4
            {0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1}, // Shape 5
            {0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1}, // Shape 6
            {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1}, // Shape 7
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1}, // Shape 8
            {0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 9
            {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1}, // Shape 10
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1}, // Shape 11
            {0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 12
            {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 13
            {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 14
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, // Shape 15
            {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1}, // Shape 16
            {0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, // Shape 17
            {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0}, // Shape 18
            {0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0}, // Shape 19
            {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, // Shape 20
            {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0}, // Shape 21
            {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0}, // Shape 22
            {0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1}, // Shape 23
            {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0}, // Shape 24
            {0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0}, // Shape 25
            {0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0}, // Shape 26
            {0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0}, // Shape 27
            {0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0}, // Shape 28
            {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0}, // Shape 29
            {0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0}, // Shape 30
            {0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0}  // Shape 31
        }
};

// Partition, Shape, Fixup
const uint8_t g_aFixUp[2][32][3] =
    {
        {
            // No fix-ups for 1st subset for BC6H or BC7
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0},
             {0, 0, 0}
        },

        {
            // BC6H/BC7 Partition Set Fixups for 2 Subsets
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 15, 0},
             {0, 2, 0},
             {0, 8, 0},
             {0, 2, 0},
             {0, 2, 0},
             {0, 8, 0},
             {0, 8, 0},
             {0, 15, 0},
             {0, 2, 0},
             {0, 8, 0},
             {0, 2, 0},
             {0, 2, 0},
             {0, 8, 0},
             {0, 8, 0},
             {0, 2, 0},
             {0, 2, 0}
        }
};

const int g_aWeights3[] = {0, 9, 18, 27, 37, 46, 55, 64};
const int g_aWeights4[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

class LDRColorA
{
public:
    uint8_t r, g, b, a;

    LDRColorA() = default;
    LDRColorA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) noexcept :
        r(_r), g(_g), b(_b), a(_a) {}

    const uint8_t& operator[](size_t uElement) const noexcept
    {
        switch (uElement)
        {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            case 3: return a;
            default: BC6H_ASSERT(false); return r;
        }
    }

    uint8_t& operator[](size_t uElement) noexcept
    {
        switch (uElement)
        {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            case 3: return a;
            default: BC6H_ASSERT(false); return r;
        }
    }

    LDRColorA operator=(const HDRColorA& c) noexcept
    {
        LDRColorA ret;
        HDRColorA tmp(c);
        tmp   = tmp.Clamp(0.0f, 1.0f) * 255.0f;
        ret.r = uint8_t(tmp.r + 0.001f);
        ret.g = uint8_t(tmp.g + 0.001f);
        ret.b = uint8_t(tmp.b + 0.001f);
        ret.a = uint8_t(tmp.a + 0.001f);
        return ret;
    }
};

static_assert(sizeof(LDRColorA) == 4, "Unexpected packing");

struct LDREndPntPair
{
    LDRColorA A;
    LDRColorA B;
};

BC6H_INLINE HDRColorA::HDRColorA(const LDRColorA& c) noexcept
{
    r = float(c.r) * (1.0f / 255.0f);
    g = float(c.g) * (1.0f / 255.0f);
    b = float(c.b) * (1.0f / 255.0f);
    a = float(c.a) * (1.0f / 255.0f);
}

BC6H_INLINE HDRColorA& HDRColorA::operator=(const LDRColorA& c) noexcept
{
    r = static_cast<float>(c.r);
    g = static_cast<float>(c.g);
    b = static_cast<float>(c.b);
    a = static_cast<float>(c.a);
    return *this;
}

class INTColor
{
public:
    int r, g, b;
    int pad;

public:
    INTColor() = default;
    INTColor(int nr, int ng, int nb) noexcept :
        r(nr), g(ng), b(nb), pad(0) {}
    INTColor(const INTColor& c) noexcept :
        r(c.r), g(c.g), b(c.b), pad(0) {}

    INTColor& operator+=(const INTColor& c) noexcept
    {
        r += c.r;
        g += c.g;
        b += c.b;
        return *this;
    }

    INTColor& operator-=(const INTColor& c) noexcept
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        return *this;
    }

    INTColor& operator&=(const INTColor& c) noexcept
    {
        r &= c.r;
        g &= c.g;
        b &= c.b;
        return *this;
    }

    int& operator[](uint8_t i) noexcept
    {
        BC6H_ASSERT(i < sizeof(INTColor) / sizeof(int));
        return reinterpret_cast<int*>(this)[i];
    }

    void Set(const HDRColorA& c, bool bSigned) noexcept
    {
        XMHALF4 aF16;

        const XMVECTOR v = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&c));
        XMStoreHalf4(&aF16, v);

        r = F16ToINT(aF16.x, bSigned);
        g = F16ToINT(aF16.y, bSigned);
        b = F16ToINT(aF16.z, bSigned);
    }

    INTColor& Clamp(int iMin, int iMax) noexcept
    {
        r = std__min<int>(iMax, std__max<int>(iMin, r));
        g = std__min<int>(iMax, std__max<int>(iMin, g));
        b = std__min<int>(iMax, std__max<int>(iMin, b));
        return *this;
    }

    INTColor& SignExtend(const LDRColorA& Prec) noexcept
    {
#define BC6H_SIGN_EXTEND(x, nb) ((((x) & (1 << ((nb)-1))) ? ((~0) ^ ((1 << (nb)) - 1)) : 0) | (x))
        r = BC6H_SIGN_EXTEND(r, int(Prec.r));
        g = BC6H_SIGN_EXTEND(g, int(Prec.g));
        b = BC6H_SIGN_EXTEND(b, int(Prec.b));
#undef BC6H_SIGN_EXTEND
        return *this;
    }

    void ToF16(HALF aF16[3], bool bSigned) const noexcept
    {
        aF16[0] = INT2F16(r, bSigned);
        aF16[1] = INT2F16(g, bSigned);
        aF16[2] = INT2F16(b, bSigned);
    }

private:
    static int F16ToINT(const HALF& f, bool bSigned) noexcept
    {
        uint16_t input = *reinterpret_cast<const uint16_t*>(&f);
        int      out, s;
        if (bSigned)
        {
            s = input & F16S_MASK;
            input &= F16EM_MASK;
            if (input > F16MAX) out = F16MAX;
            else
                out = input;
            out = s ? -out : out;
        }
        else
        {
            if (input & F16S_MASK) out = 0;
            else
                out = input;
        }
        return out;
    }

    static HALF INT2F16(int input, bool bSigned) noexcept
    {
        HALF     h;
        uint16_t out;
        if (bSigned)
        {
            int s = 0;
            if (input < 0)
            {
                s     = F16S_MASK;
                input = -input;
            }
            out = uint16_t(s | input);
        }
        else
        {
            BC6H_ASSERT(input >= 0 && input <= F16MAX);
            out = static_cast<uint16_t>(input);
        }

        *reinterpret_cast<uint16_t*>(&h) = out;
        return h;
    }
};

static_assert(sizeof(INTColor) == 16, "Unexpected packing");

struct INTEndPntPair
{
    INTColor A;
    INTColor B;
};

template <size_t SizeInBytes>
class CBits
{
public:
    uint8_t GetBit(size_t& uStartBit) const noexcept
    {
        BC6H_ASSERT(uStartBit < 128);
        const size_t uIndex = uStartBit >> 3;
        auto const   ret    = static_cast<uint8_t>((m_uBits[uIndex] >> (uStartBit - (uIndex << 3))) & 0x01);
        uStartBit++;
        return ret;
    }

    uint8_t GetBits(size_t& uStartBit, size_t uNumBits) const noexcept
    {
        if (uNumBits == 0) return 0;
        BC6H_ASSERT(uStartBit + uNumBits <= 128 && uNumBits <= 8);
        uint8_t      ret;
        const size_t uIndex = uStartBit >> 3;
        const size_t uBase  = uStartBit - (uIndex << 3);
        if (uBase + uNumBits > 8)
        {
            const size_t uFirstIndexBits = 8 - uBase;
            const size_t uNextIndexBits  = uNumBits - uFirstIndexBits;
            ret                          = static_cast<uint8_t>((unsigned(m_uBits[uIndex]) >> uBase) | ((unsigned(m_uBits[uIndex + 1]) & ((1u << uNextIndexBits) - 1)) << uFirstIndexBits));
        }
        else
        {
            ret = static_cast<uint8_t>((m_uBits[uIndex] >> uBase) & ((1 << uNumBits) - 1));
        }
        BC6H_ASSERT(ret < (1 << uNumBits));
        uStartBit += uNumBits;
        return ret;
    }

    void SetBit(size_t& uStartBit, uint8_t uValue) noexcept
    {
        BC6H_ASSERT(uStartBit < 128 && uValue < 2);
        size_t       uIndex = uStartBit >> 3;
        const size_t uBase  = uStartBit - (uIndex << 3);
        m_uBits[uIndex] &= ~(1 << uBase);
        m_uBits[uIndex] |= uValue << uBase;
        uStartBit++;
    }

    void SetBits(size_t& uStartBit, size_t uNumBits, uint8_t uValue) noexcept
    {
        if (uNumBits == 0)
            return;
        BC6H_ASSERT(uStartBit + uNumBits <= 128 && uNumBits <= 8);
        BC6H_ASSERT(uValue < (1 << uNumBits));
        size_t       uIndex = uStartBit >> 3;
        const size_t uBase  = uStartBit - (uIndex << 3);
        if (uBase + uNumBits > 8)
        {
            const size_t uFirstIndexBits = 8 - uBase;
            const size_t uNextIndexBits  = uNumBits - uFirstIndexBits;
            m_uBits[uIndex] &= ~(((1 << uFirstIndexBits) - 1) << uBase);
            m_uBits[uIndex] |= uValue << uBase;
            m_uBits[uIndex + 1] &= ~((1 << uNextIndexBits) - 1);
            m_uBits[uIndex + 1] |= uValue >> uFirstIndexBits;
        }
        else
        {
            m_uBits[uIndex] &= ~(((1 << uNumBits) - 1) << uBase);
            m_uBits[uIndex] |= uValue << uBase;
        }
        uStartBit += uNumBits;
    }

private:
    uint8_t m_uBits[SizeInBytes];
};

// BC6H compression (16 bits per texel)
class D3DX_BC6H : private CBits<16>
{
public:
    void Decode(bool bSigned, HDRColorA* pOut) const noexcept;
    void Encode(bool bSigned, const HDRColorA* const pIn) noexcept;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4480)
#endif
    enum EField : uint8_t
    {
        NA, // N/A
        M,  // Mode
        D,  // Shape
        RW,
        RX,
        RY,
        RZ,
        GW,
        GX,
        GY,
        GZ,
        BW,
        BX,
        BY,
        BZ,
    };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    struct ModeDescriptor
    {
        EField  m_eField;
        uint8_t m_uBit;
    };

    struct ModeInfo
    {
        uint8_t   uMode;
        uint8_t   uPartitions;
        bool      bTransformed;
        uint8_t   uIndexPrec;
        LDRColorA RGBAPrec[BC6H_MAX_REGIONS][2];
    };

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4512)
#endif
    struct EncodeParams
    {
        float                  fBestErr;
        const bool             bSigned;
        uint8_t                uMode;
        uint8_t                uShape;
        const HDRColorA* const aHDRPixels;
        INTEndPntPair          aUnqEndPts[BC6H_MAX_SHAPES][BC6H_MAX_REGIONS];
        INTColor               aIPixels[BC6H_NUM_PIXELS_PER_BLOCK];

        EncodeParams(const HDRColorA* const aOriginal, bool bSignedFormat) noexcept :
            fBestErr(FLT_MAX), bSigned(bSignedFormat), uMode(0), uShape(0), aHDRPixels(aOriginal), aUnqEndPts{}, aIPixels{}
        {
            for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
            {
                aIPixels[i].Set(aOriginal[i], bSigned);
            }
        }
    };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    static int Quantize(int iValue, int prec, bool bSigned) noexcept;
    static int Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned) noexcept;
    static int FinishUnquantize(int comp, bool bSigned) noexcept;

    static bool EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[]) noexcept;

    void        GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts, INTColor aPalette[]) const noexcept;
    float       MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair& endPts) const noexcept;
    float       PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch, const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const noexcept;
    void        OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr, const INTEndPntPair& aOrgEndPts, INTEndPntPair& aOptEndPts) const noexcept;
    void        OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[], const INTEndPntPair aOrgEndPts[], INTEndPntPair aOptEndPts[]) const noexcept;
    static void SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[], size_t aIndices[]) noexcept;
    void        AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[], size_t aIndices[], float aTotErr[]) const noexcept;
    void        QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* qQntEndPts) const noexcept;
    void        EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[], const size_t aIndices[]) noexcept;
    void        Refine(EncodeParams* pEP) noexcept;

    static void GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[]) noexcept;
    float       MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const noexcept;
    float       RoughMSE(EncodeParams* pEP) const noexcept;

private:
    static const ModeDescriptor ms_aDesc[][82];
    static const ModeInfo       ms_aInfo[];
    static const int            ms_aModeToInfo[];
};

// BC6H Compression
const D3DX_BC6H::ModeDescriptor D3DX_BC6H::ms_aDesc[14][82] =
    {
        {
            // Mode 1 (0x00) - 10 5 5 5
            {M, 0},
            {M, 1},
            {GY, 4},
            {BY, 4},
            {BZ, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {GZ, 4},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {BZ, 0},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BZ, 1},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {BZ, 2},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {BZ, 3},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 2 (0x01) - 7 6 6 6
            {M, 0},
            {M, 1},
            {GY, 5},
            {GZ, 4},
            {GZ, 5},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {BZ, 0},
            {BZ, 1},
            {BY, 4},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {BY, 5},
            {BZ, 2},
            {GY, 4},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BZ, 3},
            {BZ, 5},
            {BZ, 4},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {RX, 5},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {GX, 5},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BX, 5},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {RY, 5},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {RZ, 5},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 3 (0x02) - 11 5 4 4
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {RW, 10},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GW, 10},
            {BZ, 0},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BW, 10},
            {BZ, 1},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {BZ, 2},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {BZ, 3},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 4 (0x06) - 11 4 5 4
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RW, 10},
            {GZ, 4},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {GW, 10},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BW, 10},
            {BZ, 1},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {BZ, 0},
            {BZ, 2},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {GY, 4},
            {BZ, 3},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 5 (0x0a) - 11 4 4 5
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RW, 10},
            {BY, 4},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GW, 10},
            {BZ, 0},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BW, 10},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {BZ, 1},
            {BZ, 2},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {BZ, 4},
            {BZ, 3},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 6 (0x0e) - 9 5 5 5
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {BY, 4},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GY, 4},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BZ, 4},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {GZ, 4},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {BZ, 0},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BZ, 1},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {BZ, 2},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {BZ, 3},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 7 (0x12) - 8 6 5 5
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {GZ, 4},
            {BY, 4},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {BZ, 2},
            {GY, 4},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BZ, 3},
            {BZ, 4},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {RX, 5},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {BZ, 0},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BZ, 1},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {RY, 5},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {RZ, 5},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 8 (0x16) - 8 5 6 5
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {BZ, 0},
            {BY, 4},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GY, 5},
            {GY, 4},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {GZ, 5},
            {BZ, 4},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {GZ, 4},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {GX, 5},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BZ, 1},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {BZ, 2},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {BZ, 3},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 9 (0x1a) - 8 5 5 6
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {BZ, 1},
            {BY, 4},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {BY, 5},
            {GY, 4},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BZ, 5},
            {BZ, 4},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {GZ, 4},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {BZ, 0},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BX, 5},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {BZ, 2},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {BZ, 3},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 10 (0x1e) - 6 6 6 6
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {GZ, 4},
            {BZ, 0},
            {BZ, 1},
            {BY, 4},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GY, 5},
            {BY, 5},
            {BZ, 2},
            {GY, 4},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {GZ, 5},
            {BZ, 3},
            {BZ, 5},
            {BZ, 4},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {RX, 5},
            {GY, 0},
            {GY, 1},
            {GY, 2},
            {GY, 3},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {GX, 5},
            {GZ, 0},
            {GZ, 1},
            {GZ, 2},
            {GZ, 3},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BX, 5},
            {BY, 0},
            {BY, 1},
            {BY, 2},
            {BY, 3},
            {RY, 0},
            {RY, 1},
            {RY, 2},
            {RY, 3},
            {RY, 4},
            {RY, 5},
            {RZ, 0},
            {RZ, 1},
            {RZ, 2},
            {RZ, 3},
            {RZ, 4},
            {RZ, 5},
            {D, 0},
            {D, 1},
            {D, 2},
            {D, 3},
            {D, 4},
        },

        {
            // Mode 11 (0x03) - 10 10
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {RX, 5},
            {RX, 6},
            {RX, 7},
            {RX, 8},
            {RX, 9},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {GX, 5},
            {GX, 6},
            {GX, 7},
            {GX, 8},
            {GX, 9},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BX, 5},
            {BX, 6},
            {BX, 7},
            {BX, 8},
            {BX, 9},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
        },

        {
            // Mode 12 (0x07) - 11 9
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {RX, 5},
            {RX, 6},
            {RX, 7},
            {RX, 8},
            {RW, 10},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {GX, 5},
            {GX, 6},
            {GX, 7},
            {GX, 8},
            {GW, 10},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BX, 5},
            {BX, 6},
            {BX, 7},
            {BX, 8},
            {BW, 10},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
        },

        {
            // Mode 13 (0x0b) - 12 8
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RX, 4},
            {RX, 5},
            {RX, 6},
            {RX, 7},
            {RW, 11},
            {RW, 10},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GX, 4},
            {GX, 5},
            {GX, 6},
            {GX, 7},
            {GW, 11},
            {GW, 10},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BX, 4},
            {BX, 5},
            {BX, 6},
            {BX, 7},
            {BW, 11},
            {BW, 10},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
        },

        {
            // Mode 14 (0x0f) - 16 4
            {M, 0},
            {M, 1},
            {M, 2},
            {M, 3},
            {M, 4},
            {RW, 0},
            {RW, 1},
            {RW, 2},
            {RW, 3},
            {RW, 4},
            {RW, 5},
            {RW, 6},
            {RW, 7},
            {RW, 8},
            {RW, 9},
            {GW, 0},
            {GW, 1},
            {GW, 2},
            {GW, 3},
            {GW, 4},
            {GW, 5},
            {GW, 6},
            {GW, 7},
            {GW, 8},
            {GW, 9},
            {BW, 0},
            {BW, 1},
            {BW, 2},
            {BW, 3},
            {BW, 4},
            {BW, 5},
            {BW, 6},
            {BW, 7},
            {BW, 8},
            {BW, 9},
            {RX, 0},
            {RX, 1},
            {RX, 2},
            {RX, 3},
            {RW, 15},
            {RW, 14},
            {RW, 13},
            {RW, 12},
            {RW, 11},
            {RW, 10},
            {GX, 0},
            {GX, 1},
            {GX, 2},
            {GX, 3},
            {GW, 15},
            {GW, 14},
            {GW, 13},
            {GW, 12},
            {GW, 11},
            {GW, 10},
            {BX, 0},
            {BX, 1},
            {BX, 2},
            {BX, 3},
            {BW, 15},
            {BW, 14},
            {BW, 13},
            {BW, 12},
            {BW, 11},
            {BW, 10},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
            {NA, 0},
        },
};

// Mode, Partitions, Transformed, IndexPrec, RGBAPrec
const D3DX_BC6H::ModeInfo D3DX_BC6H::ms_aInfo[] =
    {
        {0x00, 1, true, 3, {{LDRColorA(10, 10, 10, 0), LDRColorA(5, 5, 5, 0)}, {LDRColorA(5, 5, 5, 0), LDRColorA(5, 5, 5, 0)}}},     // Mode 1
        {0x01, 1, true, 3, {{LDRColorA(7, 7, 7, 0), LDRColorA(6, 6, 6, 0)}, {LDRColorA(6, 6, 6, 0), LDRColorA(6, 6, 6, 0)}}},        // Mode 2
        {0x02, 1, true, 3, {{LDRColorA(11, 11, 11, 0), LDRColorA(5, 4, 4, 0)}, {LDRColorA(5, 4, 4, 0), LDRColorA(5, 4, 4, 0)}}},     // Mode 3
        {0x06, 1, true, 3, {{LDRColorA(11, 11, 11, 0), LDRColorA(4, 5, 4, 0)}, {LDRColorA(4, 5, 4, 0), LDRColorA(4, 5, 4, 0)}}},     // Mode 4
        {0x0a, 1, true, 3, {{LDRColorA(11, 11, 11, 0), LDRColorA(4, 4, 5, 0)}, {LDRColorA(4, 4, 5, 0), LDRColorA(4, 4, 5, 0)}}},     // Mode 5
        {0x0e, 1, true, 3, {{LDRColorA(9, 9, 9, 0), LDRColorA(5, 5, 5, 0)}, {LDRColorA(5, 5, 5, 0), LDRColorA(5, 5, 5, 0)}}},        // Mode 6
        {0x12, 1, true, 3, {{LDRColorA(8, 8, 8, 0), LDRColorA(6, 5, 5, 0)}, {LDRColorA(6, 5, 5, 0), LDRColorA(6, 5, 5, 0)}}},        // Mode 7
        {0x16, 1, true, 3, {{LDRColorA(8, 8, 8, 0), LDRColorA(5, 6, 5, 0)}, {LDRColorA(5, 6, 5, 0), LDRColorA(5, 6, 5, 0)}}},        // Mode 8
        {0x1a, 1, true, 3, {{LDRColorA(8, 8, 8, 0), LDRColorA(5, 5, 6, 0)}, {LDRColorA(5, 5, 6, 0), LDRColorA(5, 5, 6, 0)}}},        // Mode 9
        {0x1e, 1, false, 3, {{LDRColorA(6, 6, 6, 0), LDRColorA(6, 6, 6, 0)}, {LDRColorA(6, 6, 6, 0), LDRColorA(6, 6, 6, 0)}}},       // Mode 10
        {0x03, 0, false, 4, {{LDRColorA(10, 10, 10, 0), LDRColorA(10, 10, 10, 0)}, {LDRColorA(0, 0, 0, 0), LDRColorA(0, 0, 0, 0)}}}, // Mode 11
        {0x07, 0, true, 4, {{LDRColorA(11, 11, 11, 0), LDRColorA(9, 9, 9, 0)}, {LDRColorA(0, 0, 0, 0), LDRColorA(0, 0, 0, 0)}}},     // Mode 12
        {0x0b, 0, true, 4, {{LDRColorA(12, 12, 12, 0), LDRColorA(8, 8, 8, 0)}, {LDRColorA(0, 0, 0, 0), LDRColorA(0, 0, 0, 0)}}},     // Mode 13
        {0x0f, 0, true, 4, {{LDRColorA(16, 16, 16, 0), LDRColorA(4, 4, 4, 0)}, {LDRColorA(0, 0, 0, 0), LDRColorA(0, 0, 0, 0)}}},     // Mode 14
};

const int D3DX_BC6H::ms_aModeToInfo[] =
    {
        0,  // Mode 1   - 0x00
        1,  // Mode 2   - 0x01
        2,  // Mode 3   - 0x02
        10, // Mode 11  - 0x03
        -1, // Invalid  - 0x04
        -1, // Invalid  - 0x05
        3,  // Mode 4   - 0x06
        11, // Mode 12  - 0x07
        -1, // Invalid  - 0x08
        -1, // Invalid  - 0x09
        4,  // Mode 5   - 0x0a
        12, // Mode 13  - 0x0b
        -1, // Invalid  - 0x0c
        -1, // Invalid  - 0x0d
        5,  // Mode 6   - 0x0e
        13, // Mode 14  - 0x0f
        -1, // Invalid  - 0x10
        -1, // Invalid  - 0x11
        6,  // Mode 7   - 0x12
        -1, // Reserved - 0x13
        -1, // Invalid  - 0x14
        -1, // Invalid  - 0x15
        7,  // Mode 8   - 0x16
        -1, // Reserved - 0x17
        -1, // Invalid  - 0x18
        -1, // Invalid  - 0x19
        8,  // Mode 9   - 0x1a
        -1, // Reserved - 0x1b
        -1, // Invalid  - 0x1c
        -1, // Invalid  - 0x1d
        9,  // Mode 10  - 0x1e
        -1, // Resreved - 0x1f
};

//-------------------------------------------------------------------------------------
// Helper functions
//-------------------------------------------------------------------------------------
BC6H_INLINE bool IsFixUpOffset(size_t uPartitions, size_t uShape, size_t uOffset) noexcept
{
    BC6H_ASSERT(uPartitions < 3 && uShape < BC6H_MAX_SHAPES && uOffset < 16);
    for (size_t p = 0; p <= uPartitions; p++)
    {
        if (uOffset == g_aFixUp[uPartitions][uShape][p])
        {
            return true;
        }
    }
    return false;
}

BC6H_INLINE void TransformForward(INTEndPntPair aEndPts[]) noexcept
{
    aEndPts[0].B -= aEndPts[0].A;
    aEndPts[1].A -= aEndPts[0].A;
    aEndPts[1].B -= aEndPts[0].A;
}

BC6H_INLINE void TransformInverse(INTEndPntPair aEndPts[], const LDRColorA& Prec, bool bSigned) noexcept
{
    const INTColor WrapMask((1 << Prec.r) - 1, (1 << Prec.g) - 1, (1 << Prec.b) - 1);
    aEndPts[0].B += aEndPts[0].A;
    aEndPts[0].B &= WrapMask;
    aEndPts[1].A += aEndPts[0].A;
    aEndPts[1].A &= WrapMask;
    aEndPts[1].B += aEndPts[0].A;
    aEndPts[1].B &= WrapMask;
    if (bSigned)
    {
        aEndPts[0].B.SignExtend(Prec);
        aEndPts[1].A.SignExtend(Prec);
        aEndPts[1].B.SignExtend(Prec);
    }
}

BC6H_INLINE float Norm(const INTColor& a, const INTColor& b) noexcept
{
    const float dr = float(a.r) - float(b.r);
    const float dg = float(a.g) - float(b.g);
    const float db = float(a.b) - float(b.b);
    return dr * dr + dg * dg + db * db;
}

// return # of bits needed to store n. handle signed or unsigned cases properly
BC6H_INLINE int NBits(int n, bool bIsSigned) noexcept
{
    int nb;
    if (n == 0)
    {
        return 0; // no bits needed for 0, signed or not
    }
    else if (n > 0)
    {
        for (nb = 0; n; ++nb, n >>= 1)
            ;
        return nb + (bIsSigned ? 1 : 0);
    }
    else
    {
        BC6H_ASSERT(bIsSigned);
        for (nb = 0; n < -1; ++nb, n >>= 1)
            ;
        return nb + 1;
    }
}

float OptimizeRGB(
    const HDRColorA* const pPoints,
    HDRColorA*             pX,
    HDRColorA*             pY,
    uint32_t               cSteps,
    size_t                 cPixels,
    const size_t*          pIndex) noexcept
{
    constexpr float fError = FLT_MAX;
    const float*    pC     = (3 == cSteps) ? pC3 : pC4;
    const float*    pD     = (3 == cSteps) ? pD3 : pD4;

    // Find Min and Max points, as starting point
    HDRColorA X(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
    HDRColorA Y(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

    for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
    {
        if (pPoints[pIndex[iPoint]].r < X.r) X.r = pPoints[pIndex[iPoint]].r;
        if (pPoints[pIndex[iPoint]].g < X.g) X.g = pPoints[pIndex[iPoint]].g;
        if (pPoints[pIndex[iPoint]].b < X.b) X.b = pPoints[pIndex[iPoint]].b;
        if (pPoints[pIndex[iPoint]].r > Y.r) Y.r = pPoints[pIndex[iPoint]].r;
        if (pPoints[pIndex[iPoint]].g > Y.g) Y.g = pPoints[pIndex[iPoint]].g;
        if (pPoints[pIndex[iPoint]].b > Y.b) Y.b = pPoints[pIndex[iPoint]].b;
    }

    // Diagonal axis
    HDRColorA AB;
    AB.r = Y.r - X.r;
    AB.g = Y.g - X.g;
    AB.b = Y.b - X.b;

    const float fAB = AB.r * AB.r + AB.g * AB.g + AB.b * AB.b;

    // Single color block.. no need to root-find
    if (fAB < FLT_MIN)
    {
        pX->r = X.r;
        pX->g = X.g;
        pX->b = X.b;
        pY->r = Y.r;
        pY->g = Y.g;
        pY->b = Y.b;
        return 0.0f;
    }

    // Try all four axis directions, to determine which diagonal best fits data
    const float fABInv = 1.0f / fAB;

    HDRColorA Dir;
    Dir.r = AB.r * fABInv;
    Dir.g = AB.g * fABInv;
    Dir.b = AB.b * fABInv;

    HDRColorA Mid;
    Mid.r = (X.r + Y.r) * 0.5f;
    Mid.g = (X.g + Y.g) * 0.5f;
    Mid.b = (X.b + Y.b) * 0.5f;

    float fDir[4];
    fDir[0] = fDir[1] = fDir[2] = fDir[3] = 0.0f;

    for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
    {
        HDRColorA Pt;
        Pt.r = (pPoints[pIndex[iPoint]].r - Mid.r) * Dir.r;
        Pt.g = (pPoints[pIndex[iPoint]].g - Mid.g) * Dir.g;
        Pt.b = (pPoints[pIndex[iPoint]].b - Mid.b) * Dir.b;

        float f;
        f = Pt.r + Pt.g + Pt.b;
        fDir[0] += f * f;
        f = Pt.r + Pt.g - Pt.b;
        fDir[1] += f * f;
        f = Pt.r - Pt.g + Pt.b;
        fDir[2] += f * f;
        f = Pt.r - Pt.g - Pt.b;
        fDir[3] += f * f;
    }

    float  fDirMax = fDir[0];
    size_t iDirMax = 0;

    for (size_t iDir = 1; iDir < 4; iDir++)
    {
        if (fDir[iDir] > fDirMax)
        {
            fDirMax = fDir[iDir];
            iDirMax = iDir;
        }
    }

    if (iDirMax & 2) std__swap(X.g, Y.g);
    if (iDirMax & 1) std__swap(X.b, Y.b);

    // Two color block.. no need to root-find
    if (fAB < 1.0f / 4096.0f)
    {
        pX->r = X.r;
        pX->g = X.g;
        pX->b = X.b;
        pY->r = Y.r;
        pY->g = Y.g;
        pY->b = Y.b;
        return 0.0f;
    }

    // Use Newton's Method to find local minima of sum-of-squares error.
    auto const fSteps = static_cast<float>(cSteps - 1);

    for (size_t iIteration = 0; iIteration < 8; iIteration++)
    {
        // Calculate new steps
        HDRColorA pSteps[4] = {};

        for (size_t iStep = 0; iStep < cSteps; iStep++)
        {
            pSteps[iStep].r = X.r * pC[iStep] + Y.r * pD[iStep];
            pSteps[iStep].g = X.g * pC[iStep] + Y.g * pD[iStep];
            pSteps[iStep].b = X.b * pC[iStep] + Y.b * pD[iStep];
        }

        // Calculate color direction
        Dir.r = Y.r - X.r;
        Dir.g = Y.g - X.g;
        Dir.b = Y.b - X.b;

        const float fLen = (Dir.r * Dir.r + Dir.g * Dir.g + Dir.b * Dir.b);

        if (fLen < (1.0f / 4096.0f))
            break;

        const float fScale = fSteps / fLen;

        Dir.r *= fScale;
        Dir.g *= fScale;
        Dir.b *= fScale;

        // Evaluate function, and derivatives
        float     d2X = 0.0f, d2Y = 0.0f;
        HDRColorA dX(0.0f, 0.0f, 0.0f, 0.0f), dY(0.0f, 0.0f, 0.0f, 0.0f);

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            const float fDot = (pPoints[pIndex[iPoint]].r - X.r) * Dir.r +
                (pPoints[pIndex[iPoint]].g - X.g) * Dir.g +
                (pPoints[pIndex[iPoint]].b - X.b) * Dir.b;

            uint32_t iStep;
            if (fDot <= 0.0f)
                iStep = 0;
            else if (fDot >= fSteps)
                iStep = cSteps - 1;
            else
                iStep = uint32_t(fDot + 0.5f);

            HDRColorA Diff;
            Diff.r = pSteps[iStep].r - pPoints[pIndex[iPoint]].r;
            Diff.g = pSteps[iStep].g - pPoints[pIndex[iPoint]].g;
            Diff.b = pSteps[iStep].b - pPoints[pIndex[iPoint]].b;

            const float fC = pC[iStep] * (1.0f / 8.0f);
            const float fD = pD[iStep] * (1.0f / 8.0f);

            d2X += fC * pC[iStep];
            dX.r += fC * Diff.r;
            dX.g += fC * Diff.g;
            dX.b += fC * Diff.b;

            d2Y += fD * pD[iStep];
            dY.r += fD * Diff.r;
            dY.g += fD * Diff.g;
            dY.b += fD * Diff.b;
        }

        // Move endpoints
        if (d2X > 0.0f)
        {
            const float f = -1.0f / d2X;

            X.r += dX.r * f;
            X.g += dX.g * f;
            X.b += dX.b * f;
        }

        if (d2Y > 0.0f)
        {
            const float f = -1.0f / d2Y;

            Y.r += dY.r * f;
            Y.g += dY.g * f;
            Y.b += dY.b * f;
        }

        if ((dX.r * dX.r < fEpsilon) && (dX.g * dX.g < fEpsilon) && (dX.b * dX.b < fEpsilon) &&
            (dY.r * dY.r < fEpsilon) && (dY.g * dY.g < fEpsilon) && (dY.b * dY.b < fEpsilon))
        {
            break;
        }
    }

    pX->r = X.r;
    pX->g = X.g;
    pX->b = X.b;
    pY->r = Y.r;
    pY->g = Y.g;
    pY->b = Y.b;
    return fError;
}

void FillWithErrorColors(HDRColorA* pOut) noexcept
{
    for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
    {
#ifdef _DEBUG
        // Use Magenta in debug as a highly-visible error color
        pOut[i] = HDRColorA(1.0f, 0.0f, 1.0f, 1.0f);
#else
        // In production use, default to black
        pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);
#endif
    }
}

void D3DX_BC6H::Decode(bool bSigned, HDRColorA* pOut) const noexcept
{
    BC6H_ASSERT(pOut);

    size_t  uStartBit = 0;
    uint8_t uMode     = GetBits(uStartBit, 2u);
    if (uMode != 0x00 && uMode != 0x01)
    {
        uMode = static_cast<uint8_t>((unsigned(GetBits(uStartBit, 3)) << 2) | uMode);
    }

    BC6H_ASSERT(uMode < 32);

    if (ms_aModeToInfo[uMode] >= 0)
    {
        BC6H_ASSERT(static_cast<unsigned int>(ms_aModeToInfo[uMode]) < std__size(ms_aInfo));
        const ModeDescriptor* desc = ms_aDesc[ms_aModeToInfo[uMode]];

        BC6H_ASSERT(static_cast<unsigned int>(ms_aModeToInfo[uMode]) < std__size(ms_aDesc));
        const ModeInfo& info = ms_aInfo[ms_aModeToInfo[uMode]];

        INTEndPntPair aEndPts[BC6H_MAX_REGIONS] = {};
        uint32_t      uShape                    = 0;

        // Read header
        const size_t uHeaderBits = info.uPartitions > 0 ? 82u : 65u;
        while (uStartBit < uHeaderBits)
        {
            const size_t uCurBit = uStartBit;
            if (GetBit(uStartBit))
            {
                switch (desc[uCurBit].m_eField)
                {
                    case D: uShape |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case RW: aEndPts[0].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case RX: aEndPts[0].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case RY: aEndPts[1].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case RZ: aEndPts[1].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case GW: aEndPts[0].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case GX: aEndPts[0].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case GY: aEndPts[1].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case GZ: aEndPts[1].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case BW: aEndPts[0].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case BX: aEndPts[0].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case BY: aEndPts[1].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    case BZ: aEndPts[1].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                    default: {
#ifdef BC6H_LOG
                        BC6H_LOG("BC6H: Invalid header bits encountered during decoding\n");
#endif
                        FillWithErrorColors(pOut);
                        return;
                    }
                }
            }
        }

        BC6H_ASSERT(uShape < 64);

        // Sign extend necessary end points
        if (bSigned)
        {
            aEndPts[0].A.SignExtend(info.RGBAPrec[0][0]);
        }
        if (bSigned || info.bTransformed)
        {
            BC6H_ASSERT(info.uPartitions < BC6H_MAX_REGIONS);
            for (size_t p = 0; p <= info.uPartitions; ++p)
            {
                if (p != 0)
                {
                    aEndPts[p].A.SignExtend(info.RGBAPrec[p][0]);
                }
                aEndPts[p].B.SignExtend(info.RGBAPrec[p][1]);
            }
        }

        // Inverse transform the end points
        if (info.bTransformed)
        {
            TransformInverse(aEndPts, info.RGBAPrec[0][0], bSigned);
        }

        // Read indices
        for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
        {
            const size_t uNumBits = IsFixUpOffset(info.uPartitions, uShape, i) ? info.uIndexPrec - 1u : info.uIndexPrec;
            if (uStartBit + uNumBits > 128)
            {
#ifdef BC6H_LOG
                BC6H_LOG("BC6H: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }
            const uint8_t uIndex = GetBits(uStartBit, uNumBits);

            if (uIndex >= ((info.uPartitions > 0) ? 8 : 16))
            {
#ifdef BC6H_LOG
                BC6H_LOG("BC6H: Invalid index encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            const size_t uRegion = g_aPartitionTable[info.uPartitions][uShape][i];
            BC6H_ASSERT(uRegion < BC6H_MAX_REGIONS);

            // Unquantize endpoints and interpolate
            const int  r1       = Unquantize(aEndPts[uRegion].A.r, info.RGBAPrec[0][0].r, bSigned);
            const int  g1       = Unquantize(aEndPts[uRegion].A.g, info.RGBAPrec[0][0].g, bSigned);
            const int  b1       = Unquantize(aEndPts[uRegion].A.b, info.RGBAPrec[0][0].b, bSigned);
            const int  r2       = Unquantize(aEndPts[uRegion].B.r, info.RGBAPrec[0][0].r, bSigned);
            const int  g2       = Unquantize(aEndPts[uRegion].B.g, info.RGBAPrec[0][0].g, bSigned);
            const int  b2       = Unquantize(aEndPts[uRegion].B.b, info.RGBAPrec[0][0].b, bSigned);
            const int* aWeights = info.uPartitions > 0 ? g_aWeights3 : g_aWeights4;
            INTColor   fc;
            fc.r = FinishUnquantize((r1 * (BC6H_WEIGHT_MAX - aWeights[uIndex]) + r2 * aWeights[uIndex] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT, bSigned);
            fc.g = FinishUnquantize((g1 * (BC6H_WEIGHT_MAX - aWeights[uIndex]) + g2 * aWeights[uIndex] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT, bSigned);
            fc.b = FinishUnquantize((b1 * (BC6H_WEIGHT_MAX - aWeights[uIndex]) + b2 * aWeights[uIndex] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT, bSigned);

            HALF rgb[3];
            fc.ToF16(rgb, bSigned);

            pOut[i].r = XMConvertHalfToFloat(rgb[0]);
            pOut[i].g = XMConvertHalfToFloat(rgb[1]);
            pOut[i].b = XMConvertHalfToFloat(rgb[2]);
            pOut[i].a = 1.0f;
        }
    }
    else
    {
#ifdef BC6H_LOG
        const char* warnstr = "BC6H: Invalid mode encountered during decoding\n";
        switch (uMode)
        {
            case 0x13: warnstr = "BC6H: Reserved mode 10011 encountered during decoding\n"; break;
            case 0x17: warnstr = "BC6H: Reserved mode 10111 encountered during decoding\n"; break;
            case 0x1B: warnstr = "BC6H: Reserved mode 11011 encountered during decoding\n"; break;
            case 0x1F: warnstr = "BC6H: Reserved mode 11111 encountered during decoding\n"; break;
        }
        BC6H_LOG(warnstr);
#endif
        // Per the BC6H format spec, we must return opaque black
        for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
        {
            pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}

void D3DX_BC6H::Encode(bool bSigned, const HDRColorA* const pIn) noexcept
{
    BC6H_ASSERT(pIn);

    EncodeParams EP(pIn, bSigned);

    for (EP.uMode = 0; EP.uMode < std__size(ms_aInfo) && EP.fBestErr > 0; ++EP.uMode)
    {
        const uint8_t uShapes = ms_aInfo[EP.uMode].uPartitions ? 32u : 1u;
        // Number of rough cases to look at. reasonable values of this are 1, uShapes/4, and uShapes
        // uShapes/4 gets nearly all the cases; you can increase that a bit (say by 3 or 4) if you really want to squeeze the last bit out
        const size_t uItems = std__max<size_t>(1u, size_t(uShapes >> 2));
        float        afRoughMSE[BC6H_MAX_SHAPES];
        uint8_t      auShape[BC6H_MAX_SHAPES];

        // pick the best uItems shapes and refine these.
        for (EP.uShape = 0; EP.uShape < uShapes; ++EP.uShape)
        {
            size_t uShape      = EP.uShape;
            afRoughMSE[uShape] = RoughMSE(&EP);
            auShape[uShape]    = static_cast<uint8_t>(uShape);
        }

        // Bubble up the first uItems items
        for (size_t i = 0; i < uItems; i++)
        {
            for (size_t j = i + 1; j < uShapes; j++)
            {
                if (afRoughMSE[i] > afRoughMSE[j])
                {
                    std__swap(afRoughMSE[i], afRoughMSE[j]);
                    std__swap(auShape[i], auShape[j]);
                }
            }
        }

        for (size_t i = 0; i < uItems && EP.fBestErr > 0; i++)
        {
            EP.uShape = auShape[i];
            Refine(&EP);
        }
    }
}

int D3DX_BC6H::Quantize(int iValue, int prec, bool bSigned) noexcept
{
    BC6H_ASSERT(prec > 1); // didn't bother to make it work for 1
    int q, s = 0;
    if (bSigned)
    {
        BC6H_ASSERT(iValue >= -F16MAX && iValue <= F16MAX);
        if (iValue < 0)
        {
            s      = 1;
            iValue = -iValue;
        }
        q = (prec >= 16) ? iValue : (iValue << (prec - 1)) / (F16MAX + 1);
        if (s)
            q = -q;
        BC6H_ASSERT(q > -(1 << (prec - 1)) && q < (1 << (prec - 1)));
    }
    else
    {
        BC6H_ASSERT(iValue >= 0 && iValue <= F16MAX);
        q = (prec >= 15) ? iValue : (iValue << prec) / (F16MAX + 1);
        BC6H_ASSERT(q >= 0 && q < (1 << prec));
    }

    return q;
}

int D3DX_BC6H::Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned) noexcept
{
    int unq = 0, s = 0;
    if (bSigned)
    {
        if (uBitsPerComp >= 16)
        {
            unq = comp;
        }
        else
        {
            if (comp < 0)
            {
                s    = 1;
                comp = -comp;
            }

            if (comp == 0) unq = 0;
            else if (comp >= ((1 << (uBitsPerComp - 1)) - 1))
                unq = 0x7FFF;
            else
                unq = ((comp << 15) + 0x4000) >> (uBitsPerComp - 1);

            if (s) unq = -unq;
        }
    }
    else
    {
        if (uBitsPerComp >= 15) unq = comp;
        else if (comp == 0)
            unq = 0;
        else if (comp == ((1 << uBitsPerComp) - 1))
            unq = 0xFFFF;
        else
            unq = ((comp << 16) + 0x8000) >> uBitsPerComp;
    }

    return unq;
}

int D3DX_BC6H::FinishUnquantize(int comp, bool bSigned) noexcept
{
    if (bSigned)
    {
        return (comp < 0) ? -(((-comp) * 31) >> 5) : (comp * 31) >> 5; // scale the magnitude by 31/32
    }
    else
    {
        return (comp * 31) >> 6; // scale the magnitude by 31/64
    }
}

bool D3DX_BC6H::EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[]) noexcept
{
    BC6H_ASSERT(pEP);
    const bool       bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    const bool       bIsSigned    = pEP->bSigned;
    const LDRColorA& Prec0        = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const LDRColorA& Prec1        = ms_aInfo[pEP->uMode].RGBAPrec[0][1];
    const LDRColorA& Prec2        = ms_aInfo[pEP->uMode].RGBAPrec[1][0];
    const LDRColorA& Prec3        = ms_aInfo[pEP->uMode].RGBAPrec[1][1];

    INTColor aBits[4];
    aBits[0].r = NBits(aEndPts[0].A.r, bIsSigned);
    aBits[0].g = NBits(aEndPts[0].A.g, bIsSigned);
    aBits[0].b = NBits(aEndPts[0].A.b, bIsSigned);
    aBits[1].r = NBits(aEndPts[0].B.r, bTransformed || bIsSigned);
    aBits[1].g = NBits(aEndPts[0].B.g, bTransformed || bIsSigned);
    aBits[1].b = NBits(aEndPts[0].B.b, bTransformed || bIsSigned);
    if (aBits[0].r > Prec0.r || aBits[1].r > Prec1.r ||
        aBits[0].g > Prec0.g || aBits[1].g > Prec1.g ||
        aBits[0].b > Prec0.b || aBits[1].b > Prec1.b)
        return false;

    if (ms_aInfo[pEP->uMode].uPartitions)
    {
        aBits[2].r = NBits(aEndPts[1].A.r, bTransformed || bIsSigned);
        aBits[2].g = NBits(aEndPts[1].A.g, bTransformed || bIsSigned);
        aBits[2].b = NBits(aEndPts[1].A.b, bTransformed || bIsSigned);
        aBits[3].r = NBits(aEndPts[1].B.r, bTransformed || bIsSigned);
        aBits[3].g = NBits(aEndPts[1].B.g, bTransformed || bIsSigned);
        aBits[3].b = NBits(aEndPts[1].B.b, bTransformed || bIsSigned);

        if (aBits[2].r > Prec2.r || aBits[3].r > Prec3.r ||
            aBits[2].g > Prec2.g || aBits[3].g > Prec3.g ||
            aBits[2].b > Prec2.b || aBits[3].b > Prec3.b)
            return false;
    }

    return true;
}

void D3DX_BC6H::GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts, INTColor aPalette[]) const noexcept
{
    BC6H_ASSERT(pEP);
    const size_t uIndexPrec  = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    BC6H_ASSERT(uNumIndices > 0);
    const LDRColorA& Prec = ms_aInfo[pEP->uMode].RGBAPrec[0][0];

    // scale endpoints
    INTEndPntPair unqEndPts;
    unqEndPts.A.r = Unquantize(endPts.A.r, Prec.r, pEP->bSigned);
    unqEndPts.A.g = Unquantize(endPts.A.g, Prec.g, pEP->bSigned);
    unqEndPts.A.b = Unquantize(endPts.A.b, Prec.b, pEP->bSigned);
    unqEndPts.B.r = Unquantize(endPts.B.r, Prec.r, pEP->bSigned);
    unqEndPts.B.g = Unquantize(endPts.B.g, Prec.g, pEP->bSigned);
    unqEndPts.B.b = Unquantize(endPts.B.b, Prec.b, pEP->bSigned);

    // interpolate
    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
        case 3:
            aWeights = g_aWeights3;
            BC6H_ASSERT(uNumIndices <= 8);
            break;
        case 4:
            aWeights = g_aWeights4;
            BC6H_ASSERT(uNumIndices <= 16);
            break;
        default:
            BC6H_ASSERT(false);
            for (size_t i = 0; i < uNumIndices; ++i)
            {
//#pragma prefast(suppress : 22102 22103, "writing blocks in two halves confuses tool")
                aPalette[i] = INTColor(0, 0, 0);
            }
            return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = FinishUnquantize(
            (unqEndPts.A.r * (BC6H_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.r * aWeights[i] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].g = FinishUnquantize(
            (unqEndPts.A.g * (BC6H_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.g * aWeights[i] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].b = FinishUnquantize(
            (unqEndPts.A.b * (BC6H_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.b * aWeights[i] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT,
            pEP->bSigned);
    }
}

// given a collection of colors and quantized endpoints, generate a palette, choose best entries, and return a single toterr
float D3DX_BC6H::MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair& endPts) const noexcept
{
    BC6H_ASSERT(pEP);

    const uint8_t uIndexPrec  = ms_aInfo[pEP->uMode].uIndexPrec;
    auto const    uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    INTColor      aPalette[BC6H_MAX_INDICES];
    GeneratePaletteQuantized(pEP, endPts, aPalette);

    float fTotErr = 0;
    for (size_t i = 0; i < np; ++i)
    {
        const XMVECTOR vcolors = XMLoadSInt4(reinterpret_cast<const XMINT4*>(&aColors[i]));

        // Compute ErrorMetricRGB
        XMVECTOR tpal  = XMLoadSInt4(reinterpret_cast<const XMINT4*>(&aPalette[0]));
        tpal           = XMVectorSubtract(vcolors, tpal);
        float fBestErr = XMVectorDot(tpal, tpal);

        for (int j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            // Compute ErrorMetricRGB
            tpal             = XMLoadSInt4(reinterpret_cast<const XMINT4*>(&aPalette[j]));
            tpal             = XMVectorSubtract(vcolors, tpal);
            const float fErr = XMVectorDot(tpal, tpal);
            if (fErr > fBestErr) break; // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotErr += fBestErr;
    }
    return fTotErr;
}

float D3DX_BC6H::PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch, const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const noexcept
{
    BC6H_ASSERT(pEP);
    uint8_t uPrec;
    switch (ch)
    {
        case 0: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].r; break;
        case 1: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].g; break;
        case 2: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].b; break;
        default:
            BC6H_ASSERT(false);
            newEndPts = oldEndPts;
            return FLT_MAX;
    }
    INTEndPntPair tmpEndPts;
    float         fMinErr  = fOldErr;
    int           beststep = 0;

    // copy real endpoints so we can perturb them
    tmpEndPts = newEndPts = oldEndPts;

    // do a logarithmic search for the best error for this endpoint (which)
    for (int step = 1 << (uPrec - 1); step; step >>= 1)
    {
        bool bImproved = false;
        for (int sign = -1; sign <= 1; sign += 2)
        {
            if (do_b == 0)
            {
                tmpEndPts.A[ch] = newEndPts.A[ch] + sign * step;
                if (tmpEndPts.A[ch] < 0 || tmpEndPts.A[ch] >= (1 << uPrec))
                    continue;
            }
            else
            {
                tmpEndPts.B[ch] = newEndPts.B[ch] + sign * step;
                if (tmpEndPts.B[ch] < 0 || tmpEndPts.B[ch] >= (1 << uPrec))
                    continue;
            }

            const float fErr = MapColorsQuantized(pEP, aColors, np, tmpEndPts);

            if (fErr < fMinErr)
            {
                bImproved = true;
                fMinErr   = fErr;
                beststep  = sign * step;
            }
        }
        // if this was an improvement, move the endpoint and continue search from there
        if (bImproved)
        {
            if (do_b == 0)
                newEndPts.A[ch] += beststep;
            else
                newEndPts.B[ch] += beststep;
        }
    }
    return fMinErr;
}

void D3DX_BC6H::OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr, const INTEndPntPair& aOrgEndPts, INTEndPntPair& aOptEndPts) const noexcept
{
    BC6H_ASSERT(pEP);
    float aOptErr = aOrgErr;
    aOptEndPts.A  = aOrgEndPts.A;
    aOptEndPts.B  = aOrgEndPts.B;

    INTEndPntPair new_a, new_b;
    INTEndPntPair newEndPts;
    int           do_b;

    // now optimize each channel separately
    for (uint8_t ch = 0; ch < BC6H_NUM_CHANNELS; ++ch)
    {
        // figure out which endpoint when perturbed gives the most improvement and start there
        // if we just alternate, we can easily end up in a local minima
        const float fErr0 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_a, aOptErr, 0); // perturb endpt A
        const float fErr1 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_b, aOptErr, 1); // perturb endpt B

        if (fErr0 < fErr1)
        {
            if (fErr0 >= aOptErr) continue;
            aOptEndPts.A[ch] = new_a.A[ch];
            aOptErr          = fErr0;
            do_b             = 1; // do B next
        }
        else
        {
            if (fErr1 >= aOptErr) continue;
            aOptEndPts.B[ch] = new_b.B[ch];
            aOptErr          = fErr1;
            do_b             = 0; // do A next
        }

        // now alternate endpoints and keep trying until there is no improvement
        for (;;)
        {
            const float fErr = PerturbOne(pEP, aColors, np, ch, aOptEndPts, newEndPts, aOptErr, do_b);
            if (fErr >= aOptErr)
                break;
            if (do_b == 0)
                aOptEndPts.A[ch] = newEndPts.A[ch];
            else
                aOptEndPts.B[ch] = newEndPts.B[ch];
            aOptErr = fErr;
            do_b    = 1 - do_b; // now move the other endpoint
        }
    }
}

void D3DX_BC6H::OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[], const INTEndPntPair aOrgEndPts[], INTEndPntPair aOptEndPts[]) const noexcept
{
    BC6H_ASSERT(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    BC6H_ASSERT(uPartitions < BC6H_MAX_REGIONS);
    INTColor aPixels[BC6H_NUM_PIXELS_PER_BLOCK];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        // collect the pixels in the region
        size_t np = 0;
        for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[p][pEP->uShape][i] == p)
            {
                aPixels[np++] = pEP->aIPixels[i];
            }
        }

        OptimizeOne(pEP, aPixels, np, aOrgErr[p], aOrgEndPts[p], aOptEndPts[p]);
    }
}

// Swap endpoints as needed to ensure that the indices at fix up have a 0 high-order bit
void D3DX_BC6H::SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[], size_t aIndices[]) noexcept
{
    BC6H_ASSERT(pEP);
    const size_t uPartitions   = ms_aInfo[pEP->uMode].uPartitions;
    const size_t uNumIndices   = size_t(1) << ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uHighIndexBit = uNumIndices >> 1;

    BC6H_ASSERT(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        const size_t i = g_aFixUp[uPartitions][pEP->uShape][p];
        BC6H_ASSERT(g_aPartitionTable[uPartitions][pEP->uShape][i] == p);
        if (aIndices[i] & uHighIndexBit)
        {
            // high bit is set, swap the aEndPts and indices for this region
            std__swap(aEndPts[p].A, aEndPts[p].B);

            for (size_t j = 0; j < BC6H_NUM_PIXELS_PER_BLOCK; ++j)
                if (g_aPartitionTable[uPartitions][pEP->uShape][j] == p)
                    aIndices[j] = uNumIndices - 1 - aIndices[j];
        }
    }
}

// assign indices given a tile, shape, and quantized endpoints, return toterr for each region
void D3DX_BC6H::AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[], size_t aIndices[], float aTotErr[]) const noexcept
{
    BC6H_ASSERT(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    auto const    uNumIndices = static_cast<const uint8_t>(1u << ms_aInfo[pEP->uMode].uIndexPrec);

    BC6H_ASSERT(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    // build list of possibles
    INTColor aPalette[BC6H_MAX_REGIONS][BC6H_MAX_INDICES];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        GeneratePaletteQuantized(pEP, aEndPts[p], aPalette[p]);
        aTotErr[p] = 0;
    }

    for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
    {
        const uint8_t uRegion = g_aPartitionTable[uPartitions][pEP->uShape][i];
        BC6H_ASSERT(uRegion < BC6H_MAX_REGIONS);
        float fBestErr = Norm(pEP->aIPixels[i], aPalette[uRegion][0]);
        aIndices[i]    = 0;

        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            const float fErr = Norm(pEP->aIPixels[i], aPalette[uRegion][j]);
            if (fErr > fBestErr) break; // error increased, so we're done searching
            if (fErr < fBestErr)
            {
                fBestErr    = fErr;
                aIndices[i] = j;
            }
        }
        aTotErr[uRegion] += fBestErr;
    }
}

void D3DX_BC6H::QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* aQntEndPts) const noexcept
{
    BC6H_ASSERT(pEP && aQntEndPts);
    const INTEndPntPair* aUnqEndPts  = pEP->aUnqEndPts[pEP->uShape];
    const LDRColorA&     Prec        = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const uint8_t        uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    BC6H_ASSERT(uPartitions < BC6H_MAX_REGIONS);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        aQntEndPts[p].A.r = Quantize(aUnqEndPts[p].A.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].A.g = Quantize(aUnqEndPts[p].A.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].A.b = Quantize(aUnqEndPts[p].A.b, Prec.b, pEP->bSigned);
        aQntEndPts[p].B.r = Quantize(aUnqEndPts[p].B.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].B.g = Quantize(aUnqEndPts[p].B.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].B.b = Quantize(aUnqEndPts[p].B.b, Prec.b, pEP->bSigned);
    }
}

void D3DX_BC6H::EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[], const size_t aIndices[]) noexcept
{
    BC6H_ASSERT(pEP);
    const uint8_t         uRealMode   = ms_aInfo[pEP->uMode].uMode;
    const uint8_t         uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const uint8_t         uIndexPrec  = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t          uHeaderBits = uPartitions > 0 ? 82u : 65u;
    const ModeDescriptor* desc        = ms_aDesc[pEP->uMode];
    size_t                uStartBit   = 0;

    while (uStartBit < uHeaderBits)
    {
        switch (desc[uStartBit].m_eField)
        {
            case M: SetBit(uStartBit, uint8_t(uRealMode >> desc[uStartBit].m_uBit) & 0x01u); break;
            case D: SetBit(uStartBit, uint8_t(pEP->uShape >> desc[uStartBit].m_uBit) & 0x01u); break;
            case RW: SetBit(uStartBit, uint8_t(aEndPts[0].A.r >> desc[uStartBit].m_uBit) & 0x01u); break;
            case RX: SetBit(uStartBit, uint8_t(aEndPts[0].B.r >> desc[uStartBit].m_uBit) & 0x01u); break;
            case RY: SetBit(uStartBit, uint8_t(aEndPts[1].A.r >> desc[uStartBit].m_uBit) & 0x01u); break;
            case RZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.r >> desc[uStartBit].m_uBit) & 0x01u); break;
            case GW: SetBit(uStartBit, uint8_t(aEndPts[0].A.g >> desc[uStartBit].m_uBit) & 0x01u); break;
            case GX: SetBit(uStartBit, uint8_t(aEndPts[0].B.g >> desc[uStartBit].m_uBit) & 0x01u); break;
            case GY: SetBit(uStartBit, uint8_t(aEndPts[1].A.g >> desc[uStartBit].m_uBit) & 0x01u); break;
            case GZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.g >> desc[uStartBit].m_uBit) & 0x01u); break;
            case BW: SetBit(uStartBit, uint8_t(aEndPts[0].A.b >> desc[uStartBit].m_uBit) & 0x01u); break;
            case BX: SetBit(uStartBit, uint8_t(aEndPts[0].B.b >> desc[uStartBit].m_uBit) & 0x01u); break;
            case BY: SetBit(uStartBit, uint8_t(aEndPts[1].A.b >> desc[uStartBit].m_uBit) & 0x01u); break;
            case BZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.b >> desc[uStartBit].m_uBit) & 0x01u); break;
            default: BC6H_ASSERT(false);
        }
    }

    for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
    {
        if (IsFixUpOffset(ms_aInfo[pEP->uMode].uPartitions, pEP->uShape, i))
            SetBits(uStartBit, uIndexPrec - 1u, static_cast<uint8_t>(aIndices[i]));
        else
            SetBits(uStartBit, uIndexPrec, static_cast<uint8_t>(aIndices[i]));
    }
    BC6H_ASSERT(uStartBit == 128);
}

void D3DX_BC6H::Refine(EncodeParams* pEP) noexcept
{
    BC6H_ASSERT(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    BC6H_ASSERT(uPartitions < BC6H_MAX_REGIONS);

    const bool    bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    float         aOrgErr[BC6H_MAX_REGIONS], aOptErr[BC6H_MAX_REGIONS];
    INTEndPntPair aOrgEndPts[BC6H_MAX_REGIONS], aOptEndPts[BC6H_MAX_REGIONS];
    size_t        aOrgIdx[BC6H_NUM_PIXELS_PER_BLOCK], aOptIdx[BC6H_NUM_PIXELS_PER_BLOCK];

    QuantizeEndPts(pEP, aOrgEndPts);
    AssignIndices(pEP, aOrgEndPts, aOrgIdx, aOrgErr);
    SwapIndices(pEP, aOrgEndPts, aOrgIdx);

    if (bTransformed) TransformForward(aOrgEndPts);
    if (EndPointsFit(pEP, aOrgEndPts))
    {
        if (bTransformed) TransformInverse(aOrgEndPts, ms_aInfo[pEP->uMode].RGBAPrec[0][0], pEP->bSigned);
        OptimizeEndPoints(pEP, aOrgErr, aOrgEndPts, aOptEndPts);
        AssignIndices(pEP, aOptEndPts, aOptIdx, aOptErr);
        SwapIndices(pEP, aOptEndPts, aOptIdx);

        float fOrgTotErr = 0.0f, fOptTotErr = 0.0f;
        for (size_t p = 0; p <= uPartitions; ++p)
        {
            fOrgTotErr += aOrgErr[p];
            fOptTotErr += aOptErr[p];
        }

        if (bTransformed) TransformForward(aOptEndPts);
        if (EndPointsFit(pEP, aOptEndPts) && fOptTotErr < fOrgTotErr && fOptTotErr < pEP->fBestErr)
        {
            pEP->fBestErr = fOptTotErr;
            EmitBlock(pEP, aOptEndPts, aOptIdx);
        }
        else if (fOrgTotErr < pEP->fBestErr)
        {
            // either it stopped fitting when we optimized it, or there was no improvement
            // so go back to the unoptimized endpoints which we know will fit
            if (bTransformed) TransformForward(aOrgEndPts);
            pEP->fBestErr = fOrgTotErr;
            EmitBlock(pEP, aOrgEndPts, aOrgIdx);
        }
    }
}

void D3DX_BC6H::GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[]) noexcept
{
    BC6H_ASSERT(pEP);
    BC6H_ASSERT(uRegion < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);
    const INTEndPntPair& endPts      = pEP->aUnqEndPts[pEP->uShape][uRegion];
    const uint8_t        uIndexPrec  = ms_aInfo[pEP->uMode].uIndexPrec;
    auto const           uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    BC6H_ASSERT(uNumIndices > 0);

    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
        case 3:
            aWeights = g_aWeights3;
            BC6H_ASSERT(uNumIndices <= 8);
            break;
        case 4:
            aWeights = g_aWeights4;
            BC6H_ASSERT(uNumIndices <= 16);
            break;
        default:
            BC6H_ASSERT(false);
            for (size_t i = 0; i < uNumIndices; ++i)
            {
//#pragma prefast(suppress : 22102 22103, "writing blocks in two halves confuses tool")
                aPalette[i] = INTColor(0, 0, 0);
            }
            return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = (endPts.A.r * (BC6H_WEIGHT_MAX - aWeights[i]) + endPts.B.r * aWeights[i] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT;
        aPalette[i].g = (endPts.A.g * (BC6H_WEIGHT_MAX - aWeights[i]) + endPts.B.g * aWeights[i] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT;
        aPalette[i].b = (endPts.A.b * (BC6H_WEIGHT_MAX - aWeights[i]) + endPts.B.b * aWeights[i] + BC6H_WEIGHT_ROUND) >> BC6H_WEIGHT_SHIFT;
    }
}

float D3DX_BC6H::MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const noexcept
{
    BC6H_ASSERT(pEP);
    const uint8_t uIndexPrec  = ms_aInfo[pEP->uMode].uIndexPrec;
    auto const    uNumIndices = static_cast<const uint8_t>(1u << uIndexPrec);
    INTColor      aPalette[BC6H_MAX_INDICES];
    GeneratePaletteUnquantized(pEP, uRegion, aPalette);

    float fTotalErr = 0.0f;
    for (size_t i = 0; i < np; ++i)
    {
        float fBestErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[0]);
        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0.0f; ++j)
        {
            const float fErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[j]);
            if (fErr > fBestErr) break; // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotalErr += fBestErr;
    }

    return fTotalErr;
}

//#define BC6H_USE_AU_PIX_TABLE

#    ifdef BC6H_USE_AU_PIX_TABLE
size_t g_auPixIdx[BC6H_MAX_SHAPES][BC6H_MAX_REGIONS][BC6H_MAX_REGIONS][BC6H_NUM_PIXELS_PER_BLOCK];
size_t g_np[BC6H_MAX_SHAPES][BC6H_MAX_REGIONS][BC6H_MAX_REGIONS];

struct InitTable
{
    InitTable()
    {
        for (size_t shape = 0; shape < BC6H_MAX_SHAPES; shape++)
        {
            for (size_t uPartitions = 0; uPartitions < BC6H_MAX_REGIONS; uPartitions++)
            {
                for (size_t p = 0; p < BC6H_MAX_REGIONS; ++p)
                {
                    size_t np = 0;
                    for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
                    {
                        if (g_aPartitionTable[uPartitions][shape][i] == p)
                        {
                            g_auPixIdx[shape][uPartitions][p][np++] = i;
                        }
                    }
                    //BC6H_ASSERT(np > 0);

                    g_np[shape][uPartitions][p] = np;
                }
            }
        }
    }
};

static InitTable init_au_pix_table;
#endif

float D3DX_BC6H::RoughMSE(EncodeParams* pEP) const noexcept
{
    BC6H_ASSERT(pEP);
    BC6H_ASSERT(pEP->uShape < BC6H_MAX_SHAPES);

    INTEndPntPair* aEndPts = pEP->aUnqEndPts[pEP->uShape];

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    BC6H_ASSERT(uPartitions < BC6H_MAX_REGIONS);

    #ifndef BC6H_USE_AU_PIX_TABLE
    size_t auPixIdx[BC6H_NUM_PIXELS_PER_BLOCK];
    #endif

    float fError = 0.0f;
    for (size_t p = 0; p <= uPartitions; ++p)
    {
        #ifdef BC6H_USE_AU_PIX_TABLE
        const size_t* auPixIdx = g_auPixIdx[pEP->uShape][uPartitions][p];
        size_t        np       = g_np[pEP->uShape][uPartitions][p];
        #else
        size_t np = 0;
        for (size_t i = 0; i < BC6H_NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[uPartitions][pEP->uShape][i] == p)
            {
                auPixIdx[np++] = i;
            }
        }        
        #endif

        // handle simple cases
        BC6H_ASSERT(np > 0);
        if (np == 1)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[0]];
            continue;
        }
        else if (np == 2)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[1]];
            continue;
        }

        HDRColorA epA, epB;
        OptimizeRGB(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
        aEndPts[p].A.Set(epA, pEP->bSigned);
        aEndPts[p].B.Set(epB, pEP->bSigned);
        if (pEP->bSigned)
        {
            aEndPts[p].A.Clamp(-F16MAX, F16MAX);
            aEndPts[p].B.Clamp(-F16MAX, F16MAX);
        }
        else
        {
            aEndPts[p].A.Clamp(0, F16MAX);
            aEndPts[p].B.Clamp(0, F16MAX);
        }

        fError += MapColors(pEP, p, np, auPixIdx);
    }

    return fError;
}

}

//=====================================================================================
// Entry points
//=====================================================================================

void DecodeBC6HU(void* pDest, const void* pSrc) noexcept
{
    static_assert(sizeof(Impl::D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<const Impl::D3DX_BC6H*>(pSrc)->Decode(false, reinterpret_cast<Impl::HDRColorA*>(pDest));
}

void DecodeBC6HS(void* pDest, const void* pSrc) noexcept
{
    static_assert(sizeof(Impl::D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<const Impl::D3DX_BC6H*>(pSrc)->Decode(true, reinterpret_cast<Impl::HDRColorA*>(pDest));
}

void EncodeBC6HU(void* pDest, const void* pSrc) noexcept
{
    static_assert(sizeof(Impl::D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<Impl::D3DX_BC6H*>(pDest)->Encode(false, reinterpret_cast<const Impl::HDRColorA*>(pSrc));
}

void EncodeBC6HS(void* pDest, const void* pSrc) noexcept
{
    static_assert(sizeof(Impl::D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<Impl::D3DX_BC6H*>(pDest)->Encode(true, reinterpret_cast<const Impl::HDRColorA*>(pSrc));
}

}

#    ifdef BC6H_ASSERT_UNDEF
#        undef BC6H_ASSERT_UNDEF
#    endif

#    ifdef BC6H_HALF_TO_FLOAT_UNDEF
#        undef BC6H_HALF_TO_FLOAT_UNDEF
#        undef BC6H_HALF_TO_FLOAT
#    endif

#    ifdef BC6H_FLOAT_TO_HALF_UNDEF
#        undef BC6H_FLOAT_TO_HALF_UNDEF
#        undef BC6H_FLOAT_TO_HALF
#    endif

#    undef BC6H_INLINE

#endif
