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

#ifndef PX_TYPE_INFO_H
#define PX_TYPE_INFO_H

/** \addtogroup common
@{
*/

#include "common/PxPhysXCommonConfig.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief an enumeration of concrete classes inheriting from PxBase

Enumeration space is reserved for future PhysX core types, PhysXExtensions, 
PhysXVehicle and Custom application types.

@see PxBase, PxTypeInfo
*/

struct PxConcreteType
{
	enum Enum
	{
		eUNDEFINED,

		eHEIGHTFIELD,
		eCONVEX_MESH,
		eTRIANGLE_MESH_BVH33,	// PX_DEPRECATED
		eTRIANGLE_MESH_BVH34,
		eTETRAHEDRON_MESH,
		eSOFTBODY_MESH,

		eRIGID_DYNAMIC,
		eRIGID_STATIC,
		eSHAPE,
		eMATERIAL,
		eSOFTBODY_MATERIAL,
		eCLOTH_MATERIAL,
		ePBD_MATERIAL,
		eFLIP_MATERIAL,
		eMPM_MATERIAL,
		eCUSTOM_MATERIAL,
		eCONSTRAINT,
		eAGGREGATE,
		eARTICULATION_REDUCED_COORDINATE,
		eARTICULATION_LINK,
		eARTICULATION_JOINT_REDUCED_COORDINATE,
		eARTICULATION_SENSOR,
		eARTICULATION_SPATIAL_TENDON,
		eARTICULATION_FIXED_TENDON,
		eARTICULATION_ATTACHMENT,
		eARTICULATION_TENDON_JOINT,
		ePRUNING_STRUCTURE,
		eBVH,
		eSOFT_BODY,
		eSOFT_BODY_STATE,
		ePBD_PARTICLESYSTEM,
		eFLIP_PARTICLESYSTEM,
		eMPM_PARTICLESYSTEM,
		eCUSTOM_PARTICLESYSTEM,
		eFEM_CLOTH,
		eHAIR_SYSTEM,
		ePARTICLE_BUFFER,
		ePARTICLE_DIFFUSE_BUFFER,
		ePARTICLE_CLOTH_BUFFER,
		ePARTICLE_RIGID_BUFFER,
		
		ePHYSX_CORE_COUNT,
        eFIRST_PHYSX_EXTENSION = 256,
		eFIRST_VEHICLE_EXTENSION = 512,
        eFIRST_USER_EXTENSION = 1024
	};
};

/** 
\brief a structure containing per-type information for types inheriting from PxBase

@see PxBase, PxConcreteType
*/

template<typename T> struct PxTypeInfo {};

#define PX_DEFINE_TYPEINFO(_name, _fastType) \
	class _name; \
	template <> struct PxTypeInfo<_name>	{	static const char* name() { return #_name;	}	enum { eFastTypeId = _fastType };	};

/* the semantics of the fastType are as follows: an object A can be cast to a type B if B's fastType is defined, and A has the same fastType.
 * This implies that B has no concrete subclasses or superclasses.
 */

PX_DEFINE_TYPEINFO(PxBase,									PxConcreteType::eUNDEFINED)
PX_DEFINE_TYPEINFO(PxMaterial,								PxConcreteType::eMATERIAL)
PX_DEFINE_TYPEINFO(PxFEMSoftBodyMaterial,					PxConcreteType::eSOFTBODY_MATERIAL)
PX_DEFINE_TYPEINFO(PxFEMClothMaterial,						PxConcreteType::eCLOTH_MATERIAL)
PX_DEFINE_TYPEINFO(PxPBDMaterial,							PxConcreteType::ePBD_MATERIAL)
PX_DEFINE_TYPEINFO(PxFLIPMaterial,							PxConcreteType::eFLIP_MATERIAL)
PX_DEFINE_TYPEINFO(PxMPMMaterial,							PxConcreteType::eMPM_MATERIAL)
PX_DEFINE_TYPEINFO(PxCustomMaterial,						PxConcreteType::eCUSTOM_MATERIAL)
PX_DEFINE_TYPEINFO(PxConvexMesh,							PxConcreteType::eCONVEX_MESH)
PX_DEFINE_TYPEINFO(PxTriangleMesh,							PxConcreteType::eUNDEFINED)
PX_DEFINE_TYPEINFO(PxBVH33TriangleMesh,						PxConcreteType::eTRIANGLE_MESH_BVH33)
PX_DEFINE_TYPEINFO(PxBVH34TriangleMesh,						PxConcreteType::eTRIANGLE_MESH_BVH34)
PX_DEFINE_TYPEINFO(PxTetrahedronMesh,						PxConcreteType::eTETRAHEDRON_MESH)
PX_DEFINE_TYPEINFO(PxHeightField,							PxConcreteType::eHEIGHTFIELD)
PX_DEFINE_TYPEINFO(PxActor,									PxConcreteType::eUNDEFINED)
PX_DEFINE_TYPEINFO(PxRigidActor,							PxConcreteType::eUNDEFINED)
PX_DEFINE_TYPEINFO(PxRigidBody,								PxConcreteType::eUNDEFINED)
PX_DEFINE_TYPEINFO(PxRigidDynamic,							PxConcreteType::eRIGID_DYNAMIC)
PX_DEFINE_TYPEINFO(PxRigidStatic,							PxConcreteType::eRIGID_STATIC)
PX_DEFINE_TYPEINFO(PxArticulationLink,						PxConcreteType::eARTICULATION_LINK)
PX_DEFINE_TYPEINFO(PxArticulationJointReducedCoordinate,	PxConcreteType::eARTICULATION_JOINT_REDUCED_COORDINATE)
PX_DEFINE_TYPEINFO(PxArticulationReducedCoordinate,			PxConcreteType::eARTICULATION_REDUCED_COORDINATE)
PX_DEFINE_TYPEINFO(PxAggregate,								PxConcreteType::eAGGREGATE)
PX_DEFINE_TYPEINFO(PxConstraint,							PxConcreteType::eCONSTRAINT)
PX_DEFINE_TYPEINFO(PxShape,									PxConcreteType::eSHAPE)
PX_DEFINE_TYPEINFO(PxPruningStructure,						PxConcreteType::ePRUNING_STRUCTURE)
PX_DEFINE_TYPEINFO(PxParticleSystem,                        PxConcreteType::eUNDEFINED)
PX_DEFINE_TYPEINFO(PxPBDParticleSystem,						PxConcreteType::ePBD_PARTICLESYSTEM)
PX_DEFINE_TYPEINFO(PxFLIPParticleSystem,					PxConcreteType::eFLIP_PARTICLESYSTEM)
PX_DEFINE_TYPEINFO(PxMPMParticleSystem,						PxConcreteType::eMPM_PARTICLESYSTEM)
PX_DEFINE_TYPEINFO(PxCustomParticleSystem,					PxConcreteType::eCUSTOM_PARTICLESYSTEM)
PX_DEFINE_TYPEINFO(PxSoftBody,								PxConcreteType::eSOFT_BODY)
PX_DEFINE_TYPEINFO(PxFEMCloth,								PxConcreteType::eFEM_CLOTH)
PX_DEFINE_TYPEINFO(PxHairSystem,							PxConcreteType::eHAIR_SYSTEM)
PX_DEFINE_TYPEINFO(PxParticleBuffer,						PxConcreteType::ePARTICLE_BUFFER)
PX_DEFINE_TYPEINFO(PxParticleAndDiffuseBuffer,				PxConcreteType::ePARTICLE_DIFFUSE_BUFFER)
PX_DEFINE_TYPEINFO(PxParticleClothBuffer,					PxConcreteType::ePARTICLE_CLOTH_BUFFER)
PX_DEFINE_TYPEINFO(PxParticleRigidBuffer,					PxConcreteType::ePARTICLE_RIGID_BUFFER)

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
