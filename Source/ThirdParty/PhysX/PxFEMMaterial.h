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

#ifndef PX_FEM_MATERIAL_H
#define PX_FEM_MATERIAL_H
/** \addtogroup physics
@{
*/

#include "PxPhysXConfig.h"
#include "PxBaseMaterial.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxScene;

	/**
	\brief Material class to represent a set of FEM material properties.

	@see PxPhysics.createFEMSoftBodyMaterial
	*/
	class PxFEMMaterial : public PxBaseMaterial
	{
	public:

		/**
		\brief Sets young's modulus which defines the body's stiffness
	
		\param[in] young Young's modulus. <b>Range:</b> [0, PX_MAX_F32)

		@see getYoungsModulus()
		*/
		virtual		void	setYoungsModulus(PxReal young) = 0;

		/**
		\brief Retrieves the young's modulus value.

		\return The young's modulus value.

		@see setYoungsModulus()
		*/
		virtual		PxReal	getYoungsModulus() const = 0;

		/**
		\brief Sets the Poisson's ratio which defines the body's volume preservation. Completely incompressible materials have a poisson ratio of 0.5. Its value should not be set to exactly 0.5 because this leads to numerical problems.

		\param[in] poisson Poisson's ratio. <b>Range:</b> [0, 0.5)

		@see getPoissons()
		*/
		virtual		void	setPoissons(PxReal poisson) = 0;

		/**
		\brief Retrieves the Poisson's ratio.
		\return The Poisson's ratio.

		@see setPoissons()
		*/
		virtual		PxReal	getPoissons() const = 0;

		/**
		\brief Sets the dynamic friction value which defines the strength of resistance when two objects slide relative to each other while in contact.

		\param[in] dynamicFriction The dynamic friction value. <b>Range:</b> [0, PX_MAX_F32)

		@see getDynamicFriction()
		*/
		virtual		void	setDynamicFriction(PxReal dynamicFriction) = 0;

		/**
		\brief Retrieves the dynamic friction value
		\return The dynamic friction value

		@see setDynamicFriction()
		*/
		virtual		PxReal	getDynamicFriction() const = 0;

	protected:
		PX_INLINE			PxFEMMaterial(PxType concreteType, PxBaseFlags baseFlags) : PxBaseMaterial(concreteType, baseFlags)	{}
		PX_INLINE			PxFEMMaterial(PxBaseFlags baseFlags) : PxBaseMaterial(baseFlags) {}
		virtual				~PxFEMMaterial() {}
		virtual		bool	isKindOf(const char* name) const { return !::strcmp("PxFEMMaterial", name) || PxBaseMaterial::isKindOf(name); }
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
