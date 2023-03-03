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

#pragma once

/** \addtogroup vehicle2
  @{
*/

#include "PxFiltering.h"
#include "PxShape.h"

#if !PX_DOXYGEN
namespace physx
{

class PxGeometry;
class PxMaterial;
struct PxCookingParams;

namespace vehicle2
{
#endif

struct PxVehicleRigidBodyParams;
struct PxVehicleAxleDescription;
struct PxVehicleWheelParams;
struct PxVehiclePhysXActor;
struct PxVehicleFrame;
struct PxVehicleSuspensionParams;

class PxVehiclePhysXRigidActorParams
{
	PX_NOCOPY(PxVehiclePhysXRigidActorParams)

public:

	PxVehiclePhysXRigidActorParams(const PxVehicleRigidBodyParams& _physxActorRigidBodyParams, const char* _physxActorName)
		: rigidBodyParams(_physxActorRigidBodyParams),
		  physxActorName(_physxActorName)
	{
	}

	const PxVehicleRigidBodyParams& rigidBodyParams;
	const char* physxActorName;
};

class PxVehiclePhysXRigidActorShapeParams
{
	PX_NOCOPY(PxVehiclePhysXRigidActorShapeParams)

public:

	PxVehiclePhysXRigidActorShapeParams
	(const PxGeometry& _geometry, const PxTransform& _localPose, const PxMaterial& _material, 
	 const PxShapeFlags _flags, const PxFilterData& _simulationFilterData, const PxFilterData& _queryFilterData)
		: geometry(_geometry),
		  localPose(_localPose),
		  material(_material),
		  flags(_flags),
		  simulationFilterData(_simulationFilterData),
		  queryFilterData(_queryFilterData)
	{
	}

	const PxGeometry& geometry;
	const PxTransform& localPose;
	const PxMaterial& material;
	PxShapeFlags flags;
	PxFilterData simulationFilterData;
	PxFilterData queryFilterData;
};

class PxVehiclePhysXWheelParams
{
	PX_NOCOPY(PxVehiclePhysXWheelParams)

public:

	PxVehiclePhysXWheelParams(const PxVehicleAxleDescription& _axleDescription, const PxVehicleWheelParams* _wheelParams)
		: axleDescription(_axleDescription),
		  wheelParams(_wheelParams)
	{
	}

	const PxVehicleAxleDescription& axleDescription;
	const PxVehicleWheelParams* wheelParams;
};

class PxVehiclePhysXWheelShapeParams
{
	PX_NOCOPY(PxVehiclePhysXWheelShapeParams)

public:

	PxVehiclePhysXWheelShapeParams(const PxMaterial& _material, const PxShapeFlags _flags, const PxFilterData _simulationFilterData, const PxFilterData _queryFilterData)
		: material(_material),
		  flags(_flags),
		  simulationFilterData(_simulationFilterData),
		  queryFilterData(_queryFilterData)
	{
	}

	const PxMaterial& material;
	PxShapeFlags flags;
	PxFilterData simulationFilterData;
	PxFilterData queryFilterData;
};

/**
\brief Create a PxRigidDynamic instance, instantiate it with desired properties and populate it with PxShape instances.
\param[in] vehicleFrame describes the frame of the vehicle.
\param[in] rigidActorParams describes the mass and moment of inertia of the rigid body.
\param[in] rigidActorCmassLocalPose specifies the mapping between actor and rigid body frame.
\param[in] rigidActorShapeParams describes the collision geometry associated with the rigid body.
\param[in] wheelParams describes the radius and half-width of the wheels.
\param[in] wheelShapeParams describes the PxMaterial and PxShapeFlags to apply to the wheel shapes.
\param[in] physics is a PxPhysics instance.
\param[in] params is a PxCookingParams instance
\param[in] vehiclePhysXActor is a record of the PxRigidDynamic and PxShape instances instantiated.
\note This is an alternative to PxVehiclePhysXArticulationLinkCreate.
\note PxVehiclePhysXActorCreate primarily serves as an illustration of the instantiation of the PhysX class instances
required to simulate a vehicle with a PxRigidDynamic.
@see PxVehiclePhysXActorDestroy
*/
void PxVehiclePhysXActorCreate
(const PxVehicleFrame& vehicleFrame,
 const PxVehiclePhysXRigidActorParams& rigidActorParams, const PxTransform& rigidActorCmassLocalPose,
 const PxVehiclePhysXRigidActorShapeParams& rigidActorShapeParams,
 const PxVehiclePhysXWheelParams& wheelParams, const PxVehiclePhysXWheelShapeParams& wheelShapeParams,
 PxPhysics& physics, const PxCookingParams& params,
 PxVehiclePhysXActor& vehiclePhysXActor);


/**
\brief Configure an actor so that it is ready for vehicle simulation.
\param[in] rigidActorParams describes the mass and moment of inertia of the rigid body.
\param[in] rigidActorCmassLocalPose specifies the mapping between actor and rigid body frame.
\param[out] rigidBody is the body to be prepared for simulation.
*/
void PxVehiclePhysXActorConfigure
(const PxVehiclePhysXRigidActorParams& rigidActorParams, const PxTransform& rigidActorCmassLocalPose,
 PxRigidBody& rigidBody);

/**
\brief Create a PxArticulationReducedCoordinate and a single PxArticulationLink, 
instantiate the PxArticulationLink with desired properties and populate it with PxShape instances.
\param[in] vehicleFrame describes the frame of the vehicle.
\param[in] rigidActorParams describes the mass and moment of inertia of the rigid body.
\param[in] rigidActorCmassLocalPose specifies the mapping between actor and rigid body frame.
\param[in] rigidActorShapeParams describes the collision geometry associated with the rigid body.
\param[in] wheelParams describes the radius and half-width of the wheels.
\param[in] wheelShapeParams describes the PxMaterial and PxShapeFlags to apply to the wheel shapes.
\param[in] physics is a PxPhysics instance.
\param[in] params is a PxCookingParams instance
\param[in] vehiclePhysXActor is a record of the PxArticulationReducedCoordinate, PxArticulationLink and PxShape instances instantiated.
\note This is an alternative to PxVehiclePhysXActorCreate.  
\note PxVehiclePhysXArticulationLinkCreate primarily serves as an illustration of the instantiation of the PhysX class instances 
required to simulate a vehicle as part of an articulated ensemble.
@see PxVehiclePhysXActorDestroy
*/
void PxVehiclePhysXArticulationLinkCreate
(const PxVehicleFrame& vehicleFrame,
 const PxVehiclePhysXRigidActorParams& rigidActorParams, const PxTransform& rigidActorCmassLocalPose,
 const PxVehiclePhysXRigidActorShapeParams& rigidActorShapeParams,
 const PxVehiclePhysXWheelParams& wheelParams, const PxVehiclePhysXWheelShapeParams& wheelShapeParams,
 PxPhysics& physics, const PxCookingParams& params,
 PxVehiclePhysXActor& vehiclePhysXActor);

/**
\brief Release the PxRigidDynamic, PxArticulationReducedCoordinate, PxArticulationLink and PxShape instances 
instantiated by PxVehiclePhysXActorCreate or PxVehiclePhysXArticulationLinkCreate.
\param[in] vehiclePhysXActor is a description of the PhysX instances to be released.
*/
void PxVehiclePhysXActorDestroy(PxVehiclePhysXActor& vehiclePhysXActor);



#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
