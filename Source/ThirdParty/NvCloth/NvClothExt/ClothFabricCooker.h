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


#ifndef NV_CLOTH_EXTENSIONS_CLOTH_FABRIC_COOKER_H
#define NV_CLOTH_EXTENSIONS_CLOTH_FABRIC_COOKER_H

/** \addtogroup extensions
  @{
*/

#include "ClothMeshDesc.h"
#include "NvCloth/Fabric.h"
#include "NvCloth/Factory.h"

namespace nv
{
namespace cloth
{

struct CookedData
{
	uint32_t mNumParticles;
	Range<const uint32_t> mPhaseIndices;
	Range<const int32_t> mPhaseTypes;
	Range<const uint32_t> mSets;
	Range<const float> mRestvalues;
	Range<const float> mStiffnessValues;
	Range<const uint32_t> mIndices;
	Range<const uint32_t> mAnchors;
	Range<const float> mTetherLengths;
	Range<const uint32_t> mTriangles;
};

/**
\brief Describe type of phase in cloth fabric.
\see Fabric for an explanation of concepts on phase and set.
*/
struct ClothFabricPhaseType
{
	enum Enum
	{
		eINVALID,     //!< invalid type 
		eVERTICAL,    //!< resists stretching or compression, usually along the gravity
		eHORIZONTAL,  //!< resists stretching or compression, perpendicular to the gravity
		eBENDING,     //!< resists out-of-plane bending in angle-based formulation
		eSHEARING,    //!< resists in-plane shearing along (typically) diagonal edges,
        eCOUNT        // internal use only
	};
};

/**
\brief References a set of constraints that can be solved in parallel.
\see Fabric for an explanation of the concepts on phase and set.
*/
struct ClothFabricPhase
{
	ClothFabricPhase(ClothFabricPhaseType::Enum type = 
		ClothFabricPhaseType::eINVALID, physx::PxU32 index = 0);

	/**
	\brief Type of constraints to solve.
	*/
	ClothFabricPhaseType::Enum phaseType;

	/**
	\brief Index of the set that contains the particle indices.
	*/
	physx::PxU32 setIndex;
};

PX_INLINE ClothFabricPhase::ClothFabricPhase(
	ClothFabricPhaseType::Enum type, physx::PxU32 index)
	: phaseType(type)
	, setIndex(index)
{}

/**
\brief References all the data required to create a fabric.
\see ClothFabricCooker.getDescriptor()
*/
class ClothFabricDesc
{
public:
	/** \brief The number of particles needed when creating a PxCloth instance from the fabric. */
	physx::PxU32 nbParticles;

	/** \brief The number of solver phases. */
	physx::PxU32 nbPhases;
	/** \brief Array defining which constraints to solve each phase. See #Fabric.getPhases(). */
	const ClothFabricPhase* phases;

	/** \brief The number of sets in the fabric. */
	physx::PxU32 nbSets;
	/** \brief Array with an index per set which points one entry beyond the last constraint of the set. See #Fabric.getSets(). */
	const physx::PxU32* sets;

	/** \brief Array of particle indices which specifies the pair of constrained vertices. See #Fabric.getParticleIndices(). */
	const physx::PxU32* indices;
	/** \brief Array of rest values for each constraint. See #Fabric.getRestvalues(). */
	const physx::PxReal* restvalues;

	/** \brief Size of tetherAnchors and tetherLengths arrays, needs to be multiple of nbParticles. */
	physx::PxU32 nbTethers;
	/** \brief Array of particle indices specifying the tether anchors. See #Fabric.getTetherAnchors(). */
	const physx::PxU32* tetherAnchors;
	/** \brief Array of rest distance between tethered particle pairs. See #Fabric.getTetherLengths(). */
	const physx::PxReal* tetherLengths;

	physx::PxU32 nbTriangles;
	const physx::PxU32* triangles;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE ClothFabricDesc();

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

PX_INLINE ClothFabricDesc::ClothFabricDesc()
{
	setToDefault();
}

PX_INLINE void ClothFabricDesc::setToDefault()
{
	memset(this, 0, sizeof(ClothFabricDesc));
}

PX_INLINE bool ClothFabricDesc::isValid() const
{
	return nbParticles && nbPhases && phases && restvalues && nbSets 
		&& sets && indices && (!nbTethers || (tetherAnchors && tetherLengths))
		&& (!nbTriangles || triangles);
}

///Use NvClothCreateFabricCooker() to create an implemented instance
class NV_CLOTH_IMPORT ClothFabricCooker : public UserAllocated
{
public:
	virtual ~ClothFabricCooker(){}

	/**
	\brief Cooks a triangle mesh to a ClothFabricDesc.
	\param desc The cloth mesh descriptor on which the generation of the cooked mesh depends.
	\param gravity A normalized vector which specifies the direction of gravity. 
	This information allows the cooker to generate a fabric with higher quality simulation behavior.
	The gravity vector should point in the direction gravity will be pulling towards in the most common situation/at rest.
	e.g. For flags it might be beneficial to set the gravity horizontal if they are cooked in landscape orientation, as a flag will hang in portrait orientation at rest.
	\param useGeodesicTether A flag to indicate whether to compute geodesic distance for tether constraints.
	\note The geodesic option for tether only works for manifold input.  For non-manifold input, a simple Euclidean distance will be used.
	For more detailed cooker status for such cases, try running ClothGeodesicTetherCooker directly.
	*/
	virtual bool cook(const ClothMeshDesc& desc, physx::PxVec3 gravity, bool useGeodesicTether = true) = 0;

	/** \brief Returns fabric cooked data for creating fabrics. */
	virtual CookedData getCookedData() const = 0;

	/** \brief Returns the fabric descriptor to create the fabric. */
	virtual ClothFabricDesc getDescriptor() const = 0;

	/** \brief Saves the fabric data to a platform and version dependent stream. */
	virtual void save(physx::PxOutputStream& stream, bool platformMismatch) const = 0;
};

/** @} */

} // namespace cloth
} // namespace nv


NV_CLOTH_API(nv::cloth::ClothFabricCooker*) NvClothCreateFabricCooker();

/**
\brief Cooks a triangle mesh to a Fabric.

\param factory The factory for which the cloth is cooked.
\param desc The cloth mesh descriptor on which the generation of the cooked mesh depends.
\param gravity A normalized vector which specifies the direction of gravity. 
This information allows the cooker to generate a fabric with higher quality simulation behavior.
\param phaseTypes Optional array where phase type information can be writen to.
\param useGeodesicTether A flag to indicate whether to compute geodesic distance for tether constraints.
\return The created cloth fabric, or NULL if creation failed.
*/
NV_CLOTH_API(nv::cloth::Fabric*) NvClothCookFabricFromMesh(nv::cloth::Factory* factory,
	const nv::cloth::ClothMeshDesc& desc, const float gravity[3],
	nv::cloth::Vector<int32_t>::Type* phaseTypes = nullptr, bool useGeodesicTether = true);

#endif // NV_CLOTH_EXTENSIONS_CLOTH_FABRIC_COOKER_H
