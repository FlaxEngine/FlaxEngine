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


#ifndef NV_CLOTH_EXTENSIONS_CLOTH_EDGE_QUADIFIER_H
#define NV_CLOTH_EXTENSIONS_CLOTH_EDGE_QUADIFIER_H

/** \addtogroup extensions
@{
*/

#include "ClothMeshDesc.h"
#include "NvCloth/Callbacks.h"
#include "NvCloth/Allocator.h"

namespace nv
{
namespace cloth
{

class ClothMeshQuadifier : public UserAllocated
{
public:
	virtual ~ClothMeshQuadifier(){}

	/**
	\brief Convert triangles of ClothMeshDesc to quads.
	\details In NvCloth, quad dominant mesh representations are preferable to pre-triangulated versions.
	In cases where the mesh has been already triangulated, this class provides a meachanism to
	convert (quadify) some triangles back to quad representations.
	\see ClothFabricCooker
	\param desc The cloth mesh descriptor prepared for cooking
	*/
	virtual bool quadify(const ClothMeshDesc& desc) = 0;

    /** 
	\brief Returns a mesh descriptor with some triangle pairs converted to quads.
	\note The returned descriptor is valid only within the lifespan of ClothMeshQuadifier class.
	*/
  
	virtual ClothMeshDesc getDescriptor() const = 0;

};

} // namespace cloth
} // namespace nv

NV_CLOTH_API(nv::cloth::ClothMeshQuadifier*) NvClothCreateMeshQuadifier();

/** @} */

#endif // NV_CLOTH_EXTENSIONS_CLOTH_EDGE_QUADIFIER_H
