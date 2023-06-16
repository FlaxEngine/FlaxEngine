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

#include "foundation/PxSimpleTypes.h"

class OmniPvdWriter;
namespace physx
{
class PxAllocatorCallback;
namespace vehicle2
{
struct PxVehiclePvdAttributeHandles;
struct PxVehiclePvdObjectHandles;
}
} 

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Create the attribute handles necessary to reflect vehicles in omnipvd.
\param[in] allocator is used to allocate the memory used to store the attribute handles.
\param[in] omniWriter is used to register the attribute handles with omnipvd.
\note omniWriter must be a valid pointer to an instance of OmniPvdWriter.
@see PxVehicleSimulationContext
@see PxVehiclePVDComponent
@see PxVehiclePvdAttributesRelease
*/
PxVehiclePvdAttributeHandles* PxVehiclePvdAttributesCreate
(PxAllocatorCallback& allocator, OmniPvdWriter* omniWriter);

/**
\brief Destory the attribute handles created by PxVehiclePvdAttributesCreate().
\param[in] allocator must be the instance used by PxVehiclePvdObjectCreate().
\param[in] omniWriter must point to the same OmniPvdWriter instance used for the complementary call to PxVehiclePvdAttributesCreate(). 
\param[in] attributeHandles is the PxVehiclePvdAttributeHandles created by PxVehiclePvdAttributesCreate().
@see PxVehiclePvdAttributesCreate
*/
void PxVehiclePvdAttributesRelease
(PxAllocatorCallback& allocator, OmniPvdWriter* omniWriter, PxVehiclePvdAttributeHandles* attributeHandles);

/**
\brief Create omnipvd objects that will be used to reflect an individual veicle in omnipvd.
\param[in] nbWheels must be greater than or equal to the number of wheels on the vehicle.
\param[in] nbAntirolls must be greater than or equal to the number of antiroll bars on the vehicle.
\param[in] maxNbPhysxMaterialFrictions must be greater than or equal to the number of PxPhysXMaterialFriction instances associated with any wheel of the vehicle.
\param[in] contextHandle is typically used to associated vehicles with a particular scene or group.
\param[in] attributeHandles is the PxVehiclePvdAttributeHandles instance returned by PxVehiclePvdAttributesCreate()
\param[in] omniWriter is used to register the attribute handles with omnipvd.
\param[in] allocator is used to allocate the memory used to store handles to the created omnipvd objects.
\note PxVehiclePvdObjectCreate() must be called after PxVehiclePvdAttributesCreate().
@see PxVehicleAxleDescription
@see PxVehicleAntiRollForceParams
@see PxVehiclePhysXMaterialFrictionParams
@see PxVehiclePVDComponent
@see PxVehiclePvdAttributesCreate
*/
PxVehiclePvdObjectHandles* PxVehiclePvdObjectCreate
(const PxU32 nbWheels, const PxU32 nbAntirolls, const PxU32 maxNbPhysxMaterialFrictions,
 const PxU64 contextHandle, const PxVehiclePvdAttributeHandles& attributeHandles, 
 OmniPvdWriter* omniWriter, PxAllocatorCallback& allocator);

/**
\brief Destroy the PxVehiclePvdObjectHandles instance created by PxVehiclePvdObjectCreate().
\param[in] attributeHandles is the PxVehiclePvdAttributeHandles created by PxVehiclePvdAttributesCreate().
\param[in] omniWriter is used to register the attribute handles with omnipvd.
\param[in] allocator must be the instance used by PxVehiclePvdObjectCreate().
\param[in] objectHandles is the PxVehiclePvdObjectHandles that was created by PxVehiclePvdObjectCreate().
*/
void PxVehiclePvdObjectRelease
(const PxVehiclePvdAttributeHandles& attributeHandles, 
 OmniPvdWriter* omniWriter, PxAllocatorCallback& allocator, PxVehiclePvdObjectHandles* objectHandles);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
