// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../GPUTask.h"
#include "../GPUTasksContext.h"
#include "Engine/Graphics/GPUResourceProperty.h"

/// <summary>
/// GPU resource copy task.
/// </summary>
class GPUCopyResourceTask : public GPUTask
{
private:
    GPUResourceReference _srcResource;
    GPUResourceReference _dstResource;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUCopyResourceTask"/> class.
    /// </summary>
    /// <param name="src">The source resource.</param>
    /// <param name="dst">The destination resource.</param>
    GPUCopyResourceTask(GPUResource* src, GPUResource* dst)
        : GPUTask(Type::CopyResource)
        , _srcResource(src)
        , _dstResource(dst)
    {
        _srcResource.Released.Bind<GPUCopyResourceTask, &GPUCopyResourceTask::OnResourceReleased>(this);
        _dstResource.Released.Bind<GPUCopyResourceTask, &GPUCopyResourceTask::OnResourceReleased>(this);
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
        context->GPU->CopyResource(_dstResource, _srcResource);
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
