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


#ifndef PX_PHYSICS_FEM_PARAMETER_H
#define PX_PHYSICS_FEM_PARAMETER_H

#include "foundation/PxSimpleTypes.h"


#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Set of parameters to control the sleeping and collision behavior of FEM based objects
	*/
	struct PxFEMParameters
	{
	public:
		/**
		\brief Velocity damping value. After every timestep the velocity is reduced while the magnitude of the reduction depends on velocityDamping
		<b>Default:</b> 0.05
		*/
		PxReal	velocityDamping;
		/**
		\brief Threshold that defines the maximal magnitude of the linear motion a fem body can move in one second before it becomes a candidate for sleeping
		<b>Default:</b> 0.1
		*/
		PxReal	settlingThreshold;
		/**
		\brief Threshold that defines the maximal magnitude of the linear motion a fem body can move in one second such that it can go to sleep in the next frame
		<b>Default:</b> 0.05
		*/
		PxReal	sleepThreshold;
		/**
		\brief Damping value that damps the motion of bodies that move slow enough to be candidates for sleeping (see settlingThreshold)
		<b>Default:</b> 10
		*/
		PxReal	sleepDamping;
		/**
		\brief Penetration value that needs to get exceeded before contacts for self collision are generated. Will only have an effect if self collisions are enabled.
		<b>Default:</b> 0.1
		*/
		PxReal	selfCollisionFilterDistance;
		/**
		\brief Stress threshold to deactivate collision contacts in case the tetrahedron's stress magnitude exceeds the threshold
		<b>Default:</b> 0.9
		*/
		PxReal	selfCollisionStressTolerance;

#ifndef __CUDACC__
		PxFEMParameters()
		{
			velocityDamping = 0.05f;
			settlingThreshold = 0.1f;
			sleepThreshold = 0.05f;
			sleepDamping = 10.f;
			selfCollisionFilterDistance = 0.1f;
			selfCollisionStressTolerance = 0.9f;
		}
#endif
	};

#if !PX_DOXYGEN
}
#endif

#endif
