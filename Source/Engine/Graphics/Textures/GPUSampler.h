// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUSamplerDescription.h"
#include "../GPUResource.h"

/// <summary>
/// GPU texture sampler object.
/// </summary>
/// <seealso cref="GPUResource" />
API_CLASS(Sealed) class FLAXENGINE_API GPUSampler : public GPUResource
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUSampler);
    static GPUSampler* Spawn(const SpawnParams& params);
    static GPUSampler* New();

protected:
    GPUSamplerDescription _desc;

    GPUSampler();

public:
    /// <summary>
    /// Gets sampler description structure.
    /// </summary>
    API_PROPERTY() const GPUSamplerDescription& GetDescription() const
    {
        return _desc;
    }

public:
    /// <summary>
    /// Creates new sampler.
    /// </summary>
    /// <param name="desc">The sampler description.</param>
    /// <returns>True if cannot create sampler, otherwise false.</returns>
    API_FUNCTION() bool Init(API_PARAM(Ref) const GPUSamplerDescription& desc);

protected:
    virtual bool OnInit() = 0;

public:
    // [GPUResource]
    String ToString() const override;
    GPUResourceType GetResourceType() const final override;

protected:
    // [GPUResource]
    void OnReleaseGPU() override;
};
