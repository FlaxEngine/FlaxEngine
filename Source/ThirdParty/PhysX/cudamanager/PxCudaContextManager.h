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

#ifndef PX_CUDA_CONTEXT_MANAGER_H
#define PX_CUDA_CONTEXT_MANAGER_H

#include "foundation/PxPreprocessor.h"

#if PX_SUPPORT_GPU_PHYSX

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxErrorCallback.h"
#include "foundation/PxFlags.h"

#include "PxCudaTypes.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxCudaContext;

/** \brief Possible graphic/CUDA interoperability modes for context */
struct PxCudaInteropMode
{
    /**
     * \brief Possible graphic/CUDA interoperability modes for context
     */
	enum Enum
	{
		NO_INTEROP = 0,
		D3D10_INTEROP,
		D3D11_INTEROP,
		OGL_INTEROP,

		COUNT
	};
};

struct PxCudaInteropRegisterFlag
{
	enum Enum
	{
		eNONE           = 0x00,
		eREAD_ONLY      = 0x01,
		eWRITE_DISCARD  = 0x02,
		eSURFACE_LDST   = 0x04,
		eTEXTURE_GATHER = 0x08
	};
};

/**
\brief An interface class that the user can implement in order for PhysX to use a user-defined device memory allocator.
*/
class PxDeviceAllocatorCallback
{
public:

	/**
	\brief Allocated device memory.
	\param[in] ptr Pointer to store the allocated address
	\param[in] size The amount of memory required
	\return A boolean indicates the operation succeed or fail
	*/
	virtual bool memAlloc(void** ptr, size_t size) = 0;

	/**
	\brief Frees device memory.
	\param[in] ptr The memory to free
	\return A boolean indicates the operation succeed or fail
	*/
	virtual bool memFree(void* ptr) = 0;

protected:
	virtual ~PxDeviceAllocatorCallback() {}
};
/**
\brief collection of set bits defined in NxCudaInteropRegisterFlag.

@see NxCudaInteropRegisterFlag
*/
typedef PxFlags<PxCudaInteropRegisterFlag::Enum, uint32_t> PxCudaInteropRegisterFlags;
PX_FLAGS_OPERATORS(PxCudaInteropRegisterFlag::Enum, uint32_t)

//! \brief Descriptor used to create a PxCudaContextManager
class PxCudaContextManagerDesc
{
public:
    /**
     * \brief The CUDA context to manage
     *
     * If left NULL, the PxCudaContextManager will create a new context.  If
     * graphicsDevice is also not NULL, this new CUDA context will be bound to
     * that graphics device, enabling the use of CUDA/Graphics interop features.
     *
     * If ctx is not NULL, the specified context must be applied to the thread
     * that is allocating the PxCudaContextManager at creation time (aka, it
     * cannot be popped).  The PxCudaContextManager will take ownership of the
     * context until the manager is released.  All access to the context must be
     * gated by lock acquisition.
     *
     * If the user provides a context for the PxCudaContextManager, the context
     * _must_ have either been created on the GPU ordinal returned by
     * PxGetSuggestedCudaDeviceOrdinal() or on your graphics device.
     */
	CUcontext*	ctx;

    /**
     * \brief D3D device pointer or OpenGl context handle
     *
     * Only applicable when ctx is NULL, thus forcing a new context to be
     * created.  In that case, the created context will be bound to this
     * graphics device.
     */
	void*	graphicsDevice;

	/**
	  * \brief Application-specific GUID
	  *
	  * If your application employs PhysX modules that use CUDA you need to use a GUID 
	  * so that patches for new architectures can be released for your game.You can obtain a GUID for your 
	  * application from Nvidia.
	  */
	const char*	appGUID;

	/**
	  * \brief Application-specific device memory allocator
	  *
	  * the application can implement an device memory allocator, which inherites PxDeviceAllocatorCallback, and 
	  * pass that to the PxCudaContextManagerDesc. The SDK will use that allocator to allocate device memory instead of
	  * using the defaul CUDA device memory allocator.
	  */
	PxDeviceAllocatorCallback*	deviceAllocator;

	/**
     * \brief The CUDA/Graphics interop mode of this context
     *
     * If ctx is NULL, this value describes the nature of the graphicsDevice
     * pointer provided by the user.  Else it describes the nature of the
     * context provided by the user.
     */
	PxCudaInteropMode::Enum interopMode;

	PX_INLINE PxCudaContextManagerDesc() :
		ctx				(NULL),
		graphicsDevice	(NULL),
		appGUID			(NULL),
		deviceAllocator	(NULL),
		interopMode		(PxCudaInteropMode::NO_INTEROP)
	{
	}
};

/**
\brief A cuda kernel index providing an index to the cuda module and the function name
*/
struct PxKernelIndex
{
    PxU32 moduleIndex;
    const char* functionName;
};

/**
 * \brief Manages thread locks, and task scheduling for a CUDA context
 *
 * A PxCudaContextManager manages access to a single CUDA context, allowing it to
 * be shared between multiple scenes.
 * The context must be acquired from the manager before using any CUDA APIs unless stated differently.
 *
 * The PxCudaContextManager is based on the CUDA driver API and explicitly does not
 * support the CUDA runtime API (aka, CUDART).
 */
class PxCudaContextManager
{
public:
	/**
	* \brief Schedules clear operation for a device memory buffer on the specified stream
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void clearDeviceBufferAsync(T* deviceBuffer, PxU32 numElements, CUstream stream, PxI32 value = 0)
	{
		clearDeviceBufferAsyncInternal(deviceBuffer, numElements * sizeof(T), stream, value);
	}

	/**
	* \brief Copies a device buffer to the host
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void copyDToH(T* hostBuffer, const T* deviceBuffer, PxU32 numElements)
	{
		copyDToHInternal(hostBuffer, deviceBuffer, numElements * sizeof(T));
	}

	/**
	* \brief Copies a host buffer to the device
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void copyHToD(T* deviceBuffer, const T* hostBuffer, PxU32 numElements)
	{
		copyHToDInternal(deviceBuffer, hostBuffer, numElements * sizeof(T));
	}

	/**
	* \brief Schedules device to host copy operation on the specified stream
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void copyDToHAsync(T* hostBuffer, const T* deviceBuffer, PxU32 numElements, CUstream stream)
	{
		copyDToHAsyncInternal(hostBuffer, deviceBuffer, numElements * sizeof(T), stream);
	}

	/**
	* \brief Schedules host to device copy operation on the specified stream
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void copyHToDAsync(T* deviceBuffer, const T* hostBuffer, PxU32 numElements, CUstream stream)
	{
		copyHToDAsyncInternal(deviceBuffer, hostBuffer, numElements * sizeof(T), stream);
	}

	/**
	* \brief Schedules device to device copy operation on the specified stream
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void copyDToDAsync(T* dstDeviceBuffer, const T* srcDeviceBuffer, PxU32 numElements, CUstream stream)
	{
		copyDToDAsyncInternal(dstDeviceBuffer, srcDeviceBuffer, numElements * sizeof(T), stream);
	}

	/**
	* \brief Allocates a device buffer
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void allocDeviceBuffer(T*& deviceBuffer, PxU32 numElements, const char* filename = __FILE__, PxI32 line = __LINE__)
	{
		void* ptr = allocDeviceBufferInternal(numElements * sizeof(T), filename, line);
		deviceBuffer = reinterpret_cast<T*>(ptr);
	}

	/**
	* \brief Allocates a device buffer and returns the pointer to the memory
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	T* allocDeviceBuffer(PxU32 numElements, const char* filename = __FILE__, PxI32 line = __LINE__)
	{
		void* ptr = allocDeviceBufferInternal(numElements * sizeof(T), filename, line);
		return reinterpret_cast<T*>(ptr);
	}

	/**
	* \brief Frees a device buffer
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void freeDeviceBuffer(T*& deviceBuffer)
	{
		freeDeviceBufferInternal(deviceBuffer);
		deviceBuffer = NULL;
	}

	/**
	* \brief Allocates a pinned host buffer
	*
	* A pinned host buffer can be used on the gpu after getting a mapped device pointer from the pinned host buffer pointer, see getMappedDevicePtr
	* The cuda context will get acquired automatically
	* @see getMappedDevicePtr
	*/
	template<typename T>
	void allocPinnedHostBuffer(T*& pinnedHostBuffer, PxU32 numElements, const char* filename = __FILE__, PxI32 line = __LINE__)
	{
		void* ptr = allocPinnedHostBufferInternal(numElements * sizeof(T), filename, line);
		pinnedHostBuffer = reinterpret_cast<T*>(ptr);
	}

	/**
	* \brief Allocates a pinned host buffer and returns the pointer to the memory
	*
	* A pinned host buffer can be used on the gpu after getting a mapped device pointer from the pinned host buffer pointer, see getMappedDevicePtr
	* The cuda context will get acquired automatically
	* @see getMappedDevicePtr
	*/
	template<typename T>
	T* allocPinnedHostBuffer(PxU32 numElements, const char* filename = __FILE__, PxI32 line = __LINE__)
	{
		void* ptr = allocPinnedHostBufferInternal(numElements * sizeof(T), filename, line);
		return reinterpret_cast<T*>(ptr);
	}

	/**
	* \brief Frees a pinned host buffer
	*
	* The cuda context will get acquired automatically
	*/
	template<typename T>
	void freePinnedHostBuffer(T*& pinnedHostBuffer)
	{
		freePinnedHostBufferInternal(pinnedHostBuffer);
		pinnedHostBuffer = NULL;
	}

	/**
	* \brief Gets a mapped pointer from a pinned host buffer that can be used in cuda kernels directly
	*
	* Data access performance with a mapped pinned host pointer will be slower than using a device pointer directly 
	* but the changes done in the kernel will be available on the host immediately.
	* The cuda context will get acquired automatically
	*/
	virtual CUdeviceptr getMappedDevicePtr(void* pinnedHostBuffer) = 0;

    /**
     * \brief Acquire the CUDA context for the current thread
     *
     * Acquisitions are allowed to be recursive within a single thread.
     * You can acquire the context multiple times so long as you release
     * it the same count.
     *
     * The context must be acquired before using most CUDA functions.
     */
    virtual void acquireContext() = 0;

    /**
     * \brief Release the CUDA context from the current thread
     *
     * The CUDA context should be released as soon as practically
     * possible, to allow other CPU threads to work efficiently.
     */
    virtual void releaseContext() = 0;

	/**
	* \brief Return the CUcontext
	*/
	virtual CUcontext getContext() = 0;

	/**
	* \brief Return the CudaContext
	*/
	virtual PxCudaContext* getCudaContext() = 0;

    /**
     * \brief Context manager has a valid CUDA context
     *
     * This method should be called after creating a PxCudaContextManager,
     * especially if the manager was responsible for allocating its own
     * CUDA context (desc.ctx == NULL).
     */
    virtual bool contextIsValid() const = 0;

	/* Query CUDA context and device properties, without acquiring context */

    virtual bool supportsArchSM10() const = 0;  //!< G80
    virtual bool supportsArchSM11() const = 0;  //!< G92
    virtual bool supportsArchSM12() const = 0;  //!< GT200
    virtual bool supportsArchSM13() const = 0;  //!< GT260
    virtual bool supportsArchSM20() const = 0;  //!< GF100
    virtual bool supportsArchSM30() const = 0;  //!< GK100
	virtual bool supportsArchSM35() const = 0;  //!< GK110
	virtual bool supportsArchSM50() const = 0;  //!< GM100
	virtual bool supportsArchSM52() const = 0;  //!< GM200
	virtual bool supportsArchSM60() const = 0;  //!< GP100
	virtual bool isIntegrated() const = 0;      //!< true if GPU is an integrated (MCP) part
	virtual bool canMapHostMemory() const = 0;  //!< true if GPU map host memory to GPU (0-copy)
	virtual int  getDriverVersion() const = 0;  //!< returns cached value of cuGetDriverVersion()
	virtual size_t getDeviceTotalMemBytes() const = 0; //!< returns cached value of device memory size
	virtual int	getMultiprocessorCount() const = 0; //!< returns cache value of SM unit count
    virtual unsigned int getClockRate() const = 0; //!< returns cached value of SM clock frequency
    virtual int  getSharedMemPerBlock() const = 0; //!< returns total amount of shared memory available per block in bytes
	virtual int  getSharedMemPerMultiprocessor() const = 0; //!< returns total amount of shared memory available per multiprocessor in bytes
	virtual unsigned int getMaxThreadsPerBlock() const = 0; //!< returns the maximum number of threads per block
    virtual const char *getDeviceName() const = 0; //!< returns device name retrieved from driver
	virtual CUdevice getDevice() const = 0; //!< returns device handle retrieved from driver
	virtual PxCudaInteropMode::Enum getInteropMode() const = 0; //!< interop mode the context was created with

	virtual void setUsingConcurrentStreams(bool) = 0; //!< turn on/off using concurrent streams for GPU work
	virtual bool getUsingConcurrentStreams() const = 0; //!< true if GPU work can run in concurrent streams
    /* End query methods that don't require context to be acquired */

    /**
     * \brief Register a rendering resource with CUDA
     *
     * This function is called to register render resources (allocated
     * from OpenGL) with CUDA so that the memory may be shared
     * between the two systems.  This is only required for render
     * resources that are designed for interop use.  In APEX, each
     * render resource descriptor that could support interop has a
     * 'registerInCUDA' boolean variable.
     *
     * The function must be called again any time your graphics device
     * is reset, to re-register the resource.
     *
     * Returns true if the registration succeeded.  A registered
     * resource must be unregistered before it can be released.
     *
     * \param resource [OUT] the handle to the resource that can be used with CUDA
     * \param buffer [IN] GLuint buffer index to be mapped to cuda
     * \param flags [IN] cuda interop registration flags
     */
    virtual bool registerResourceInCudaGL(CUgraphicsResource& resource, uint32_t buffer, PxCudaInteropRegisterFlags flags = PxCudaInteropRegisterFlags()) = 0;

     /**
     * \brief Register a rendering resource with CUDA
     *
     * This function is called to register render resources (allocated
     * from Direct3D) with CUDA so that the memory may be shared
     * between the two systems.  This is only required for render
     * resources that are designed for interop use.  In APEX, each
     * render resource descriptor that could support interop has a
     * 'registerInCUDA' boolean variable.
     *
     * The function must be called again any time your graphics device
     * is reset, to re-register the resource.
     *
     * Returns true if the registration succeeded.  A registered
     * resource must be unregistered before it can be released.
     *
     * \param resource [OUT] the handle to the resource that can be used with CUDA
     * \param resourcePointer [IN] A pointer to either IDirect3DResource9, or ID3D10Device, or ID3D11Resource to be registered.
     * \param flags [IN] cuda interop registration flags
     */
    virtual bool registerResourceInCudaD3D(CUgraphicsResource& resource, void* resourcePointer, PxCudaInteropRegisterFlags flags = PxCudaInteropRegisterFlags()) = 0;

    /**
     * \brief Unregister a rendering resource with CUDA
     *
     * If a render resource was successfully registered with CUDA using
     * the registerResourceInCuda***() methods, this function must be called
     * to unregister the resource before the it can be released.
     */
    virtual bool unregisterResourceInCuda(CUgraphicsResource resource) = 0;

	/**
	 * \brief Determine if the user has configured a dedicated PhysX GPU in the NV Control Panel
	 * \note If using CUDA Interop, this will always return false
	 * \returns	1 if there is a dedicated GPU
	 *			0 if there is NOT a dedicated GPU
	 *			-1 if the routine is not implemented
	*/
	virtual int	usingDedicatedGPU() const = 0;

    /**
     * \brief Get the cuda modules that have been loaded into this context on construction
     * \return Pointer to the cuda modules
     */
	virtual CUmodule* getCuModules() = 0;

    /**
     * \brief Release the PxCudaContextManager
     *
     * If the PxCudaContextManager created the CUDA context it was 
	 * responsible for, it also frees that context.
     *
     * Do not release the PxCudaContextManager if there are any scenes
     * using it.  Those scenes must be released first.
     *
     */
	virtual void release() = 0;

protected:

    /**
     * \brief protected destructor, use release() method
     */
    virtual ~PxCudaContextManager() {}
	
	virtual void* allocDeviceBufferInternal(PxU32 numBytes, const char* filename = NULL, PxI32 line = -1) = 0;	
	virtual void* allocPinnedHostBufferInternal(PxU32 numBytes, const char* filename = NULL, PxI32 line = -1) = 0;
	
	virtual void freeDeviceBufferInternal(void* deviceBuffer) = 0;	
	virtual void freePinnedHostBufferInternal(void* pinnedHostBuffer) = 0;	 

	virtual void clearDeviceBufferAsyncInternal(void* deviceBuffer, PxU32 numBytes, CUstream stream, PxI32 value) = 0;
	 
	virtual void copyDToHAsyncInternal(void* hostBuffer, const void* deviceBuffer, PxU32 numBytes, CUstream stream) = 0;
	virtual void copyHToDAsyncInternal(void* deviceBuffer, const void* hostBuffer, PxU32 numBytes, CUstream stream) = 0;
	virtual void copyDToDAsyncInternal(void* dstDeviceBuffer, const void* srcDeviceBuffer, PxU32 numBytes, CUstream stream) = 0;
	 
	virtual void copyDToHInternal(void* hostBuffer, const void* deviceBuffer, PxU32 numBytes) = 0;
	virtual void copyHToDInternal(void* deviceBuffer, const void* hostBuffer, PxU32 numBytes) = 0;
};

#define PX_DEVICE_ALLOC(cudaContextManager, deviceBuffer, numElements) cudaContextManager->allocDeviceBuffer(deviceBuffer, numElements, __FILE__, __LINE__)
#define PX_DEVICE_ALLOC_T(T, cudaContextManager, numElements) cudaContextManager->allocDeviceBuffer<T>(numElements, __FILE__, __LINE__)
#define PX_DEVICE_FREE(cudaContextManager, deviceBuffer) cudaContextManager->freeDeviceBuffer(deviceBuffer);

#define PX_PINNED_HOST_ALLOC(cudaContextManager, pinnedHostBuffer, numElements) cudaContextManager->allocPinnedHostBuffer(pinnedHostBuffer, numElements, __FILE__, __LINE__)
#define PX_PINNED_HOST_ALLOC_T(T, cudaContextManager, numElements) cudaContextManager->allocPinnedHostBuffer<T>(numElements, __FILE__, __LINE__)
#define PX_PINNED_HOST_FREE(cudaContextManager, pinnedHostBuffer) cudaContextManager->freePinnedHostBuffer(pinnedHostBuffer);

/**
 * \brief Convenience class for holding CUDA lock within a scope
 */
class PxScopedCudaLock
{
public:
    /**
     * \brief ScopedCudaLock constructor
     */
	PxScopedCudaLock(PxCudaContextManager& ctx) : mCtx(&ctx)
	{
		mCtx->acquireContext();
	}

    /**
     * \brief ScopedCudaLock destructor
     */
	~PxScopedCudaLock()
	{
		mCtx->releaseContext();
	}

protected:

    /**
     * \brief CUDA context manager pointer (initialized in the constructor)
     */
    PxCudaContextManager* mCtx;
};

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif // PX_SUPPORT_GPU_PHYSX
#endif
