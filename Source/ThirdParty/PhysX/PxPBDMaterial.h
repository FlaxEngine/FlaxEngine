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

#ifndef PX_PBD_MATERIAL_H
#define PX_PBD_MATERIAL_H
/** \addtogroup physics
@{
*/

#include "PxParticleMaterial.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxScene;
	/**
	\brief Material class to represent a set of PBD particle material properties.

	@see #PxPhysics.createPBDMaterial
	*/
	class PxPBDMaterial : public PxParticleMaterial
	{
	public:
		
		/**
		\brief Sets viscosity
	
		\param[in] viscosity Viscosity. <b>Range:</b> [0, PX_MAX_F32)

		@see #getViscosity()
		*/
		virtual		void	setViscosity(PxReal viscosity) = 0;

		/**
		\brief Retrieves the viscosity value.

		\return The viscosity value.

		@see #setViscosity()
		*/
		virtual		PxReal	getViscosity() const = 0;

		/**
		\brief Sets material vorticity confinement coefficient

		\param[in] vorticityConfinement Material vorticity confinement coefficient. <b>Range:</b> [0, PX_MAX_F32)

		@see #getVorticityConfinement()
		*/
		virtual		void	setVorticityConfinement(PxReal vorticityConfinement) = 0;

		/**
		\brief Retrieves the vorticity confinement coefficient.
		\return The vorticity confinement coefficient.

		@see #setVorticityConfinement()
		*/
		virtual		PxReal	getVorticityConfinement() const = 0;

		/**
		\brief Sets material surface tension coefficient

		\param[in] surfaceTension Material surface tension coefficient. <b>Range:</b> [0, PX_MAX_F32)

		@see #getSurfaceTension()
		*/
		virtual		void	setSurfaceTension(PxReal surfaceTension) = 0;

		/**
		\brief Retrieves the surface tension coefficient.
		\return The surface tension coefficient.

		@see #setSurfaceTension()
		*/
		virtual		PxReal	getSurfaceTension() const = 0;

		/**
		\brief Sets material cohesion coefficient

		\param[in] cohesion Material cohesion coefficient. <b>Range:</b> [0, PX_MAX_F32)

		@see #getCohesion()
		*/
		virtual		void	setCohesion(PxReal cohesion) = 0;

		/**
		\brief Retrieves the cohesion coefficient.
		\return The cohesion coefficient.

		@see #setCohesion()
		*/
		virtual		PxReal	getCohesion() const = 0;

		/**
		\brief Sets material lift coefficient

		\param[in] lift Material lift coefficient. <b>Range:</b> [0, PX_MAX_F32)

		@see #getLift()
		*/
		virtual		void	setLift(PxReal lift) = 0;

		/**
		\brief Retrieves the lift coefficient.
		\return The lift coefficient.

		@see #setLift()
		*/
		virtual		PxReal	getLift() const = 0;

		/**
		\brief Sets material drag coefficient

		\param[in] drag Material drag coefficient. <b>Range:</b> [0, PX_MAX_F32)

		@see #getDrag()
		*/
		virtual		void	setDrag(PxReal drag) = 0;

		/**
		\brief Retrieves the drag coefficient.
		\return The drag coefficient.

		@see #setDrag()
		*/
		virtual		PxReal	getDrag() const = 0;

		/**
		\brief Sets the CFL coefficient.
		
		\param[in] coefficient CFL coefficient. This coefficient scales the CFL term used to limit relative motion between fluid particles. <b>Range:</b> [1.f, PX_MAX_F32)
		*/
		virtual		void	setCFLCoefficient(PxReal coefficient) = 0;

		/**
		\brief Retrieves the CFL coefficient.
		\return The CFL coefficient.

		@see #setCFLCoefficient()
		*/
		virtual		PxReal	getCFLCoefficient() const = 0;

		/**
		\brief Sets material particle friction scale. This allows the application to scale up/down the frictional effect between particles independent of the friction 
		coefficient, which also defines frictional behavior between the particle and rigid bodies/soft bodies/cloth etc.

		\param[in] scale particle friction scale. <b>Range:</b> [0, PX_MAX_F32)

		@see #getParticleFrictionScale()
		*/
		virtual		void	setParticleFrictionScale(PxReal scale) = 0;

		/**
		\brief Retrieves the particle friction scale.
		\return The particle friction scale.

		@see #setParticleFrictionScale()
		*/
		virtual		PxReal	getParticleFrictionScale() const = 0;

		/**
		\brief Sets material particle adhesion scale value. This is the adhesive value between particles defined as a scaled multiple of the adhesion parameter.

		\param[in] adhesion particle adhesion scale value. <b>Range:</b> [0, PX_MAX_F32)

		@see #getParticleAdhesionScale()
		*/
		virtual		void	setParticleAdhesionScale(PxReal adhesion) = 0;

		/**
		\brief Retrieves the particle adhesion scale value.
		\return The particle adhesion scale value.

		@see #setParticleAdhesionScale()
		*/
		virtual		PxReal	getParticleAdhesionScale() const = 0;

		virtual		const char*		getConcreteTypeName() const { return "PxPBDMaterial"; }

	protected:
		PX_INLINE			PxPBDMaterial(PxType concreteType, PxBaseFlags baseFlags) : PxParticleMaterial(concreteType, baseFlags) {}
		PX_INLINE			PxPBDMaterial(PxBaseFlags baseFlags) : PxParticleMaterial(baseFlags) {}
		virtual				~PxPBDMaterial() {}
		virtual		bool	isKindOf(const char* name) const { return !::strcmp("PxPBDMaterial", name) || PxParticleMaterial::isKindOf(name); }
	};

	class PxCustomMaterial : public PxParticleMaterial
	{
	public:

		virtual		const char*		getConcreteTypeName() const { return "PxCustomMaterial"; }

	protected:
		PX_INLINE			PxCustomMaterial(PxType concreteType, PxBaseFlags baseFlags) : PxParticleMaterial(concreteType, baseFlags) {}
		PX_INLINE			PxCustomMaterial(PxBaseFlags baseFlags) : PxParticleMaterial(baseFlags) {}
		virtual				~PxCustomMaterial() {}
		virtual		bool	isKindOf(const char* name) const { return !::strcmp("PxCustomMaterial", name) || PxParticleMaterial::isKindOf(name); }
	};
#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
