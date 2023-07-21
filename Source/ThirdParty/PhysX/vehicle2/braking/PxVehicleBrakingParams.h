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

#include "foundation/PxFoundation.h"

#include "vehicle2/PxVehicleParams.h"

#include "vehicle2/commands/PxVehicleCommandParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Distribute a brake response to the wheels of a vehicle.
\note The brake torque of each wheel on the ith wheel is brakeCommand * maxResponse * wheelResponseMultipliers[i].
\note A typical use case is to set maxResponse to be the vehicle's maximum achievable brake torque
that occurs when the brake command is equal to 1.0.  The array wheelResponseMultipliers[i] would then be used
to specify the maximum achievable brake torque per wheel as a fractional multiplier of the vehicle's maximum achievable brake torque.
*/
struct PxVehicleBrakeCommandResponseParams : public PxVehicleCommandResponseParams
{
	PX_FORCE_INLINE PxVehicleBrakeCommandResponseParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PxVehicleBrakeCommandResponseParams r = *this;
		const PxReal scale = trgScale.scale/srcScale.scale;
		r.maxResponse *= (scale*scale);		//maxResponse is a torque!
		return r;
	}

	PX_FORCE_INLINE bool isValid(const PxVehicleAxleDescription& axleDesc) const
	{
		if (!axleDesc.isValid())
			return false;
		PX_CHECK_AND_RETURN_VAL(maxResponse >= 0.0f, "PxVehicleBrakeCommandResponseParams.maxResponse must be greater than or equal to zero", false);
		for (PxU32 i = 0; i < axleDesc.nbWheels; i++)
		{
			PX_CHECK_AND_RETURN_VAL(wheelResponseMultipliers[axleDesc.wheelIdsInAxleOrder[i]] >= 0.0f, "PxVehicleBrakeCommandResponseParams.wheelResponseMultipliers[i] must be greater than or equal to zero.", false);
		}
		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
