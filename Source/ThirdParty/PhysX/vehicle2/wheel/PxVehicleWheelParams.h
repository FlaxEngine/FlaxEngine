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

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleWheelParams
{
	/**
	\brief Radius of unit that includes metal wheel plus rubber tire.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> length
	*/
	PxReal radius;

	/**
	\brief Half-width of unit that includes wheel plus tire.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> length
	*/
	PxReal halfWidth;

	/**
	\brief Mass of unit that includes wheel plus tire.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> mass
	*/
	PxReal mass;

	/**
	\brief Moment of inertia of unit that includes wheel plus tire about the rolling axis.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> mass * (length^2)
	*/
	PxReal moi;

	/**
	\brief Damping rate applied to wheel.

	<b>Range:</b> [0, inf)<br>
	<b>Unit:</b> torque * time = mass * (length^2) / time
	*/
	PxReal dampingRate;

	PX_FORCE_INLINE PxVehicleWheelParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PxVehicleWheelParams r = *this;
		const PxReal scale = trgScale.scale/srcScale.scale;
		r.radius *= scale;
		r.halfWidth *= scale;
		r.moi *= (scale*scale);
		r.dampingRate *= (scale*scale);
		return r;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(radius > 0.0f, "PxVehicleWheelParams.radius must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(halfWidth > 0.0f, "PxVehicleWheelParams.halfWidth must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(mass > 0.0f, "PxVehicleWheelParams.mass must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(moi > 0.0f, "PxVehicleWheelParams.moi must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(dampingRate >= 0.0f, "PxVehicleWheelParams.dampingRate must be greater than or equal to zero", false);
		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */

