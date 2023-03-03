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

#ifndef PX_FEM_CLOTH_MATERIAL_H
#define PX_FEM_CLOTH_MATERIAL_H
/** \addtogroup physics
@{
*/

#include "PxFEMMaterial.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Material class to represent a set of FEM material properties.

	@see PxPhysics.createFEMClothMaterial
	*/
	class PxFEMClothMaterial : public PxFEMMaterial
	{
	public:

		/**
		\brief Sets material thickness

		\param[in] thickness Material thickness.

		@see getThickness
		*/
		virtual		void			setThickness(PxReal thickness) = 0;

		/**
		\brief Retrieves the material thickness.
		\return thickness.

		@see setDamping()
		*/
		virtual		PxReal			getThickness() const = 0;

		/**
		\brief Sets the elasticity damping for the internal cloth solver.

		\param[in] damping The elasticity damping term. <b>Range:</b> [0.0, Inf)

		@see getElasticityDamping()
		*/
		virtual		void			setElasticityDamping(PxReal damping) = 0;

		/**
		\brief Retrieves the elasticity damping term.
		\return The elasticity damping term.

		@see setElasticityDamping()
		*/
		virtual		PxReal			getElasticityDamping() const = 0;

		/**
		\brief Sets the bending coefficient for bending constraints.

		\param[in] damping The bending coefficient. <b>Range:</b> [0.0, Inf)

		@see getBendingDamping()
		*/
		virtual		void			setBendingDamping(PxReal damping) = 0;

		/**
		\brief Retrieves the bending coefficient for bending constraints.
		\return The bending coefficient.

		@see setBendingDamping()
		*/
		virtual		PxReal			getBendingDamping() const = 0;

		virtual		const char*		getConcreteTypeName() const { return "PxFEMClothMaterial"; }

	protected:
		PX_INLINE					PxFEMClothMaterial(PxType concreteType, PxBaseFlags baseFlags) : PxFEMMaterial(concreteType, baseFlags) {}
		PX_INLINE					PxFEMClothMaterial(PxBaseFlags baseFlags) : PxFEMMaterial(baseFlags) {}
		virtual						~PxFEMClothMaterial() {}
		virtual		bool			isKindOf(const char* name) const { return !::strcmp("PxFEMClothMaterial", name) || PxRefCounted::isKindOf(name); }
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
