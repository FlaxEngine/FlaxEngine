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


#include "foundation/PxTransform.h"
#include "foundation/PxFoundation.h"

#include "common/PxCoreUtilityTypes.h"

#include "vehicle2/PxVehicleParams.h"
#include "vehicle2/PxVehicleFunctions.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleSuspensionParams
{
	/**
	\brief suspensionAttachment specifies the wheel pose at maximum compression.
	\note suspensionAttachment is specified in the frame of the rigid body.
	\note camber, steer and toe angles are all applied in the suspension frame.
	*/
	PxTransform suspensionAttachment;	

	/**
	\brief suspensionTravelDir specifies the direction of suspension travel.
	\note suspensionTravelDir is specified in the frame of the rigid body.
	*/
	PxVec3 suspensionTravelDir;			

	/**
	\brief suspensionTravelDist is the maximum distance that the suspenson can elongate along #suspensionTravelDir
	from the pose specified by #suspensionAttachment.
	\note The position suspensionAttachment.p + #suspensionTravelDir*#suspensionTravelDist corresponds to the 
	the suspension at maximum droop in the rigid body frame.
	*/
	PxReal suspensionTravelDist;

	/**
	\brief wheelAttachment is the pose of the wheel in the suspension frame.
	\note The rotation angle around the wheel's lateral axis is applied in the wheel attachment frame.
	*/
	PxTransform wheelAttachment;		


	PX_FORCE_INLINE PxVehicleSuspensionParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PxVehicleSuspensionParams r = *this;
		r.suspensionAttachment= PxVehicleTransformFrameToFrame(srcFrame, trgFrame, srcScale, trgScale, suspensionAttachment);
		r.suspensionTravelDir = PxVehicleTransformFrameToFrame(srcFrame, trgFrame, suspensionTravelDir);
		r.suspensionTravelDist *= (trgScale.scale/srcScale.scale);
		r.wheelAttachment = PxVehicleTransformFrameToFrame(srcFrame, trgFrame, srcScale, trgScale, wheelAttachment);
		return r;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(suspensionAttachment.isValid(), "PxVehicleSuspensionParams.suspensionAttachment must be a valid transform", false);
		PX_CHECK_AND_RETURN_VAL(suspensionTravelDir.isFinite(), "PxVehicleSuspensionParams.suspensionTravelDir must be a valid vector", false);
		PX_CHECK_AND_RETURN_VAL(suspensionTravelDir.isNormalized(), "PxVehicleSuspensionParams.suspensionTravelDir must be a unit vector", false);
		PX_CHECK_AND_RETURN_VAL(suspensionTravelDist > 0.0f, "PxVehicleSuspensionParams.suspensionTravelDist must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(wheelAttachment.isValid(), "PxVehicleSuspensionParams.wheelAttachment must be a valid transform", false);
		return true;
	}
};

struct PxVehicleSuspensionJounceCalculationType
{
	enum Enum
	{
		eRAYCAST,   //!< The jounce is calculated using a raycast against the plane of the road geometry state
		eSWEEP,     //!< The jounce is calculated by sweeping a cylinder against the plane of the road geometry state
		eMAX_NB
	};
};

struct PxVehicleSuspensionStateCalculationParams
{
	PxVehicleSuspensionJounceCalculationType::Enum suspensionJounceCalculationType;

	/**
	\brief Limit the suspension expansion dynamics.
	
	If a hit with the ground is detected, the suspension jounce will be set such that the wheel
	is placed on the ground. This can result in large changes to jounce within a single
	simulation frame, if the ground surface has high frequency or if the simulation time step
	is large. As a result, large damping forces can evolve and cause undesired behavior. If this
	parameter is set to true, the suspension expansion speed will be limited to what can be
	achieved given the time step, suspension stiffness etc. As a consequence, handling of the
	vehicle will be affected as the wheel might loose contact with the ground more easily.
	*/
	bool limitSuspensionExpansionVelocity;

	PX_FORCE_INLINE PxVehicleSuspensionStateCalculationParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PX_UNUSED(srcScale);
		PX_UNUSED(trgScale);
		return *this;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		return true;
	}
};

/**
\brief Compliance describes how toe and camber angle and force application points are affected by suspension compression.
\note Each compliance term is in the form of a graph with up to 3 points. 
\note Each point in the graph has form (jounce/suspensionTravelDist, complianceValue).
\note The sequence of points must respresent monotonically increasing values of jounce.
\note The compliance value can be computed by linear interpolation.
\note If any graph has zero points in it, a value of 0.0 is used for the compliance value. 
\note If any graph has 1 point in it, the compliance value of that point is used directly. 
*/
struct PxVehicleSuspensionComplianceParams
{
	/**
	\brief A graph of toe angle against jounce/suspensionTravelDist with the toe angle expressed in radians.
	\note The toe angle is applied in the suspension frame.
	*/
	PxVehicleFixedSizeLookupTable<PxReal, 3> wheelToeAngle;

	/**
	\brief A graph of camber angle against jounce/suspensionTravelDist with the camber angle expressed in radians.
	\note The camber angle is applied in the suspension frame.
	*/
	PxVehicleFixedSizeLookupTable<PxReal, 3> wheelCamberAngle;

	/**
	\brief Suspension forces are applied at an offset from the suspension frame. suspForceAppPoint
	specifies the (X, Y, Z) components of that offset as a function of jounce/suspensionTravelDist.
	*/
	PxVehicleFixedSizeLookupTable<PxVec3, 3> suspForceAppPoint;

	/**
	\brief Tire forces are applied at an offset from the suspension frame. tireForceAppPoint
	specifies the (X, Y, Z) components of that offset as a function of jounce/suspensionTravelDist.
	*/
	PxVehicleFixedSizeLookupTable<PxVec3, 3> tireForceAppPoint;

	PX_FORCE_INLINE PxVehicleSuspensionComplianceParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PxVehicleSuspensionComplianceParams r = *this;

		const  PxReal scale = trgScale.scale / srcScale.scale;
		for (PxU32 i = 0; i < r.suspForceAppPoint.nbDataPairs; i++)
		{
			r.suspForceAppPoint.yVals[i] = PxVehicleTransformFrameToFrame(srcFrame, trgFrame, suspForceAppPoint.yVals[i]);
			r.suspForceAppPoint.yVals[i] *= scale;
		}
		for (PxU32 i = 0; i < r.tireForceAppPoint.nbDataPairs; i++)
		{
			r.tireForceAppPoint.yVals[i] = PxVehicleTransformFrameToFrame(srcFrame, trgFrame, tireForceAppPoint.yVals[i]);
			r.tireForceAppPoint.yVals[i] *= scale;
		}
		return r;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(wheelToeAngle.isValid(), "PxVehicleSuspensionComplianceParams.wheelToeAngle is invalid", false);
		PX_CHECK_AND_RETURN_VAL(wheelCamberAngle.isValid(), "PxVehicleSuspensionComplianceParams.wheelCamberAngle is invalid", false);
		PX_CHECK_AND_RETURN_VAL(suspForceAppPoint.isValid(), "PxVehicleSuspensionComplianceParams.wheelToeAngle is invalid", false);
		PX_CHECK_AND_RETURN_VAL(tireForceAppPoint.isValid(), "PxVehicleSuspensionComplianceParams.wheelCamberAngle is invalid", false);

		for (PxU32 i = 0; i < wheelToeAngle.nbDataPairs; i++)
		{
			PX_CHECK_AND_RETURN_VAL(wheelToeAngle.xVals[i] >= 0.0f && wheelToeAngle.xVals[i] <= 1.0f, "PxVehicleSuspensionComplianceParams.wheelToeAngle must be an array of points (x,y) with x in range [0, 1]", false);
			PX_CHECK_AND_RETURN_VAL(wheelToeAngle.yVals[i] >= -PxPi && wheelToeAngle.yVals[i] <=  PxPi, "PxVehicleSuspensionComplianceParams.wheelToeAngle must be an array of points (x,y) with y in range [-Pi, Pi]", false);
		}
		for (PxU32 i = 0; i < wheelCamberAngle.nbDataPairs; i++)
		{
			PX_CHECK_AND_RETURN_VAL(wheelCamberAngle.xVals[i] >= 0.0f && wheelCamberAngle.xVals[i] <= 1.0f, "PxVehicleSuspensionComplianceParams.wheelCamberAngle must be an array of points (x,y) with x in range [0, 1]", false);
			PX_CHECK_AND_RETURN_VAL(wheelCamberAngle.yVals[i] >= -PxPi && wheelCamberAngle.yVals[i] <= PxPi, "PxVehicleSuspensionComplianceParams.wheelCamberAngle must be an array of points (x,y) with y in range [-Pi, Pi]", false);
		}

		for (PxU32 i = 0; i < suspForceAppPoint.nbDataPairs; i++)
		{
			PX_CHECK_AND_RETURN_VAL(suspForceAppPoint.xVals[i] >= 0.0f && suspForceAppPoint.xVals[i] <= 1.0f, "PxVehicleSuspensionComplianceParams.suspForceAppPoint[0] must be an array of points (x,y) with x in range [0, 1]", false);
		}
		for (PxU32 i = 0; i < tireForceAppPoint.nbDataPairs; i++)
		{
			PX_CHECK_AND_RETURN_VAL(tireForceAppPoint.xVals[i] >= 0.0f && tireForceAppPoint.xVals[i] <= 1.0f, "PxVehicleSuspensionComplianceParams.tireForceAppPoint[0] must be an array of points (x,y) with x in range [0, 1]", false);
		}
		return true;
	}
};

/**
\brief Suspension force is computed by converting suspenson state to suspension force under the assumption of a linear spring.
@see PxVehicleSuspensionForceUpdate
*/
struct PxVehicleSuspensionForceParams
{
	/**
	\brief Spring strength of suspension.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> mass / (time^2)
	*/
	PxReal stiffness;

	/**
	\brief Spring damper rate of suspension.

	<b>Range:</b> [0, inf)<br>
	<b>Unit:</b> mass / time
	*/
	PxReal damping;

	/**
	\brief Part of the vehicle mass that is supported by the suspension spring.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> mass
	*/
	PxReal sprungMass;

	PX_FORCE_INLINE PxVehicleSuspensionForceParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PX_UNUSED(srcScale);
		PX_UNUSED(trgScale);
		return *this;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(stiffness > 0.0f, "PxVehicleSuspensionForceParams.stiffness must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(damping >= 0.0f, "PxVehicleSuspensionForceParams.damping must be greater than or equal to zero", false);
		PX_CHECK_AND_RETURN_VAL(sprungMass > 0.0f, "PxVehicleSuspensionForceParams.sprungMass must be greater than zero", false);
		return true;
	}
};

/**
\brief Suspension force is computed by converting suspenson state to suspension force under the assumption of a linear spring.
@see PxVehicleSuspensionLegacyForceUpdate
@deprecated
*/
struct PX_DEPRECATED PxVehicleSuspensionForceLegacyParams
{
	/**
	\brief Spring strength of suspension.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> mass / (time^2)
	*/
	PxReal stiffness;

	/**
	\brief Spring damper rate of suspension.

	<b>Range:</b> [0, inf)<br>
	<b>Unit:</b> mass / time
	*/
	PxReal damping;

	/**
	\brief The suspension compression that balances the gravitational force acting on the sprung mass.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> length
	*/
	PxReal restDistance;

	/**
	\brief The mass supported by the suspension spring.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> mass
	*/
	PxReal sprungMass;

	PX_FORCE_INLINE PxVehicleSuspensionForceLegacyParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PxVehicleSuspensionForceLegacyParams r = *this;
		r.restDistance *= (trgScale.scale / srcScale.scale);
		return *this;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(stiffness > 0.0f, "PxVehicleSuspensionForceLegacyParams.stiffness must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(damping >= 0.0f, "PxVehicleSuspensionForceLegacyParams.damping must be greater than or equal to zero", false);
		PX_CHECK_AND_RETURN_VAL(restDistance > 0.0f, "PxVehicleSuspensionForceLegacyParams.restDistance must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(sprungMass > 0.0f, "PxVehicleSuspensionForceLegacyParams.sprungMass must be greater than zero", false);
		return true;
	}

};

/**
\brief The purpose of the anti-roll bar is to generate a torque to apply to the vehicle's rigid body that will reduce the jounce difference arising
between any pair of chosen wheels. If the chosen wheels share an axle, the anti-roll bar will attempt to reduce the roll angle of the vehicle's rigid body.
Alternatively, if the chosen wheels are the front and rear wheels along one side of the vehicle, the anti-roll bar will attempt to reduce the pitch angle of the 
vehicle's rigid body.
*/
struct PxVehicleAntiRollForceParams
{
	/*
	\brief The anti-roll bar connects two wheels with indices wheel0 and wheel1
	\note wheel0 and wheel1 may be chosen to have the effect of an anti-dive bar or to have the effect of an anti-roll bar. 
	*/
	PxU32 wheel0;

	/*
	\brief The anti-roll bar connects two wheels with indices wheel0 and wheel1
	\note wheel0 and wheel1 may be chosen to have the effect of an anti-dive bar or to have the effect of an anti-roll bar.
	*/
	PxU32 wheel1;

	/*
	\brief The linear stiffness of the anti-roll bar.
	\note A positive stiffness will work to reduce the discrepancy in jounce between wheel0 and wheel1.
	\note A negative stiffness will work to increase the discrepancy in jounce between wheel0 and wheel1.

	<b>Unit:</b> mass / (time^2)
	*/
	PxReal stiffness;

	PX_FORCE_INLINE PxVehicleAntiRollForceParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PX_UNUSED(srcScale);
		PX_UNUSED(trgScale);
		return *this;
	}

	PX_FORCE_INLINE bool isValid(const PxVehicleAxleDescription& axleDesc) const
	{
		if (!PxIsFinite(stiffness))
			return false;
		if (wheel0 == wheel1)
			return false;

		//Check that each wheel id is a valid wheel.
		const PxU32 wheelIds[2] = { wheel0,  wheel1 };
		for (PxU32 k = 0; k < 2; k++)
		{
			const PxU32 wheelToFind = wheelIds[k];
			bool foundWheelInAxleDescription = false;
			for (PxU32 i = 0; i < axleDesc.nbWheels; i++)
			{
				const PxU32 wheel = axleDesc.wheelIdsInAxleOrder[i];
				if (wheel == wheelToFind)
				{
					foundWheelInAxleDescription = true;
				}
			}
			if (!foundWheelInAxleDescription)
				return false;
		}

		return true;
	}
};


#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
