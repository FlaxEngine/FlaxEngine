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

#ifndef PX_TETRAHEDRON_MESH_ANALYSIS_RESULT_H
#define PX_TETRAHEDRON_MESH_ANALYSIS_RESULT_H


#include "PxPhysXConfig.h"
#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief These flags indicate what kind of deficiencies a tetrahedron mesh has and describe if the mesh is considered ok, problematic or invalid for softbody cooking
	*/
	class PxTetrahedronMeshAnalysisResult
	{
	public:
		enum Enum
		{
			eVALID = 0,
			eDEGENERATE_TETRAHEDRON = (1 << 0),					//!< At least one tetrahedron has zero or negative volume. This can happen when the input triangle mesh contains triangles that are very elongated, e. g. one edge is a lot shorther than the other two.

			eMESH_IS_PROBLEMATIC = (1 << 1),					//!< flag is set if the mesh is categorized as problematic
			eMESH_IS_INVALID = (1 << 2)							//!< flag is set if the mesh is categorized as invalid
		};
	};
	typedef PxFlags<PxTetrahedronMeshAnalysisResult::Enum, PxU32> PxTetrahedronMeshAnalysisResults;
	PX_FLAGS_OPERATORS(PxTetrahedronMeshAnalysisResult::Enum, PxU32)

#if !PX_DOXYGEN
}
#endif

#endif
