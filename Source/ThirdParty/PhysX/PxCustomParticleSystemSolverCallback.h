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

#ifndef PX_CUSTOM_PARTICLE_SYSTEM_SOLVER_CALLBACK_H
#define PX_CUSTOM_PARTICLE_SYSTEM_SOLVER_CALLBACK_H
/** \addtogroup physics
@{ */

#include "foundation/PxSimpleTypes.h"
#include "cudamanager/PxCudaTypes.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4435)
#endif
	
class PxGpuParticleSystem;


class PxCustomParticleSystemSolverCallback
{
public:
	/**
	\brief callback called when the particle solver begins.

	This is called once per frame. It occurs after external forces have been pre-integrated into the particle state.
	It is called prior to the particles being reordered by spatial hash index, so state can be accessed in the unsorted buffers only at this stage. This provides an opportunity to add custom
	forces and modifications to position or velocity.

	\param [in] gpuParticleSystem A pointer to the GPU particle system. This pointer points to a mirror of the particle system in device memory.
	\param [in] dt The time-step.
	\param [in] stream The CUDA stream currently being used by the particle system. Additional kernels should either be launched on this stream, or synchronization events should be used to avoid race conditions.
	*/
	virtual void onBegin(PxGpuParticleSystem* gpuParticleSystem, PxReal dt, CUstream stream) = 0;

	/**
	\brief callback called during the iterative particle solve stage.

	This callback will potentially be called multiple times between onBegin and onFinalize.

	\param [in] gpuParticleSystem A pointer to the GPU particle system. This pointer points to a mirror of the particle system in device memory.
	\param [in] dt The time-step.
	\param [in] stream The CUDA stream currently being used by the particle system. Additional kernels should either be launched on this stream, or synchronization events should be used to avoid race conditions.
	*/
	virtual void onSolve(PxGpuParticleSystem* gpuParticleSystem, PxReal dt, CUstream stream) = 0;

	/**
	\brief callback called after all solver iterations have completed.

	This callback will be called once per frame, after integration has completed.

	\param [in] gpuParticleSystem A pointer to the GPU particle system. This pointer points to a mirror of the particle system in device memory.
	\param [in] dt The time-step.
	\param [in] stream The CUDA stream currently being used by the particle system. Additional kernels should either be launched on this stream, or synchronization events should be used to avoid race conditions.
	*/
	virtual void onFinalize(PxGpuParticleSystem* gpuParticleSystem, PxReal dt, CUstream stream) = 0;

	/**
	\brief Destructor
	*/
	virtual ~PxCustomParticleSystemSolverCallback() {}
};


#if PX_VC
#pragma warning(pop)
#endif


#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
