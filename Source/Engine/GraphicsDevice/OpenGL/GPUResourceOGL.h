// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Config.h"

#if GRAPHICS_API_OPENGL

#include "Engine/Graphics/GPUResource.h"
#include "Engine/Core/Log.h"
#include "IncludeOpenGLHeaders.h"
#include "GPUDeviceOGL.h"

/// <summary>
/// Describes base implementation of Graphics Device resource for OpenGL (BaseType must be GPUResource or inherit from)
/// </summary>
template<class BaseType>
class GPUResourceOGL : public BaseType
{
protected:

	GPUDeviceOGL* _device;
	uint64 _memoryUsage;

private:

#if GPU_ENABLE_RESOURCE_NAMING
	String _name;
#endif

protected:

	/// <summary>
	/// Init
	/// </summary>
	/// <param name="device">Graphics Device handle</param>
	/// <param name="name">Resource name</param>
	GPUResourceOGL(GPUDeviceOGL* device, const String& name)
		: _device(device)
		, _memoryUsage(0)
#if GPU_ENABLE_RESOURCE_NAMING
		, _name(name)
#endif
	{
		ASSERT(device);

		// Register
		device->Resources.Add(this);
	}

public:

	/// <summary>
	/// Destructor
	/// </summary>
	virtual ~GPUResourceOGL()
	{
		// Unregister
		if (_device)
			_device->Resources.Remove(this);

		// Check if is still using any GPU memory
		if (_memoryUsage != 0)
		{
			LOG(Fatal, "GPU resource \'{0}\' has not been fully disposed.", GPUResource::ToString());
		}
	}

public:

	/// <summary>
	/// Gets graphics device
	/// </summary>
	/// <returns>Device</returns>
	FORCE_INLINE GPUDeviceOGL* GetDevice() const
	{
		return _device;
	}

public:

	// [GPUResource]
	uint64 GetMemoryUsage() const override
	{
		return _memoryUsage;
	}
#if GPU_ENABLE_RESOURCE_NAMING
	String GetName() const override
	{
		return _name;
	}
#endif
	void OnDeviceDispose() override
	{
		// Base
		GPUResource::OnDeviceDispose();

		// Unlink device handle
		_device = nullptr;
	}
};

#endif
