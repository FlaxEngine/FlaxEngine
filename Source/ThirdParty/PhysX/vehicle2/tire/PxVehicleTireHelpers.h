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

#include "vehicle2/PxVehicleParams.h"
#include "vehicle2/wheel/PxVehicleWheelStates.h"
#include "PxVehicleTireStates.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Compute the intention to accelerate by inspecting the actuation states of the wheels of a powered vehicle.
\param[in] poweredVehicleAxleDesc describes the axles and wheels of a powered vehicle in a jointed ensemble of vehicles.
\param[in] poweredVehicleActuationStates describes the drive state of each wheel of the powered vehicle.
@see PxVehicleTireStickyStateReset
*/
bool PxVehicleAccelerationIntentCompute
(const PxVehicleAxleDescription& poweredVehicleAxleDesc, const PxVehicleArrayData<const PxVehicleWheelActuationState>& poweredVehicleActuationStates);

/**
\brief Reset the sticky tire states of an unpowered vehicle if it is in a jointed ensemble of vehicles with at least one powered vehicle. 
\param[in] poweredVehicleIntentionToAccelerate describes the state of the powered vehicle in an ensemble of jointed vehicles.
\param[in] unpoweredVehicleAxleDesc describes the axles and wheels of an unpowered vehicle towed by a powered vehicle.
\param[out] unpoweredVehicleStickyState is the sticky state of the wheels of an unpowered vehicle towed by a powered vehicle. 
\note If any wheel on the powered vehicle is to receive a drive torque, the sticky tire states of the towed vehicle will be reset to the deactivated state.
\note poweredVehicleIntentionToAccelerate may be computed using PxVehicleAccelerationIntentCompute().
@see PxVehicleAccelerationIntentCompute
*/
void PxVehicleTireStickyStateReset
(const bool poweredVehicleIntentionToAccelerate,
 const PxVehicleAxleDescription& unpoweredVehicleAxleDesc,
 PxVehicleArrayData<PxVehicleTireStickyState>& unpoweredVehicleStickyState);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
