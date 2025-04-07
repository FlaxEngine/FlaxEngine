// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "VertexElement.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Core/Collections/Array.h"

class GPUBuffer;

/// <summary>
/// Defines input layout of vertex buffer data passed to the Vertex Shader.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API GPUVertexLayout : public GPUResource
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUVertexLayout);
    typedef Array<VertexElement, FixedAllocation<GPU_MAX_VS_ELEMENTS>> Elements;

private:
    Elements _elements;
    uint32 _stride;

protected:
    GPUVertexLayout();

    void SetElements(const Elements& elements, bool explicitOffsets);

public:
    /// <summary>
    /// Gets the list of elements used by this layout.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const Array<VertexElement, FixedAllocation<GPU_MAX_VS_ELEMENTS>>& GetElements() const
    {
        return _elements;
    }

    /// <summary>
    /// Gets the list of elements used by this layout as a text (each element in a new line).
    /// </summary>
    API_PROPERTY() String GetElementsString() const;

    /// <summary>
    /// Gets the size in bytes of all elements in the layout structure (including their offsets).
    /// </summary>
    API_PROPERTY() FORCE_INLINE uint32 GetStride() const
    {
        return _stride;
    }

    /// <summary>
    /// Searches for a given element type in a layout.
    /// </summary>
    /// <param name="type">The type of element to find.</param>
    /// <returns>Found element with properties or empty if missing.</returns>
    API_FUNCTION() VertexElement FindElement(VertexElement::Types type) const;

    /// <summary>
    /// Gets the vertex layout for a given list of elements. Uses internal cache to skip creating layout if it's already exists for a given list.
    /// </summary>
    /// <param name="elements">The list of elements for the layout.</param>
    /// <param name="explicitOffsets">If set to true, input elements offsets will be used without automatic calculations (offsets with value 0).</param>
    /// <returns>Vertex layout object. Doesn't need to be cleared as it's cached for an application lifetime.</returns>
    API_FUNCTION() static GPUVertexLayout* Get(const Array<VertexElement, FixedAllocation<GPU_MAX_VS_ELEMENTS>>& elements, bool explicitOffsets = false);

    /// <summary>
    /// Gets the vertex layout for a given list of vertex buffers (sequence of binding slots based on layouts set on those buffers). Uses internal cache to skip creating layout if it's already exists for a given list.
    /// </summary>
    /// <param name="vertexBuffers">The list of vertex buffers for the layout.</param>
    /// <returns>Vertex layout object. Doesn't need to be cleared as it's cached for an application lifetime.</returns>
    API_FUNCTION() static GPUVertexLayout* Get(const Span<GPUBuffer*>& vertexBuffers);

    /// <summary>
    /// Merges list of layouts in a single one. Uses internal cache to skip creating layout if it's already exists for a given list.
    /// </summary>
    /// <param name="layouts">The list of layouts to merge.</param>
    /// <returns>Vertex layout object. Doesn't need to be cleared as it's cached for an application lifetime.</returns>
    API_FUNCTION() static GPUVertexLayout* Get(const Span<GPUVertexLayout*>& layouts);

    /// <summary>
    /// Merges reference vertex elements into the given set of elements to ensure the reference list is satisfied (vertex shader input requirement). Returns base layout if it's valid.
    /// </summary>
    /// <param name="base">The list of vertex buffers for the layout.</param>
    /// <param name="reference">The list of reference inputs.</param>
    /// <param name="removeUnused">True to remove elements from base layout that don't exist in a reference layout.</param>
    /// <param name="addMissing">True to add missing elements to base layout that exist in a reference layout.</param>
    /// <param name="missingSlotOverride">Allows to override the input slot for missing elements. Use value -1 to inherit slot from the reference layout.</param>
    /// <returns>Vertex layout object. Doesn't need to be cleared as it's cached for an application lifetime.</returns>
    static GPUVertexLayout* Merge(GPUVertexLayout* base, GPUVertexLayout* reference, bool removeUnused = false, bool addMissing = true, int32 missingSlotOverride = -1);

public:
    // [GPUResource]
    GPUResourceType GetResourceType() const override
    {
        return GPUResourceType::Descriptor;
    }
};
