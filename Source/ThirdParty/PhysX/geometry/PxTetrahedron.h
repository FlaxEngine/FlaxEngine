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

#ifndef PX_TETRAHEDRON_H
#define PX_TETRAHEDRON_H
/** \addtogroup geomutils
@{
*/

#include "common/PxPhysXCommonConfig.h"
#include "foundation/PxVec3.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Tetrahedron class.
	*/
	class PxTetrahedron
	{
	public:
		/**
		\brief Constructor
		*/
		PX_FORCE_INLINE			PxTetrahedron() {}

		/**
		\brief Constructor

		\param[in] p0 Point 0
		\param[in] p1 Point 1
		\param[in] p2 Point 2
		\param[in] p3 Point 3
		*/
		PX_FORCE_INLINE			PxTetrahedron(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2, const PxVec3& p3)
		{
			verts[0] = p0;
			verts[1] = p1;
			verts[2] = p2;
			verts[3] = p3;
		}

		/**
		\brief Copy constructor

		\param[in] tetrahedron copy
		*/
		PX_FORCE_INLINE			PxTetrahedron(const PxTetrahedron& tetrahedron)
		{
			verts[0] = tetrahedron.verts[0];
			verts[1] = tetrahedron.verts[1];
			verts[2] = tetrahedron.verts[2];
			verts[3] = tetrahedron.verts[3];
		}

		/**
		\brief Destructor
		*/
		PX_FORCE_INLINE			~PxTetrahedron() {}

		/**
		\brief Assignment operator
		*/
		PX_FORCE_INLINE void operator=(const PxTetrahedron& tetrahedron)
		{
			verts[0] = tetrahedron.verts[0];
			verts[1] = tetrahedron.verts[1];
			verts[2] = tetrahedron.verts[2];
			verts[3] = tetrahedron.verts[3];
		}
		/**
		\brief Array of Vertices.
		*/
		PxVec3		verts[4];

	};


#if !PX_DOXYGEN
}
#endif

/** @} */
#endif
