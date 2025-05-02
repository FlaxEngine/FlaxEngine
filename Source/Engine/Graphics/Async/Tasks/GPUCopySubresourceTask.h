// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../GPUTask.h"
#include "../GPUTasksContext.h"
#include "Engine/Graphics/GPUResourceProperty.h"

/// <summary>
/// GPU subresource copy task.
/// </summary>
class GPUCopySubresourceTask : public GPUTask
{
private:
    GPUResourceReference _srcResource;
    GPUResourceReference _dstResource;
    uint32 _srcSubresource, _dstSubresource;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="src">The source resource.</param>
    /// <param name="dst">The destination resource.</param>
    /// <param name="srcSubresource">The source subresource index.</param>
    /// <param name="dstSubresource">The destination subresource index.</param>
    GPUCopySubresourceTask(GPUResource* src, GPUResource* dst, uint32 srcSubresource, uint32 dstSubresource)
        : GPUTask(Type::CopyResource)
        , _srcResource(src)
        , _dstResource(dst)
        , _srcSubresource(srcSubresource)
        , _dstSubresource(dstSubresource)
    {
        _srcResource.Released.Bind<GPUCopySubresourceTask, &GPUCopySubresourceTask::OnResourceReleased>(this);
        _dstResource.Released.Bind<GPUCopySubresourceTask, &GPUCopySubresourceTask::OnResourceReleased>(this);
    }

private:
    void OnResourceReleased()
    {
        Cancel();
    }

public:
    // [GPUTask]
    bool HasReference(Object* resource) const override
    {
        return _srcResource == resource || _dstResource == resource;
    }

protected:
    // [GPUTask]
    Result run(GPUTasksContext* context) override
    {
        if (!_srcResource || !_dstResource)
            return Result::MissingResources;
        context->GPU->CopySubresource(_dstResource, _dstSubresource, _srcResource, _srcSubresource);
        return Result::Ok;
    }
    void OnEnd() override
    {
        _srcResource.Unlink();
        _dstResource.Unlink();

        // Base
        GPUTask::OnEnd();
    }
};
