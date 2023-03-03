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

#ifndef PX_FPU_H
#define PX_FPU_H

#include "foundation/PxIntrinsics.h"
#include "foundation/PxAssert.h"
#include "foundation/PxFoundationConfig.h"

#define PX_IR(x) ((PxU32&)(x))	// integer representation of a floating-point value.
#define PX_SIR(x) ((PxI32&)(x))	// signed integer representation of a floating-point value.
#define PX_FR(x) ((PxReal&)(x))		// floating-point representation of a integer value.

#define PX_FPU_GUARD PxFPUGuard scopedFpGuard;
#define PX_SIMD_GUARD PxSIMDGuard scopedFpGuard;
#define PX_SIMD_GUARD_CNDT(x) PxSIMDGuard scopedFpGuard(x);

#if !PX_DOXYGEN
namespace physx
{
#endif
// sets the default SDK state for scalar and SIMD units
class PX_FOUNDATION_API PxFPUGuard
{
  public:
	PxFPUGuard();  // set fpu control word for PhysX
	~PxFPUGuard(); // restore fpu control word
  private:
	PxU32 mControlWords[8];
};

// sets default SDK state for simd unit only, lighter weight than FPUGuard
class PxSIMDGuard
{
  public:
	PX_INLINE PxSIMDGuard(bool enable = true);  // set simd control word for PhysX
	PX_INLINE ~PxSIMDGuard(); // restore simd control word
  private:
#if !(PX_LINUX || PX_OSX) || (!PX_EMSCRIPTEN && PX_INTEL_FAMILY)
  PxU32			mControlWord;
  bool			mEnabled;
#endif
};

/**
\brief Enables floating point exceptions for the scalar and SIMD unit
*/
PX_FOUNDATION_API void PxEnableFPExceptions();

/**
\brief Disables floating point exceptions for the scalar and SIMD unit
*/
PX_FOUNDATION_API void PxDisableFPExceptions();

#if !PX_DOXYGEN
} // namespace physx
#endif

#if PX_WINDOWS_FAMILY
#include "foundation/windows/PxWindowsFPU.h"
#elif ((PX_LINUX || PX_OSX || PX_PS4 || PX_PS5) && PX_SSE2)
#include "foundation/unix/PxUnixFPU.h"
#else
PX_INLINE physx::PxSIMDGuard::PxSIMDGuard(bool)
{
}
PX_INLINE physx::PxSIMDGuard::~PxSIMDGuard()
{
}
#endif

#endif

