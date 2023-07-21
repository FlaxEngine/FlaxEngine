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

#ifndef PX_GPU_H
#define PX_GPU_H

#include "PxPhysXConfig.h"


#if PX_SUPPORT_GPU_PHYSX

#include "cudamanager/PxCudaContextManager.h"
#include "foundation/Px.h"
#include "foundation/PxPreprocessor.h"
#include "foundation/PxFoundation.h"
#include "common/PxPhysXCommonConfig.h"

/**
\brief PxGpuLoadHook

This is a helper class for loading the PhysXGpu dll. 
If a PhysXGpu dll with a non-default file name needs to be loaded, 
PxGpuLoadHook can be sub-classed to provide the custom filenames.

Once the names are set, the instance must be set for use by PhysX.dll using PxSetPhysXGpuLoadHook(), 

@see PxSetPhysXGpuLoadHook()
*/
class PxGpuLoadHook
{
public:
	PxGpuLoadHook() {}
	virtual ~PxGpuLoadHook() {}

	virtual const char* getPhysXGpuDllName() const = 0;

protected:
private:
};

/**
\brief Sets GPU load hook instance for PhysX dll.

\param[in] hook GPU load hook.

@see PxGpuLoadHook
*/
PX_C_EXPORT PX_PHYSX_CORE_API void PX_CALL_CONV PxSetPhysXGpuLoadHook(const PxGpuLoadHook* hook);

/**
 * \brief Ask the NVIDIA control panel which GPU has been selected for use by
 * PhysX.  Returns -1 if no PhysX capable GPU is found or GPU PhysX has
 * been disabled.
 */
PX_C_EXPORT PX_PHYSX_CORE_API int PX_CALL_CONV PxGetSuggestedCudaDeviceOrdinal(physx::PxErrorCallback& errc);

/**
 * \brief Allocate a CUDA Context manager, complete with heaps.
 * You only need one CUDA context manager per GPU device you intend to use for
 * CUDA tasks. 
 \param[in] foundation PhysXFoundation instance.
 \param[in] desc Cuda context manager desc.
 \param[in] profilerCallback PhysX profiler callback instance.

 @see PxGetProfilerCallback()
 */
PX_C_EXPORT PX_PHYSX_CORE_API physx::PxCudaContextManager* PX_CALL_CONV PxCreateCudaContextManager(physx::PxFoundation& foundation, const physx::PxCudaContextManagerDesc& desc, physx::PxProfilerCallback* profilerCallback = NULL);

/**
 * \brief Sets profiler callback to PhysX GPU
 \param[in] profilerCallback PhysX profiler callback instance.

 @see PxGetProfilerCallback()
 */
PX_C_EXPORT PX_PHYSX_CORE_API void PX_CALL_CONV PxSetPhysXGpuProfilerCallback(physx::PxProfilerCallback* profilerCallback);


/**
\brief Internally used callback to register function names of cuda kernels
*/
PX_C_EXPORT PX_PHYSX_CORE_API void PX_CALL_CONV PxCudaRegisterFunction(int moduleIndex, const char* functionName);

/**
\brief Internally used callback to register cuda modules at load time
*/
PX_C_EXPORT PX_PHYSX_CORE_API void** PX_CALL_CONV PxCudaRegisterFatBinary(void*);

/**
\brief Access to the registered cuda modules
*/
PX_C_EXPORT PX_PHYSX_CORE_API void** PX_CALL_CONV PxGetCudaModuleTable();

/**
\brief Number of registered cuda modules
*/
PX_C_EXPORT PX_PHYSX_CORE_API physx::PxU32 PX_CALL_CONV PxGetCudaModuleTableSize();

/**
\brief Access to the loaded cuda functions (kernels)
*/
PX_C_EXPORT PX_PHYSX_CORE_API physx::PxKernelIndex* PX_CALL_CONV PxGetCudaFunctionTable();

/**
\brief Number of loaded cuda functions (kernels)
*/
PX_C_EXPORT PX_PHYSX_CORE_API physx::PxU32 PX_CALL_CONV  PxGetCudaFunctionTableSize();


#endif // PX_SUPPORT_GPU_PHYSX

#endif
