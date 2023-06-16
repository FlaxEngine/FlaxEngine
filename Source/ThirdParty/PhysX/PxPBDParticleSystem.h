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

#ifndef PX_PBD_PARTICLE_SYSTEM_H
#define PX_PBD_PARTICLE_SYSTEM_H
/** \addtogroup physics
@{ */

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxVec3.h"

#include "PxParticleSystem.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4435)
#endif

/**
\brief A particle system that uses the position based dynamics(PBD) solver.

The position based dynamics solver for particle systems supports behaviors like
fluid, cloth, inflatables etc. 

*/
class PxPBDParticleSystem : public PxParticleSystem
{
public:

	virtual                             ~PxPBDParticleSystem() {}
	
	/**
	\brief Set wind direction and intensity

	\param[in] wind The wind direction and intensity
	*/
	virtual     void                    setWind(const PxVec3& wind) = 0;

	/**
	\brief Retrieves the wind direction and intensity.

	\return The wind direction and intensity
	*/
	virtual     PxVec3                  getWind() const = 0;

	/**
	\brief Set the fluid boundary density scale

	Defines how strong of a contribution the boundary (typically a rigid surface) should have on a fluid particle's density.

	\param[in] fluidBoundaryDensityScale  <b>Range:</b> (0.0, 1.0)
	*/
	virtual     void                    setFluidBoundaryDensityScale(PxReal fluidBoundaryDensityScale) = 0;

	/**
	\brief Return the fluid boundary density scale
	\return the fluid boundary density scale

	See #setFluidBoundaryDensityScale()
	*/
	virtual     PxReal                  getFluidBoundaryDensityScale() const = 0;

	/**
	\brief Set the fluid rest offset

	Two fluid particles will come to rest at a distance equal to twice the fluidRestOffset value.

	\param[in] fluidRestOffset  <b>Range:</b> (0, particleContactOffset)
	*/
	virtual     void                    setFluidRestOffset(PxReal fluidRestOffset) = 0;

	/**
	\brief Return the fluid rest offset
	\return the fluid rest offset

	See #setFluidRestOffset()
	*/
	virtual     PxReal                  getFluidRestOffset() const = 0;

	/**
	\brief Set the particle system grid size x dimension

	\param[in] gridSizeX x dimension in the particle grid
	*/
	virtual     void                    setGridSizeX(PxU32 gridSizeX) = 0;

	/**
	\brief Set the particle system grid size y dimension

	\param[in] gridSizeY y dimension in the particle grid
	*/
	virtual     void                    setGridSizeY(PxU32 gridSizeY) = 0;

	/**
	\brief Set the particle system grid size z dimension

	\param[in] gridSizeZ z dimension in the particle grid
	*/
	virtual     void                    setGridSizeZ(PxU32 gridSizeZ) = 0;
	
	virtual     const char*             getConcreteTypeName() const PX_OVERRIDE { return "PxPBDParticleSystem"; }

protected:
	PX_INLINE                           PxPBDParticleSystem(PxType concreteType, PxBaseFlags baseFlags) : PxParticleSystem(concreteType, baseFlags) {}
	PX_INLINE                           PxPBDParticleSystem(PxBaseFlags baseFlags) : PxParticleSystem(baseFlags) {}
	virtual     bool                    isKindOf(const char* name) const PX_OVERRIDE { return !::strcmp("PxPBDParticleSystem", name) || PxParticleSystem::isKindOf(name); }
};

#if PX_VC
#pragma warning(pop)
#endif


#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
