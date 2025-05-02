// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Version.h"
#include "Engine/Scripting/ScriptingObject.h"

// GPU vendors IDs
#define GPU_VENDOR_ID_AMD 0x1002
#define GPU_VENDOR_ID_INTEL 0x8086
#define GPU_VENDOR_ID_NVIDIA 0x10DE
#define GPU_VENDOR_ID_MICROSOFT 0x1414
#define GPU_VENDOR_ID_APPLE 0x106B

/// <summary>
/// Interface for GPU device adapter.
/// </summary>
API_CLASS(NoSpawn, Attributes="HideInEditor") class FLAXENGINE_API GPUAdapter : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUDevice);
public:
    GPUAdapter()
        : ScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    {
    }

    GPUAdapter(const GPUAdapter& other)
        : GPUAdapter()
    {
        *this = other;
    }

    GPUAdapter& operator=(const GPUAdapter& other)
    {
        return *this;
    }

public:
    /// <summary>
    /// Checks if adapter is valid and returns true if it is.
    /// </summary>
    /// <returns>True if valid, otherwise false.</returns>
    virtual bool IsValid() const = 0;

    /// <summary>
    /// Gets the native pointer to the underlying graphics device adapter. It's a low-level platform-specific handle.
    /// </summary>
    API_PROPERTY() virtual void* GetNativePtr() const = 0;

    /// <summary>
    /// Gets the GPU vendor identifier.
    /// </summary>
    API_PROPERTY() virtual uint32 GetVendorId() const = 0;

    /// <summary>
    /// Gets a string that contains the adapter description. Used for presentation to the user.
    /// </summary>
    API_PROPERTY() virtual String GetDescription() const = 0;

    /// <summary>
    /// Gets the GPU driver version.
    /// </summary>
    API_PROPERTY() virtual Version GetDriverVersion() const = 0;

public:
    // Returns true if adapter's vendor is AMD.
    API_PROPERTY() FORCE_INLINE bool IsAMD() const
    {
        return GetVendorId() == GPU_VENDOR_ID_AMD;
    }

    // Returns true if adapter's vendor is Intel.
    API_PROPERTY() FORCE_INLINE bool IsIntel() const
    {
        return GetVendorId() == GPU_VENDOR_ID_INTEL;
    }

    // Returns true if adapter's vendor is Nvidia.
    API_PROPERTY() FORCE_INLINE bool IsNVIDIA() const
    {
        return GetVendorId() == GPU_VENDOR_ID_NVIDIA;
    }

    // Returns true if adapter's vendor is Microsoft.
    API_PROPERTY() FORCE_INLINE bool IsMicrosoft() const
    {
        return GetVendorId() == GPU_VENDOR_ID_MICROSOFT;
    }
};
