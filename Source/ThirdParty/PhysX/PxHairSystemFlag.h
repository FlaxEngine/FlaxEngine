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

#ifndef PX_HAIR_SYSTEM_FLAG_H
#define PX_HAIR_SYSTEM_FLAG_H

#include "PxPhysXConfig.h"
#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Identifies input and output buffers for PxHairSystem
	\see PxHairSystemData::readData(), PxHairSystemData::writeData(), PxBuffer
	*/
	struct PxHairSystemData
	{
		enum Enum
		{
			eNONE = 0,												//!< No data specified
			ePOSITION_INVMASS = 1 << 0,								//!< Specifies the position (first 3 floats) and inverse mass (last float) data (array of PxVec4 * max number of vertices)
			eVELOCITY = 1 << 1,										//!< Specifies the velocity (first 3 floats) data (array of PxVec4 * max number of vertices)
			eALL = ePOSITION_INVMASS | eVELOCITY					//!< Specifies everything
		};
	};
	typedef PxFlags<PxHairSystemData::Enum, PxU32> PxHairSystemDataFlags;

	/**
	\brief Binary settings for hair system simulation
	*/
	struct PxHairSystemFlag
	{
		enum Enum
		{
			eDISABLE_SELF_COLLISION =		1 << 0, //!< Determines if self-collision between hair vertices is ignored
			eDISABLE_EXTERNAL_COLLISION =	1 << 1, //!< Determines if collision between hair and external bodies is ignored
			eDISABLE_TWOSIDED_ATTACHMENT =	1 << 2  //!< Determines if attachment constraint is also felt by body to which the hair is attached
		};
	};
	typedef PxFlags<PxHairSystemFlag::Enum, PxU32> PxHairSystemFlags;

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
