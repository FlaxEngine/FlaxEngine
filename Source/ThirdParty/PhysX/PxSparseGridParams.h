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

#ifndef PX_SPARSE_GRID_PARAMS_H
#define PX_SPARSE_GRID_PARAMS_H
/** \addtogroup physics
@{ */

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxMath.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Parameters to define the sparse grid settings like grid spacing, maximal number of subgrids etc.
	*/
	struct PxSparseGridParams
	{
		/**
		\brief Default constructor.
		*/
		PX_INLINE PxSparseGridParams()
		{
			maxNumSubgrids = 512;
			subgridSizeX = 32;
			subgridSizeY = 32;
			subgridSizeZ = 32;
			gridSpacing = 0.2f;
			haloSize = 1;
		}

		/**
		\brief Copy constructor.
		*/
		PX_CUDA_CALLABLE PX_INLINE PxSparseGridParams(const PxSparseGridParams& params)
		{
			maxNumSubgrids = params.maxNumSubgrids;
			subgridSizeX = params.subgridSizeX;
			subgridSizeY = params.subgridSizeY;
			subgridSizeZ = params.subgridSizeZ;
			gridSpacing = params.gridSpacing;
			haloSize = params.haloSize;
		}

		PX_CUDA_CALLABLE PX_INLINE PxU32 getNumCellsPerSubgrid() const
		{
			return subgridSizeX * subgridSizeY * subgridSizeZ;
		}

		PX_CUDA_CALLABLE PX_INLINE PxReal getSqrt3dx() const
		{
			return PxSqrt(3.0f) * gridSpacing;
		}


		/**
		\brief (re)sets the structure to the default.
		*/
		PX_INLINE void setToDefault()
		{
			*this = PxSparseGridParams();
		}

		/**
		\brief Assignment operator
		*/
		PX_INLINE void operator = (const PxSparseGridParams& params)
		{
			maxNumSubgrids = params.maxNumSubgrids;
			subgridSizeX = params.subgridSizeX;
			subgridSizeY = params.subgridSizeY;
			subgridSizeZ = params.subgridSizeZ;
			gridSpacing = params.gridSpacing;
			haloSize = params.haloSize;
		}

		PxU32	maxNumSubgrids;			//!< Maximum number of subgrids
		PxReal	gridSpacing;			//!< Grid spacing for the grid
		PxU16	subgridSizeX;			//!< Subgrid resolution in x dimension (must be an even number)
		PxU16	subgridSizeY;			//!< Subgrid resolution in y dimension (must be an even number)
		PxU16	subgridSizeZ;			//!< Subgrid resolution in z dimension (must be an even number)
		PxU16	haloSize;				//!< Number of halo cell layers around every subgrid cell. Only 0 and 1 are valid values
	};


#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
