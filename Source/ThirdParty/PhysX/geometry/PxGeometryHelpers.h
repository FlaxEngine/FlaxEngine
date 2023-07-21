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

#ifndef PX_GEOMETRY_HELPERS_H
#define PX_GEOMETRY_HELPERS_H
/** \addtogroup geomutils
@{
*/

#include "foundation/PxPlane.h"
#include "foundation/PxTransform.h"
#include "foundation/PxUnionCast.h"
#include "common/PxPhysXCommonConfig.h"
#include "geometry/PxGeometry.h"
#include "geometry/PxBoxGeometry.h"
#include "geometry/PxSphereGeometry.h"
#include "geometry/PxCapsuleGeometry.h"
#include "geometry/PxPlaneGeometry.h"
#include "geometry/PxConvexMeshGeometry.h"
#include "geometry/PxHeightFieldGeometry.h"
#include "geometry/PxParticleSystemGeometry.h"
#include "geometry/PxHairSystemGeometry.h"
#include "geometry/PxTetrahedronMeshGeometry.h"
#include "geometry/PxCustomGeometry.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Geometry holder class

This class contains enough space to hold a value of any PxGeometry subtype.

Its principal use is as a convenience class to allow geometries to be returned polymorphically from functions.
*/

PX_ALIGN_PREFIX(4)
class PxGeometryHolder
{
	class PxInvalidGeometry : public PxGeometry 
	{
	public:
		PX_INLINE PxInvalidGeometry() : PxGeometry(PxGeometryType::eINVALID)	{}
	};

public:
	PX_FORCE_INLINE PxGeometryType::Enum getType() const
	{
		return any().getType();
	}

	PX_FORCE_INLINE PxGeometry&	any()
	{
		return *PxUnionCast<PxGeometry*>(&bytes.geometry);
	}

	PX_FORCE_INLINE const PxGeometry&	any()	const
	{
		return *PxUnionCast<const PxGeometry*>(&bytes.geometry);
	}

//! @cond
	PX_FORCE_INLINE PxSphereGeometry&					sphere()				{ return get<PxSphereGeometry, PxGeometryType::eSPHERE>();							}
	PX_FORCE_INLINE const PxSphereGeometry&				sphere()		const	{ return get<const PxSphereGeometry, PxGeometryType::eSPHERE>();					}

	PX_FORCE_INLINE PxPlaneGeometry&					plane()					{ return get<PxPlaneGeometry, PxGeometryType::ePLANE>();							}
	PX_FORCE_INLINE const PxPlaneGeometry&				plane()			const	{ return get<const PxPlaneGeometry, PxGeometryType::ePLANE>();						}

	PX_FORCE_INLINE PxCapsuleGeometry&					capsule()				{ return get<PxCapsuleGeometry, PxGeometryType::eCAPSULE>();						}
	PX_FORCE_INLINE const PxCapsuleGeometry&			capsule()		const	{ return get<const PxCapsuleGeometry, PxGeometryType::eCAPSULE>();					}

	PX_FORCE_INLINE PxBoxGeometry&						box()					{ return get<PxBoxGeometry, PxGeometryType::eBOX>();								}
	PX_FORCE_INLINE const PxBoxGeometry&				box()			const	{ return get<const PxBoxGeometry, PxGeometryType::eBOX>();							}

	PX_FORCE_INLINE PxConvexMeshGeometry&				convexMesh()			{ return get<PxConvexMeshGeometry, PxGeometryType::eCONVEXMESH>();					}
	PX_FORCE_INLINE const PxConvexMeshGeometry&			convexMesh()	const	{ return get<const PxConvexMeshGeometry, PxGeometryType::eCONVEXMESH>();			}

	PX_FORCE_INLINE PxTetrahedronMeshGeometry&			tetMesh()				{ return get<PxTetrahedronMeshGeometry, PxGeometryType::eTETRAHEDRONMESH>();		}
	PX_FORCE_INLINE const PxTetrahedronMeshGeometry&	tetMesh()		const	{ return get<const PxTetrahedronMeshGeometry, PxGeometryType::eTETRAHEDRONMESH>();	}

	PX_FORCE_INLINE PxTriangleMeshGeometry&				triangleMesh()			{ return get<PxTriangleMeshGeometry, PxGeometryType::eTRIANGLEMESH>();				}
	PX_FORCE_INLINE const PxTriangleMeshGeometry&		triangleMesh()	const	{ return get<const PxTriangleMeshGeometry, PxGeometryType::eTRIANGLEMESH>();		}

	PX_FORCE_INLINE PxHeightFieldGeometry&				heightField()			{ return get<PxHeightFieldGeometry, PxGeometryType::eHEIGHTFIELD>();				}
	PX_FORCE_INLINE const PxHeightFieldGeometry&		heightField()	const	{ return get<const PxHeightFieldGeometry, PxGeometryType::eHEIGHTFIELD>();			}

	PX_FORCE_INLINE PxParticleSystemGeometry&			particleSystem()		{ return get<PxParticleSystemGeometry, PxGeometryType::ePARTICLESYSTEM>();			}
	PX_FORCE_INLINE const PxParticleSystemGeometry&		particleSystem() const	{ return get<const PxParticleSystemGeometry, PxGeometryType::ePARTICLESYSTEM>();	}

	PX_FORCE_INLINE PxHairSystemGeometry&				hairSystem()			{ return get<PxHairSystemGeometry, PxGeometryType::eHAIRSYSTEM>();					}
	PX_FORCE_INLINE const PxHairSystemGeometry&			hairSystem()	const	{ return get<const PxHairSystemGeometry, PxGeometryType::eHAIRSYSTEM>();			}

	PX_FORCE_INLINE PxCustomGeometry&					custom()				{ return get<PxCustomGeometry, PxGeometryType::eCUSTOM>();							}
	PX_FORCE_INLINE const PxCustomGeometry&				custom()		const	{ return get<const PxCustomGeometry, PxGeometryType::eCUSTOM>();					}
//! @endcond

	PX_FORCE_INLINE void storeAny(const PxGeometry& geometry)
	{
		PX_ASSERT_WITH_MESSAGE(	(geometry.getType() >= PxGeometryType::eSPHERE) &&
								(geometry.getType() < PxGeometryType::eGEOMETRY_COUNT),
								"Unexpected GeometryType in PxGeometryHolder::storeAny");

		switch(geometry.getType())
		{
		case PxGeometryType::eSPHERE:			put<PxSphereGeometry>(geometry);			break;
		case PxGeometryType::ePLANE:			put<PxPlaneGeometry>(geometry);				break;
		case PxGeometryType::eCAPSULE:			put<PxCapsuleGeometry>(geometry);			break;
		case PxGeometryType::eBOX:				put<PxBoxGeometry>(geometry);				break;
		case PxGeometryType::eCONVEXMESH:		put<PxConvexMeshGeometry>(geometry);		break;
		case PxGeometryType::eTRIANGLEMESH:		put<PxTriangleMeshGeometry>(geometry);		break;
		case PxGeometryType::eTETRAHEDRONMESH:	put<PxTetrahedronMeshGeometry>(geometry);	break;
		case PxGeometryType::eHEIGHTFIELD:		put<PxHeightFieldGeometry>(geometry);		break;
		case PxGeometryType::ePARTICLESYSTEM:	put<PxParticleSystemGeometry>(geometry);	break;
		case PxGeometryType::eHAIRSYSTEM:		put<PxHairSystemGeometry>(geometry);		break;
		case PxGeometryType::eCUSTOM:			put<PxCustomGeometry>(geometry);			break;
		case PxGeometryType::eGEOMETRY_COUNT:
		case PxGeometryType::eINVALID:														break;
		}
	}

	PX_FORCE_INLINE	PxGeometryHolder()							{ put<PxInvalidGeometry>(PxInvalidGeometry());	}
	PX_FORCE_INLINE	PxGeometryHolder(const PxGeometry& geometry){ storeAny(geometry);							}

	private:
		template<typename T> void put(const PxGeometry& geometry)
		{
			static_cast<T&>(any()) = static_cast<const T&>(geometry);
		}

		template<typename T, PxGeometryType::Enum type> T& get()
		{
			PX_ASSERT(getType() == type);
			return static_cast<T&>(any());
		}

		template<typename T, PxGeometryType::Enum type> T& get() const
		{
			PX_ASSERT(getType() == type);
			return static_cast<T&>(any());
		}

	union {
		PxU8	geometry[sizeof(PxGeometry)];
		PxU8	box[sizeof(PxBoxGeometry)];
		PxU8	sphere[sizeof(PxSphereGeometry)];
		PxU8	capsule[sizeof(PxCapsuleGeometry)];
		PxU8	plane[sizeof(PxPlaneGeometry)];
		PxU8	convex[sizeof(PxConvexMeshGeometry)];
		PxU8	tetMesh[sizeof(PxTetrahedronMeshGeometry)];
		PxU8	mesh[sizeof(PxTriangleMeshGeometry)];
		PxU8	heightfield[sizeof(PxHeightFieldGeometry)];
		PxU8	particleSystem[sizeof(PxParticleSystemGeometry)];
		PxU8	hairSystem[sizeof(PxHairSystemGeometry)];
		PxU8	custom[sizeof(PxCustomGeometry)];
	} bytes;
}
PX_ALIGN_SUFFIX(4);

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
