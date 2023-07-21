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

#ifndef PX_VEHICLE_TIREFRICTION_H
#define PX_VEHICLE_TIREFRICTION_H

#include "foundation/PxSimpleTypes.h"
#include "common/PxSerialFramework.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxMaterial;
class PxCollection;
class PxOutputStream;

/**
\brief Driving surface type. Each PxMaterial is associated with a corresponding PxVehicleDrivableSurfaceType.
@see PxMaterial, PxVehicleDrivableSurfaceToTireFrictionPairs
*/	
struct PX_DEPRECATED PxVehicleDrivableSurfaceType
{
	enum
	{
		eSURFACE_TYPE_UNKNOWN=0xffffffff
	};
	PxU32 mType;
};

/**
\brief Friction for each combination of driving surface type and tire type.
@see PxVehicleDrivableSurfaceType, PxVehicleTireData::mType
*/
class PX_DEPRECATED PxVehicleDrivableSurfaceToTireFrictionPairs
{
public:

	friend class VehicleSurfaceTypeHashTable;

	enum
	{
		eMAX_NB_SURFACE_TYPES=256
	};

	/**
	\brief Allocate the memory for a PxVehicleDrivableSurfaceToTireFrictionPairs instance
	that can hold data for combinations of tire type and surface type with up to maxNbTireTypes types of tire and maxNbSurfaceTypes types of surface.
	
	\param[in] maxNbTireTypes is the maximum number of allowed tire types.
	\param[in] maxNbSurfaceTypes is the maximum number of allowed surface types.  Must be less than or equal to eMAX_NB_SURFACE_TYPES
	
	\return a PxVehicleDrivableSurfaceToTireFrictionPairs instance that can be reused later with new type and friction data.

	@see setup
	*/
	static PxVehicleDrivableSurfaceToTireFrictionPairs* allocate
		(const PxU32 maxNbTireTypes, const PxU32 maxNbSurfaceTypes);

	/**
	\brief Set up a PxVehicleDrivableSurfaceToTireFrictionPairs instance for combinations of nbTireTypes tire types and nbSurfaceTypes surface types.
	
	\param[in] nbTireTypes is the number of different types of tire.  This value must be less than or equal to maxNbTireTypes specified in allocate().
	\param[in] nbSurfaceTypes is the number of different types of surface. This value must be less than or equal to maxNbSurfaceTypes specified in allocate().
	\param[in] drivableSurfaceMaterials is an array of PxMaterial pointers of length nbSurfaceTypes.
	\param[in] drivableSurfaceTypes is an array of PxVehicleDrivableSurfaceType instances of length nbSurfaceTypes.
	
	\note If the pointer to the PxMaterial that touches the tire is found in drivableSurfaceMaterials[x] then the surface type is drivableSurfaceTypes[x].mType 
	and the friction is the value that is set with setTypePairFriction(drivableSurfaceTypes[x].mType, PxVehicleTireData::mType, frictionValue).
	
	\note A friction value of 1.0 will be assigned as default to each combination of tire and surface type.  To override this use setTypePairFriction.
	@see release, setTypePairFriction, getTypePairFriction, PxVehicleTireData.mType
	*/
	void setup
		(const PxU32 nbTireTypes, const PxU32 nbSurfaceTypes, 
		 const PxMaterial** drivableSurfaceMaterials, const PxVehicleDrivableSurfaceType* drivableSurfaceTypes);

	/**
	\brief Deallocate a PxVehicleDrivableSurfaceToTireFrictionPairs instance
	*/
	void release();

	/**
	\brief Set the friction for a specified pair of tire type and drivable surface type.

	\param[in] surfaceType describes the surface type
	\param[in] tireType describes the tire type.
	\param[in] value describes the friction coefficient for the combination of surface type and tire type.
	*/
	void setTypePairFriction(const PxU32 surfaceType, const PxU32 tireType, const PxReal value);


	/**
	\brief Compute the surface type associated with a specified PxMaterial instance.
	\param[in] surfaceMaterial is the material to be queried for its associated surface type.
	\note The surface type may be used to query the friction of a surface type/tire type pair using getTypePairFriction()
	\return The surface type associated with a specified PxMaterial instance.  
	If surfaceMaterial is not referenced by the PxVehicleDrivableSurfaceToTireFrictionPairs a value of 0 will be returned.
	@see setup
	@see getTypePairFriction
	*/
	PxU32 getSurfaceType(const PxMaterial& surfaceMaterial) const;

	/**
	\brief Return the friction for a specified combination of surface type and tire type.
	\return The friction for a specified combination of surface type and tire type.
	\note The final friction value used by the tire model is the value returned by getTypePairFriction
	multiplied by the value computed from PxVehicleTireData::mFrictionVsSlipGraph
	\note The surface type is associated with a PxMaterial.  The mapping between the two may be queried using getSurfaceType().
	@see PxVehicleTireData::mFrictionVsSlipGraph
	@see getSurfaceType
	*/
	PxReal getTypePairFriction(const PxU32 surfaceType, const PxU32 tireType) const;

	/**
	\brief Return the friction for a specified combination of PxMaterial and tire type.
	\return The friction for a specified combination of PxMaterial and tire type.
	\note The final friction value used by the tire model is the value returned by getTypePairFriction
	multiplied by the value computed from PxVehicleTireData::mFrictionVsSlipGraph
	\note If surfaceMaterial is not referenced by the PxVehicleDrivableSurfaceToTireFrictionPairs 
	a surfaceType of value 0 will be assumed and the corresponding friction value will be returned.
	@see PxVehicleTireData::mFrictionVsSlipGraph
	*/
	PxReal getTypePairFriction(const PxMaterial& surfaceMaterial, const PxU32 tireType) const;

	/**
	\brief Return the maximum number of surface types
	\return The maximum number of surface types
	@see allocate
	*/
	PX_FORCE_INLINE PxU32 getMaxNbSurfaceTypes() const {return mMaxNbSurfaceTypes;}

	/**
	\brief Return the maximum number of tire types
	\return The maximum number of tire types
	@see allocate
	*/
	PX_FORCE_INLINE PxU32 getMaxNbTireTypes() const {return mMaxNbTireTypes;}

	/**
	\brief Binary serialization of a PxVehicleDrivableSurfaceToTireFrictionPairs instance.  
	The PxVehicleDrivableSurfaceToTireFrictionPairs instance is serialized to a PxOutputStream.  
	The materials referenced by the PxVehicleDrivableSurfaceToTireFrictionPairs instance are
	serialized to a PxCollection. 
	\param[in] frictionTable is the PxVehicleDrivableSurfaceToTireFrictionPairs instance to be serialized.
	\param[in] materialIds are unique ids that will be used to add the materials to the collection.
	\param[in] nbMaterialIds is the length of the materialIds array. It must be sufficient to cover all materials. 
	\param[out] collection is the PxCollection instance that is to be used to serialize the PxMaterial instances referenced by the 
	PxVehicleDrivableSurfaceToTireFrictionPairs instance.
	\param[out] stream contains the memory block for the binary serialized friction table.
	\note If a material has already been added to the collection with a PxSerialObjectId, it will not be added again.
	\note If all materials have already been added to the collection with a PxSerialObjectId, it is legal to pass a NULL ptr for the materialIds array. 
	\note frictionTable references PxMaterial instances, which are serialized using PxCollection. 
	The PxCollection instance may be used to serialize an entire scene that also references some or none of those material instances 
	or particular objects in a scene or nothing at all.  The complementary deserialize() function requires the same collection instance 
	or more typically a deserialized copy of the collection to be passed as a function argument. 
	@see deserializeFromBinary
	*/
	static void serializeToBinary(const PxVehicleDrivableSurfaceToTireFrictionPairs& frictionTable, const PxSerialObjectId* materialIds, const PxU32 nbMaterialIds, PxCollection* collection, PxOutputStream& stream);

	/**
	\brief Deserialize from a memory block to create a PxVehicleDrivableSurfaceToTireFrictionPairs instance.
	\param[in] collection contains the PxMaterial instances that will be referenced by the friction table.
	\param[in] memBlock is a binary array that may be retrieved or copied from the stream in the complementary serializeToBinary function.
	\return A PxVehicleDrivableSurfaceToTireFrictionPairs instance whose base address is equal to the memBlock ptr.
	@see serializeToBinary
	*/
	static PxVehicleDrivableSurfaceToTireFrictionPairs* deserializeFromBinary(const PxCollection& collection, void* memBlock);

private:

	/**
	\brief Ptr to base address of a 2d PxReal array with dimensions [mNbSurfaceTypes][mNbTireTypes]
	
	\note Each element of the array describes the maximum friction provided by a surface type-tire type combination.
	eg the friction corresponding to a combination of surface type x and tire type y is  mPairs[x][y]
	*/
	PxReal* mPairs;					

	/** 
	\brief Ptr to 1d array of material ptrs that is of length mNbSurfaceTypes.
	
	\note If the PxMaterial that touches the tire corresponds to mDrivableSurfaceMaterials[x] then the drivable surface
	type is mDrivableSurfaceTypes[x].mType and the friction for that contact is mPairs[mDrivableSurfaceTypes[x].mType][y], 
	assuming a tire type y.
	
	\note If the PxMaterial that touches the tire is not found in mDrivableSurfaceMaterials then the friction is 
	mPairs[0][y], assuming a tire type y.
	*/
	const PxMaterial** mDrivableSurfaceMaterials;

	/**
	\brief Ptr to 1d array of PxVehicleDrivableSurfaceType that is of length mNbSurfaceTypes.
	
	\note If the PxMaterial that touches the tire is found in mDrivableSurfaceMaterials[x] then the drivable surface
	type is mDrivableSurfaceTypes[x].mType and the friction for that contact is mPairs[mDrivableSurfaceTypes[x].mType][y], 
	assuming a tire type y.

	\note If the PxMaterial that touches the tire is not found in mDrivableSurfaceMaterials then the friction is 
	mPairs[0][y], assuming a tire type y.
	*/
	PxVehicleDrivableSurfaceType* mDrivableSurfaceTypes;

	/**
	\brief A PxSerialObjectId per surface type used internally for serialization.
	*/
	PxSerialObjectId* mMaterialSerialIds;

	/**
	\brief Number of different driving surface types.
	
	\note mDrivableSurfaceMaterials and mDrivableSurfaceTypes are both 1d arrays of length mMaxNbSurfaceTypes.
	
	\note mNbSurfaceTypes must be less than or equal to mMaxNbSurfaceTypes.
	*/	
	PxU32 mNbSurfaceTypes;			

	/**
	\brief Maximum number of different driving surface types.
	
	\note mMaxNbSurfaceTypes must be less than or equal to eMAX_NB_SURFACE_TYPES.
	*/	
	PxU32 mMaxNbSurfaceTypes;			

	/**
	\brief Number of different tire types.
	
	\note Tire types stored in PxVehicleTireData.mType
	*/	
	PxU32 mNbTireTypes;			

	/**
	\brief Maximum number of different tire types.
	
	\note Tire types stored in PxVehicleTireData.mType
	*/	
	PxU32 mMaxNbTireTypes;			

	PxVehicleDrivableSurfaceToTireFrictionPairs(){}
	~PxVehicleDrivableSurfaceToTireFrictionPairs(){}
};
PX_COMPILE_TIME_ASSERT(0==(sizeof(PxVehicleDrivableSurfaceToTireFrictionPairs) & 15));

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
