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


#ifndef PX_HAIRSYSTEM_DESC_H
#define PX_HAIRSYSTEM_DESC_H
/** \addtogroup geomutils
@{
*/

#include "foundation/PxFlags.h"
#include "common/PxCoreUtilityTypes.h"

#if PX_ENABLE_FEATURES_UNDER_CONSTRUCTION

#if !PX_DOXYGEN
namespace physx
{
#endif

	struct PxHairSystemDescFlag
	{
		enum Enum
		{
			/**
			Determines whether or not to allocate memory on device (GPU) or on Host (CPU)
			 */
			eDEVICE_MEMORY = (1<<0)
		};
	};

	/**
	\brief collection of set bits defined in PxHairSystemDescFlag
	\see PxHairSystemDescFlag
	*/
	typedef PxFlags<PxHairSystemDescFlag::Enum, PxU16> PxHairSystemDescFlags;
	PX_FLAGS_OPERATORS(PxHairSystemDescFlag::Enum, PxU16)

	/**
	\brief Descriptor class for #PxHairSystem
	\note The data is *copied* when a PxHairSystem object is created from this
		descriptor. The user may discard the data after the call.

	\see PxHairSystem PxHairSystemGeometry PxShape PxPhysics.createHairSystem()
		PxCooking.createHairSystem()
	*/
	class PxHairSystemDesc
	{
	public:
		/**
		\brief The number of strands in this hair system

		<b>Default:</b> 0
		*/
		PxU32 numStrands;

		/**
		\brief The length of a hair segment

		<b>Default:</b> 0.1
		*/
		PxReal segmentLength;

		/**
		\brief The radius of a hair segment

		<b>Default:</b> 0.01
		*/
		PxReal segmentRadius;

		/**
		\brief Specifies the number of vertices each strand is composed of.
			Length must be equal to numStrands, elements assumed to be of
			type PxU32. Number of segments = numVerticesPerStrand - 1.

		<b>Default:</b> NULL
		*/
		PxBoundedData numVerticesPerStrand;

		/**
		\brief Vertex positions and inverse mass [x,y,z,1/m] in PxBoundedData format.
			If count equal to numStrands, assumed to be strand root positions,
			otherwise positions of all vertices sorted by strands and increasing
			from root towards tip of strand.
			Type assumed to be of PxReal.

		<b>Default:</b> NULL
		*/
		PxBoundedData vertices;

		/**
		\brief Vertex velocities in PxBoundedData format.
			If NULL, zero velocity is assumed.
			Type assumed to be of PxReal.

		<b>Default:</b> NULL
		*/
		PxBoundedData velocities;

		/**
		\brief Flags bits, combined from values of the enum ::PxHairSystemDesc

		<b>Default:</b> 0
		*/
		PxHairSystemDescFlags flags;

		/**
		\brief Constructor with default initialization
		*/
		PX_INLINE PxHairSystemDesc();

		/**
		\brief (re)sets the structure to the default.
		*/
		PX_INLINE void setToDefault();

		/**
		\brief Check whether the descriptor is valid
		\return True if the current settings are valid
		*/
		PX_INLINE bool isValid() const;
	};

	PX_INLINE PxHairSystemDesc::PxHairSystemDesc()
	{
		numStrands = 0;
		segmentLength = 0.1f;
		segmentRadius = 0.01f;
	}

	PX_INLINE void PxHairSystemDesc::setToDefault()
	{
		*this = PxHairSystemDesc();
	}

	PX_INLINE bool PxHairSystemDesc::isValid() const
	{
		if (segmentLength < 0.0f || segmentRadius < 0.0f)
			return false;

		if (2.0f * segmentRadius >= segmentLength)
			return false;

		if (numStrands == 0)
			return false;

		if (numVerticesPerStrand.count != numStrands)
			return false;

		PxU32 totalNumVertices = 0;
		for (PxU32 i = 0; i < numVerticesPerStrand.count; i++)
		{
			const PxU32 numVertices = numVerticesPerStrand.at<PxU32>(i);
			totalNumVertices += numVertices;
			if (numVertices < 2)
			{
				return false;
			}
		}

		if (vertices.count != totalNumVertices && vertices.count != numStrands)
			return false;

		if (velocities.count != totalNumVertices && velocities.count != 0)
			return false;

		return true;
	}

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

/** @} */
#endif
