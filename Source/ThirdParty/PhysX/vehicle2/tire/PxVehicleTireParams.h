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

#include "PxVehicleTireStates.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleTireForceParams
{
	/**
	\brief Tire lateral stiffness is a graph of tire load that has linear behavior near zero load and
	flattens at large loads. latStiffX describes the minimum normalized load (load/restLoad) that gives a
	flat lateral stiffness response to load.
	\note A value of 0.0 indicates that the tire lateral stiffness is independent of load and will adopt 
	the value #latStiffY for all values of tire load.
	*/
	PxReal latStiffX;

	/**
	\brief Tire lateral stiffness is a graph of tire load that has linear behavior near zero load and
	flattens at large loads. latStiffY describes the maximum possible value of lateral stiffness that occurs
	when (load/restLoad) >= #latStiffX.

	<b>Unit:</b> force per lateral slip = mass * length / (time^2)
	*/
	PxReal latStiffY;

	/**
	\brief Tire Longitudinal stiffness
	\note Longitudinal force can be approximated as longStiff*longitudinalSlip.

	<b>Unit:</b> force per longitudinal slip = mass * length / (time^2)
	*/
	PxReal longStiff;

	/**
	\brief Tire camber stiffness
	\note Camber force can be approximated as camberStiff*camberAngle.

	<b>Unit:</b> force per radian = mass * length / (time^2)
	*/
	PxReal camberStiff;

	/**
	\brief Graph of friction vs longitudinal slip with 3 points.
	\note frictionVsSlip[0][0] is always zero.
	\note frictionVsSlip[0][1] is the friction available at zero longitudinal slip.
	\note frictionVsSlip[1][0] is the value of longitudinal slip with maximum friction.
	\note frictionVsSlip[1][1] is the maximum friction.
	\note frictionVsSlip[2][0] is the end point of the graph.
	\note frictionVsSlip[2][1] is the value of friction for slips greater than frictionVsSlip[2][0].
	\note The friction value is computed from the friction vs longitudinal slip graph using linear interpolation. 
	\note The friction value computed from the friction vs longitudinal slip graph is used to scale the friction
	value of the road geometry.
	\note frictionVsSlip[2][0] > frictionVsSlip[1][0] > frictionVsSlip[0][0]
	\note frictionVsSlip[1][1] is typically greater than frictionVsSlip[0][1]
	\note frictionVsSlip[2][1] is typically smaller than frictionVsSlip[1][1]
	\note longitudinal slips > frictionVsSlip[2][0] use friction multiplier frictionVsSlip[2][1]
	*/
	PxReal frictionVsSlip[3][2]; //3 (x,y) points

	/**
	\brief The rest load is the load that develops on the tire when the vehicle is at rest on a flat plane.
	\note The rest load is approximately the product of gravitational acceleration and (sprungMass + wheelMass).

	<b>Unit:</b> force = mass * length / (time^2)
	*/
	PxReal restLoad;

	/**
	\brief Tire load variation can be strongly dependent on the time-step so it is a good idea to filter it
	to give less jerky handling behavior.
	\note Tire load filtering is implemented by linear interpolating a graph containing just two points.
	The x-axis of the graph is normalized tire load, while the y-axis is the filtered normalized tire load that is 
	to be applied during the tire force calculation.
	\note The normalized load is the force acting downwards on the tire divided by restLoad.
	\note The minimum possible normalized load is zero.
	\note There are two points on the graph: (minNormalisedLoad, minNormalisedFilteredLoad) and (maxNormalisedLoad, maxFilteredNormalisedLoad).
	\note Normalized loads less than minNormalisedLoad have filtered normalized load = minNormalisedFilteredLoad.
	\note Normalized loads greater than maxNormalisedLoad have filtered normalized load = maxFilteredNormalisedLoad.
	\note Normalized loads in-between are linearly interpolated between minNormalisedFilteredLoad and maxFilteredNormalisedLoad.
	\note The tire load applied as input to the tire force computation is the filtered normalized load multiplied by the rest load.
	\note loadFilter[0][0] is minNormalisedLoad
	\note loadFilter[0][1] is minFilteredNormalisedLoad
	\note loadFilter[1][0] is maxNormalisedLoad
	\note loadFilter[1][1] is maxFilteredNormalisedLoad
	*/
	PxReal loadFilter[2][2]; //2 (x,y) points

	PX_FORCE_INLINE PxVehicleTireForceParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PxVehicleTireForceParams r = *this;
		const PxReal scale = trgScale.scale / srcScale.scale;
		r.latStiffY *= scale;
		r.longStiff *= scale;
		r.camberStiff *= scale;
		r.restLoad *= scale;
		return r;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(latStiffX >= 0, "PxVehicleTireForceParams.latStiffX must be greater than or equal to zero", false);
		PX_CHECK_AND_RETURN_VAL(latStiffY > 0, "PxVehicleTireForceParams.latStiffY must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(longStiff > 0, "PxVehicleTireForceParams.longStiff must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(camberStiff >= 0, "PxVehicleTireForceParams.camberStiff must be greater than or equal zero", false);
		PX_CHECK_AND_RETURN_VAL(restLoad > 0, "PxVehicleTireForceParams.restLoad must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(loadFilter[1][0] >= loadFilter[0][0], "PxVehicleTireForceParams.loadFilter[1][0] must be greater than or equal to PxVehicleTireForceParams.loadFilter[0][0]", false);
		PX_CHECK_AND_RETURN_VAL(loadFilter[1][1] > 0, "PxVehicleTireLoadFilterData.loadFilter[1][1] must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(0.0f == loadFilter[0][0], "PxVehicleTireLoadFilterData.loadFilter[0][0] must be equal to zero", false);
		PX_CHECK_AND_RETURN_VAL(frictionVsSlip[0][0] >= 0.0f && frictionVsSlip[0][1] >= 0.0f, "Illegal values for frictionVsSlip[0]", false);
		PX_CHECK_AND_RETURN_VAL(frictionVsSlip[1][0] >= 0.0f && frictionVsSlip[1][1] >= 0.0f, "Illegal values for frictionVsSlip[1]", false);
		PX_CHECK_AND_RETURN_VAL(frictionVsSlip[2][0] >= 0.0f && frictionVsSlip[2][1] >= 0.0f, "Illegal values for frictionVsSlip[2]", false);
		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
