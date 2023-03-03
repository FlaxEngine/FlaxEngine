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

#ifndef PX_INSERTION_CALLBACK_H
#define PX_INSERTION_CALLBACK_H

#include "common/PxBase.h"

/** \addtogroup common
@{
*/

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Callback interface that permits TriangleMesh, Heightfield, ConvexMesh or BVH to be used
	directly without the need to store the cooking results into a stream.

	Using this is advised only if real-time cooking is required; using "offline" cooking and
	streams is otherwise preferred.

	The default PxInsertionCallback implementations must be used. The PxPhysics
	default callback can be obtained using the PxPhysics::getPhysicsInsertionCallback().
	The PxCooking default callback can be obtained using the PxCooking::getStandaloneInsertionCallback().

	@see PxCooking PxPhysics
	*/
	class PxInsertionCallback
	{
	public:
		PxInsertionCallback()				{}

		/**
		\brief Builds object (TriangleMesh, Heightfield, ConvexMesh or BVH) from given data in PxPhysics.		

		\param type Object type to build.
		\param data Object data
		\return PxBase Created object in PxPhysics.
		*/
		virtual PxBase* buildObjectFromData(PxConcreteType::Enum type, void* data) = 0;

	protected:
		virtual ~PxInsertionCallback()		{}
	};

	typedef PxInsertionCallback PxPhysicsInsertionCallback;

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
