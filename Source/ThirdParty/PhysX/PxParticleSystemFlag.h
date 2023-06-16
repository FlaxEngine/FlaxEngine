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

#ifndef PX_PARTICLE_SYSTEM_FLAG_H
#define PX_PARTICLE_SYSTEM_FLAG_H

#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Identifies dirty particle buffers that need to be updated in the particle system.

This flag can be used mark the device user buffers that are dirty and need to be written to the particle system.
*/
struct PxParticleBufferFlag
{
	enum Enum
	{
		eNONE = 0,								//!< No data specified

		eUPDATE_POSITION = 1 << 0,				//!< Specifies the position (first 3 floats) and inverse mass (last float) data (array of PxVec4 * number of particles)
		eUPDATE_VELOCITY = 1 << 1,				//!< Specifies the velocity (first 3 floats) data (array of PxVec4 * number of particles)
		eUPDATE_PHASE = 1 << 2,					//!< Specifies the per-particle phase flag data (array of PxU32 * number of particles)
		eUPDATE_RESTPOSITION = 1 << 3,			//!< Specifies the rest position (first 3 floats) data for cloth buffers
		eUPDATE_CLOTH = 1 << 5,					//!< Specifies the cloth buffer (see PxParticleClothBuffer)
		eUPDATE_RIGID = 1 << 6,					//!< Specifies the rigid buffer (see PxParticleRigidBuffer)
		eUPDATE_DIFFUSE_PARAM = 1 << 7,			//!< Specifies the diffuse particle parameter buffer (see PxDiffuseParticleParams)
		eUPDATE_ATTACHMENTS = 1 << 8,			//!< Specifies the attachments.

		eALL =
		eUPDATE_POSITION | eUPDATE_VELOCITY | eUPDATE_PHASE | eUPDATE_RESTPOSITION | eUPDATE_CLOTH | eUPDATE_RIGID | eUPDATE_DIFFUSE_PARAM | eUPDATE_ATTACHMENTS
	};
};

typedef PxFlags<PxParticleBufferFlag::Enum, PxU32> PxParticleBufferFlags;

/**
\brief A pair of particle buffer unique id and GPU particle system index. 

@see PxScene::applyParticleBufferData
*/
struct PxGpuParticleBufferIndexPair
{
	PxU32 		systemIndex; // gpu particle system index	
	PxU32		bufferIndex; // particle buffer unique id
};

/**
\brief Identifies per-particle behavior for a PxParticleSystem.

See #PxParticleSystem::createPhase().
*/
struct PxParticlePhaseFlag
{
	enum Enum
	{
		eParticlePhaseGroupMask = 0x000fffff,			//!< Bits [ 0, 19] represent the particle group for controlling collisions
		eParticlePhaseFlagsMask = 0xfff00000,			//!< Bits [20, 23] hold flags about how the particle behave 

		eParticlePhaseSelfCollide = 1 << 20,			//!< If set this particle will interact with particles of the same group
		eParticlePhaseSelfCollideFilter = 1 << 21,		//!< If set this particle will ignore collisions with particles closer than the radius in the rest pose, this flag should not be specified unless valid rest positions have been specified using setRestParticles()
		eParticlePhaseFluid = 1 << 22					//!< If set this particle will generate fluid density constraints for its overlapping neighbors
	};
};

typedef PxFlags<PxParticlePhaseFlag::Enum, PxU32> PxParticlePhaseFlags;
	
#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
