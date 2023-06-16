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

#ifndef PX_SDF_DESC_H
#define PX_SDF_DESC_H
/** \addtogroup cooking
@{
*/

#include "PxPhysXConfig.h"
#include "geometry/PxSimpleTriangleMesh.h"
#include "foundation/PxBounds3.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief A helper structure to define dimensions in 3D
	*/
	struct PxDim3
	{
		PxU32 x, y, z;
	};

	/**
	\brief Defines the number of bits per subgrid pixel
	*/
	class PxSdfBitsPerSubgridPixel
	{
	public:
		enum Enum
		{
			e8_BIT_PER_PIXEL = 1,	//!< 8 bit per subgrid pixel (values will be stored as normalized integers)
			e16_BIT_PER_PIXEL = 2,	//!< 16 bit per subgrid pixel (values will be stored as normalized integers)
			e32_BIT_PER_PIXEL = 4	//!< 32 bit per subgrid pixel (values will be stored as floats in world scale units)
		};
	};

	/**
	\brief A structure describing signed distance field for mesh.
	*/
	class PxSDFDesc
	{
	public:

		/**
		\brief Pointer to first sdf array element.
		*/
		PxBoundedData sdf;

		/**
		\brief Dimensions of sdf
		*/
		PxDim3 dims;

		/**
		\brief The Lower bound of the original mesh
		*/
		PxVec3 meshLower;

		/**
		\brief The spacing of each sdf voxel
		*/
		PxReal spacing;

		
		/**
		\brief The number of cells in a sparse subgrid block (full block has subgridSize^3 cells and (subgridSize+1)^3 samples). If set to zero, this indicates that only a dense background grid SDF is used without sparse blocks
		*/
		PxU32 subgridSize;
		
		/**
		\brief Enumeration that defines the number of bits per subgrid pixel (either 32, 16 or 8bits)
		*/
		PxSdfBitsPerSubgridPixel::Enum bitsPerSubgridPixel;
		
		/**
		\brief Number of subgrid blocks in the 3d texture. The full texture dimension will be sdfSubgrids3DTexBlockDim*(subgridSize+1).
		*/
		PxDim3 sdfSubgrids3DTexBlockDim;
		
		/**
		\brief The data to create the 3d texture containg the packed subgrid blocks. Stored as PxU8 to support multiple formats (8, 16 and 32 bits per pixel)
		*/
		PxBoundedData sdfSubgrids;
		
		/**
		\brief Array with start indices into the subgrid texture for every subgrid block. 10bits for z coordinate, 10bits for y and 10bits for x. Encoding is as follows: slot = (z << 20) | (y << 10) | x
		*/
		PxBoundedData sdfStartSlots;

		/**
		\brief The minimum value over all subgrid blocks. Used if normalized textures are used which is the case for 8 and 16bit formats
		*/
		PxReal subgridsMinSdfValue;

		/**
		\brief The maximum value over all subgrid blocks. Used if normalized textures are used which is the case for 8 and 16bit formats
		*/
		PxReal subgridsMaxSdfValue;

		/**
		\brief The bounds of the sdf. If left unassigned (empty), the bounds of the mesh will be used
		*/
		PxBounds3 sdfBounds;

		/**
		\brief Narrow band thickness as a fraction of the bounds diagonal length. Every subgrid block that 
		overlaps with the narrow band around the mesh surface will be kept providing high resultion around the mesh surface. 
		The valid range of this parameter is (0, 1). The higher the value, the more subgrids will get created, the more memory will be required.
		*/
		PxReal narrowBandThicknessRelativeToSdfBoundsDiagonal;

		/**
		\brief The number of threads that are launched to compute the signed distance field
		*/
		PxU32 numThreadsForSdfConstruction;

		/**
		\brief Constructor
		*/
		PX_INLINE PxSDFDesc();

		/**
		\brief Returns true if the descriptor is valid.
		\return true if the current settings are valid
		*/
		PX_INLINE bool isValid() const;
	};

	PX_INLINE PxSDFDesc::PxSDFDesc()
	{
		sdf.data = NULL;
		dims.x = 0;
		dims.y = 0;
		dims.z = 0;
		spacing = 0;
		meshLower = PxVec3(PxZero);
		subgridSize = 0;
		subgridsMinSdfValue = 0.0f;
		subgridsMaxSdfValue = 0.0f;
		sdfBounds = PxBounds3::empty();
		bitsPerSubgridPixel = PxSdfBitsPerSubgridPixel::e16_BIT_PER_PIXEL;
		narrowBandThicknessRelativeToSdfBoundsDiagonal = 0.01f;
		numThreadsForSdfConstruction = 1;
	}

	PX_INLINE bool PxSDFDesc::isValid() const
	{
		// Check validity of user's input(if any)
		if (sdf.data)
		{
			if (dims.x < 1 || dims.y < 1 || dims.z < 1)
				return false;
			if (!meshLower.isFinite())
				return false;
			if (spacing <= 0)
				return false;
		}

		return true;
	}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
