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

#include "foundation/PxVec3.h"
#include "foundation/PxSimpleTypes.h"
#include "vehicle2/PxVehicleParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleWheelParams;
struct PxVehicleSuspensionParams;
struct PxVehicleSuspensionStateCalculationParams;
struct PxVehicleRoadGeometryState;
struct PxVehicleRigidBodyState;
struct PxVehicleSuspensionState;
struct PxVehicleSuspensionComplianceParams;
struct PxVehicleSuspensionComplianceState;
struct PxVehicleSuspensionForceParams;
struct PxVehicleSuspensionForce;
struct PxVehicleSuspensionForceLegacyParams;
struct PxVehicleAntiRollForceParams;
struct PxVehicleAntiRollTorque;

/**
\brief Compute the suspension compression and compression speed for a single suspension.
\param[in] wheelParams is a description of the radius and half-width of the wheel on the suspension.
\param[in] suspensionParams is a description of the suspension and wheel frames.
\param[in] suspensionStateCalcParams specifies whether to compute the suspension compression by either
raycasting or sweeping against the plane of the road geometry under the wheel.
\param[in] suspensionStiffness is the stiffness of the suspension
\param[in] suspensionDamping is the damping rate of the suspension.
or whether to apply a limit to the expansion speed so that the wheel may not reach the ground. 
\param[in] steerAngle is the yaw angle (in radians) of the wheel.
\param[in] roadGeometryState describes the plane under the wheel.
\param[in] rigidBodyState describes the pose of the rigid body.
\param[in] dt is the simulation time that has lapsed since the last call to PxVehicleSuspensionStateUpdate
\param[in] frame describes the longitudinal, lateral and vertical axes of the vehicle.
\param[in] gravity is the gravitational acceleration that acts on the suspension sprung mass.
\param[in,out] suspState is the compression (jounce) and compression speed of the suspension.
*/
void PxVehicleSuspensionStateUpdate
(const PxVehicleWheelParams& wheelParams, const PxVehicleSuspensionParams& suspensionParams, const PxVehicleSuspensionStateCalculationParams& suspensionStateCalcParams,
 const PxReal suspensionStiffness, const PxReal suspensionDamping,
 const PxReal steerAngle, const PxVehicleRoadGeometryState& roadGeometryState, const PxVehicleRigidBodyState& rigidBodyState,
 const PxReal dt, const PxVehicleFrame& frame, const PxVec3& gravity,
 PxVehicleSuspensionState& suspState);

/**
\brief Compute the toe, camber and force application points that are affected by suspension compression.
\param[in] suspensionParams is a description of the suspension and wheel frames.
\param[in] complianceParams describes how toe, camber and force application points are affected by suspension compression.
\param[in] suspensionState describes the current suspension compression.
\param[in] complianceState is the computed toe, camber and force application points.
*/
void PxVehicleSuspensionComplianceUpdate
(const PxVehicleSuspensionParams& suspensionParams,
 const PxVehicleSuspensionComplianceParams& complianceParams,
 const PxVehicleSuspensionState& suspensionState,
 PxVehicleSuspensionComplianceState& complianceState);

/**
\brief Compute the suspension force and torque arising from suspension compression and speed.
\param[in] suspensionParams is a description of the suspension and wheel frames.
\param[in] suspensionForceParams describes the conversion of suspension state to suspension force.
\param[in] roadGeometryState describes the plane under the wheel of the suspension.
\param[in] suspensionState is the current compression state of the suspension.
\param[in] complianceState is the current compliance state of the suspension.
\param[in] rigidBodyState describes the current pose of the rigid body.
\param[in] gravity is the gravitational acceleration.
\param[in] vehicleMass is the rigid body mass.
\param[out] suspensionForce is the force and torque to apply to the rigid body arising from the suspension state.
*/
void PxVehicleSuspensionForceUpdate
(const PxVehicleSuspensionParams& suspensionParams,
 const PxVehicleSuspensionForceParams& suspensionForceParams,
 const PxVehicleRoadGeometryState& roadGeometryState, const PxVehicleSuspensionState& suspensionState,
 const PxVehicleSuspensionComplianceState& complianceState, const PxVehicleRigidBodyState& rigidBodyState,
 const PxVec3& gravity, const PxReal vehicleMass,
 PxVehicleSuspensionForce& suspensionForce);


/**
\brief Compute the suspension force and torque arising from suspension compression and speed.
\param[in] suspensionParams is a description of the suspension and wheel frames.
\param[in] suspensionForceParams describes the conversion of suspension state to suspension force.
\param[in] roadGeometryState describes the plane under the wheel of the suspension.
\param[in] suspensionState is the current compression state of the suspension.
\param[in] complianceState is the current compliance state of the suspension.
\param[in] rigidBodyState describes the current pose of the rigid body.
\param[in] gravity is the gravitational acceleration.
\param[out] suspensionForce is the force and torque to apply to the rigid body arising from the suspension state.
\note PxVehicleSuspensionLegacyForceUpdate implements the legacy force computation of PhysX 5.0 and earlier. 
@deprecated
*/
PX_DEPRECATED void PxVehicleSuspensionLegacyForceUpdate
(const PxVehicleSuspensionParams& suspensionParams,
 const PxVehicleSuspensionForceLegacyParams& suspensionForceParams,
 const PxVehicleRoadGeometryState& roadGeometryState, const PxVehicleSuspensionState& suspensionState,
 const PxVehicleSuspensionComplianceState& complianceState, const PxVehicleRigidBodyState& rigidBodyState,
 const PxVec3& gravity,
 PxVehicleSuspensionForce& suspensionForce);

/**
\brief Compute the accumulated anti-roll torque to apply to the vehicle's rigid body.
\param[in] suspensionParams The suspension parameters for each wheel.
\param[in] antiRollParams describes the wheel pairs connected by anti-roll bars and the strength of each anti-roll bar.
\param[in] suspensionStates The suspension states for each wheel.
\param[in] complianceStates The suspension compliance states for each wheel.
\param[in] rigidBodyState describes the pose and momentum of the vehicle's rigid body in the world frame.
\param[in] antiRollTorque is the accumulated anti-roll torque that is computed by iterating over all anti-roll bars 
describes in *antiRollParams*.
\note suspensionParams must contain an entry for each wheel index referenced by *antiRollParams*.
\note suspensionStates must contain an entry for each wheel index referenced by *antiRollParams*.
\note complianceStates must contain an entry for each wheel index referenced by *antiRollParams*.
\note antiRollTorque is expressed in the world frame.
*/
void PxVehicleAntiRollForceUpdate
(const PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
 const PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams>& antiRollParams,
 const PxVehicleArrayData<const PxVehicleSuspensionState>& suspensionStates,
 const PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& complianceStates,
 const PxVehicleRigidBodyState& rigidBodyState,
 PxVehicleAntiRollTorque& antiRollTorque);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
