// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUVertexLayout.h"
#include "Engine/Core/Log.h"
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

    GPUVertexLayout* AddCache(const VertexBufferLayouts& key, int32 count)
    {
        GPUVertexLayout::Elements elements;
        bool anyValid = false;
        for (int32 slot = 0; slot < count; slot++)
        {
            if (key.Layouts[slot])
            {
                anyValid = true;
                int32 start = elements.Count();
                elements.Add(key.Layouts[slot]->GetElements());
                for (int32 j = start; j < elements.Count(); j++)
                    elements.Get()[j].Slot = (byte)slot;
            }
        }
        GPUVertexLayout* result = anyValid ? GPUVertexLayout::Get(elements) : nullptr;
        VertexBufferCache.Add(key, result);
        return result;
    }
}

String VertexElement::ToString() const
{
#if GPU_ENABLE_RESOURCE_NAMING
    return String::Format(TEXT("{}, {}, offset {}, {}, slot {}"), ScriptingEnum::ToString(Type), ScriptingEnum::ToString(Format), Offset, PerInstance ? TEXT("per-instance") : TEXT("per-vertex"), Slot);
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

void GPUVertexLayout::SetElements(const Elements& elements, bool explicitOffsets)
{
    uint32 offsets[GPU_MAX_VB_BINDED + 1] = {};
    _elements = elements;
    for (int32 i = 0; i < _elements.Count(); i++)
    {
        VertexElement& e = _elements[i];
        ASSERT(e.Slot <= GPU_MAX_VB_BINDED); // One special slot after all VBs for any missing vertex elements binding (on Vulkan)
        uint32& offset = offsets[e.Slot];
        if (e.Offset != 0 || explicitOffsets)
            offset = e.Offset;
        else
            e.Offset = (byte)offset;
        offset += PixelFormatExtensions::SizeInBytes(e.Format);
    }
    _stride = 0;
    for (uint32 offset : offsets)
        _stride += offset;
}

String GPUVertexLayout::GetElementsString() const
{
    String result;
    for (int32 i = 0; i < _elements.Count(); i++)
    {
        if (i != 0)
            result += '\n';
        result += _elements[i].ToString();
    }
    return result;
}

VertexElement GPUVertexLayout::FindElement(VertexElement::Types type) const
{
    for (const VertexElement& e : _elements)
    {
        if (e.Type == type)
            return e;
    }
    return VertexElement();
}

GPUVertexLayout* GPUVertexLayout::Get(const Elements& elements, bool explicitOffsets)
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
        result = GPUDevice::Instance->CreateVertexLayout(elements, explicitOffsets);
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
    VertexBufferLayouts key;
    for (int32 i = 0; i < vertexBuffers.Length() && i < GPU_MAX_VB_BINDED; i++)
        key.Layouts[i] = vertexBuffers.Get()[i] ? vertexBuffers.Get()[i]->GetVertexLayout() : nullptr;
    for (int32 i = vertexBuffers.Length(); i < GPU_MAX_VB_BINDED; i++)
        key.Layouts[i] = nullptr;

    // Lookup existing cache
    CacheLocker.Lock();
    GPUVertexLayout* result;
    if (!VertexBufferCache.TryGet(key, result))
        result = AddCache(key, vertexBuffers.Length());
    CacheLocker.Unlock();

    return result;
}

GPUVertexLayout* GPUVertexLayout::Get(const Span<GPUVertexLayout*>& layouts)
{
    if (layouts.Length() == 0)
        return nullptr;
    if (layouts.Length() == 1)
        return layouts.Get()[0] ? layouts.Get()[0] : nullptr;

    // Build hash key for set of buffers (in case there is layout sharing by different sets of buffers)
    VertexBufferLayouts key;
    for (int32 i = 0; i < layouts.Length() && i < GPU_MAX_VB_BINDED; i++)
        key.Layouts[i] = layouts.Get()[i];
    for (int32 i = layouts.Length(); i < GPU_MAX_VB_BINDED; i++)
        key.Layouts[i] = nullptr;

    // Lookup existing cache
    CacheLocker.Lock();
    GPUVertexLayout* result;
    if (!VertexBufferCache.TryGet(key, result))
        result = AddCache(key, layouts.Length());
    CacheLocker.Unlock();

    return result;
}

GPUVertexLayout* GPUVertexLayout::Merge(GPUVertexLayout* base, GPUVertexLayout* reference, bool removeUnused, bool addMissing, int32 missingSlotOverride)
{
    GPUVertexLayout* result = base ? base : reference;
    if (base && reference && base != reference)
    {
        bool elementsModified = false;
        Elements newElements = base->GetElements();
        if (removeUnused)
        {
            for (int32 i = newElements.Count() - 1; i >= 0; i--)
            {
                bool missing = true;
                const VertexElement& e = newElements.Get()[i];
                for (const VertexElement& ee : reference->GetElements())
                {
                    if (ee.Type == e.Type)
                    {
                        missing = false;
                        break;
                    }
                }
                if (missing)
                {
                    // Remove unused element
                    newElements.RemoveAtKeepOrder(i);
                    elementsModified = true;
                }
            }
        }
        if (addMissing)
        {
            for (const VertexElement& e : reference->GetElements())
            {
                bool missing = true;
                for (const VertexElement& ee : base->GetElements())
                {
                    if (ee.Type == e.Type)
                    {
                        missing = false;
                        break;
                    }
                }
                if (missing)
                {
                    // Insert any missing elements
                    VertexElement ne = { e.Type, missingSlotOverride != -1 ? (byte)missingSlotOverride : e.Slot, 0, e.PerInstance, e.Format };
                    if (e.Type == VertexElement::Types::TexCoord1 || 
                        e.Type == VertexElement::Types::TexCoord2 || 
                        e.Type == VertexElement::Types::TexCoord3)
                    {
                        // Alias missing texcoords with existing texcoords
                        for (const VertexElement& ee : newElements)
                        {
                            if (ee.Type == VertexElement::Types::TexCoord0)
                            {
                                ne = ee;
                                ne.Type = e.Type;
                                break;
                            }
                        }
                    }
                    newElements.Add(ne);
                    elementsModified = true;
                }
            }
        }
        if (elementsModified)
            result = Get(newElements, true);
    }
    return result;
}

void ClearVertexLayoutCache()
{
    for (const auto& e : LayoutCache)
        Delete(e.Value);
    LayoutCache.Clear();
    VertexBufferCache.Clear();
}
