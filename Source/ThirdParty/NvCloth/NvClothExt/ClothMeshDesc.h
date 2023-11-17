// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef NV_CLOTH_EXTENSIONS_CLOTHMESHDESC
#define NV_CLOTH_EXTENSIONS_CLOTHMESHDESC
/** \addtogroup extensions
@{
*/

#include "foundation/PxVec3.h"

namespace nv
{
namespace cloth
{

struct StridedData
{
	/**
	\brief The offset in bytes between consecutive samples in the data.

	<b>Default:</b> 0
	*/
	physx::PxU32 stride;
	const void* data;

	StridedData() : stride( 0 ), data( NULL ) {}

	template<typename TDataType>
	PX_INLINE const TDataType& at( physx::PxU32 idx ) const
	{
		physx::PxU32 theStride( stride );
		if ( theStride == 0 )
			theStride = sizeof( TDataType );
		physx::PxU32 offset( theStride * idx );
		return *(reinterpret_cast<const TDataType*>( reinterpret_cast< const physx::PxU8* >( data ) + offset ));
	}
};

struct BoundedData : public StridedData
{
	physx::PxU32 count;
	BoundedData() : count( 0 ) {}
};

/**
\brief Enum with flag values to be used in ClothMeshDesc.
*/
struct MeshFlag
{
	enum Enum
	{
		e16_BIT_INDICES		=	(1<<1)	//!< Denotes the use of 16-bit vertex indices
	};
};

/**
\brief Descriptor class for a cloth mesh.
*/
class ClothMeshDesc
{
public:

	/**
	\brief Pointer to first vertex point.
	*/
	BoundedData points;

	/**
	\brief Pointer to first stiffness value in stiffnes per vertex array. empty if unused.
	*/
	BoundedData pointsStiffness;

	/**
	\brief Determines whether particle is simulated or static.
	A positive value denotes that the particle is being simulated, zero denotes a static particle.
	This data is used to generate tether and zero stretch constraints.
	If invMasses.data is null, all particles are assumed to be simulated 
	and no tether and zero stretch constraints are being generated.
	*/
	BoundedData invMasses;

	/**
	\brief Pointer to the first triangle.

	These are triplets of 0 based indices:
	vert0 vert1 vert2
	vert0 vert1 vert2
	vert0 vert1 vert2
	...

	where vert* is either a 32 or 16 bit unsigned integer. There are a total of 3*count indices.
	The stride determines the byte offset to the next index triple.
	
	This is declared as a void pointer because it is actually either an physx::PxU16 or a physx::PxU32 pointer.
	*/
	BoundedData triangles;

	/**
	\brief Pointer to the first quad.

	These are quadruples of 0 based indices:
	vert0 vert1 vert2 vert3
	vert0 vert1 vert2 vert3
	vert0 vert1 vert2 vert3
	...

	where vert* is either a 32 or 16 bit unsigned integer. There are a total of 4*count indices.
	The stride determines the byte offset to the next index quadruple.

	This is declared as a void pointer because it is actually either an physx::PxU16 or a physx::PxU32 pointer.
	*/
	BoundedData quads;

	/**
	\brief Flags bits, combined from values of the enum ::MeshFlag
	*/
	unsigned int flags;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE ClothMeshDesc();
	/**
	\brief (re)sets the structure to the default.	
	*/
	PX_INLINE void setToDefault();
	/**
	\brief Returns true if the descriptor is valid.
	\return True if the current settings are valid
	*/
	PX_INLINE bool isValid() const;
};

PX_INLINE ClothMeshDesc::ClothMeshDesc()
{
	flags = 0;
}

PX_INLINE void ClothMeshDesc::setToDefault()
{
	*this = ClothMeshDesc();
}

PX_INLINE bool ClothMeshDesc::isValid() const
{
	if (points.count < 3) 	// at least 1 triangle
		return false;
	if ((pointsStiffness.count != points.count) && pointsStiffness.count != 0)
		return false; // either all or none of the points can have stiffness information
	if (points.count > 0xffff && flags & MeshFlag::e16_BIT_INDICES)
		return false;
	if (!points.data)
		return false;
	if (points.stride < sizeof(physx::PxVec3))	// should be at least one point
		return false;

	if (invMasses.data && invMasses.stride < sizeof(float))
		return false;
	if (invMasses.data && invMasses.count != points.count)
		return false;

	if (!triangles.count && !quads.count)	// no support for non-indexed mesh
		return false;
	if (triangles.count && !triangles.data)
		return false;
	if (quads.count && !quads.data)
		return false;

	physx::PxU32 indexSize = (flags & MeshFlag::e16_BIT_INDICES) ? sizeof(physx::PxU16) : sizeof(physx::PxU32);
	if (triangles.count && triangles.stride < indexSize*3) 
		return false; 
	if (quads.count && quads.stride < indexSize*4)
		return false;

	return true;
}

} // namespace cloth
} // namespace nv


/** @} */
#endif
