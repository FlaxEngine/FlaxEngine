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

#ifndef PX_PARTICLE_MATERIAL_H
#define PX_PARTICLE_MATERIAL_H
/** \addtogroup physics
@{
*/

#include "foundation/PxSimpleTypes.h"

#include "PxBaseMaterial.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Material class to represent a set of particle material properties.

@see #PxPhysics.createPBDMaterial, #PxPhysics.createFLIPMaterial, #PxPhysics.createMPMMaterial
*/
class PxParticleMaterial : public PxBaseMaterial
{
public:

	/**
	\brief Sets friction
	
	\param[in] friction Friction. <b>Range:</b> [0, PX_MAX_F32)

	@see #getFriction()
	*/
	virtual		void	setFriction(PxReal friction) = 0;

	/**
	\brief Retrieves the friction value.

	\return The friction value.

	@see #setFriction()
	*/
	virtual		PxReal	getFriction() const = 0;

	/**
	\brief Sets velocity damping term

	\param[in] damping Velocity damping term. <b>Range:</b> [0, PX_MAX_F32)

	@see #getDamping
	*/
	virtual		void	setDamping(PxReal damping) = 0;

	/**
	\brief Retrieves the velocity damping term
	\return The velocity damping term.

	@see #setDamping()
	*/
	virtual		PxReal	getDamping() const = 0;

	/**
	\brief Sets adhesion term

	\param[in] adhesion adhesion coefficient. <b>Range:</b> [0, PX_MAX_F32)

	@see #getAdhesion
	*/
	virtual		void	setAdhesion(PxReal adhesion) = 0;

	/**
	\brief Retrieves the adhesion term
	\return The adhesion term.

	@see #setAdhesion()
	*/
	virtual		PxReal	getAdhesion() const = 0;

	/**
	\brief Sets gravity scale term

	\param[in] scale gravity scale coefficient. <b>Range:</b> (-PX_MAX_F32, PX_MAX_F32)

	@see #getAdhesion
	*/
	virtual		void	setGravityScale(PxReal scale) = 0;

	/**
	\brief Retrieves the gravity scale term
	\return The gravity scale term.

	@see #setAdhesion()
	*/
	virtual		PxReal	getGravityScale() const = 0;

	/**
	\brief Sets material adhesion radius scale. This is multiplied by the particle rest offset to compute the fall-off distance
	at which point adhesion ceases to operate.

	\param[in] scale Material adhesion radius scale. <b>Range:</b> [0, PX_MAX_F32)

	@see #getAdhesionRadiusScale
	*/
	virtual		void	setAdhesionRadiusScale(PxReal scale) = 0;

	/**
	\brief Retrieves the adhesion radius scale.
	\return The adhesion radius scale.

	@see #setAdhesionRadiusScale()
	*/
	virtual		PxReal	getAdhesionRadiusScale() const = 0;

protected:
	PX_INLINE			PxParticleMaterial(PxType concreteType, PxBaseFlags baseFlags) : PxBaseMaterial(concreteType, baseFlags)	{}
	PX_INLINE			PxParticleMaterial(PxBaseFlags baseFlags) : PxBaseMaterial(baseFlags) {}
	virtual				~PxParticleMaterial() {}
	virtual		bool	isKindOf(const char* name) const { return !::strcmp("PxParticleMaterial", name) || PxBaseMaterial::isKindOf(name); }
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
