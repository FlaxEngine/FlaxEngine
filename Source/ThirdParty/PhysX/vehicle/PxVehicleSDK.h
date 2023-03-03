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

#ifndef PX_VEHICLE_SDK_H
#define PX_VEHICLE_SDK_H

#include "foundation/Px.h"
#include "foundation/PxVec3.h"
#include "common/PxTypeInfo.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxPhysics;
class PxSerializationRegistry;

/**
\brief Initialize the PhysXVehicle library. 

Call this before using any of the vehicle functions.

\param physics The PxPhysics instance.
\param serializationRegistry PxSerializationRegistry instance, if NULL vehicle serialization is not supported.

\note This function must be called after PxFoundation and PxPhysics instances have been created.
\note If a PxSerializationRegistry instance is specified then PhysXVehicle is also dependent on PhysXExtensions.

@see PxCloseVehicleSDK
*/
PX_DEPRECATED PX_C_EXPORT bool PX_CALL_CONV PxInitVehicleSDK(PxPhysics& physics, PxSerializationRegistry* serializationRegistry = NULL);


/**
\brief Shut down the PhysXVehicle library. 

Call this function as part of the physx shutdown process.

\param serializationRegistry PxSerializationRegistry instance, if non-NULL must be the same as passed into PxInitVehicleSDK.

\note This function must be called prior to shutdown of PxFoundation and PxPhysics.
\note If the PxSerializationRegistry instance is specified this function must additionally be called prior to shutdown of PhysXExtensions.

@see PxInitVehicleSDK
*/
PX_DEPRECATED PX_C_EXPORT void PX_CALL_CONV PxCloseVehicleSDK(PxSerializationRegistry* serializationRegistry = NULL);


/**
\brief This number is the maximum number of wheels allowed for a vehicle.
*/
#define PX_MAX_NB_WHEELS (20)


/**
\brief Compiler setting to enable recording of telemetry data

@see PxVehicleUpdateSingleVehicleAndStoreTelemetryData, PxVehicleTelemetryData
*/
#define PX_DEBUG_VEHICLE_ON (1)


/**
@see PxVehicleDrive4W, PxVehicleDriveTank, PxVehicleDriveNW, PxVehicleNoDrive, PxVehicleWheels::getVehicleType
*/
struct PX_DEPRECATED PxVehicleTypes
{
	enum Enum
	{
		eDRIVE4W=0,
		eDRIVENW,
		eDRIVETANK,
		eNODRIVE,
		eUSER1,
		eUSER2,
		eUSER3,
		eMAX_NB_VEHICLE_TYPES
	};
};


/**
\brief An enumeration of concrete vehicle classes inheriting from PxBase.
\note This enum can be used to identify a vehicle object stored in a PxCollection.
@see PxBase, PxTypeInfo, PxBase::getConcreteType
*/
struct PX_DEPRECATED PxVehicleConcreteType
{
	enum Enum
	{
		eVehicleNoDrive = PxConcreteType::eFIRST_VEHICLE_EXTENSION,
		eVehicleDrive4W,
		eVehicleDriveNW,
		eVehicleDriveTank
	};
};


/**
\brief Set the basis vectors of the vehicle simulation 

See PxVehicleContext for the default values.

Call this function before using PxVehicleUpdates unless the default values are correct
or the settings structure is explicitly provided.

@see PxVehicleContext
*/
PX_DEPRECATED void PxVehicleSetBasisVectors(const PxVec3& up, const PxVec3& forward);


/**
@see PxVehicleSetUpdateMode
*/
struct PX_DEPRECATED PxVehicleUpdateMode
{
	enum Enum
	{
		eVELOCITY_CHANGE,
		eACCELERATION	
	};
};


/**
\brief Set the effect of PxVehicleUpdates to be either to modify each vehicle's rigid body actor

with an acceleration to be applied in the next PhysX SDK update or as an immediate velocity modification.

See PxVehicleContext for the default value.

Call this function before using PxVehicleUpdates for the first time if the default is not the desired behavior
or if the settings structure is not explicitly provided.

@see PxVehicleUpdates, PxVehicleContext
*/
PX_DEPRECATED void PxVehicleSetUpdateMode(PxVehicleUpdateMode::Enum vehicleUpdateMode);

/** 

\brief Set threshold angles that are used to determine if a wheel hit is to be resolved by vehicle suspension or by rigid body collision.


\note	                ^
		                N  ___
		                  |**
                              **
		                         **
			   %%%    %%%           **
		  %%%              %%%		   **	/
										   /
	  %%%                      %%%        /
										 /
	 %%%                         %%%    /
	                C                  /
	 %%%            | **         %%%  /
	                |      **        /
	 %%%            |          **%%%/
	                |              X**     
		%%%         |        %%%  /      **_|  ^
		            |            /             D
			  %%%   | %%%       /
			        |          /
					|		  /
                    |        /
                    |                   
               ^    |
		       S   \|/   

The diagram above depicts a wheel centered at "C" that has hit an inclined plane at point "X".  
The inclined plane has unit normal "N", while the suspension direction has unit vector "S".
The unit vector from the wheel center to the hit point is "D".
Hit points are analyzed by comparing the unit vectors D and N with the suspension direction S.
This analysis is performed in the contact modification callback PxVehicleModifyWheelContacts (when enabled) and in 
PxVehicleUpdates (when non-blocking sweeps are enabled).
If the angle between D and S is less than pointRejectAngle the hit is accepted by the suspension in PxVehicleUpdates and rejected 
by the contact modification callback PxVehicleModifyWheelContacts.
If the angle between -N and S is less than normalRejectAngle the hit is accepted by the suspension in PxVehicleUpdates and rejected
by the contact modification callback PxVehicleModifyWheelContacts.

\param pointRejectAngle is the threshold angle used when comparing the angle between D and S.

\param normalRejectAngle is the threshold angle used when comparing the angle between -N and S.

\note PxVehicleUpdates ignores the rejection angles for raycasts and for sweeps that return blocking hits.

\note Both angles have default values of Pi/4.

@see PxVehicleSuspensionSweeps, PxVehicleModifyWheelContacts, PxVehicleContext
*/
PX_DEPRECATED void PxVehicleSetSweepHitRejectionAngles(const PxF32 pointRejectAngle, const PxF32 normalRejectAngle);


/**
\brief Determine the maximum acceleration experienced by PxRigidDynamic instances that are found to be in contact 
with a wheel.

\note Newton's Third Law states that every force has an equal and opposite force.  As a consequence, forces applied to 
the suspension must be applied to dynamic objects that lie under the wheel. This can lead to instabilities, particularly
when a heavy wheel is driving on a light object.  The value of maxHitActorAcceleration clamps the applied force so that it never 
generates an acceleration greater than the specified value.

See PxVehicleContext for the default value.

@see PxVehicleContext
*/
PX_DEPRECATED void PxVehicleSetMaxHitActorAcceleration(const PxF32 maxHitActorAcceleration);


/**
\brief Common parameters and settings used for the vehicle simulation.

To be passed into PxVehicleUpdates(), for example.

@see PxVehicleUpdates()
*/
class PX_DEPRECATED PxVehicleContext
{
public:

	/**
	\brief The axis denoting the up direction for vehicles.

	<b>Range:</b> unit length vector<br>
	<b>Default:</b> PxVec3(0,1,0)

	@see PxVehicleSetBasisVectors()
	*/
	PxVec3 upAxis;

	/**
	\brief The axis denoting the forward direction for vehicles.

	<b>Range:</b> unit length vector<br>
	<b>Default:</b> PxVec3(0,0,1)

	@see PxVehicleSetBasisVectors()
	*/
	PxVec3 forwardAxis;

	/**
	\brief The axis denoting the side direction for vehicles.

	Has to be the cross product of the up- and forward-axis. The method
	computeSideAxis() can be used to do that computation for you.
	
	<b>Range:</b> unit length vector<br>
	<b>Default:</b> PxVec3(1,0,0)

	@see PxVehicleSetBasisVectors(), computeSideAxis()
	*/
	PxVec3 sideAxis;

	/**
	\brief Apply vehicle simulation results as acceleration or velocity modification.
	
	See PxVehicleSetUpdateMode() for details.

	<b>Default:</b> eVELOCITY_CHANGE

	@see PxVehicleSetUpdateMode()
	*/
	PxVehicleUpdateMode::Enum updateMode;

	/**
	\brief Cosine of threshold angle for rejecting sweep hits.
	
	See PxVehicleSetSweepHitRejectionAngles() for details.

	<b>Range:</b> (1, -1)<br>
	<b>Default:</b> 0.707f (cosine of 45 degrees)

	@see PxVehicleSetSweepHitRejectionAngles()
	*/
	PxF32 pointRejectAngleThresholdCosine;

	/**
	\brief Cosine of threshold angle for rejecting sweep hits.
	
	See PxVehicleSetSweepHitRejectionAngles() for details.

	<b>Range:</b> (1, -1)<br>
	<b>Default:</b> 0.707f (cosine of 45 degrees)

	@see PxVehicleSetSweepHitRejectionAngles()
	*/
	PxF32 normalRejectAngleThresholdCosine;

	/**
	\brief Maximum acceleration experienced by PxRigidDynamic instances that are found to be in contact with a wheel.
	
	See PxVehicleSetMaxHitActorAcceleration() for details.

	<b>Range:</b> [0, PX_MAX_REAL]<br>
	<b>Default:</b> PX_MAX_REAL

	@see PxVehicleSetMaxHitActorAcceleration()
	*/
	PxF32 maxHitActorAcceleration;


public:
	/**
	\brief Constructor sets to default.
	*/	
	PX_INLINE PxVehicleContext();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void setToDefault();

	/**
	\brief Check if the settings descriptor is valid.

	\return True if the current settings are valid.
	*/
	PX_INLINE bool isValid() const;

	/**
	\brief Compute the side-axis from the up- and forward-axis
	*/
	PX_INLINE void computeSideAxis();
};

PX_INLINE PxVehicleContext::PxVehicleContext():
	upAxis(0.0f, 1.0f, 0.0f),
	forwardAxis(0.0f, 0.0f, 1.0f),
	sideAxis(1.0f, 0.0f, 0.0f),
	updateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE),
	pointRejectAngleThresholdCosine(0.707f),  // cosine of 45 degrees
	normalRejectAngleThresholdCosine(0.707f), // cosine of 45 degrees
	maxHitActorAcceleration(PX_MAX_REAL)
{
}

PX_INLINE void PxVehicleContext::setToDefault()
{
	*this = PxVehicleContext();
}

PX_INLINE bool PxVehicleContext::isValid() const
{
	if (!upAxis.isNormalized())
		return false;

	if (!forwardAxis.isNormalized())
		return false;

	if (!sideAxis.isNormalized())
		return false;

	if (((upAxis.cross(forwardAxis)) - sideAxis).magnitude() > 0.02f)  // somewhat above 1 degree assuming both have unit length
		return false;

	if ((pointRejectAngleThresholdCosine >= 1) || (pointRejectAngleThresholdCosine <= -1))
		return false;

	if ((normalRejectAngleThresholdCosine >= 1) || (normalRejectAngleThresholdCosine <= -1))
		return false;

	if (maxHitActorAcceleration < 0.0f)
		return false;

	return true;
}

PX_INLINE void PxVehicleContext::computeSideAxis()
{
	sideAxis = upAxis.cross(forwardAxis);
}


/**
\brief Get the default vehicle context.

Will be used if the corresponding parameters are not specified in methods like
PxVehicleUpdates() etc.

To set the default values, see the methods PxVehicleSetBasisVectors(),
PxVehicleSetUpdateMode() etc.

\return The default vehicle context.

@see PxVehicleSetBasisVectors() PxVehicleSetUpdateMode()
*/
PX_DEPRECATED const PxVehicleContext& PxVehicleGetDefaultContext();


#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
