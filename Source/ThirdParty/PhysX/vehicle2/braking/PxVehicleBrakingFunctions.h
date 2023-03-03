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
#include "foundation/PxMath.h"
#include "PxVehicleBrakingParams.h"
#include "../commands/PxVehicleCommandStates.h"
#include "../commands/PxVehicleCommandHelpers.h"
#include "../drivetrain/PxVehicleDrivetrainParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Compute the brake torque response to an array of brake commands.
\param[in] brakeCommands is the array of input brake commands to be applied to the vehicle.
\param[in] nbBrakeCommands is the number of input brake commands to be applied to the vehicle.
\param[in] longitudinalSpeed is the longitudinal speed of the vehicle.
\param[in] wheelId specifies the wheel that is to have its brake response computed.
\param[in] brakeResponseParams specifies the per wheel brake torque response to each brake command as a nonlinear function of brake command and longitudinal speed.
\param[out] brakeResponseState is the brake torque response to the input brake command. 
\note commands.brakes[i] and brakeResponseParams[i] are treated as pairs of brake command and brake command response.
*/
PX_FORCE_INLINE void PxVehicleBrakeCommandResponseUpdate
(const PxReal* brakeCommands, const PxU32 nbBrakeCommands, const PxReal longitudinalSpeed,
 const PxU32 wheelId, const PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams>& brakeResponseParams,
 PxReal& brakeResponseState)
{
	PX_CHECK_AND_RETURN(nbBrakeCommands <= brakeResponseParams.size, "PxVehicleBrakeCommandLinearUpdate: nbBrakes must be less than or equal to brakeResponseParams.size");
	PxReal sum = 0.0f;
	for (PxU32 i = 0; i < nbBrakeCommands; i++)
	{
		sum += PxVehicleNonLinearResponseCompute(brakeCommands[i], longitudinalSpeed, wheelId, brakeResponseParams[i]);
	}
	brakeResponseState = sum;
}


#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
