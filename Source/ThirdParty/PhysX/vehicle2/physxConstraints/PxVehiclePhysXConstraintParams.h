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

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleFrame;
struct PxVehicleScale;

/**
\brief A description of the PhysX models employed to resolve suspension limit constraints.
@see PxVehiclePhysXConstraintState
*/
struct PxVehiclePhysXSuspensionLimitConstraintParams
{
	/**
	\brief restitution is used by the restitution model used to generate a target velocity when resolving suspension limit
	constraints.
	\note A value of 0.0 means that the restitution model is not employed.
	\note Restitution has no effect if directionForSuspensionLimitConstraint has value Enum::eNONE.
	@see Px1DConstraintFlag::eRESTITUTION	
	@see Px1DConstraint::RestitutionModifiers::restitution
	*/
	PxReal restitution;

	/**
	\brief Set the direction to apply a constraint impulse when the suspension cannot place the wheel on the ground
	and simultaneously respect the limits of suspension travel. The choices are to push along the ground normal to resolve the 
	geometric error or to push along the suspension direction. The former choice can be thought of as mimicing a force applied 
	by the tire's contact with the ground, while the latter can be thought of as mimicing a force arising from a suspension limit spring.
	When the ground normal and the suspension direction are approximately aligned, both do an equivalent job of maintaining the wheel above
	the ground. When the vehicle is on its side, eSUSPENSION does a better job of keeping the wheels above 
	the ground but comes at the cost of an unnaturally strong torque that can lead to unwanted self-righting behaviour. 
	eROAD_GEOMETRY_NORMAL is a good choice to avoid self-righting behaviour and still do a reasonable job at maintaining
	the wheel above the ground in the event that the vehicle is tending towards a roll onto its side.  
	eNONE should be chosen if it is desired that no extra impulse is applied when the suspension alone cannot keep the wheels above
	the ground plane.
	*/
	enum DirectionSpecifier
	{
		eSUSPENSION,
		eROAD_GEOMETRY_NORMAL,
		eNONE
	};
	DirectionSpecifier directionForSuspensionLimitConstraint;

	PX_FORCE_INLINE PxVehiclePhysXSuspensionLimitConstraintParams transformAndScale(
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
		PX_CHECK_AND_RETURN_VAL(
			PxVehiclePhysXSuspensionLimitConstraintParams::eSUSPENSION == directionForSuspensionLimitConstraint ||
			PxVehiclePhysXSuspensionLimitConstraintParams::eROAD_GEOMETRY_NORMAL == directionForSuspensionLimitConstraint ||
			PxVehiclePhysXSuspensionLimitConstraintParams::eNONE == directionForSuspensionLimitConstraint, "PxVehiclePhysXSuspensionLimitConstraintParams.directionForSuspensionLimitConstraint must have legal value", false);
		PX_CHECK_AND_RETURN_VAL(restitution >= 0.0f && restitution <= 1.0f, "PxVehiclePhysXSuspensionLimitConstraintParams.restitution must be in range [0, 1]", false);
		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
