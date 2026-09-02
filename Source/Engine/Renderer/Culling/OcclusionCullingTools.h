// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Config.h"

GPU_CB_STRUCT(OcclusionCullingData {
    Matrix ViewProjectionMatrix;
    float RTSizeX;
    float RTSizeY;
    float MaxMipLevel;
    uint32 CullCount;
    });

/// <summary>
/// Utility for occlusion culling implementations to manage stable CullingId for objects with state tracking (over multiple frames).
/// </summary>
template<typename Item>
class OcclusionCullingItems : public Array<Item>
{
private:
    volatile int64 _freeItemsCount = 0;
    volatile int64 _newItemsCount = 0;
    Array<uint32> _freeItems;

public:
    void BeginFrame()
    {
        // Remove used free items
        _freeItems.Resize(Math::Max((int32)_freeItemsCount, 0));

#if 0 // TODO: find a different way as there might be some invisible object with CullingId assigned and drawing it later will overlap with reused IDs
        // Trim history
        constexpr int32 frameTTL = 20;
        if (_frameCounter % 10 == 0 && _frameCounter > frameTTL)
        {
            const int32 lastFrame = _frameCounter - frameTTL;
            for (int32 i = 0; i < this->Count(); i++)
            {
                auto& item = this->Get()[i];
                if (item.LastUsedFrame && item.LastUsedFrame < lastFrame)
                {
                    Platform::MemoryClear(&item, sizeof(item));
                    _freeItems.Add(i);
                }
            }
        }
#endif

        // Allocate new items (as requested during the previous frame)
        if (_newItemsCount > 0)
        {
            int32 itemsStart = this->Count(), count = (int32)_newItemsCount, freeStart = _freeItems.Count();
            if (itemsStart == 0)
                count++; // 0 is invalid for cullingId
            this->AddZeroed(count);
            _freeItems.AddUninitialized(count);
            for (int32 i = 0; i < count; i++)
                _freeItems.Get()[freeStart + i] = itemsStart + i;
            if (itemsStart == 0)
                _freeItems.RemoveAt(0); // 0 is invalid for cullingId
            _newItemsCount = 0;
        }
        _freeItemsCount = _freeItems.Count();
    }

    bool GetCullingId(uint32& cullingId)
    {
        // Check if object doesn't have ID assigned yet
        if (cullingId == 0 || cullingId >= (uint32)this->Count())
        {
            int64 freeIndex = Platform::InterlockedDecrement(&_freeItemsCount);
            if (freeIndex >= 0)
            {
                // Use the ID from the free list
                ASSERT_LOW_LAYER(freeIndex < _freeItems.Count());
                cullingId = _freeItems.Get()[freeIndex];
            }
            else
            {
                // Count space needed to contain all objects (for the next frame)
                Platform::InterlockedIncrement(&_newItemsCount);
                return true;
            }
        }

        return false;
    }
};
