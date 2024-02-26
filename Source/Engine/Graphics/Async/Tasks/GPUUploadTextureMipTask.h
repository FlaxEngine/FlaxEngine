// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../GPUTask.h"
#include "../GPUTasksContext.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/GPUResourceProperty.h"
#include "Engine/Core/Types/DataContainer.h"

/// <summary>
/// GPU texture mip upload task.
/// </summary>
class GPUUploadTextureMipTask : public GPUTask
{
protected:
    GPUTextureReference _texture;
    int32 _mipIndex, _rowPitch, _slicePitch;
    BytesContainer _data;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUUploadTextureMipTask"/> class.
    /// </summary>
    /// <param name="texture">The target texture.</param>
    /// <param name="mipIndex">The target texture mip data.</param>
    /// <param name="data">The data to upload.</param>
    /// <param name="rowPitch">The data row pitch.</param>
    /// <param name="slicePitch">The data slice pitch.</param>
    /// <param name="copyData">True if copy data to the temporary buffer, otherwise the input data will be used directly. Then ensure it is valid during the copy operation period (for the next few frames).</param>
    GPUUploadTextureMipTask(GPUTexture* texture, int32 mipIndex, Span<byte> data, int32 rowPitch, int32 slicePitch, bool copyData)
        : GPUTask(Type::UploadTexture)
        , _texture(texture)
        , _mipIndex(mipIndex)
        , _rowPitch(rowPitch)
        , _slicePitch(slicePitch)
    {
        _texture.Released.Bind<GPUUploadTextureMipTask, &GPUUploadTextureMipTask::OnResourceReleased>(this);

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
        return _texture == resource;
    }

protected:
    // [GPUTask]
    Result run(GPUTasksContext* context) override
    {
        const auto texture = _texture.Get();
        if (texture == nullptr)
            return Result::MissingResources;
        ASSERT(texture->IsAllocated());

        // Update all array slices
        const byte* dataSource = _data.Get();
        const int32 arraySize = texture->ArraySize();
        ASSERT(_data.Length() >= _slicePitch * arraySize);
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            context->GPU->UpdateTexture(texture, arrayIndex, _mipIndex, dataSource, _rowPitch, _slicePitch);
            dataSource += _slicePitch;
        }

        return Result::Ok;
    }

    void OnSync() override
    {
        auto texture = _texture.Get();
        if (texture)
        {
            // Check if the new mips has been just uploaded
            if (_mipIndex == texture->HighestResidentMipIndex() - 1)
            {
                // Mark as mip loaded
                texture->SetResidentMipLevels(texture->ResidentMipLevels() + 1);
            }
        }

        // Base
        GPUTask::OnSync();
    }

    void OnEnd() override
    {
        _texture.Unlink();

        // Base
        GPUTask::OnEnd();
    }
};
