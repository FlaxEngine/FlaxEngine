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

#ifndef PX_MPM_MATERIAL_H
#define PX_MPM_MATERIAL_H
/** \addtogroup physics
@{
*/

#include "PxParticleMaterial.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief MPM material models
	*/
	struct PxMPMMaterialModel
	{
		enum Enum
		{
			eATTACHED = 1 << 0,		//!< Marker to indicate that all particles with an attached material should be treated as attached to whatever object they are located in
			eNEO_HOOKEAN = 1 << 1,	//!< A Neo-Hookean material model will be used
		    eELASTIC = 1 << 2,		//!< A corotaional cauchy strain based material model will be used
		    eSNOW = 1 << 3,			//!< A corotaional cauchy strain based material model with strain limiting and hardening will be used
		    eSAND = 1 << 4,			//!< A Ducker-Prager elastoplasticity material model will be used 
			eVON_MISES = 1 << 5		//!< A von Mises material model will be used 
		};
	};

	/**
	\brief MPM surface types that influence interaction between particles and obstacles
	*/
	struct PxMPMSurfaceType
	{
		enum Enum
		{
			eDEFAULT = 0,		//!< Normal surface with friction in tangential direction
			eSTICKY = 1 << 0,	//!< Surface will always have friction in the tangential and the normal direction
			eSLIPPERY = 1 << 1	//!< Surface will not cause any friction
		};
	};

	/**
	\brief Optional MPM modes that improve the quality of fracture and/or cutting
	*/
	struct PxMPMCuttingFlag
	{
		enum Enum
		{
			eNONE = 0,							//!< No special processing to support cutting will be performed
			eSUPPORT_THIN_BLADES = 1 << 0,		//!< Special collision detection will be performed to improve support for blade like objects that are thinner than the mpm grid spacing
			eENABLE_DAMAGE_TRACKING = 1 << 1	//!< A damage value will get updated on every particle to simulate material weakening to get more realistic crack propagation
		};
	};
	typedef PxFlags<PxMPMCuttingFlag::Enum,PxU16> PxMPMCuttingFlags;
	PX_FLAGS_OPERATORS(PxMPMCuttingFlag::Enum,PxU16)

	class PxScene;

	/**
	\brief Material class to represent a set of MPM particle material properties.

	@see PxPhysics.createMPMMaterial
	*/
	class PxMPMMaterial : public PxParticleMaterial
	{
	public:
		/**
		\brief Sets stretch and shear damping which dampens stretch and shear motion of MPM bodies. The effect is comparable to viscosity for fluids.

		\param[in] stretchAndShearDamping The stretch and shear damping

		@see getStretchAndShearDamping()
		*/
		virtual		void	setStretchAndShearDamping(PxReal stretchAndShearDamping) = 0;

		/**
		\brief Retrieves the stretch and shear damping.

		\return The stretch and shear damping

		@see setStretchAndShearDamping()
		*/
		virtual PxReal getStretchAndShearDamping() const = 0;


		/**
		\brief Sets the rotational damping which dampens rotations of mpm bodies

		\param[in] rotationalDamping The rotational damping

		@see getRotationalDamping()
		*/
		virtual		void	setRotationalDamping(PxReal rotationalDamping) = 0;

		/**
		\brief Retrieves the rotational damping.

		\return The rotational damping

		@see setRotationalDamping()
		*/
		virtual PxReal getRotationalDamping() const = 0;


		/**
		\brief Sets density which influences the body's weight

		\param[in] density The material's density

		@see getDensity()
		*/
		virtual		void	setDensity(PxReal density) = 0;

		/**
		\brief Retrieves the density value.

		\return The density

		@see setDensity()
		*/
		virtual PxReal getDensity() const = 0;


		/**
		\brief Sets the material model which influences interaction between MPM particles

		\param[in] materialModel The material model

		@see getMaterialModel()
		*/
		virtual		void	setMaterialModel(PxMPMMaterialModel::Enum materialModel) = 0;
		
		/**
		\brief Retrieves the material model.

		\return The material model

		@see setMaterialModel()
		*/
		virtual PxMPMMaterialModel::Enum getMaterialModel() const = 0;


		/**
		\brief Sets the cutting flags which can enable damage tracking or thin blade support

		\param[in] cuttingFlags The cutting flags

		@see getCuttingFlags()
		*/
		virtual		void	setCuttingFlags(PxMPMCuttingFlags cuttingFlags) = 0;

		/**
		\brief Retrieves the cutting flags.

		\return The cutting flags

		@see setCuttingFlags()
		*/
		virtual PxMPMCuttingFlags getCuttingFlags() const = 0;


		/**
		\brief Sets the sand friction angle, only applied if the material model is set to sand

		\param[in] sandFrictionAngle The sand friction angle

		@see getSandFrictionAngle()
		*/
		virtual		void	setSandFrictionAngle(PxReal sandFrictionAngle) = 0;
		
		/**
		\brief Retrieves the sand friction angle.

		\return The sand friction angle

		@see setSandFrictionAngle()
		*/
		virtual PxReal	getSandFrictionAngle() const = 0;


		/**
		\brief Sets the yield stress, only applied if the material model is set to Von Mises

		\param[in] yieldStress The yield stress

		@see getYieldStress()
		*/
		virtual		void	setYieldStress(PxReal yieldStress) = 0;
		
		
		/**
		\brief Retrieves the yield stress.

		\return The yield stress

		@see setYieldStress()
		*/
		virtual PxReal getYieldStress() const = 0;


		/**
		\brief Set material to plastic

		\param[in] isPlastic True if plastic

		@see getIsPlastic()
		*/
		virtual		void	setIsPlastic(bool isPlastic) = 0;

		/**
		\brief Returns true if material is plastic

		\return True if plastic

		@see setIsPlastic()
		*/
		virtual		bool	getIsPlastic() const = 0;

		/**
		\brief Sets Young's modulus which defines the body's stiffness

		\param[in] young Young's modulus. <b>Range:</b> [0, PX_MAX_F32)

		@see getYoungsModulus()
		*/
		virtual		void	setYoungsModulus(PxReal young) = 0;

		/**
		\brief Retrieves the Young's modulus value.

		\return The Young's modulus value.

		@see setYoungsModulus()
		*/
		virtual		PxReal	getYoungsModulus() const = 0;

		/**
		\brief Sets Poisson's ratio defines the body's volume preservation. Completely incompressible materials have a Poisson ratio of 0.5 which will lead to numerical problems.

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
		\brief Sets material hardening coefficient

		Tendency to get more rigid under compression. <b>Range:</b> [0, PX_MAX_F32)

		\param[in] hardening Material hardening coefficient.

		@see getHardening
		*/
		virtual		void	setHardening(PxReal hardening) = 0;

		/**
		\brief Retrieves the hardening coefficient.

		\return The hardening coefficient.

		@see setHardening()
		*/
		virtual		PxReal	getHardening() const = 0;

		/**
		\brief Sets material critical compression coefficient

		Compression clamping threshold (higher means more compression is allowed before yield). <b>Range:</b> [0, 1)

		\param[in] criticalCompression Material critical compression coefficient.

		@see getCriticalCompression
		*/
		virtual		void	setCriticalCompression(PxReal criticalCompression) = 0;

		/**
		\brief Retrieves the critical compression coefficient.
		
		\return The criticalCompression coefficient.

		@see setCriticalCompression()
		*/
		virtual		PxReal	getCriticalCompression() const = 0;
		
		/**
		\brief Sets material critical stretch coefficient

		Stretch clamping threshold (higher means more stretching is allowed before yield). <b>Range:</b> [0, 1]

		\param[in] criticalStretch Material critical stretch coefficient.

		@see getCriticalStretch
		*/
		virtual		void	setCriticalStretch(PxReal criticalStretch) = 0;

		/**
		\brief Retrieves the critical stretch coefficient.

		\return The criticalStretch coefficient.

		@see setCriticalStretch()
		*/
		virtual		PxReal	getCriticalStretch() const = 0;
		
		/**
		\brief Sets material tensile damage sensitivity coefficient

		Sensitivity to tensile loads. The higher the sensitivity, the quicker damage will occur under tensile loads. <b>Range:</b> [0, PX_MAX_U32)

		\param[in] tensileDamageSensitivity Material tensile damage sensitivity coefficient.

		@see getTensileDamageSensitivity
		*/
		virtual		void	setTensileDamageSensitivity(PxReal tensileDamageSensitivity) = 0;

		/**
		\brief Retrieves the tensile damage sensitivity coefficient.

		\return The tensileDamageSensitivity coefficient.

		@see setTensileDamageSensitivity()
		*/
		virtual		PxReal	getTensileDamageSensitivity() const = 0;
		
		/**
		\brief Sets material compressive damage sensitivity coefficient

		Sensitivity to compressive loads. The higher the sensitivity, the quicker damage will occur under compressive loads <b>Range:</b> [0, PX_MAX_U32)

		\param[in] compressiveDamageSensitivity Material compressive damage sensitivity coefficient.

		@see getCompressiveDamageSensitivity
		*/
		virtual		void	setCompressiveDamageSensitivity(PxReal compressiveDamageSensitivity) = 0;

		/**
		\brief Retrieves the compressive damage sensitivity coefficient.

		\return The compressiveDamageSensitivity coefficient.

		@see setCompressiveDamageSensitivity()
		*/
		virtual		PxReal	getCompressiveDamageSensitivity() const = 0;
		
		/**
		\brief Sets material attractive force residual coefficient

		Relative amount of attractive force a fully damaged particle can exert on other particles compared to an undamaged one. <b>Range:</b> [0, 1]

		\param[in] attractiveForceResidual Material attractive force residual coefficient.

		@see getAttractiveForceResidual
		*/
		virtual		void	setAttractiveForceResidual(PxReal attractiveForceResidual) = 0;

		/**
		\brief Retrieves the attractive force residual coefficient.
		\return The attractiveForceResidual coefficient.

		@see setAttractiveForceResidual()
		*/
		virtual		PxReal	getAttractiveForceResidual() const = 0;

		virtual		const char*		getConcreteTypeName() const { return "PxMPMMaterial"; }

	protected:
		PX_INLINE			PxMPMMaterial(PxType concreteType, PxBaseFlags baseFlags) : PxParticleMaterial(concreteType, baseFlags) {}
		PX_INLINE			PxMPMMaterial(PxBaseFlags baseFlags) : PxParticleMaterial(baseFlags) {}
		virtual				~PxMPMMaterial() {}
		virtual		bool	isKindOf(const char* name) const { return !::strcmp("PxMPMMaterial", name) || PxParticleMaterial::isKindOf(name); }
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
