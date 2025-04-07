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
    GPUShaderProgramCS* _preSortCS;
    GPUShaderProgramCS* _innerSortCS;
    GPUShaderProgramCS* _outerSortCS;
    GPUShaderProgramCS* _copyIndicesCS;

public:

    /// <summary>
    /// Sorts the specified buffer of index-key pairs.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    /// <param name="sortingKeysBuffer">The sorting keys buffer. Used as a structured buffer of type Item (see above).</param>
    /// <param name="countBuffer">The buffer that contains a items counter value.</param>
    /// <param name="counterOffset">The offset into counter buffer to find count for this list. Must be a multiple of 4 bytes.</param>
    /// <param name="sortAscending">True to sort in ascending order (smallest to largest), otherwise false to sort in descending order.</param>
    /// <param name="sortedIndicesBuffer">The output buffer for sorted values extracted from the sorted sortingKeysBuffer after algorithm run. Valid for uint value types - used as RWBuffer.</param>
    void Sort(GPUContext* context, GPUBuffer* sortingKeysBuffer, GPUBuffer* countBuffer, uint32 counterOffset, bool sortAscending, GPUBuffer* sortedIndicesBuffer);

public:

    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _preSortCS = nullptr;
        _innerSortCS = nullptr;
        _outerSortCS = nullptr;
        invalidateResources();
    }
#endif

protected:

    // [RendererPass]
    bool setupResources() override;
};
