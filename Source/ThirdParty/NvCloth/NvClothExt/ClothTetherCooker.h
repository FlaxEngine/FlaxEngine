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


#ifndef NV_CLOTH_EXTENSIONS_CLOTH_TETHER_COOKER_H
#define NV_CLOTH_EXTENSIONS_CLOTH_TETHER_COOKER_H

/** \addtogroup extensions
@{
*/

#include "ClothMeshDesc.h"
#include "NvCloth/Allocator.h"

namespace nv
{
namespace cloth
{

class ClothTetherCooker : public UserAllocated
{
public:
	virtual ~ClothTetherCooker(){}

	/**
	\brief Compute tether data from ClothMeshDesc with simple distance measure.
	\details The tether constraint in NvCloth requires rest distance and anchor index to be precomputed during cooking time.
	This cooker computes a simple Euclidean distance to closest anchor point.
	The Euclidean distance measure works reasonably for flat cloth and flags and computation time is very fast.
	With this cooker, there is only one tether anchor point per particle.
	\see ClothTetherGeodesicCooker for more accurate distance estimation.
	\param desc The cloth mesh descriptor prepared for cooking
	*/
	virtual bool cook(const ClothMeshDesc &desc) = 0;

	/**
	\brief Returns cooker status
	\details This function returns cooker status after cooker computation is done.
	A non-zero return value indicates a failure.
	*/
	virtual uint32_t getCookerStatus() const = 0; //From APEX

	/**
	\brief Returns number of tether anchors per particle
	\note Returned number indicates the maximum anchors.  
	If some particles are assigned fewer anchors, the anchor indices will be physx::PxU32(-1) 
	\note If there is no attached point in the input mesh descriptor, this will return 0 and no tether data will be generated.
	*/
	virtual physx::PxU32 getNbTethersPerParticle() const = 0;

    /** 
	\brief Returns computed tether data.
	\details This function returns anchor indices for each particle as well as desired distance between the tether anchor and the particle.
	The user buffers should be at least as large as number of particles.
	*/
    virtual void getTetherData(physx::PxU32* userTetherAnchors, physx::PxReal* userTetherLengths) const = 0;

};

} // namespace cloth
} // namespace nv

NV_CLOTH_API(nv::cloth::ClothTetherCooker*) NvClothCreateSimpleTetherCooker();
NV_CLOTH_API(nv::cloth::ClothTetherCooker*) NvClothCreateGeodesicTetherCooker();

/** @} */

#endif // NV_CLOTH_EXTENSIONS_CLOTH_TETHER_COOKER_H
