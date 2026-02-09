// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

/// <summary>
/// Bitonic Sort implementation using GPU compute shaders. 
/// It has a complexity of O(n*(log n)^2), which is inferior to most traditional sorting algorithms, but because GPUs have so many threads, 
/// and because each thread can be utilized, the algorithm can fully load the GPU, taking advantage of its high ALU and bandwidth capabilities.
/// </summary>
class BitonicSort : public RendererPass<BitonicSort>
{
private:

    AssetReference<Shader> _shader;
    GPUBuffer* _dispatchArgsBuffer = nullptr;
    GPUConstantBuffer* _cb;
    GPUShaderProgramCS* _indirectArgsCS;
    ComputeShaderPermutation<2> _preSortCS;
    GPUShaderProgramCS* _innerSortCS;
    GPUShaderProgramCS* _outerSortCS;
    GPUShaderProgramCS* _copyIndicesCS;

public:

    /// <summary>
    /// Sorts the specified buffers of index-key pairs.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    /// <param name="indicesBuffer">The sorting indices buffer with an index for each item (sequence of: 0, 1, 2, 3...). After sorting represents actual items order based on their keys. Valid for uint value types - used as RWBuffer.</param>
    /// <param name="keysBuffer">The sorting keys buffer with a sort value for each item (must match order of items in indicesBuffer). Valid for float value types - used as RWBuffer.</param>
    /// <param name="countBuffer">The buffer that contains a items counter value.</param>
    /// <param name="counterOffset">The offset into counter buffer to find count for this list. Must be a multiple of 4 bytes.</param>
    /// <param name="sortAscending">True to sort in ascending order (smallest to largest), otherwise false to sort in descending order.</param>
    /// <param name="maxElements">Optional upper limit of elements to sort. Cna be used to optimize indirect dispatches allocation. If non-zero, then it gets calculated based on the input item buffer size.</param>
    void Sort(GPUContext* context, GPUBuffer* indicesBuffer, GPUBuffer* keysBuffer, GPUBuffer* countBuffer, uint32 counterOffset, bool sortAscending, int32 maxElements = 0);

public:

    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _preSortCS.Clear();
        _innerSortCS = nullptr;
        _outerSortCS = nullptr;
        invalidateResources();
    }
#endif

protected:

    // [RendererPass]
    bool setupResources() override;
};
