// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

#include "NvCloth/Range.h"
#include "NvCloth/PhaseConfig.h"
#include <foundation/PxVec3.h>
#include "NvCloth/Allocator.h"

struct ID3D11Buffer;

namespace nv
{
namespace cloth
{

class Factory;
class Fabric;
class Cloth;

#ifdef _MSC_VER 
#pragma warning(disable : 4371) // layout of class may have changed from a previous version of the compiler due to
                                // better packing of member
#endif

template <typename T>
struct MappedRange : public Range<T>
{
	MappedRange(T* first, T* last, const Cloth& cloth, void (Cloth::*lock)() const, void (Cloth::*unlock)() const)
	: Range<T>(first, last), mCloth(cloth), mLock(lock), mUnlock(unlock)
	{
	}

	MappedRange(const MappedRange& other)
	: Range<T>(other), mCloth(other.mCloth), mLock(other.mLock), mUnlock(other.mUnlock)
	{
		(mCloth.*mLock)();
	}

	~MappedRange()
	{
		(mCloth.*mUnlock)();
	}

  private:
	MappedRange& operator = (const MappedRange&);

	const Cloth& mCloth;
	void (Cloth::*mLock)() const;
	void (Cloth::*mUnlock)() const;
};

struct GpuParticles
{
	physx::PxVec4* mCurrent;
	physx::PxVec4* mPrevious;
	ID3D11Buffer* mBuffer;
};

// abstract cloth instance
class Cloth : public UserAllocated
{
  protected:
	Cloth() {}
	Cloth(const Cloth&);
	Cloth& operator = (const Cloth&);

  public:
	virtual ~Cloth() {}

	/**	\brief Creates a duplicate of this Cloth instance.
		Same as:
		\code
		getFactory().clone(*this);
		\endcode
		*/
	virtual Cloth* clone(Factory& factory) const = 0;

	/** \brief Returns the fabric used to create this Cloth.*/
	virtual Fabric& getFabric() const = 0;
	/** \brief Returns the Factory used to create this Cloth.*/
	virtual Factory& getFactory() const = 0;

	/* particle properties */
	/// Returns the number of particles simulated by this fabric.
	virtual uint32_t getNumParticles() const = 0;
	/** \brief Used internally to synchronize CPU and GPU particle memory.*/
	virtual void lockParticles() const = 0; //Might be better if it was called map/unmapParticles
	/** \brief Used internally to synchronize CPU and GPU particle memory.*/
	virtual void unlockParticles() const = 0;

	/** \brief Returns the simulation particles of the current frame.
		Each PxVec4 element contains the particle position in the XYZ components and the inverse mass in the W component.
		The returned memory may be overwritten (to change attachment point locations for animation for example).
		Setting the inverse mass to 0 locks the particle in place.
		*/
	virtual MappedRange<physx::PxVec4> getCurrentParticles() = 0;

	/** \brief Returns the simulation particles of the current frame, read only.
		Similar to the non-const version of this function.
		This version is preferred as it doesn't wake up the cloth to account for the possibility that particles were changed.
	*/
	virtual MappedRange<const physx::PxVec4> getCurrentParticles() const = 0;

	/** \brief Returns the simulation particles of the previous frame.
		Similar to getCurrentParticles().
		*/
	virtual MappedRange<physx::PxVec4> getPreviousParticles() = 0;

	/** \brief Returns the simulation particles of the previous frame.
		Similar to getCurrentParticles() const.
	*/
	virtual MappedRange<const physx::PxVec4> getPreviousParticles() const = 0;

	/** \brief Returns platform dependent pointers to the current GPU particle memory.*/
	virtual GpuParticles getGpuParticles() = 0;


	/** \brief Set the translation of the local space simulation after next call to simulate(). 
		This applies a force to make the cloth behave as if it was moved through space.
		This does not move the particles as they are in local space.
		Use the graphics transformation matrices to render the cloth in the proper location.
		The applied force is proportional to the value set with Cloth::setLinearInertia().
		*/
	virtual void setTranslation(const physx::PxVec3& trans) = 0;

	/** \brief Set the rotation of the local space simulation after next call to simulate(). 
		Similar to Cloth::setTranslation().
		The applied force is proportional to the value set with Cloth::setAngularInertia() and Cloth::setCentrifugalInertia().
		*/
	virtual void setRotation(const physx::PxQuat& rot) = 0;

	/** \brief Returns the current translation value that was set using setTranslation().*/
	virtual const physx::PxVec3& getTranslation() const = 0;
	/** \brief Returns the current rotation value that was set using setRotation().*/
	virtual const physx::PxQuat& getRotation() const = 0;

	/** \brief Set inertia derived from setTranslation() and setRotation() to zero (once).*/
	virtual void clearInertia() = 0;

	/** \brief Adjust the position of the cloth without affecting the dynamics (to call after a world origin shift, for example). */
	virtual void teleport(const physx::PxVec3& delta) = 0;

	/** \brief Adjust the position and rotation of the cloth without affecting the dynamics (to call after a world origin shift, for example). 
		The velocity will be set to zero this frame, unless setTranslation/setRotation is called with a different value after this function is called.
		The correct order to use this is:
		\code
		cloth->teleportToLocation(pos, rot);
		pos += velocity * dt;
		rot += 0.5 * angularVelocity * rot * dt;
		cloth->setTranslation(pos);
		cloth->setRotation(rot);
		\endcode
		*/
	virtual void teleportToLocation(const physx::PxVec3& translation, const physx::PxQuat& rotation) = 0;

	/** \brief Don't recalculate the velocity based on the values provided by setTranslation and setRotation for one frame (so it acts as if the velocity was the same as last frame).
		This is useful when the cloth is moving while teleported, but the integration of the cloth position for that frame is already included in the teleport.
		Example:
		\code
		pos += velocity * dt;
		rot += 0.5 * angularVelocity * rot * dt;
		cloth->teleportToLocation(pos, rot);
		cloth->ignoreVelocityDiscontinuity();
		\endcode
		*/
	virtual void ignoreVelocityDiscontinuity() = 0;


	/* solver parameters */

	/** \brief Returns the delta time used for previous iteration.*/
	virtual float getPreviousIterationDt() const = 0;

	/** \brief Sets gravity in global coordinates.*/
	virtual void setGravity(const physx::PxVec3&) = 0;
	/// Returns gravity set with setGravity().
	virtual physx::PxVec3 getGravity() const = 0;

	/** \brief Sets damping of local particle velocity (1/stiffnessFrequency).
		0 (default): velocity is unaffected, 1: velocity is zeroed
	*/
	virtual void setDamping(const physx::PxVec3&) = 0;
	/// Returns value set with setDamping().
	virtual physx::PxVec3 getDamping() const = 0;

	// portion of local frame velocity applied to particles
	// 0 (default): particles are unaffected
	// same as damping: damp global particle velocity
	virtual void setLinearDrag(const physx::PxVec3&) = 0;
	virtual physx::PxVec3 getLinearDrag() const = 0;
	virtual void setAngularDrag(const physx::PxVec3&) = 0;
	virtual physx::PxVec3 getAngularDrag() const = 0;

	/** \brief Set the portion of local frame linear acceleration applied to particles.
		0: particles are unaffected, 1 (default): physically correct.
		*/
	virtual void setLinearInertia(const physx::PxVec3&) = 0;
	/// Returns value set with getLinearInertia().
	virtual physx::PxVec3 getLinearInertia() const = 0;
	/** \brief Similar to setLinearInertia(), but for angular inertia.*/
	virtual void setAngularInertia(const physx::PxVec3&) = 0;
	/// Returns value set with setAngularInertia().
	virtual physx::PxVec3 getAngularInertia() const = 0;
	/** \brief Similar to setLinearInertia(), but for centrifugal inertia.*/
	virtual void setCentrifugalInertia(const physx::PxVec3&) = 0;
	///Returns value set with setCentrifugalInertia().
	virtual physx::PxVec3 getCentrifugalInertia() const = 0;

	/** \brief Set target solver iterations per second.
		At least 1 iteration per frame will be solved regardless of the value set.
		*/
	virtual void setSolverFrequency(float) = 0;
	/// Returns gravity set with getSolverFrequency().*/
	virtual float getSolverFrequency() const = 0;

	// damp, drag, stiffness exponent per second
	virtual void setStiffnessFrequency(float) = 0;
	virtual float getStiffnessFrequency() const = 0;

	// filter width for averaging dt^2 factor of gravity and
	// external acceleration, in numbers of iterations (default=30).
	virtual void setAcceleationFilterWidth(uint32_t) = 0;
	virtual uint32_t getAccelerationFilterWidth() const = 0;

	// setup edge constraint solver iteration
	virtual void setPhaseConfig(Range<const PhaseConfig> configs) = 0;

	/* collision parameters */

	/** \brief Set spheres for collision detection.
		Elements of spheres contain PxVec4(x,y,z,r) where [x,y,z] is the center and r the radius of the sphere.
		The values currently in range[first, last[ will be replaced with the content of spheres.
		\code
		cloth->setSpheres(Range<const PxVec4>(), 0, cloth->getNumSpheres()); //Removes all spheres
		\endcode
	  */
	virtual void setSpheres(Range<const physx::PxVec4> spheres, uint32_t first, uint32_t last) = 0;
	virtual void setSpheres(Range<const physx::PxVec4> startSpheres, Range<const physx::PxVec4> targetSpheres) = 0;
	/// Returns the number of spheres currently set.
	virtual uint32_t getNumSpheres() const = 0;


	/** \brief Set indices for capsule collision detection.
		The indices define the spheres that form the end points between the capsule.
		Every two elements in capsules define one capsule.
		The values currently in range[first, last[ will be replaced with the content of capsules.
		Note that first and last are indices to whole capsules consisting of 2 indices each. So if
		 you want to update the first two capsules (without changing the total number of capsules)
		 you would use the following code:
		\code
		uint32_t capsules[4] = { 0,1,  1,2 }; //Define indices for 2 capsules
		//updates the indices of the first 2 capsules in cloth
		cloth->setCapsules(Range<const uint32_t>(capsules, capsules + 4), 0, 2);
		\endcode
	*/
	virtual void setCapsules(Range<const uint32_t> capsules, uint32_t first, uint32_t last) = 0;
	/// Returns the number of capsules (which is half the number of capsule indices).
	virtual uint32_t getNumCapsules() const = 0;

	/** \brief Sets plane values to be used with convex collision detection.
		The planes are specified in the form ax + by + cz + d = 0, where elements in planes contain PxVec4(x,y,z,d).
		[x,y,z] is required to be normalized.
		The values currently in range [first, last[ will be replaced with the content of planes.
		Use setConvexes to enable planes for collision detection.
	  */
	virtual void setPlanes(Range<const physx::PxVec4> planes, uint32_t first, uint32_t last) = 0;
	virtual void setPlanes(Range<const physx::PxVec4> startPlanes, Range<const physx::PxVec4> targetPlanes) = 0;
	/// Returns the number of planes currently set.
	virtual uint32_t getNumPlanes() const = 0;

	/** \brief Enable planes for collision.
		convexMasks must contain masks of the form (1<<planeIndex1)|(1<<planeIndex2)|...|(1<<planeIndexN).
		All planes masked in a single element of convexMasks form a single convex polyhedron.
		The values currently in range [first, last[ will be replaced with the content of convexMasks.
	  */
	virtual void setConvexes(Range<const uint32_t> convexMasks, uint32_t first, uint32_t last) = 0;
	/// Returns the number of convexMasks currently set.
	virtual uint32_t getNumConvexes() const = 0;

	/** \brief Set triangles for collision.
		Each triangle in the list is defined by of 3 vertices.
		The values currently in range [first, last[ will be replaced with the content of triangles.
		*/
	virtual void setTriangles(Range<const physx::PxVec3> triangles, uint32_t first, uint32_t last) = 0;
	virtual void setTriangles(Range<const physx::PxVec3> startTriangles, Range<const physx::PxVec3> targetTriangles, uint32_t first) = 0;
	/// Returns the number of triangles currently set.
	virtual uint32_t getNumTriangles() const = 0;

	/// Returns true if we use ccd
	virtual bool isContinuousCollisionEnabled() const = 0;
	/// Set if we use ccd or not (disabled by default)
	virtual void enableContinuousCollision(bool) = 0;

	// controls how quickly mass is increased during collisions
	virtual float getCollisionMassScale() const = 0;
	virtual void setCollisionMassScale(float) = 0;

	/** \brief Set the cloth collision shape friction coefficient.*/
	virtual void setFriction(float) = 0;
	///Returns value set with setFriction().
	virtual float getFriction() const = 0;

	// set virtual particles for collision handling.
	// each indices element consists of 3 particle
	// indices and an index into the lerp weights array.
	virtual void setVirtualParticles(Range<const uint32_t[4]> indices, Range<const physx::PxVec3> weights) = 0;
	virtual uint32_t getNumVirtualParticles() const = 0;
	virtual uint32_t getNumVirtualParticleWeights() const = 0;

	/* tether constraint parameters */

	/** \brief Set Tether constraint scale.
		1.0 is the original scale of the Fabric.
		0.0 disables tether constraints in the Solver.
		*/
	virtual void setTetherConstraintScale(float scale) = 0;
	///Returns value set with setTetherConstraintScale().
	virtual float getTetherConstraintScale() const = 0;
	/** \brief Set Tether constraint stiffness..
	1.0 is the default.
	<1.0 makes the constraints behave springy.
	*/
	virtual void setTetherConstraintStiffness(float stiffness) = 0;
	///Returns value set with setTetherConstraintStiffness().
	virtual float getTetherConstraintStiffness() const = 0;

	/* motion constraint parameters */

	/** \brief Returns reference to motion constraints (position, radius)
		The entire range must be written after calling this function.
	*/
	virtual Range<physx::PxVec4> getMotionConstraints() = 0;
	/** \brief Removes all motion constraints.
	*/
	virtual void clearMotionConstraints() = 0;
	virtual uint32_t getNumMotionConstraints() const = 0;
	virtual void setMotionConstraintScaleBias(float scale, float bias) = 0;
	virtual float getMotionConstraintScale() const = 0;
	virtual float getMotionConstraintBias() const = 0;
	virtual void setMotionConstraintStiffness(float stiffness) = 0;
	virtual float getMotionConstraintStiffness() const = 0;

	/* separation constraint parameters */

	// return reference to separation constraints (position, radius)
	// The entire range must be written after calling this function.
	virtual Range<physx::PxVec4> getSeparationConstraints() = 0;
	virtual void clearSeparationConstraints() = 0;
	virtual uint32_t getNumSeparationConstraints() const = 0;

	/* clear interpolation */

	// assign current to previous positions for
	// collision spheres, motion, and separation constraints
	virtual void clearInterpolation() = 0;

	/* particle acceleration parameters */

	// return reference to particle accelerations (in local coordinates)
	// The entire range must be written after calling this function.
	virtual Range<physx::PxVec4> getParticleAccelerations() = 0;
	virtual void clearParticleAccelerations() = 0;
	virtual uint32_t getNumParticleAccelerations() const = 0;

	/* wind */

	/** /brief Set wind in global coordinates. Acts on the fabric's triangles. */
	virtual void setWindVelocity(physx::PxVec3) = 0;
	///Returns value set with setWindVelocity().
	virtual physx::PxVec3 getWindVelocity() const = 0;
	/** /brief Sets the air drag coefficient. */
	virtual void setDragCoefficient(float) = 0;
	///Returns value set with setDragCoefficient().
	virtual float getDragCoefficient() const = 0;
	/** /brief Sets the air lift coefficient. */
	virtual void setLiftCoefficient(float) = 0;
	///Returns value set with setLiftCoefficient().
	virtual float getLiftCoefficient() const = 0;
	/** /brief Sets the fluid density used for air drag/lift calculations. */
	virtual void setFluidDensity(float) = 0;
	///Returns value set with setFluidDensity().
	virtual float getFluidDensity() const = 0;

	/* self collision */

	/** /brief Set the distance particles need to be separated from each other withing the cloth. */
	virtual void setSelfCollisionDistance(float distance) = 0;
	///Returns value set with setSelfCollisionDistance().
	virtual float getSelfCollisionDistance() const = 0;
	/** /brief Set the constraint stiffness for the self collision constraints. */
	virtual void setSelfCollisionStiffness(float stiffness) = 0;
	///Returns value set with setSelfCollisionStiffness().
	virtual float getSelfCollisionStiffness() const = 0;

	/** \brief Set self collision indices.
		Each index in the range indicates that the particle at that index should be used for self collision.
		If set to an empty range (default) all particles will be used.
	*/
	virtual void setSelfCollisionIndices(Range<const uint32_t>) = 0;
	///Returns the number of self collision indices set.
	virtual uint32_t getNumSelfCollisionIndices() const = 0;

	/* rest positions */

	// set rest particle positions used during self-collision
	virtual void setRestPositions(Range<const physx::PxVec4>) = 0;
	virtual uint32_t getNumRestPositions() const = 0;

	/* bounding box */

	/** \brief Returns current particle position bounds center in local space */
	virtual const physx::PxVec3& getBoundingBoxCenter() const = 0;
	/** \brief Returns current particle position bounds size in local space */
	virtual const physx::PxVec3& getBoundingBoxScale() const = 0;

	/* sleeping (disabled by default) */

	// max particle velocity (per axis) to pass sleep test
	virtual void setSleepThreshold(float) = 0;
	virtual float getSleepThreshold() const = 0;
	// test sleep condition every nth millisecond
	virtual void setSleepTestInterval(uint32_t) = 0;
	virtual uint32_t getSleepTestInterval() const = 0;
	// put cloth to sleep when n consecutive sleep tests pass
	virtual void setSleepAfterCount(uint32_t) = 0;
	virtual uint32_t getSleepAfterCount() const = 0;
	virtual uint32_t getSleepPassCount() const = 0;
	virtual bool isAsleep() const = 0;
	virtual void putToSleep() = 0;
	virtual void wakeUp() = 0;

	/**  \brief Set user data. Not used internally.	*/
	virtual void setUserData(void*) = 0;
	// Returns value set by setUserData().
	virtual void* getUserData() const = 0;
};

// wrappers to prevent non-const overload from marking particles dirty
inline MappedRange<const physx::PxVec4> readCurrentParticles(const Cloth& cloth)
{
	return cloth.getCurrentParticles();
}
inline MappedRange<const physx::PxVec4> readPreviousParticles(const Cloth& cloth)
{
	return cloth.getPreviousParticles();
}

} // namespace cloth
} // namespace nv
