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

#include "foundation/PxPreprocessor.h"
#include "foundation/PxSimpleTypes.h"
#include "foundation/PxMemory.h"
#include "vehicle2/PxVehicleLimits.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif


/**
\brief A description of the state of commands that are applied to the vehicle
\note brakes[0] and brakes[1] may be used to distinguish brake and handbrake controls.
*/
struct PxVehicleCommandState
{
	PxReal brakes[2];	 //!< The instantaneous state of the brake controllers in range [0,1] with 1 denoting fully pressed and 0 fully depressed.
	PxU32 nbBrakes;		 //|< The number of brake commands.
	PxReal throttle;	 //!< The instantaneous state of the throttle controller in range [0,1] with 1 denoting fully pressed and 0 fully depressed.
	PxReal steer;		 //!< The instantaneous state of the steer controller in range [-1,1].

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleCommandState));
	}
};

/**
\brief A description of the state of transmission-related commands that are applied to a vehicle with direct drive.
*/
struct PxVehicleDirectDriveTransmissionCommandState
{
	/**
	\brief Direct drive vehicles only have reverse, neutral or forward gear.
	*/
	enum Enum
	{
		eREVERSE = 0,
		eNEUTRAL,
		eFORWARD
	};

	Enum gear;	//!< The desired gear of the input gear controller.

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleDirectDriveTransmissionCommandState));
	}
};

/**
\brief A description of the state of transmission-related commands that are applied to a vehicle with engine drive.
*/
struct PxVehicleEngineDriveTransmissionCommandState
{
	enum Enum
	{
		/**
		\brief Special gear value to denote the automatic shift mode (often referred to as DRIVE).
		
		When using automatic transmission, setting this value as target gear will enable automatic 
		gear shifts between first and highest gear. If the current gear is a reverse gear or
		the neutral gear, then this value will trigger a shift to first gear. If this value is
		used even though there is no automatic transmission available, the gear state will remain
		unchanged.
		*/
		eAUTOMATIC_GEAR = 0xff
	};

	PxReal clutch;		//!< The instantaneous state of the clutch controller in range [0,1] with 1 denoting fully pressed and 0 fully depressed.
	PxU32 targetGear;	//!< The desired gear of the input gear controller.

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleEngineDriveTransmissionCommandState));
	}
};

/**
\brief A description of the state of transmission-related commands that are applied to a vehicle with tank drive.
*/
struct PxVehicleTankDriveTransmissionCommandState : public PxVehicleEngineDriveTransmissionCommandState
{
	/**
	\brief The wheels of each tank track are either all connected to thrusts[0] or all connected to thrusts[1].
	\note The thrust commands are used to divert torque from the engine to the wheels of the tank tracks controlled by each thrust.
	\note thrusts[0] and thrusts[1] are in range [-1,1] with the sign dictating whether the thrust will be applied positively or negatively with respect to the gearing ratio.
	*/
	PxReal thrusts[2];

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTankDriveTransmissionCommandState));
	}
};


#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
