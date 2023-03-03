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
#include "foundation/PxAssert.h"
#include "foundation/PxMemory.h"
#include "foundation/PxVec3.h"
#include "foundation/PxMat33.h"

#include "PxVehicleLimits.h"

class OmniPvdWriter;

#if !PX_DOXYGEN
namespace physx
{
class PxConvexMesh;
class PxScene;
namespace vehicle2
{
#endif

struct PxVehicleAxleDescription
{
	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleAxleDescription));
	}

	/**
	\brief Add an axle to the vehicle by specifying the number of wheels on the axle and an array of wheel ids specifying each wheel on the axle.
	\param[in] nbWheelsOnAxle is the number of wheels on the axle to be added.
	\param[in] wheelIdsOnAxle is an array of wheel ids specifying all the wheels on the axle to be added.
	*/
	void addAxle(const PxU32 nbWheelsOnAxle, const PxU32* const wheelIdsOnAxle)
	{
		PX_ASSERT((nbWheels + nbWheelsOnAxle) < PxVehicleLimits::eMAX_NB_WHEELS);
		PX_ASSERT(nbAxles < PxVehicleLimits::eMAX_NB_AXLES);
		nbWheelsPerAxle[nbAxles] = nbWheelsOnAxle;
		axleToWheelIds[nbAxles] = nbWheels;
		for (PxU32 i = 0; i < nbWheelsOnAxle; i++)
		{
			wheelIdsInAxleOrder[nbWheels + i] = wheelIdsOnAxle[i];
		}
		nbWheels += nbWheelsOnAxle;
		nbAxles++;
	}

	/**
	\brief Return the number of axles on the vehicle.
	\return The number of axles.
	@see getNbWheelsOnAxle()
	*/
	PX_FORCE_INLINE PxU32 getNbAxles() const 
	{
		return nbAxles;
	}

	/**
	\brief Return the number of wheels on the ith axle.
	\param[in] i specifies the axle to be queried for its wheel count.
	\return The number of wheels on the specified axle.
	@see getWheelOnAxle()
	*/
	PX_FORCE_INLINE PxU32 getNbWheelsOnAxle(const PxU32 i) const 
	{
		return nbWheelsPerAxle[i];
	}

	/**
	\brief Return the wheel id of the jth wheel on the ith axle.
	\param[in] j specifies that the wheel id to be returned is the jth wheel in the list of wheels on the specified axle. 
	\param[in] i specifies the axle to be queried.
	\return The wheel id of the jth wheel on the ith axle.
	@see getNbWheelsOnAxle()
	*/
	PX_FORCE_INLINE PxU32 getWheelOnAxle(const PxU32 j, const PxU32 i) const
	{
		return wheelIdsInAxleOrder[axleToWheelIds[i] + j];
	}

	/**
	\brief Return the number of wheels on the vehicle.
	\return The number of wheels.
	*/
	PX_FORCE_INLINE PxU32 getNbWheels() const
	{
		return nbWheels;
	}

	/**
	\brief Return the axle of a specified wheel.
	\param[in] wheelId is the wheel whose axle is to be queried.
	\return The axle of the specified wheel.
	*/
	PX_FORCE_INLINE PxU32 getAxle(const PxU32 wheelId) const
	{
		for (PxU32 i = 0; i < getNbAxles(); i++)
		{
			for (PxU32 j = 0; j < getNbWheelsOnAxle(i); j++)
			{
				if (getWheelOnAxle(j, i) == wheelId)
					return i;
			}
		}
		return 0xffffffff;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(nbAxles > 0, "PxVehicleAxleDescription.nbAxles must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(nbWheels > 0, "PxVehicleAxleDescription.nbWheels must be greater than zero", false);
		return true;
	}

	PxU32 nbAxles;													//!< The number of axles on the vehicle
	PxU32 nbWheelsPerAxle[PxVehicleLimits::eMAX_NB_AXLES];			//!< The number of wheels on each axle.
	PxU32 axleToWheelIds[PxVehicleLimits::eMAX_NB_AXLES];			//!< The list of wheel ids for the ith axle begins at wheelIdsInAxleOrder[axleToWheelIds[i]]
	
	PxU32 wheelIdsInAxleOrder[PxVehicleLimits::eMAX_NB_WHEELS];		//!< The list of all wheel ids on the vehicle.	
	PxU32 nbWheels;													//!< The number of wheels on the vehicle.


	PX_COMPILE_TIME_ASSERT(PxVehicleLimits::eMAX_NB_AXLES == PxVehicleLimits::eMAX_NB_WHEELS);
	// It should be possible to support cases where each wheel is controlled individually and thus
	// having a wheel per axle for up to the max wheel count.
};


struct PxVehicleAxes
{
	enum Enum
	{
		ePosX = 0,  //!< The +x axis
		eNegX,      //!< The -x axis
		ePosY,      //!< The +y axis
		eNegY,      //!< The -y axis
		ePosZ,      //!< The +z axis
		eNegZ,      //!< The -z axis
		eMAX_NB_AXES
	};
};

struct PxVehicleFrame
{
	PxVehicleAxes::Enum lngAxis;  //!< The axis defining the longitudinal (forward) direction of the vehicle.
	PxVehicleAxes::Enum latAxis;  //!< The axis defining the lateral (side) direction of the vehicle.
	PxVehicleAxes::Enum vrtAxis;  //!< The axis defining the vertical (up) direction of the vehicle.

	PX_FORCE_INLINE void setToDefault()
	{
		lngAxis = PxVehicleAxes::ePosX;
		latAxis = PxVehicleAxes::ePosY;
		vrtAxis = PxVehicleAxes::ePosZ;
	}

	PX_FORCE_INLINE PxMat33 getFrame() const
	{
		const PxVec3 basisDirs[6] = { PxVec3(1,0,0), PxVec3(-1,0,0), PxVec3(0,1,0), PxVec3(0,-1,0), PxVec3(0,0,1), PxVec3(0,0,-1) };
		const PxMat33 mat33(basisDirs[lngAxis], basisDirs[latAxis], basisDirs[vrtAxis]);
		return mat33;
	}

	PX_FORCE_INLINE PxVec3 getLngAxis() const 
	{
		const PxVec3 basisDirs[6] = { PxVec3(1,0,0), PxVec3(-1,0,0), PxVec3(0,1,0), PxVec3(0,-1,0), PxVec3(0,0,1), PxVec3(0,0,-1) };
		return basisDirs[lngAxis];
	}

	PX_FORCE_INLINE PxVec3 getLatAxis() const
	{
		const PxVec3 basisDirs[6] = { PxVec3(1,0,0), PxVec3(-1,0,0), PxVec3(0,1,0), PxVec3(0,-1,0), PxVec3(0,0,1), PxVec3(0,0,-1) };
		return basisDirs[latAxis];
	}

	PX_FORCE_INLINE PxVec3 getVrtAxis() const
	{
		const PxVec3 basisDirs[6] = { PxVec3(1,0,0), PxVec3(-1,0,0), PxVec3(0,1,0), PxVec3(0,-1,0), PxVec3(0,0,1), PxVec3(0,0,-1) };
		return basisDirs[vrtAxis];
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(lngAxis < PxVehicleAxes::eMAX_NB_AXES, "PxVehicleFrame.lngAxis is invalid", false);
		PX_CHECK_AND_RETURN_VAL(latAxis < PxVehicleAxes::eMAX_NB_AXES, "PxVehicleFrame.latAxis is invalid", false);
		PX_CHECK_AND_RETURN_VAL(vrtAxis < PxVehicleAxes::eMAX_NB_AXES, "PxVehicleFrame.vrtAxis is invalid", false);
		const PxMat33 frame = getFrame();
		const PxQuat quat(frame);
		PX_CHECK_AND_RETURN_VAL(quat.isFinite() && quat.isUnit() && quat.isSane(), "PxVehicleFrame is not a legal frame", false);
		return true;
	}
};

struct PxVehicleScale
{
	PxReal scale;  //!< The length scale used for the vehicle. For example, if 1.0 is considered meters, then 100.0 would be for centimeters.

	PX_FORCE_INLINE void setToDefault()
	{
		scale = 1.0f;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(scale > 0.0f, "PxVehicleScale.scale must be greater than zero", false);
		return true;
	}
};

/**
\brief Helper struct to pass array type data to vehice components and functions.

The Vehicle SDK tries to give the user a certain freedom in how the parameters and
states are stored. This helper struct presents a way to either use array of structs
or array of pointers to structs to pass data into the provided vehicle components
and functions.
*/
template<typename T>
struct PxVehicleArrayData
{
	enum DataFormat
	{
		eARRAY_OF_STRUCTS = 0,  //!< The data is provided as an array of structs and stored in #arrayOfStructs.
		eARRAY_OF_POINTERS      //!< The data is provided as an array of pointers and stored in #arrayOfPointers.
	};

	/**
	\brief Set the data as an array of structs.

	\param[in] data The data as an array of structs.
	*/
	PX_FORCE_INLINE void setData(T* data)
	{
		arrayOfStructs = data;
		dataFormat = eARRAY_OF_STRUCTS;
	}

	/**
	\brief Set the data as an array of pointers.

	\param[in] data The data as an array of pointers.
	*/
	PX_FORCE_INLINE void setData(T*const* data)
	{
		arrayOfPointers= data;
		dataFormat = eARRAY_OF_POINTERS;
	}

	PX_FORCE_INLINE PxVehicleArrayData()
	{
	}

	PX_FORCE_INLINE explicit PxVehicleArrayData(T* data)
	{
		setData(data);
	}

	PX_FORCE_INLINE explicit PxVehicleArrayData(T*const* data)
	{
		setData(data);
	}

	/**
	\brief Get the data entry at a given index.

	\param[in] index The index to retrieve the data entry for.
	\return Reference to the requested data entry.
	*/
	PX_FORCE_INLINE T& getData(PxU32 index)
	{
		if (dataFormat == eARRAY_OF_STRUCTS)
			return arrayOfStructs[index];
		else
			return *arrayOfPointers[index];
	}

	PX_FORCE_INLINE T& operator[](PxU32 index)
	{
		return getData(index);
	}

	/**
	\brief Get the data entry at a given index.

	\param[in] index The index to retrieve the data entry for.
	\return Reference to the requested data entry.
	*/
	PX_FORCE_INLINE const T& getData(PxU32 index) const
	{
		if (dataFormat == eARRAY_OF_STRUCTS)
			return arrayOfStructs[index];
		else
			return *arrayOfPointers[index];
	}

	PX_FORCE_INLINE const T& operator[](PxU32 index) const
	{
		return getData(index);
	}

	/**
	\brief Set as empty.
	*/
	PX_FORCE_INLINE void setEmpty()
	{
		arrayOfStructs = NULL;
	}

	/**
	\brief Check if declared as empty.

	\return True if empty, else false.
	*/
	PX_FORCE_INLINE bool isEmpty() const
	{
		return (arrayOfStructs == NULL);
	}

	/**
	\brief Get a reference to the array but read only.

	\return Read only version of the data.
	*/
	PX_FORCE_INLINE const PxVehicleArrayData<const T>& getConst() const
	{
		return reinterpret_cast<const PxVehicleArrayData<const T>&>(*this);
	}


	union
	{
		T* arrayOfStructs;         //!< The data stored as an array of structs.
		T*const* arrayOfPointers;  //!< The data stored as an array of pointers.
	};
	PxU8 dataFormat;
};

template<typename T>
struct PxVehicleSizedArrayData : public PxVehicleArrayData<T>
{
	/**
	\brief Set the data as an array of structs and set the number of data entries.

	\param[in] data The data as an array of structs.
	\param[in] count The number of entries in the data array.
	*/
	PX_FORCE_INLINE void setDataAndCount(T* data, const PxU32 count)
	{
		PxVehicleArrayData<T>::setData(data);
		size = count;
	}

	/**
	\brief Set the data as an array of pointers and set the number of data entries.

	\param[in] data The data as an array of pointers.
	\param[in] count The number of entries in the data array.
	*/
	PX_FORCE_INLINE void setDataAndCount(T*const* data, const PxU32 count)
	{
		PxVehicleArrayData<T>::setData(data);
		size = count;
	}

	/**
	\brief Set as empty.
	*/
	PX_FORCE_INLINE void setEmpty()
	{
		PxVehicleArrayData<T>::setEmpty();
		size = 0;
	}

	/**
	\brief Check if declared as empty.

	\return True if empty, else false.
	*/
	PX_FORCE_INLINE bool isEmpty() const
	{
		return ((size == 0) || PxVehicleArrayData<T>::isEmpty());
	}

	PxU32 size;
};

/**
\brief Determine whether the PhysX actor associated with a vehicle is to be updated with a velocity change or an acceleration change.
A velocity change will be immediately reflected in linear and angular velocity queries against the vehicle.  An acceleration change, on the other hand,
will leave the linear and angular velocities unchanged until the next PhysX scene update has applied the acceleration update to the actor's linear and
angular velocities.
@see PxVehiclePhysXActorEndComponent
@see PxVehicleWriteRigidBodyStateToPhysXActor
*/
struct PxVehiclePhysXActorUpdateMode
{
	enum Enum
	{
		eAPPLY_VELOCITY = 0,
		eAPPLY_ACCELERATION
	};
};

/**
\brief Tire slip values are computed using ratios with potential for divide-by-zero errors. PxVehicleTireSlipParams
introduces a minimum value for the denominator of each of these ratios.
*/
struct PxVehicleTireSlipParams
{
	/**
	\brief The lateral slip angle is typically computed as a function of the ratio of lateral and longitudinal speeds
	of the rigid body in the tire's frame. This leads to a divide-by-zero in the event that the longitudinal speed
	approaches zero. The parameter minLatSlipDenominator sets a minimum denominator for the ratio of speeds used to
	compute the lateral slip angle.
	\note Larger timesteps typically require larger values of minLatSlipDenominator.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> velocity = length / time
	*/
	PxReal minLatSlipDenominator;

	/**
	\brief The longitudinal slip represents the difference between the longitudinal speed of the rigid body in the tire's
	frame and the linear speed arising from the rotation of the wheel. This is typically normalized using the reciprocal
	of the longitudinal speed of the rigid body in the tire's frame. This leads to a divide-by-zero in the event that the
	longitudinal speed approaches zero. The parameter minPassiveLongSlipDenominator sets a minimum denominator for the normalized
	longitudinal slip when the wheel experiences zero drive torque and zero brake torque and zero handbrake torque. The aim is
	to bring the vehicle to rest without experiencing wheel rotational speeds that oscillate around zero.
	\note The vehicle will come to rest more smoothly with larger values of minPassiveLongSlipDenominator, particularly
	with large timesteps that often lead to oscillation in wheel rotation speeds when the wheel rotation speed approaches
	zero.
	\note It is recommended that minActiveLongSlipDenominator < minPassiveLongSlipDenominator.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> velocity = length / time
	*/
	PxReal minPassiveLongSlipDenominator;

	/**
	\brief The longitudinal slip represents the difference between the longitudinal speed of the rigid body in the tire's
	frame and the linear speed arising from the rotation of the wheel. This is typically normalized using the reciprocal
	of the longitudinal speed of the rigid body in the tire's frame. This leads to a divide-by-zero in the event that the
	longitudinal speed approaches zero. The parameter minActiveLongSlipDenominator sets a minimum denominator for the normalized
	longitudinal slip when the wheel experiences either a non-zero drive torque or a non-zero brake torque or a non-zero handbrake
	torque.
	\note Larger timesteps typically require larger values of minActiveLongSlipDenominator to avoid instabilities occurring when
	the vehicle is aggressively throttled from rest.
	\note It is recommended that minActiveLongSlipDenominator < minPassiveLongSlipDenominator.

	<b>Range:</b> (0, inf)<br>
	<b>Unit:</b> velocity = length / time
	*/
	PxReal minActiveLongSlipDenominator;

	PX_FORCE_INLINE void setToDefault()
	{
		minLatSlipDenominator = 1.0f;
		minActiveLongSlipDenominator = 0.1f;
		minPassiveLongSlipDenominator = 4.0f;
	}

	PX_FORCE_INLINE PxVehicleTireSlipParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PxVehicleTireSlipParams p = *this;
		const PxReal scaleRatio = trgScale.scale / srcScale.scale;
		p.minLatSlipDenominator *= scaleRatio;
		p.minPassiveLongSlipDenominator *= scaleRatio;
		p.minActiveLongSlipDenominator *= scaleRatio;
		return p;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(minLatSlipDenominator > 0.0f, "PxVehicleTireSlipParams.minLatSlipDenominator must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(minPassiveLongSlipDenominator > 0.0f, "PxVehicleTireSlipParams.minPassiveLongSlipDenominator must be greater than zero", false);
		PX_CHECK_AND_RETURN_VAL(minActiveLongSlipDenominator > 0.0f, "PxVehicleTireSlipParams.minActiveLongSlipDenominator must be greater than zero", false);
		return true;
	}
};

/**
\brief Tires have two important directions for the purposes of tire force computation: longitudinal and lateral.
*/
struct PxVehicleTireDirectionModes
{
	enum Enum
	{
		eLONGITUDINAL = 0,
		eLATERAL,
		eMAX_NB_PLANAR_DIRECTIONS
	};
};

/**
\brief The low speed regime often presents numerical difficulties for the tire model due to the potential for divide-by-zero errors.
This particularly affects scenarios where the vehicle is slowing down due to damping and drag. In scenarios where there is no
significant brake or drive torque, numerical error begins to dominate and it can be difficult to bring the vehicle to rest. A solution
to this problem is to recognise that the vehicle is close to rest and to replace the tire forces with velocity constraints that will
bring the vehicle to rest. This regime is known as the "sticky tire" regime. PxVehicleTireAxisStickyParams describes velocity and time
thresholds that categorise the "sticky tire" regime. It also describes the rate at which the velocity constraints approach zero speed.
*/
struct PxVehicleTireAxisStickyParams
{
	/**
	\brief A tire enters the "sticky tire" regime when it has been below a speed specified by #thresholdSpeed for a continuous time
	specified by #thresholdTime.

	<b>Range:</b> [0, inf)<br>
	<b>Unit:</b> velocity = length / time
	*/
	PxReal thresholdSpeed;

	/**
	\brief A tire enters the "sticky tire" regime when it has been below a speed specified by #thresholdSpeed for a continuous time
	specified by #thresholdTime.

	<b>Range:</b> [0, inf)<br>
	<b>Unit:</b> time
	*/
	PxReal thresholdTime;

	/**
	\brief The rate at which the velocity constraint approaches zero is controlled by the damping parameter.
	\note Larger values of damping lead to faster approaches to zero. Since the damping behaves like a
	      stiffness with respect to the velocity, too large a value can lead to instabilities.

	<b>Range:</b> [0, inf)<br>
	<b>Unit:</b> 1 / time (acceleration instead of force based damping, thus not mass/time)
	*/
	PxReal damping;


	PX_FORCE_INLINE PxVehicleTireAxisStickyParams transformAndScale( 
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PxVehicleTireAxisStickyParams p = *this;
		const PxReal scaleRatio = trgScale.scale / srcScale.scale;
		p.thresholdSpeed *= scaleRatio;
		return p;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(thresholdSpeed >= 0.0f, "PxVehicleTireAxisStickyParams.thresholdSpeed must be greater than or equal to zero", false);
		PX_CHECK_AND_RETURN_VAL(thresholdTime >= 0.0f, "PxVehicleTireAxisStickyParams.thresholdTime must be greater than or equal to zero", false);
		PX_CHECK_AND_RETURN_VAL(damping >= 0.0f, "PxVehicleTireAxisStickyParams.damping must be greater than or equal to zero", false);
		return true;
	}
};

/**
\brief For each tire, the forces of the tire model may be replaced by velocity constraints when the tire enters the "sticky tire"
regime. The "sticky tire" regime of the lateral and longitudinal directions of the tire are managed separately.
*/
struct PxVehicleTireStickyParams
{
	/**
	The "sticky tire" regime of the lateral and longitudinal directions of the tire are managed separately and are individually
	parameterized.
	*/
	PxVehicleTireAxisStickyParams stickyParams[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];

	PX_FORCE_INLINE void setToDefault()
	{
		stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL].thresholdSpeed = 0.2f;
		stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL].thresholdTime = 1.0f;
		stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL].damping = 1.0f; 
		stickyParams[PxVehicleTireDirectionModes::eLATERAL].thresholdSpeed = 0.2f;
		stickyParams[PxVehicleTireDirectionModes::eLATERAL].thresholdTime = 1.0f;
		stickyParams[PxVehicleTireDirectionModes::eLATERAL].damping = 0.1f;
	}

	PX_FORCE_INLINE PxVehicleTireStickyParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PxVehicleTireStickyParams p = *this;
		p.stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL] = 
			stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL].transformAndScale(srcFrame, trgFrame, srcScale, trgScale);
		p.stickyParams[PxVehicleTireDirectionModes::eLATERAL] = 
			stickyParams[PxVehicleTireDirectionModes::eLATERAL].transformAndScale(srcFrame, trgFrame, srcScale, trgScale);
		return p;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		if (!stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL].isValid())
			return false;
		if (!stickyParams[PxVehicleTireDirectionModes::eLATERAL].isValid())
			return false;
		return true;
	}
};

struct PxVehicleSimulationContextType
{
	enum Enum
	{
		eDEFAULT,    //!< The simulation context inherits from PxVehicleSimulationContext
		ePHYSX       //!< The simulation context inherits from PxVehiclePhysXSimulationContext
	};
};

/**
\brief Structure to support Omni PVD, the PhysX Visual Debugger.
*/
struct PxVehiclePvdContext
{
public:
	PX_FORCE_INLINE void setToDefault()
	{
		attributeHandles = NULL;
		writer = NULL;
	}


	/**
	\brief The attribute handles used to reflect vehicle parameter and state data in omnipvd.
	\note A null value will result in no values being reflected in omnipvd.
	\note #attributeHandles and #writer both need to be non-NULL to reflect vehicle values in omnipvd. 
	@see PxVehiclePvdAttributesCreate
	@see PxVehiclePvdAttributesRelease
	@see PxVehiclePVDComponent
	*/
	const struct PxVehiclePvdAttributeHandles* attributeHandles;

	/**
	\brief An instance of OmniPvdWriter used to write vehicle prameter and state data to omnipvd.
	\note A null value will result in no values being reflected in omnipvd.
	\note #attributeHandles and #writer both need to be non-NULL to reflect vehicle values in omnipvd. 
	@see PxVehiclePvdAttributesCreate
	@see PxVehiclePvdAttributesRelease
	@see PxVehiclePVDComponent
	*/
	OmniPvdWriter* writer;
};

struct PxVehicleSimulationContext
{
	PxVehicleSimulationContext()
		: type(PxVehicleSimulationContextType::eDEFAULT)
	{}

	PxVec3 gravity;

	PxVehicleFrame frame;
	PxVehicleScale scale;

	//Tire 
	PxVehicleTireSlipParams tireSlipParams;
	PxVehicleTireStickyParams tireStickyParams;

	/**
	\brief Forward wheel speed below which the wheel rotation speed gets blended with the rolling speed.

	The blended rotation speed is used to integrate the wheel rotation angle. At low forward wheel speed, 
	the wheel rotation speed can get unstable (depending on the tire model used) and, for example, oscillate.
	
	\note If brake or throttle is applied, there will be no blending.

	<b>Unit:</b> velocity = length / time
	*/
	PxReal thresholdForwardSpeedForWheelAngleIntegration;

	/**
	\brief Structure to support Omni PVD, the PhysX Visual Debugger.
	*/
	PxVehiclePvdContext pvdContext;

protected:
	PxVehicleSimulationContextType::Enum type;


public:
	PX_FORCE_INLINE PxVehicleSimulationContextType::Enum getType() const { return type; }

	PX_FORCE_INLINE void setToDefault()
	{
		frame.setToDefault();
		scale.setToDefault();

		gravity = frame.getVrtAxis() * (-9.81f * scale.scale);

		tireSlipParams.setToDefault();
		tireStickyParams.setToDefault();

		thresholdForwardSpeedForWheelAngleIntegration = 5.0f * scale.scale;

		pvdContext.setToDefault();
	}

	PX_FORCE_INLINE PxVehicleSimulationContext transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PxVehicleSimulationContext c = *this;
		const PxReal scaleRatio = trgScale.scale / srcScale.scale;
		c.gravity = trgFrame.getFrame()*srcFrame.getFrame().getTranspose()*c.gravity;
		c.gravity *= scaleRatio;
		c.tireSlipParams = tireSlipParams.transformAndScale(srcFrame, trgFrame, srcScale, trgScale);
		c.tireStickyParams = tireStickyParams.transformAndScale(srcFrame, trgFrame, srcScale, trgScale);
		c.thresholdForwardSpeedForWheelAngleIntegration *= scaleRatio;
		c.frame = trgFrame;
		c.scale = trgScale;
		return c;
	}
};

struct PxVehiclePhysXSimulationContext : public PxVehicleSimulationContext
{
	PxVehiclePhysXSimulationContext()
		: PxVehicleSimulationContext()
	{
		type = PxVehicleSimulationContextType::ePHYSX;
	}

	//Road geometry queries to find the plane under the wheel.
	const PxConvexMesh* physxUnitCylinderSweepMesh;
	const PxScene* physxScene;

	//PhysX actor update
	PxVehiclePhysXActorUpdateMode::Enum physxActorUpdateMode;

	/**
	\brief Wake counter value to set on the physx actor if a reset is required.

	Certain vehicle states should keep a physx actor of a vehicle awake. This
	will be achieved by resetting the wake counter value if needed. The wake
	counter value is the minimum simulation time that a physx actor will stay
	awake.

	<b>Unit:</b> time
	
	@see physxActorWakeCounterThreshold PxVehiclePhysxActorKeepAwakeCheck
	*/
	PxReal physxActorWakeCounterResetValue;

	/**
	\brief Threshold below which to check whether the physx actor wake counter
	       should get reset.

	<b>Unit:</b> time
	
	@see physxActorWakeCounterResetValue PxVehiclePhysxActorKeepAwakeCheck
	*/
	PxReal physxActorWakeCounterThreshold;


	PX_FORCE_INLINE void setToDefault()
	{
		PxVehicleSimulationContext::setToDefault();

		physxUnitCylinderSweepMesh = NULL;
		physxScene = NULL;

		physxActorUpdateMode = PxVehiclePhysXActorUpdateMode::eAPPLY_VELOCITY;

		physxActorWakeCounterResetValue = 20.0f * 0.02f;  // 20 timesteps of size 0.02
		physxActorWakeCounterThreshold = 0.5f * physxActorWakeCounterResetValue;
	}

	PX_FORCE_INLINE PxVehiclePhysXSimulationContext transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PxVehiclePhysXSimulationContext r = *this;
		static_cast<PxVehicleSimulationContext&>(r) = PxVehicleSimulationContext::transformAndScale(srcFrame, trgFrame, srcScale, trgScale);
		return r;
	}
};

/**
* \brief Express a function as a sequence of points {(x, y)} that form a piecewise polynomial.
*/
template <class T, unsigned int NB_ELEMENTS>
class PxVehicleFixedSizeLookupTable
{
public:

	PxVehicleFixedSizeLookupTable()
		: nbDataPairs(0)
	{
	}

	PxVehicleFixedSizeLookupTable(const PxVehicleFixedSizeLookupTable& src)
	{
		PxMemCopy(xVals, src.xVals, sizeof(PxReal)* src.nbDataPairs);
		PxMemCopy(yVals, src.yVals, sizeof(T)*src.nbDataPairs);
		nbDataPairs = src.nbDataPairs;
	}

	~PxVehicleFixedSizeLookupTable()
	{
	}

	PxVehicleFixedSizeLookupTable& operator=(const PxVehicleFixedSizeLookupTable& src)
	{
		PxMemCopy(xVals, src.xVals, sizeof(PxReal)*src.nbDataPairs);
		PxMemCopy(yVals, src.yVals, sizeof(T)*src.nbDataPairs);
		nbDataPairs = src.nbDataPairs;
		return *this;
	}

	/**
	\brief Add one more point to create one more polynomial segment of a piecewise polynomial.
	*/
	PX_FORCE_INLINE bool addPair(const PxReal x, const T y)
	{
		PX_CHECK_AND_RETURN_VAL(nbDataPairs < NB_ELEMENTS, "PxVehicleFixedSizeLookupTable::addPair() exceeded fixed size capacity", false);
		xVals[nbDataPairs] = x;
		yVals[nbDataPairs] = y;
		nbDataPairs++;
		return true;
	}

	/**
	\brief Identify the segment of the piecewise polynomial that includes x and compute the corresponding y value by linearly interpolating the gradient of the segment.
	\param[in] x is the value on the x-axis of the piecewise polynomial.
	\return Returns the y value that corresponds to the input x. 
	*/
	PX_FORCE_INLINE T interpolate(const PxReal x) const
	{
		if (0 == nbDataPairs)
		{
			return T(0);
		}

		if (1 == nbDataPairs || x < xVals[0])
		{
			return yVals[0];
		}

		PxReal x0 = xVals[0];
		T y0 = yVals[0];

		for (PxU32 i = 1; i < nbDataPairs; i++)
		{
			const PxReal x1 = xVals[i];
			const T y1 = yVals[i];

			if ((x >= x0) && (x < x1))
			{
				return (y0 + (y1 - y0) * (x - x0) / (x1 - x0));
			}

			x0 = x1;
			y0 = y1;
		}

		PX_ASSERT(x >= xVals[nbDataPairs - 1]);
		return yVals[nbDataPairs - 1];
	}

	void clear()
	{
		PxMemSet(xVals, 0, NB_ELEMENTS * sizeof(PxReal));
		PxMemSet(yVals, 0, NB_ELEMENTS * sizeof(T));
		nbDataPairs = 0;
	}

	PxReal xVals[NB_ELEMENTS];
	T yVals[NB_ELEMENTS];
	PxU32 nbDataPairs;

	PX_FORCE_INLINE bool isValid() const
	{
		for (PxU32 i = 1; i < nbDataPairs; i++)
		{
			PX_CHECK_AND_RETURN_VAL(xVals[i] > xVals[i - 1], "PxVehicleFixedSizeLookupTable:: xVals[i+1] must be greater than xVals[i]", false);
		}
		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif




/** @} */
