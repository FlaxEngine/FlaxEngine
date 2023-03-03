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
#include "PxVehicleCommandParams.h"
#include "PxVehicleCommandStates.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Compute the linear response to a command.
\param[in] command is a normalised command value.
\param[in] wheelId specifies the wheel that is to respond to the command.
\param[in] responseParams specifies the wheel responses for all wheels on a vehicle.
\return The linear response of the specified  wheel to the command.
*/
PX_FORCE_INLINE PxReal PxVehicleLinearResponseCompute
(const PxReal command, const PxU32 wheelId, const PxVehicleCommandResponseParams& responseParams)
{
	return command*responseParams.maxResponse*responseParams.wheelResponseMultipliers[wheelId];
}

/**
\brief Compute the non-linear response to a command.
\param[in] command is a normalised command value.
\param[in] longitudinalSpeed is the longitudional speed of the vehicle. 
\param[in] wheelId specifies the wheel that is to respond to the command.
\param[in] responseParams specifies the wheel responses for all wheels on a vehicle.
\note responseParams is used to compute an interpolated normalized response to the combination of command and longitudinalSpeed. 
The interpolated normalized response is then used in place of the command as input to PxVehicleComputeLinearResponse().
*/
PxReal PxVehicleNonLinearResponseCompute
(const PxReal command, const PxReal longitudinalSpeed, const PxU32 wheelId, const PxVehicleCommandResponseParams& responseParams);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
