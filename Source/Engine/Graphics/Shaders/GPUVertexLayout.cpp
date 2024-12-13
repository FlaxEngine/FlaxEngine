// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GPUVertexLayout.h"
#if GPU_ENABLE_ASSERTION_LOW_LAYERS
#include "Engine/Core/Log.h"
#endif
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/GPUDevice.h"
#if GPU_ENABLE_RESOURCE_NAMING
#include "Engine/Scripting/Enums.h"
#endif

// VertexElement has been designed to be POD and memory-comparable for faster hashing and comparision.
struct VertexElementRaw
{
    uint32 Words[2];
};

static_assert(sizeof(VertexElement) == sizeof(VertexElementRaw), "Incorrect size of the VertexElement!");

namespace
{
    CriticalSection CacheLocker;
    Dictionary<uint32, GPUVertexLayout*> LayoutCache;
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

void ClearVertexLayoutCache()
{
    for (const auto& e : LayoutCache)
        Delete(e.Value);
    LayoutCache.Clear();
}
