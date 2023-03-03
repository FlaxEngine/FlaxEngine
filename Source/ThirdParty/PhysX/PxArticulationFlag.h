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

#ifndef PX_ARTICULATION_FLAG_H
#define PX_ARTICULATION_FLAG_H

#include "PxPhysXConfig.h"
#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief A description of the types of articulation data that may be directly written to and read from the GPU using the functions
	PxScene::copyArticulationData() and PxScene::applyArticulationData(). Types that are read-only may only be used in conjunction with
	PxScene::copyArticulationData(). Types that are write-only may only be used in conjunction with PxScene::applyArticulationData().
	A subset of data types may be used in conjunction with both PxScene::applyArticulationData() and PxScene::applyArticulationData().

	@see PxArticulationCache, PxScene::copyArticulationData(), PxScene::applyArticulationData()
	*/
	class PxArticulationGpuDataType
	{
	public:
		enum Enum
		{
			eJOINT_POSITION = 0,		//!< The joint positions, read and write, see PxScene::copyArticulationData(), PxScene::applyArticulationData()
			eJOINT_VELOCITY,			//!< The joint velocities, read and write,  see PxScene::copyArticulationData(), PxScene::applyArticulationData()
			eJOINT_ACCELERATION,		//!< The joint accelerations, read only, see PxScene::copyArticulationData()
			eJOINT_FORCE,				//!< The applied joint forces, write only, see PxScene::applyArticulationData()
			eJOINT_SOLVER_FORCE,		//!< The computed joint constraint solver forces, read only, see PxScene::copyArticulationData()()
			eJOINT_TARGET_VELOCITY,		//!< The velocity targets for the joint drives, write only, see PxScene::applyArticulationData()
			eJOINT_TARGET_POSITION,		//!< The position targets for the joint drives, write only, see PxScene::applyArticulationData()
			eSENSOR_FORCE,				//!< The spatial sensor forces, read only, see PxScene::copyArticulationData()
			eROOT_TRANSFORM,			//!< The root link transform, read and write, see PxScene::copyArticulationData(), PxScene::applyArticulationData()
			eROOT_VELOCITY,				//!< The root link velocity, read and write, see PxScene::copyArticulationData(), PxScene::applyArticulationData()
			eLINK_TRANSFORM,			//!< The link transforms including root link, read only, see PxScene::copyArticulationData()
			eLINK_VELOCITY,				//!< The link velocities including root link, read only, see PxScene::copyArticulationData()
			eLINK_FORCE,				//!< The forces to apply to links, write only, see PxScene::applyArticulationData()
			eLINK_TORQUE,				//!< The torques to apply to links, write only, see PxScene::applyArticulationData()
			eFIXED_TENDON,				//!< Fixed tendon data, write only, see PxScene::applyArticulationData()
			eFIXED_TENDON_JOINT,		//!< Fixed tendon joint data, write only, see PxScene::applyArticulationData()
			eSPATIAL_TENDON,			//!< Spatial tendon data, write only, see PxScene::applyArticulationData()
			eSPATIAL_TENDON_ATTACHMENT  //!< Spatial tendon attachment data, write only, see PxScene::applyArticulationData()
		};
	};


	/**
	\brief These flags determine what data is read or written to the internal articulation data via cache.

	@see PxArticulationCache PxArticulationReducedCoordinate::copyInternalStateToCache PxArticulationReducedCoordinate::applyCache
	*/
	class PxArticulationCacheFlag
	{
	public:
		enum Enum
		{
			eVELOCITY = (1 << 0),				//!< The joint velocities, see PxArticulationCache::jointVelocity.
			eACCELERATION = (1 << 1),			//!< The joint accelerations, see PxArticulationCache::jointAcceleration.
			ePOSITION = (1 << 2),				//!< The joint positions, see PxArticulationCache::jointPosition.
			eFORCE = (1 << 3),					//!< The joint forces, see PxArticulationCache::jointForce.
			eLINK_VELOCITY = (1 << 4),			//!< The link velocities, see PxArticulationCache::linkVelocity.
			eLINK_ACCELERATION = (1 << 5),		//!< The link accelerations, see PxArticulationCache::linkAcceleration.
			eROOT_TRANSFORM = (1 << 6),			//!< Root link transform, see PxArticulationCache::rootLinkData.
			eROOT_VELOCITIES = (1 << 7),		//!< Root link velocities (read/write) and accelerations (read), see PxArticulationCache::rootLinkData.
			eSENSOR_FORCES = (1 << 8),			//!< The spatial sensor forces, see PxArticulationCache::sensorForces.
			eJOINT_SOLVER_FORCES = (1 << 9),	//!< Solver constraint joint forces, see PxArticulationCache::jointSolverForces.
			eALL = (eVELOCITY | eACCELERATION | ePOSITION | eLINK_VELOCITY | eLINK_ACCELERATION | eROOT_TRANSFORM | eROOT_VELOCITIES)
		};
	};

	typedef PxFlags<PxArticulationCacheFlag::Enum, PxU32> PxArticulationCacheFlags;
	PX_FLAGS_OPERATORS(PxArticulationCacheFlag::Enum, PxU32)

#if !PX_DOXYGEN
}
#endif

#endif
