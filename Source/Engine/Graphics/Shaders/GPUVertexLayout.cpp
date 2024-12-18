// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GPUVertexLayout.h"
#if GPU_ENABLE_ASSERTION_LOW_LAYERS
#include "Engine/Core/Log.h"
#endif
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#if GPU_ENABLE_RESOURCE_NAMING
#include "Engine/Scripting/Enums.h"
#endif

// VertexElement has been designed to be POD and memory-comparable for faster hashing and comparision.
struct VertexElementRaw
{
    uint32 Words[2];
};

static_assert(sizeof(VertexElement) == sizeof(VertexElementRaw), "Incorrect size of the VertexElement!");

struct VertexBufferLayouts
{
    GPUVertexLayout* Layouts[GPU_MAX_VB_BINDED];

    bool operator==(const VertexBufferLayouts& other) const
    {
        return Platform::MemoryCompare(&Layouts, &other.Layouts, sizeof(Layouts)) == 0;
    }
};

uint32 GetHash(const VertexBufferLayouts& key)
{
    uint32 hash = GetHash(key.Layouts[0]);
    for (int32 i = 1; i < GPU_MAX_VB_BINDED; i++)
        CombineHash(hash, GetHash(key.Layouts[i]));
    return hash;
}

namespace
{
    CriticalSection CacheLocker;
    Dictionary<uint32, GPUVertexLayout*> LayoutCache;
    Dictionary<VertexBufferLayouts, GPUVertexLayout*> VertexBufferCache;
}

String VertexElement::ToString() const
{
#if GPU_ENABLE_RESOURCE_NAMING
    return String::Format(TEXT("{}, format {}, offset {}, per-instance {}, slot {}"), ScriptingEnum::ToString(Type), ScriptingEnum::ToString(Format), Offset, PerInstance, Slot);
#else
    return TEXT("VertexElement");
#endif
}

bool VertexElement::operator==(const VertexElement& other) const
{
    auto thisRaw = (const VertexElementRaw*)this;
    auto otherRaw = (const VertexElementRaw*)&other;
    return thisRaw->Words[0] == otherRaw->Words[0] && thisRaw->Words[1] == otherRaw->Words[1];
}

uint32 GetHash(const VertexElement& key)
{
    auto keyRaw = (const VertexElementRaw*)&key;
    uint32 hash = keyRaw->Words[0];
    CombineHash(hash, keyRaw->Words[1]);
    return hash;
}

GPUVertexLayout::GPUVertexLayout()
    : GPUResource(SpawnParams(Guid::New(), TypeInitializer))
{
}

void GPUVertexLayout::SetElements(const Elements& elements, uint32 offsets[GPU_MAX_VS_ELEMENTS])
{
    _elements = elements;
    uint32 strides[GPU_MAX_VB_BINDED] = {};
    for (int32 i = 0; i < elements.Count(); i++)
    {
        const VertexElement& e = elements[i];
        ASSERT(e.Slot < GPU_MAX_VB_BINDED);
        strides[e.Slot] = Math::Max(strides[e.Slot], offsets[e.Slot]);
    }
    _stride = 0;
    for (int32 i = 0; i < GPU_MAX_VB_BINDED; i++)
        _stride += strides[i];
}

GPUVertexLayout* GPUVertexLayout::Get(const Elements& elements)
{
    // Hash input layout
    uint32 hash = 0;
    for (const VertexElement& element : elements)
    {
        CombineHash(hash, GetHash(element));
    }

    // Lookup existing cache
    CacheLocker.Lock();
    GPUVertexLayout* result;
    if (!LayoutCache.TryGet(hash, result))
    {
        result = GPUDevice::Instance->CreateVertexLayout(elements);
        if (!result)
        {
#if GPU_ENABLE_ASSERTION_LOW_LAYERS
            for (auto& e : elements)
                LOG(Error, " {}", e.ToString());
#endif
            LOG(Error, "Failed to create vertex layout");
            CacheLocker.Unlock();
            return nullptr;
        }
        LayoutCache.Add(hash, result);
    }
#if GPU_ENABLE_ASSERTION_LOW_LAYERS
    else if (result->GetElements() != elements)
    {
        for (auto& e : result->GetElements())
            LOG(Error, " (a) {}", e.ToString());
        for (auto& e : elements)
            LOG(Error, " (b) {}", e.ToString());
        LOG(Fatal, "Vertex layout cache collision for hash {}", hash);
    }
#endif
    CacheLocker.Unlock();

    return result;
}

GPUVertexLayout* GPUVertexLayout::Get(const Span<GPUBuffer*>& vertexBuffers)
{
    if (vertexBuffers.Length() == 0)
        return nullptr;
    if (vertexBuffers.Length() == 1)
        return vertexBuffers.Get()[0] ? vertexBuffers.Get()[0]->GetVertexLayout() : nullptr;

    // Build hash key for set of buffers (in case there is layout sharing by different sets of buffers)
    VertexBufferLayouts layouts;
    for (int32 i = 0; i < vertexBuffers.Length(); i++)
        layouts.Layouts[i] = vertexBuffers.Get()[i] ? vertexBuffers.Get()[i]->GetVertexLayout() : nullptr;
    for (int32 i = vertexBuffers.Length(); i < GPU_MAX_VB_BINDED; i++)
        layouts.Layouts[i] = nullptr;

    // Lookup existing cache
    CacheLocker.Lock();
    GPUVertexLayout* result;
    if (!VertexBufferCache.TryGet(layouts, result))
    {
        Elements elements;
        bool anyValid = false;
        for (int32 slot = 0; slot < vertexBuffers.Length(); slot++)
        {
            if (layouts.Layouts[slot])
            {
                anyValid = true;
                int32 start = elements.Count();
                elements.Add(layouts.Layouts[slot]->GetElements());
                for (int32 j = start; j < elements.Count(); j++)
                    elements.Get()[j].Slot = (byte)slot;
            }
        }
        result = anyValid ? Get(elements) : nullptr;
        VertexBufferCache.Add(layouts, result);
    }
    CacheLocker.Unlock();

    return result;
}

void ClearVertexLayoutCache()
{
    for (const auto& e : LayoutCache)
        Delete(e.Value);
    LayoutCache.Clear();
    VertexBufferCache.Clear();
}
