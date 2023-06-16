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

#include "foundation/PxPreprocessor.h"

/** \addtogroup vehicle2
  @{
*/

#if !PX_DOXYGEN
namespace physx
{

class PxPhysics;
class PxRigidBody;

namespace vehicle2
{
#endif

struct PxVehicleAxleDescription;
struct PxVehiclePhysXConstraints;

/**
\brief Instantiate the PhysX custom constraints.

Custom constraints will resolve excess suspension compression and velocity constraints that serve as
a replacement low speed tire model.

\param[in] axleDescription describes the axles of the vehicle and the wheels on each axle.
\param[in] physics is a PxPhysics instance.
\param[in] physxActor is the vehicle's PhysX representation as a PxRigidBody
\param[in] vehicleConstraints is a wrapper class that holds pointers to PhysX objects required to implement the custom constraint.
*/
void PxVehicleConstraintsCreate
(const PxVehicleAxleDescription& axleDescription,
 PxPhysics& physics, PxRigidBody& physxActor,
 PxVehiclePhysXConstraints& vehicleConstraints);

/**
\brief To ensure the constraints are processed by the PhysX scene they are marked as dirty prior to each simulate step.

\param[in] vehicleConstraints is a wrapper class that holds pointers to PhysX objects required to implement the custom constraint.

@see PxVehicleConstraintsCreate
*/
void PxVehicleConstraintsDirtyStateUpdate
(PxVehiclePhysXConstraints& vehicleConstraints);

/**
\brief Destroy the PhysX custom constraints.

\param[in,out] vehicleConstraints describes the PhysX custom constraints to be released.

@see PxVehicleConstraintsCreate
*/
void PxVehicleConstraintsDestroy
(PxVehiclePhysXConstraints& vehicleConstraints);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
