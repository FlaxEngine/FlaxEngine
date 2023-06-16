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

#include "foundation/PxAssert.h"
#include "foundation/PxTransform.h"
#include "extensions/PxConstraintExt.h"

#include "vehicle2/PxVehicleLimits.h"
#include "vehicle2/tire/PxVehicleTireStates.h"

#include "PxConstraint.h"
#include "PxConstraintDesc.h"

#if !PX_DOXYGEN
namespace physx
{

class PxConstraint;

namespace vehicle2
{
#endif


/**
\brief A description of the number of PxConstraintConnector instances per vehicle required to maintain suspension limit
and sticky tire instances.
*/
struct PxVehiclePhysXConstraintLimits
{
	enum Enum
	{
		eNB_DOFS_PER_PXCONSTRAINT = 12,
		eNB_DOFS_PER_WHEEL = 3,
		eNB_WHEELS_PER_PXCONSTRAINT = eNB_DOFS_PER_PXCONSTRAINT / eNB_DOFS_PER_WHEEL,
		eNB_CONSTRAINTS_PER_VEHICLE = (PxVehicleLimits::eMAX_NB_WHEELS + (eNB_WHEELS_PER_PXCONSTRAINT - 1)) / (eNB_WHEELS_PER_PXCONSTRAINT)
	};
};

/**
\brief PxVehiclePhysXConstraintState is a data structure used to write 
constraint data to the internal state of the associated PxScene.
@see Px1dConstraint
*/
struct PxVehiclePhysXConstraintState
{
	/**
	\brief a boolean describing whether to trigger a low speed constraint along the tire longitudinal and lateral directions.
	*/
	bool tireActiveStatus[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];
	/**
	\brief linear component of velocity jacobian in world space for the tire's longitudinal and lateral directions.
	*/
	PxVec3 tireLinears[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];
	/**
	\brief angular component of velocity jacobian in world space for the tire's longitudinal and lateral directions.
	*/
	PxVec3 tireAngulars[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];
	/**
	\brief damping coefficient applied to the tire's longitudinal and lateral velocities.
	
	The constraint sets a target velocity of 0 and the damping coefficient will impact the size of the
	impulse applied to reach the target. Since damping acts as a stiffness with respect to the velocity,
	too large a value can cause instabilities.
	*/
	PxReal tireDamping[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];


	/**
	\brief a boolean describing whether to trigger a suspension limit constraint.
	*/
	bool suspActiveStatus;
	/**
	\brief linear component of velocity jacobian in the world frame.
	*/
	PxVec3 suspLinear;
	/**
	\brief angular component of velocity jacobian in the world frame.
	*/
	PxVec3 suspAngular;
	/**
	\brief the excess suspension compression to be resolved by the constraint that cannot be resolved due to the travel limit
	of the suspension spring.
	
	\note The expected error value is the excess suspension compression projected onto the ground plane normal and should have
	      a negative sign.
	*/
	PxReal suspGeometricError;
	/**
	\brief restitution value of the restitution model used to generate a target velocity that will resolve the geometric error.
	\note A value of 0.0 means that the restitution model is not employed.
	@see Px1DConstraintFlag::eRESTITUTION
	@see Px1DConstraint::RestitutionModifiers::restitution
	*/
	PxReal restitution;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehiclePhysXConstraintState));
	}
};

//TAG:solverprepshader
PX_FORCE_INLINE PxU32 vehicleConstraintSolverPrep
(Px1DConstraint* constraints,
	PxVec3p& body0WorldOffset,
	PxU32 maxConstraints,
	PxConstraintInvMassScale&,
	const void* constantBlock,
	const PxTransform& bodyAToWorld,
	const PxTransform& bodyBToWorld,
	bool,
	PxVec3p& cA2w, PxVec3p& cB2w)
{
	PX_UNUSED(maxConstraints);
	PX_UNUSED(body0WorldOffset);
	PX_UNUSED(bodyBToWorld);
	PX_ASSERT(bodyAToWorld.isValid()); PX_ASSERT(bodyBToWorld.isValid());

	const PxVehiclePhysXConstraintState* data = static_cast<const PxVehiclePhysXConstraintState*>(constantBlock);
	PxU32 numActive = 0;

	//KS - the TGS solver will use raXn to try to add to the angular part of the linear constraints.
	//We overcome this by setting the ra and rb offsets to be 0.
	cA2w = bodyAToWorld.p;
	cB2w = bodyBToWorld.p;
	// note: this is only needed for PxSolverType::eTGS and even then it should not have an effect as
	//       long as a constraint raises Px1DConstraintFlag::eANGULAR_CONSTRAINT

	//Susp limit constraints.
	for (PxU32 i = 0; i < PxVehiclePhysXConstraintLimits::eNB_WHEELS_PER_PXCONSTRAINT; i++)
	{
		if (data[i].suspActiveStatus)
		{
			// Going beyond max suspension compression should be treated similar to rigid body contacts.
			// Thus setting up constraints that try to emulate such contacts.
			//
			// linear l = contact normal = n
			// angular a = suspension force application offset x contact normal = cross(r, n)
			//
			// velocity at contact:
			// vl: part from linear vehicle velocity v
			// vl = dot(n, v) = dot(l, v)
			//
			// va: part from angular vehicle velocity w
			// va = dot(n, cross(w, r)) = dot(w, cross(r, n)) = dot(w, a)
			//
			// ve: part from excess suspension compression
			// ve = (geomError / dt)    (note: geomError is expected to be negative here)
			//
			// velocity target vt = vl + va + ve
			// => should become 0 by applying positive impulse along l. If vt is positive,
			//    nothing will happen as a negative impulse would have to be applied (but
			//    min impulse is set to 0). If vt is negative, a positive impulse will get
			//    applied to push vt towards 0.
			//

			Px1DConstraint& p = constraints[numActive];
			p.linear0 = data[i].suspLinear;
			p.angular0 = data[i].suspAngular;
			p.geometricError = data[i].suspGeometricError;
			p.linear1 = PxVec3(0);
			p.angular1 = PxVec3(0);
			p.minImpulse = 0;
			p.maxImpulse = FLT_MAX;
			p.velocityTarget = 0;
			p.solveHint = PxConstraintSolveHint::eINEQUALITY;

			// note: this is only needed for PxSolverType::eTGS to not have the angular part
			//       be modified based on the linear part during substeps. Basically, it will
			//       disable the constraint re-projection etc. to emulate PxSolverType::ePGS.
			p.flags |= Px1DConstraintFlag::eANGULAR_CONSTRAINT;

			if (data[i].restitution > 0.0f)
			{
				p.flags |= Px1DConstraintFlag::eRESTITUTION;
				p.mods.bounce.restitution = data[i].restitution;
				p.mods.bounce.velocityThreshold = -FLT_MAX;
			}
			numActive++;
		}
	}

	//Sticky tire friction constraints.
	for (PxU32 i = 0; i < PxVehiclePhysXConstraintLimits::eNB_WHEELS_PER_PXCONSTRAINT; i++)
	{
		if (data[i].tireActiveStatus[PxVehicleTireDirectionModes::eLONGITUDINAL])
		{
			Px1DConstraint& p = constraints[numActive];
			p.linear0 = data[i].tireLinears[PxVehicleTireDirectionModes::eLONGITUDINAL];
			p.angular0 = data[i].tireAngulars[PxVehicleTireDirectionModes::eLONGITUDINAL];
			p.geometricError = 0.0f;
			p.linear1 = PxVec3(0);
			p.angular1 = PxVec3(0);
			p.minImpulse = -FLT_MAX;
			p.maxImpulse = FLT_MAX;
			p.velocityTarget = 0.0f;
			p.mods.spring.damping = data[i].tireDamping[PxVehicleTireDirectionModes::eLONGITUDINAL];
			// note: no stiffness specified as this will have no effect with geometricError=0
			p.flags = Px1DConstraintFlag::eSPRING | Px1DConstraintFlag::eACCELERATION_SPRING;
			p.flags |= Px1DConstraintFlag::eANGULAR_CONSTRAINT;  // see explanation of same flag usage further above
			numActive++;
		}
	}

	//Sticky tire friction constraints.
	for (PxU32 i = 0; i < PxVehiclePhysXConstraintLimits::eNB_WHEELS_PER_PXCONSTRAINT; i++)
	{
		if (data[i].tireActiveStatus[PxVehicleTireDirectionModes::eLATERAL])
		{
			Px1DConstraint& p = constraints[numActive];
			p.linear0 = data[i].tireLinears[PxVehicleTireDirectionModes::eLATERAL];
			p.angular0 = data[i].tireAngulars[PxVehicleTireDirectionModes::eLATERAL];
			p.geometricError = 0.0f;
			p.linear1 = PxVec3(0);
			p.angular1 = PxVec3(0);
			p.minImpulse = -FLT_MAX;
			p.maxImpulse = FLT_MAX;
			p.velocityTarget = 0.0f;
			p.mods.spring.damping = data[i].tireDamping[PxVehicleTireDirectionModes::eLATERAL];
			// note: no stiffness specified as this will have no effect with geometricError=0
			p.flags = Px1DConstraintFlag::eSPRING | Px1DConstraintFlag::eACCELERATION_SPRING;
			p.flags |= Px1DConstraintFlag::eANGULAR_CONSTRAINT;  // see explanation of same flag usage further above
			numActive++;
		}
	}

	return numActive;
}

PX_FORCE_INLINE void visualiseVehicleConstraint
(PxConstraintVisualizer &viz,
	const void* constantBlock,
	const PxTransform& body0Transform,
	const PxTransform& body1Transform,
	PxU32 flags)
{
	PX_UNUSED(&viz);
	PX_UNUSED(constantBlock);
	PX_UNUSED(body0Transform);
	PX_UNUSED(body1Transform);
	PX_UNUSED(flags);
	PX_ASSERT(body0Transform.isValid());
	PX_ASSERT(body1Transform.isValid());
}

class PxVehicleConstraintConnector : public PxConstraintConnector
{
public:

	PxVehicleConstraintConnector() : mVehicleConstraintState(NULL) {}
	PxVehicleConstraintConnector(PxVehiclePhysXConstraintState* vehicleConstraintState) : mVehicleConstraintState(vehicleConstraintState) {}
	~PxVehicleConstraintConnector() {}

	void setConstraintState(PxVehiclePhysXConstraintState* constraintState) { mVehicleConstraintState = constraintState; }

	virtual void* prepareData() { return mVehicleConstraintState; }
	virtual const void*	getConstantBlock() const { return mVehicleConstraintState; }
	virtual PxConstraintSolverPrep getPrep() const { return vehicleConstraintSolverPrep; }

	//Is this necessary if physx no longer supports double-buffering?
	virtual void onConstraintRelease() { }

	//Can be empty functions.
	virtual bool updatePvdProperties(physx::pvdsdk::PvdDataStream& pvdConnection, const PxConstraint* c, PxPvdUpdateType::Enum updateType) const
	{
		PX_UNUSED(pvdConnection);
		PX_UNUSED(c);
		PX_UNUSED(updateType);
		return true;
	}
	virtual void updateOmniPvdProperties() const { }
	virtual void onComShift(PxU32 actor) { PX_UNUSED(actor); }
	virtual void onOriginShift(const PxVec3& shift) { PX_UNUSED(shift); }
	virtual void*	getExternalReference(PxU32& typeID) { typeID = PxConstraintExtIDs::eVEHICLE_JOINT; return this; }
	virtual PxBase*	getSerializable() { return NULL; }

private:

	PxVehiclePhysXConstraintState* mVehicleConstraintState;
};

/**
\brief A mapping between constraint state data and the associated PxConstraint instances.
*/
struct PxVehiclePhysXConstraints
{
	/**
	\brief PxVehiclePhysXConstraintComponent writes to the constraintStates array and a 
	callback invoked by PxScene::simulate() reads a portion from it for a block of wheels 
	and writes that portion to an associated PxConstraint instance.
	*/
	PxVehiclePhysXConstraintState constraintStates[PxVehicleLimits::eMAX_NB_WHEELS];

	/**
	\brief PxVehiclePhysXConstraintComponent writes to the constraintStates array and a
	callback invoked by PxScene::simulate() reads a portion from it for a block of wheels
	and writes that portion to an associated PxConstraint instance.
	*/
	PxConstraint* constraints[PxVehiclePhysXConstraintLimits::eNB_CONSTRAINTS_PER_VEHICLE];

	/**
	\brief A constraint connector is necessary to connect each PxConstraint to a portion of the constraintStates array.
	*/
	PxVehicleConstraintConnector* constraintConnectors[PxVehiclePhysXConstraintLimits::eNB_CONSTRAINTS_PER_VEHICLE];

	PX_FORCE_INLINE void setToDefault()
	{
		for (PxU32 i = 0; i < PxVehicleLimits::eMAX_NB_WHEELS; i++)
		{
			constraintStates[i].setToDefault();
		}
		for(PxU32 i = 0; i < PxVehiclePhysXConstraintLimits::eNB_CONSTRAINTS_PER_VEHICLE; i++)
		{
			constraints[i] = NULL;
			constraintConnectors[i] = NULL;
		}
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */

