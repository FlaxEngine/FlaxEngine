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

#include "foundation/PxSimpleTypes.h"

#include "vehicle2/PxVehicleParams.h"

#if !PX_DOXYGEN
namespace physx
{

class PxRigidBody;
class PxShape;

namespace vehicle2
{
#endif

struct PxVehicleCommandState;
struct PxVehicleEngineDriveTransmissionCommandState;
struct PxVehicleEngineParams;
struct PxVehicleEngineState;
struct PxVehicleGearboxParams;
struct PxVehicleGearboxState;
struct PxVehicleRigidBodyState;
struct PxVehiclePhysXConstraints;
struct PxVehicleWheelLocalPose;
struct PxVehicleWheelParams;
struct PxVehicleWheelRigidBody1dState;
struct PxVehiclePhysXSteerState;

/**
\brief Wake up the physx actor if the actor is asleep and the commands signal an intent to 
change the state of the vehicle.
\param[in] commands are the brake, throttle and steer values that will drive the vehicle.
\param[in] transmissionCommands are the target gear and clutch values that will control
		   the transmission. If the target gear is different from the current gearbox
		   target gear, then the physx actor will get woken up. Can be set to NULL if the
		   vehicle does not have a gearbox or if this is not a desired behavior. If
		   specified, then gearParams and gearState has to be specifed too.
\param[in] gearParams The gearbox parameters. Can be set to NULL if the vehicle does
		   not have a gearbox and transmissionCommands is NULL.
\param[in] gearState The state of the gearbox. Can be set to NULL if the vehicle does
		   not have a gearbox and transmissionCommands is NULL.
\param[in] physxActor is the PxRigidBody instance associated with the vehicle.
\param[in,out] physxSteerState and commands are compared to
determine if the steering state has changed since the last call to PxVehiclePhysxActorWakeup().
\note If the steering has changed, the actor will be woken up.
\note On exit from PxVehiclePhysxActorWakeup, physxSteerState.previousSteerCommand is assigned to the value
of commands.steer so that the steer state may be propagated to the subsequent call to PxVehiclePhysxActorWakeup().
\note If physxSteerState.previousSteerCommand has value PX_VEHICLE_UNSPECIFIED_STEER_STATE, the steering state
is treated as though it has not changed.
*/
void PxVehiclePhysxActorWakeup(
 const PxVehicleCommandState& commands,
 const PxVehicleEngineDriveTransmissionCommandState* transmissionCommands,
 const PxVehicleGearboxParams* gearParams,
 const PxVehicleGearboxState* gearState,
 PxRigidBody& physxActor,
 PxVehiclePhysXSteerState& physxSteerState);

/**
\brief Check if the physx actor is sleeping and clear certain vehicle states if it is.

\param[in] axleDescription identifies the wheels on each axle.
\param[in] physxActor is the PxRigidBody instance associated with the vehicle.
\param[in] engineParams The engine parameters. Can be set to NULL if the vehicle does
		   not have an engine. Must be specified, if engineState is specified.
\param[in,out] rigidBodyState is the state of the rigid body used by the Vehicle SDK.
\param[in,out] physxConstraints The state of the suspension limit and low speed tire constraints.
			   If the vehicle actor is sleeping and constraints are active, they will be
			   deactivated and marked as dirty.
\param[in,out] wheelRigidBody1dStates describes the angular speed of the wheels.
\param[out] engineState The engine state. Can be set to NULL if the vehicle does
			not have an engine. If specified, then engineParams has to be specifed too.
			The engine rotation speed will get set to the idle rotation speed if
			the actor is sleeping.
\return True if the actor was sleeping, else false.
*/
bool PxVehiclePhysxActorSleepCheck
(const PxVehicleAxleDescription& axleDescription,
 const PxRigidBody& physxActor,
 const PxVehicleEngineParams* engineParams,
 PxVehicleRigidBodyState& rigidBodyState,
 PxVehiclePhysXConstraints& physxConstraints,
 PxVehicleArrayData<PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
 PxVehicleEngineState* engineState);

/**
\brief Check if the physx actor has to be kept awake.

Certain criteria should keep the vehicle physx actor awake, for example, if the
(mass normalized) rotational kinetic energy of the wheels is above a certain 
threshold or if a gear change is pending. This method will reset the wake
counter of the physx actor to a specified value, if any of the mentioned
criterias are met.

\note The physx actor's sleep threshold will be used as threshold to test against
      for the energy criteria.

\param[in] axleDescription identifies the wheels on each axle.
\param[in] wheelParams describes the radius, mass etc. of the wheels.
\param[in] wheelRigidBody1dStates describes the angular speed of the wheels.
\param[in] wakeCounterThreshold Once the wake counter of the physx actor falls
           below this threshold, the method will start testing if the wake
		   counter needs to be reset.
\param[in] wakeCounterResetValue The value to set the physx actor wake counter
           to, if any of the criteria to do so are met.
\param[in] gearState The gear state. Can be set to NULL if the vehicle does
	       not have gears or if the mentioned behavior is not desired.
\param[in] physxActor is the PxRigidBody instance associated with the vehicle.
*/
void PxVehiclePhysxActorKeepAwakeCheck
(const PxVehicleAxleDescription& axleDescription,
 const PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
 const PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
 const PxReal wakeCounterThreshold,
 const PxReal wakeCounterResetValue,
 const PxVehicleGearboxState* gearState,
 PxRigidBody& physxActor);


/**
\brief Read the rigid body state from a PhysX actor.
\param[in] physxActor is a reference to a PhysX actor.
\param[out] rigidBodyState is the state of the rigid body used by the Vehicle SDK.
*/
void PxVehicleReadRigidBodyStateFromPhysXActor
(const PxRigidBody& physxActor,
 PxVehicleRigidBodyState& rigidBodyState);

/**
\brief Update the local pose of a PxShape that is associated with a wheel.
\param[in] wheelLocalPose describes the local pose of each wheel in the rigid body frame.
\param[in] wheelShapeLocalPose describes the local pose to apply to the PxShape instance in the wheel's frame. 
\param[in] shape is the target PxShape.
*/
void PxVehicleWriteWheelLocalPoseToPhysXWheelShape
(const PxTransform& wheelLocalPose, const PxTransform& wheelShapeLocalPose, PxShape* shape);

/**
\brief Write the rigid body state to a PhysX actor.
\param[in] physxActorUpdateMode controls whether the PhysX actor is to be updated with 
instantaneous velocity changes or with accumulated accelerations to be applied in 
the next simulation step of the associated PxScene.
\param[in] rigidBodyState is the state of the rigid body.
\param[in] dt is the simulation time that has elapsed since the last call to 
PxVehicleWriteRigidBodyStateToPhysXActor().
\param[out] physXActor is a reference to the PhysX actor.
*/
void PxVehicleWriteRigidBodyStateToPhysXActor
(const PxVehiclePhysXActorUpdateMode::Enum physxActorUpdateMode,
 const PxVehicleRigidBodyState& rigidBodyState,
 const PxReal dt,
 PxRigidBody& physXActor);



#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
