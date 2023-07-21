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

#include "foundation/PxPreprocessor.h"
#include "foundation/PxSimpleTypes.h"

#include "vehicle2/PxVehicleParams.h"
#include "PxVehicleSteeringParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleCommandState;

/**
\brief Compute the yaw angle response to a steer command.
\param[in] steer is the input steer command value.
\param[in] longitudinalSpeed is the longitudinal speed of the vehicle.
\param[in] wheelId specifies the wheel to have its steer response computed.
\param[in] steerResponseParmas specifies the per wheel yaw angle response to the steer command as a nonlinear function of steer command and longitudinal speed.
\param[out] steerResponseState is the yaw angle response to the input steer command.
*/
void PxVehicleSteerCommandResponseUpdate
(const PxReal steer, const PxReal longitudinalSpeed,
 const PxU32 wheelId, const PxVehicleSteerCommandResponseParams& steerResponseParmas,
 PxReal& steerResponseState);

/**
\brief Account for Ackermann correction by modifying the per wheel steer response multipliers to engineer an asymmetric steer response across axles.
\param[in] steer is the input steer command value.
\param[in] steerResponseParmas describes the maximum response and a response multiplier per axle.
\param[in] ackermannParams is an array that describes the wheels affected by Ackerman steer correction.
\param[in,out] steerResponseStates contains the corrected per wheel steer response multipliers that take account of Ackermann steer correction.
*/
void  PxVehicleAckermannSteerUpdate
(const PxReal steer, const PxVehicleSteerCommandResponseParams& steerResponseParmas,
 const PxVehicleSizedArrayData<const PxVehicleAckermannParams>& ackermannParams,
 PxVehicleArrayData<PxReal>& steerResponseStates);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
