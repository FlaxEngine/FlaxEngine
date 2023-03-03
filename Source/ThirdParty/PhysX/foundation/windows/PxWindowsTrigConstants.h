// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef PX_WINDOWS_TRIG_CONSTANTS_H
#define PX_WINDOWS_TRIG_CONSTANTS_H

namespace physx
{
namespace aos
{

#define PX_GLOBALCONST extern const __declspec(selectany)

__declspec(align(16)) struct PX_VECTORF32
{
	float f[4];
};

PX_GLOBALCONST PX_VECTORF32 g_PXSinCoefficients0 = { { 1.0f, -0.166666667f, 8.333333333e-3f, -1.984126984e-4f } };
PX_GLOBALCONST PX_VECTORF32
g_PXSinCoefficients1 = { { 2.755731922e-6f, -2.505210839e-8f, 1.605904384e-10f, -7.647163732e-13f } };
PX_GLOBALCONST PX_VECTORF32
g_PXSinCoefficients2 = { { 2.811457254e-15f, -8.220635247e-18f, 1.957294106e-20f, -3.868170171e-23f } };
PX_GLOBALCONST PX_VECTORF32 g_PXCosCoefficients0 = { { 1.0f, -0.5f, 4.166666667e-2f, -1.388888889e-3f } };
PX_GLOBALCONST PX_VECTORF32
g_PXCosCoefficients1 = { { 2.480158730e-5f, -2.755731922e-7f, 2.087675699e-9f, -1.147074560e-11f } };
PX_GLOBALCONST PX_VECTORF32
g_PXCosCoefficients2 = { { 4.779477332e-14f, -1.561920697e-16f, 4.110317623e-19f, -8.896791392e-22f } };
PX_GLOBALCONST PX_VECTORF32 g_PXReciprocalTwoPi = { { PxInvTwoPi, PxInvTwoPi, PxInvTwoPi, PxInvTwoPi } };
PX_GLOBALCONST PX_VECTORF32 g_PXTwoPi = { { PxTwoPi, PxTwoPi, PxTwoPi, PxTwoPi } };

} // namespace aos
} // namespace physx

#endif
