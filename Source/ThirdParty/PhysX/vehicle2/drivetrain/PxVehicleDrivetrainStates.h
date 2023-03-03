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

#include "vehicle2/commands/PxVehicleCommandStates.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif


struct PxVehicleClutchCommandResponseState
{
	PxReal normalisedCommandResponse;
	PxReal commandResponse;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleClutchCommandResponseState));
	}
};

struct PxVehicleEngineDriveThrottleCommandResponseState
{
	PxReal commandResponse;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleEngineDriveThrottleCommandResponseState));
	}
};

struct PxVehicleEngineState
{
	/**
	\brief The rotation speed of the engine (radians per second).

	<b>Unit:</b> radians / time
	*/
	PxReal rotationSpeed;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleEngineState));
	}
};

#define PX_VEHICLE_NO_GEAR_SWITCH_PENDING -1.0f
#define PX_VEHICLE_GEAR_SWITCH_INITIATED -2.0f

struct PxVehicleGearboxState
{
	/**
	\brief Current gear
	*/
	PxU32 currentGear;

	/**
	\brief Target gear (different from current gear if a gear change is underway)
	*/
	PxU32 targetGear;

	/**
	\brief Reported time that has passed since gear change started.

	The special value PX_VEHICLE_NO_GEAR_SWITCH_PENDING denotes that there is currently
	no gear change underway.

	If a gear switch was initiated, the special value PX_VEHICLE_GEAR_SWITCH_INITIATED
	will be used temporarily but get translated to 0 in the gearbox update immediately.
	This state might only get encountered, if the vehicle component update is split into
	multiple sequences that do not run in one go.

	<b>Unit:</b> time
	*/
	PxReal gearSwitchTime;

	PX_FORCE_INLINE void setToDefault()
	{
		currentGear = 0;
		targetGear = 0;
		gearSwitchTime = PX_VEHICLE_NO_GEAR_SWITCH_PENDING;
	}
};

#define PX_VEHICLE_UNSPECIFIED_TIME_SINCE_LAST_SHIFT PX_MAX_F32

struct PxVehicleAutoboxState
{
	/**
	\brief Time that has lapsed since the last autobox gear shift.

	<b>Unit:</b> time
	*/
	PxReal timeSinceLastShift;

	/**
	\brief Describes whether a gear shift triggered by the autobox is still in flight.
	*/
	bool activeAutoboxGearShift;

	PX_FORCE_INLINE void setToDefault()
	{
		timeSinceLastShift = PX_VEHICLE_UNSPECIFIED_TIME_SINCE_LAST_SHIFT;
		activeAutoboxGearShift = false;
	}
};

struct PxVehicleDifferentialState
{
	/**
	\brief A list of wheel indices that are connected to the differential.
	*/
	PxU32 connectedWheels[PxVehicleLimits::eMAX_NB_WHEELS];

	/**
	\brief The number of wheels that are connected to the differential.
	*/
	PxU32 nbConnectedWheels;

	/**
	\brief The fraction of available torque that is delivered to each wheel through the differential.
	\note If a wheel is not connected to the differential then the fraction of available torque delivered to that wheel will be zero.
	\note A negative torque ratio for a wheel indicates a negative gearing is to be applied to that wheel. 
	\note The sum of the absolute value of each fraction must equal 1.0.
	*/
	PxReal torqueRatiosAllWheels[PxVehicleLimits::eMAX_NB_WHEELS];

	/**
	\brief The contribution of each wheel to the average wheel rotation speed measured at the clutch.
	\note If a wheel is not connected to the differential then the contribution to the average rotation speed measured at the clutch must be zero.
	\note The sum of all contributions must equal 1.0.
	*/
	PxReal aveWheelSpeedContributionAllWheels[PxVehicleLimits::eMAX_NB_WHEELS];

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleDifferentialState));
	}
};

/**
\brief Specify groups of wheels that are to be constrained to have pre-determined angular velocity relationship.
*/
struct PxVehicleWheelConstraintGroupState
{
	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleWheelConstraintGroupState));
	}

	/**
	\brief Add a wheel constraint group by specifying the number of wheels in the group, an array of wheel ids specifying each wheel in the group
	and a desired rotational speed relationship.
	\param[in] nbWheelsInGroupToAdd is the number of wheels in the group to be added.
	\param[in] wheelIdsInGroupToAdd is an array of wheel ids specifying all the wheels in the group to be added.
	\param[in] constraintMultipliers is an array of constraint multipliers describing the desired relationship of the wheel rotational speeds.
	\note constraintMultipliers[j] specifies the target rotational speed of the jth wheel in the constraint group as a multiplier of the rotational 
	speed of the zeroth wheel in the group. 
	*/
	void addConstraintGroup(const PxU32 nbWheelsInGroupToAdd, const PxU32* const wheelIdsInGroupToAdd, const PxF32* constraintMultipliers)
	{
		PX_ASSERT((nbWheelsInGroups + nbWheelsInGroupToAdd) < PxVehicleLimits::eMAX_NB_WHEELS);
		PX_ASSERT(nbGroups < PxVehicleLimits::eMAX_NB_WHEELS);
		nbWheelsPerGroup[nbGroups] = nbWheelsInGroupToAdd;
		groupToWheelIds[nbGroups] = nbWheelsInGroups;
		for (PxU32 i = 0; i < nbWheelsInGroupToAdd; i++)
		{
			wheelIdsInGroupOrder[nbWheelsInGroups + i] = wheelIdsInGroupToAdd[i];
			wheelMultipliersInGroupOrder[nbWheelsInGroups + i] = constraintMultipliers[i];
		}
		nbWheelsInGroups += nbWheelsInGroupToAdd;
		nbGroups++;
	}

	/**
	\brief Return the number of wheel constraint groups in the vehicle.
	\return The number of wheel constraint groups.
	@see getNbWheelsInConstraintGroup()
	*/
	PX_FORCE_INLINE PxU32 getNbConstraintGroups() const
	{
		return nbGroups;
	}

	/**
	\brief Return the number of wheels in the ith constraint group.
	\param[in] i specifies the constraint group to be queried for its wheel count.
	\return The number of wheels in the specified constraint group.
	@see getWheelInConstraintGroup()
	*/
	PX_FORCE_INLINE PxU32 getNbWheelsInConstraintGroup(const PxU32 i) const
	{
		return nbWheelsPerGroup[i];
	}

	/**
	\brief Return the wheel id of the jth wheel in the ith constraint group.
	\param[in] j specifies that the wheel id to be returned is the jth wheel in the list of wheels on the specified constraint group.
	\param[in] i specifies the constraint group to be queried.
	\return The wheel id of the jth wheel in the ith constraint group.
	@see getNbWheelsInConstraintGroup()
	*/
	PX_FORCE_INLINE PxU32 getWheelInConstraintGroup(const PxU32 j, const PxU32 i) const
	{
		return wheelIdsInGroupOrder[groupToWheelIds[i] + j];
	}

	/**
	\brief Return the constraint multiplier of the jth wheel in the ith constraint group
	\param[in] j specifies that the wheel id to be returned is the jth wheel in the list of wheels on the specified constraint group.
	\param[in] i specifies the constraint group to be queried.
	\return The constraint multiplier of the jth wheel in the ith constraint group.
	*/
	PX_FORCE_INLINE PxReal getMultiplierInConstraintGroup(const PxU32 j, const PxU32 i) const
	{
		return wheelMultipliersInGroupOrder[groupToWheelIds[i] + j];
	}

	PxU32 nbGroups;														//!< The number of constraint groups in the vehicle
	PxU32 nbWheelsPerGroup[PxVehicleLimits::eMAX_NB_AXLES];				//!< The number of wheels in each group
	PxU32 groupToWheelIds[PxVehicleLimits::eMAX_NB_AXLES];				//!< The list of wheel ids for the ith group begins at wheelIdsInGroupOrder[groupToWheelIds[i]]

	PxU32 wheelIdsInGroupOrder[PxVehicleLimits::eMAX_NB_WHEELS];		//!< The list of all wheel ids in constraint groups 
	PxF32 wheelMultipliersInGroupOrder[PxVehicleLimits::eMAX_NB_WHEELS];//!< The constraint multipliers for each constraint group.
	PxU32 nbWheelsInGroups;												//!< The number of wheels in a constraint group.
};

/**
\brief The clutch is modelled as two spinning plates with one connected to the wheels through the gearing and the other connected to the engine. The clutch slip is angular speed difference of the two plates.
*/
struct PxVehicleClutchSlipState
{
	/**
	\brief The slip at the clutch.

	<b>Unit:</b> radians / time
	*/
	PxReal clutchSlip;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleClutchSlipState));
	}
};




#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
