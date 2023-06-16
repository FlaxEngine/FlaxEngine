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

#ifndef PX_REMESHING_EXT_H
#define PX_REMESHING_EXT_H
/** \addtogroup extensions
  @{
*/

#include "foundation/PxVec3.h"
#include "foundation/PxArray.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
	
	/**
	\brief Provides methods to adjust the tessellation of meshes
	*/
	class PxRemeshingExt
	{
	public:
		/**
		\brief Processes a triangle mesh and makes sure that no triangle edge is longer than the maximal edge length specified

		To shorten edges that are too long, additional points get inserted at their center leading to a subdivision of the input mesh.
		This process is executed repeatedly until the maximum edge length criterion is satisfied
				
		\param[in,out] triangles			The triangles of the mesh where a maximum edge length should be enforced. They will be modified in place during the process.
		\param[in,out] points				The vertices of the mesh where a maximum edge length should be enforced. They will be modified in place during the process.
		\param[in] maxEdgeLength			The maximum edge length allowed after processing the input	
		\param[in] maxIterations			The maximum number of subdivision iterations
		\param[out] triangleMap				An optional map that provides the index of the original triangle for every triangle after the subdivision
		\param[in] triangleCountThreshold	Optional limit to the number of triangles. Not guaranteed to match exactly, the algorithm will just stop as soon as possible after reaching the limit.

		\return True if any remeshing was applied
		*/
		static bool limitMaxEdgeLength(PxArray<PxU32>& triangles, PxArray<PxVec3>& points, PxReal maxEdgeLength, 
			PxU32 maxIterations = 100, PxArray<PxU32>* triangleMap = NULL, PxU32 triangleCountThreshold = 0xFFFFFFFF);
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
