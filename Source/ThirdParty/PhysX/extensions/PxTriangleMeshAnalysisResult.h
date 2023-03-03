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

#ifndef PX_TRIANGLE_MESH_ANALYSIS_RESULT_H
#define PX_TRIANGLE_MESH_ANALYSIS_RESULT_H


#include "PxPhysXConfig.h"
#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief These flags indicate what kind of deficiencies a triangle mesh has and describe if the mesh is considered ok, problematic or invalid for tetmeshing
	*/
	class PxTriangleMeshAnalysisResult
	{
	public:
		enum Enum
		{
			eVALID = 0,
			eZERO_VOLUME = (1 << 0),							//!< invalid:		Flat mesh without meaningful amount of volume - cannot be meshed since a tetmesh is volumetric	
			eOPEN_BOUNDARIES = (1 << 1),						//!< problematic:	Open boundary means that the mesh is not watertight and that there are holes. The mesher can fill holes but the surface might have an unexpected shape where the hole was.
			eSELF_INTERSECTIONS = (1 << 2),						//!< problematic:	The surface of the resulting mesh won't match exactly at locations of self-intersections. The tetmesh might be connected at self-intersections even if the input triangle mesh is not
			eINCONSISTENT_TRIANGLE_ORIENTATION = (1 << 3),		//!< invalid:		It is not possible to distinguish what is inside and outside of the mesh. If there are no self-intersections and not edges shared by more than two triangles, a call to makeTriOrientationConsistent can fix this. Without fixing it, the output from the tetmesher will be incorrect
			eCONTAINS_ACUTE_ANGLED_TRIANGLES = (1 << 4),		//!< problematic:	An ideal mesh for a softbody has triangles with similar angles and evenly distributed vertices. Acute angles can be handled but might lead to a poor quality tetmesh.
			eEDGE_SHARED_BY_MORE_THAN_TWO_TRIANGLES = (1 << 5),	//!< problematic:	Border case of a self-intersecting mesh. The tetmesh might not match the surace exactly near such edges.
			eCONTAINS_DUPLICATE_POINTS = (1 << 6),				//!< ok:			Duplicate points can be handled by the mesher without problems. The resulting tetmesh will only make use of first unique point that is found, duplicate points will get mapped to that unique point in the tetmesh. Therefore the tetmesh can contain points that are not accessed by a tet.
			eCONTAINS_INVALID_POINTS = (1 << 7),				//!< invalid:		Points contain NAN, infinity or similar values that will lead to an invalid mesh
			eREQUIRES_32BIT_INDEX_BUFFER = (1 << 8),			//!< invalid:		Mesh contains more indices than a 16bit index buffer can address

			eMESH_IS_PROBLEMATIC = (1 << 9),					//!< flag is set if the mesh is categorized as problematic
			eMESH_IS_INVALID = (1 << 10)						//!< flag is set if the mesh is categorized as invalid
		};
	};
	typedef PxFlags<PxTriangleMeshAnalysisResult::Enum, PxU32> PxTriangleMeshAnalysisResults;
	PX_FLAGS_OPERATORS(PxTriangleMeshAnalysisResult::Enum, PxU32)

#if !PX_DOXYGEN
}
#endif

#endif
