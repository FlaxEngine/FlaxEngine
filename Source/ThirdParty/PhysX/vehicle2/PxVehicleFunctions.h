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
#include "foundation/PxMat33.h"
#include "foundation/PxSimpleTypes.h"
#include "PxRigidBody.h"
#include "PxVehicleParams.h"
#include "roadGeometry/PxVehicleRoadGeometryState.h"
#include "rigidBody/PxVehicleRigidBodyStates.h"
#include "physxRoadGeometry/PxVehiclePhysXRoadGeometryState.h"
#include "physxActor/PxVehiclePhysXActorStates.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif
PX_FORCE_INLINE PxVec3 PxVehicleTransformFrameToFrame
(const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVec3& v)
{
	PxVec3 result = v;
	if ((srcFrame.lngAxis != trgFrame.lngAxis) || (srcFrame.latAxis != trgFrame.latAxis) || (srcFrame.vrtAxis != trgFrame.vrtAxis))
	{
		const PxMat33 a = srcFrame.getFrame();
		const PxMat33 r = trgFrame.getFrame();
		result = (r * a.getTranspose() * v);
	}
	return result;
}

PX_FORCE_INLINE PxVec3 PxVehicleTransformFrameToFrame
(const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, 
 const PxVehicleScale& srcScale, const PxVehicleScale& trgScale, 
 const PxVec3& v)
{
	PxVec3 result = PxVehicleTransformFrameToFrame(srcFrame, trgFrame, v);
	if((srcScale.scale != trgScale.scale))
		result *= (trgScale.scale / srcScale.scale);
	return result;
}

PX_FORCE_INLINE PxTransform PxVehicleTransformFrameToFrame
(const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, 
 const PxVehicleScale& srcScale, const PxVehicleScale& trgScale,
 const PxTransform& v)
{
	PxTransform result(PxVehicleTransformFrameToFrame(srcFrame, trgFrame, srcScale, trgScale, v.p), v.q);
	if ((srcFrame.lngAxis != trgFrame.lngAxis) || (srcFrame.latAxis != trgFrame.latAxis) || (srcFrame.vrtAxis != trgFrame.vrtAxis))
	{
		PxF32 angle;
		PxVec3 axis;
		v.q.toRadiansAndUnitAxis(angle, axis);
		result.q = PxQuat(angle, PxVehicleTransformFrameToFrame(srcFrame, trgFrame, axis));
	}
	return result;
}

PX_FORCE_INLINE PxVec3 PxVehicleComputeTranslation(const PxVehicleFrame& frame, const PxReal lng, const PxReal lat, const PxReal vrt)
{
	const PxVec3 v = frame.getFrame()*PxVec3(lng, lat, vrt);
	return v;
}

PX_FORCE_INLINE PxQuat PxVehicleComputeRotation(const PxVehicleFrame& frame, const PxReal roll, const PxReal pitch, const PxReal yaw)
{
	const PxMat33 m = frame.getFrame();
	const PxVec3& lngAxis = m.column0;
	const PxVec3& latAxis = m.column1;
	const PxVec3& vrtAxis = m.column2;
	const PxQuat quatPitch(pitch, latAxis);
	const PxQuat quatRoll(roll, lngAxis);
	const PxQuat quatYaw(yaw, vrtAxis);
	const PxQuat result = quatYaw * quatRoll * quatPitch;
	return result;
}

PX_FORCE_INLINE PxF32 PxVehicleComputeSign(const PxReal f)
{
	return physx::intrinsics::fsel(f, physx::intrinsics::fsel(-f, 0.0f, 1.0f), -1.0f);
}


/**
\brief Shift the origin of a vehicle by the specified vector.

Call this method to adjust the internal data structures of vehicles to reflect the shifted origin location
(the shift vector will get subtracted from all world space spatial data).

\param[in] axleDesc is a description of the wheels on the vehicle.
\param[in] shift is the translation vector used to shift the origin.
\param[in] rigidBodyState stores the current position of the vehicle
\param[in] roadGeometryStates stores the hit plane under each wheel.
\param[in] physxActor stores the PxRigidActor that is the vehicle's PhysX representation. 
\param[in] physxQueryStates stores the hit point of the most recent execution of PxVehiclePhysXRoadGeometryQueryUpdate() for each wheel.
\note It is the user's responsibility to keep track of the summed total origin shift and adjust all input/output to/from the vehicle accordingly.
\note This call will not automatically shift the PhysX scene and its objects. PxScene::shiftOrigin() must be called seperately to keep the systems in sync.
\note If there is no associated PxRigidActor then set physxActor to NULL.
\note If there is an associated PxRigidActor and it is already in a PxScene then the complementary call to PxScene::shiftOrigin() will take care of
shifting the associated PxRigidActor.  This being the case, set physxActor to NULL.  physxActor should be a non-NULL pointer only when there is an
associated PxRigidActor and it is not part of a PxScene.  This can occur if the associated PxRigidActor is updated using PhysX immediate mode.
\note If scene queries are independent of PhysX geometry then set queryStates to NULL.
*/
PX_FORCE_INLINE void PxVehicleShiftOrigin
(const PxVehicleAxleDescription& axleDesc, const PxVec3& shift, 
 PxVehicleRigidBodyState& rigidBodyState, PxVehicleRoadGeometryState* roadGeometryStates, 
 PxVehiclePhysXActor* physxActor = NULL, PxVehiclePhysXRoadGeometryQueryState* physxQueryStates = NULL)
{
	//Adjust the vehicle's internal pose.
	rigidBodyState.pose.p -= shift;

	//Optionally adjust the PxRigidActor pose.
	if (physxActor && !physxActor->rigidBody->getScene())
	{
		const PxTransform oldPose = physxActor->rigidBody->getGlobalPose();
		const PxTransform newPose(oldPose.p - shift, oldPose.q);
		physxActor->rigidBody->setGlobalPose(newPose);
	}

	for (PxU32 i = 0; i < axleDesc.nbWheels; i++)
	{
		const PxU32 wheelId = axleDesc.wheelIdsInAxleOrder[i];

		//Optionally adjust the hit position.
		if (physxQueryStates && physxQueryStates[wheelId].actor)
			physxQueryStates[wheelId].hitPosition -= shift;

		//Adjust the hit plane.
		if (roadGeometryStates[wheelId].hitState)
		{
			const PxPlane plane = roadGeometryStates[wheelId].plane;
			PxU32 largestNormalComponentAxis = 0;
			PxReal largestNormalComponent = 0.0f;
			const PxF32 normalComponents[3] = { plane.n.x, plane.n.y, plane.n.z };
			for (PxU32 k = 0; k < 3; k++)
			{
				if (PxAbs(normalComponents[k]) > largestNormalComponent)
				{
					largestNormalComponent = PxAbs(normalComponents[k]);
					largestNormalComponentAxis = k;
				}
			}
			PxVec3 pointInPlane(PxZero);
			switch (largestNormalComponentAxis)
			{
			case 0:
				pointInPlane.x = -plane.d / plane.n.x;
				break;
			case 1:
				pointInPlane.y = -plane.d / plane.n.y;
				break;
			case 2:
				pointInPlane.z = -plane.d / plane.n.z;
				break;
			default:
				break;
			}
			roadGeometryStates[wheelId].plane.d = -plane.n.dot(pointInPlane - shift);
		}
	}
}

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
