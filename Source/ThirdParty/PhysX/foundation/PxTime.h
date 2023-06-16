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

#ifndef PX_TIME_H
#define PX_TIME_H

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxFoundationConfig.h"

#if PX_LINUX || PX_ANDROID
#include <time.h>
#endif

#if !PX_DOXYGEN
namespace physx
{
#endif

struct PxCounterFrequencyToTensOfNanos
{
	PxU64 mNumerator;
	PxU64 mDenominator;
	PxCounterFrequencyToTensOfNanos(PxU64 inNum, PxU64 inDenom) : mNumerator(inNum), mDenominator(inDenom)
	{
	}

	// quite slow.
	PxU64 toTensOfNanos(PxU64 inCounter) const
	{
		return (inCounter * mNumerator) / mDenominator;
	}
};

class PX_FOUNDATION_API PxTime
{
  public:
	typedef PxF64 Second;
	static const PxU64 sNumTensOfNanoSecondsInASecond = 100000000;
	// This is supposedly guaranteed to not change after system boot
	// regardless of processors, speedstep, etc.
	static const PxCounterFrequencyToTensOfNanos& getBootCounterFrequency();

	static PxCounterFrequencyToTensOfNanos getCounterFrequency();

	static PxU64 getCurrentCounterValue();

	// SLOW!!
	// Thar be a 64 bit divide in thar!
	static PxU64 getCurrentTimeInTensOfNanoSeconds()
	{
		PxU64 ticks = getCurrentCounterValue();
		return getBootCounterFrequency().toTensOfNanos(ticks);
	}

	PxTime();
	Second getElapsedSeconds();
	Second peekElapsedSeconds();
	Second getLastTime() const;

  private:
#if PX_LINUX || PX_ANDROID || PX_APPLE_FAMILY || PX_PS4 || PX_PS5
	Second mLastTime;
#else
	PxI64 mTickCount;
#endif
};
#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

