// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../GPUTask.h"
#include "../GPUTasksContext.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUResourceProperty.h"
#include "Engine/Core/Types/DataContainer.h"

/// <summary>
/// GPU buffer upload task.
/// </summary>
class GPUUploadBufferTask : public GPUTask
{
protected:
    BufferReference _buffer;
    int32 _offset;
    BytesContainer _data;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUUploadBufferTask"/> class.
    /// </summary>
    /// <param name="buffer">The target buffer.</param>
    /// <param name="offset">The target buffer offset to copy data to. The default value is 0.</param>
    /// <param name="data">The data to upload.</param>
    /// <param name="copyData">True if copy data to the temporary buffer, otherwise the input data will be used directly. Then ensure it is valid during the copy operation period (for the next few frames).</param>
    GPUUploadBufferTask(GPUBuffer* buffer, int32 offset, Span<byte> data, bool copyData)
        : GPUTask(Type::UploadBuffer)
        , _buffer(buffer)
        , _offset(offset)
    {
        _buffer.Released.Bind<GPUUploadBufferTask, &GPUUploadBufferTask::OnResourceReleased>(this);

        if (copyData)
            _data.Copy(data);
        else
            _data.Link(data);
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
        return _buffer == resource;
    }

protected:
    // [GPUTask]
    Result run(GPUTasksContext* context) override
    {
        if (!_buffer)
            return Result::MissingResources;
        context->GPU->UpdateBuffer(_buffer, _data.Get(), _data.Length(), _offset);
        return Result::Ok;
    }
    void OnEnd() override
    {
        _buffer.Unlink();

        // Base
        GPUTask::OnEnd();
    }
};
