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

#ifndef PX_PARTICLE_CLOTH_COOKER_H
#define PX_PARTICLE_CLOTH_COOKER_H
/** \addtogroup extensions
  @{
*/

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxVec4.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

namespace ExtGpu
{

/**
\brief Holds all the information for a particle cloth constraint used in the PxParticleClothCooker.
*/
struct PxParticleClothConstraint
{
	enum
	{
		eTYPE_INVALID_CONSTRAINT = 0,
		eTYPE_HORIZONTAL_CONSTRAINT = 1,
		eTYPE_VERTICAL_CONSTRAINT = 2,
		eTYPE_DIAGONAL_CONSTRAINT = 4,
		eTYPE_BENDING_CONSTRAINT = 8,
		eTYPE_DIAGONAL_BENDING_CONSTRAINT = 16,
		eTYPE_ALL = eTYPE_HORIZONTAL_CONSTRAINT | eTYPE_VERTICAL_CONSTRAINT | eTYPE_DIAGONAL_CONSTRAINT | eTYPE_BENDING_CONSTRAINT | eTYPE_DIAGONAL_BENDING_CONSTRAINT
	};
	PxU32 particleIndexA;	//!< The first particle index of this constraint.
	PxU32 particleIndexB;	//!< The second particle index of this constraint.
	PxReal length;			//!< The distance between particle A and B.
	PxU32 constraintType;	//!< The type of constraint, see the constraint type enum.
};

/*
\brief Generates PxParticleClothConstraint constraints that connect the individual particles of a particle cloth.
*/
class PxParticleClothCooker
{
public:
	virtual void release() = 0;

	/**
	\brief Generate the constraint list and triangle index list.

	\param[in] constraints A pointer to an array of PxParticleClothConstraint constraints. If NULL, the cooker will generate all the constraints. Otherwise, the user-provided constraints will be added. 
	\param[in] numConstraints The number of user-provided PxParticleClothConstraint s. 
	*/
	virtual void cookConstraints(const PxParticleClothConstraint* constraints = NULL, const PxU32 numConstraints = 0) = 0;

	virtual PxU32* getTriangleIndices() = 0;					//!< \return A pointer to the triangle indices.
	virtual PxU32 getTriangleIndicesCount() = 0;				//!< \return The number of triangle indices.
	virtual PxParticleClothConstraint* getConstraints() = 0;	//!< \return A pointer to the PxParticleClothConstraint constraints.
	virtual PxU32 getConstraintCount() = 0;						//!< \return The number of constraints.
	virtual void calculateMeshVolume() = 0;						//!< Computes the volume of a closed mesh and the contraintScale. Expects vertices in local space - 'close' to origin.
	virtual PxReal getMeshVolume() = 0;							//!< \return The mesh volume calculated by PxParticleClothCooker::calculateMeshVolume.

protected:
	virtual ~PxParticleClothCooker() {}
};

} // namespace ExtGpu

/**
\brief Creates a PxParticleClothCooker.

\param[in] vertexCount The number of vertices of the particle cloth.
\param[in] inVertices The vertex positions of the particle cloth.
\param[in] triangleIndexCount The number of triangles of the cloth mesh.
\param[in] inTriangleIndices The triangle indices of the cloth mesh
\param[in] constraintTypeFlags The types of constraints to generate. See PxParticleClothConstraint.
\param[in] verticalDirection The vertical direction of the cloth mesh. This is needed to generate the correct horizontal and vertical constraints to model shear stiffness.
\param[in] bendingConstraintMaxAngle The maximum angle considered in the bending constraints.

\return A pointer to the new PxParticleClothCooker.
*/
ExtGpu::PxParticleClothCooker* PxCreateParticleClothCooker(PxU32 vertexCount, physx::PxVec4* inVertices, PxU32 triangleIndexCount, PxU32* inTriangleIndices,
	PxU32 constraintTypeFlags = ExtGpu::PxParticleClothConstraint::eTYPE_ALL, PxVec3 verticalDirection = PxVec3(0.0f,1.0f,0.0f), PxReal bendingConstraintMaxAngle = 20.0f/360.0f*PxTwoPi
);


#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
