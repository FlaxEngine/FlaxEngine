// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUPipelineState.h"
#include "Shaders/GPUShader.h"
#include "GPUDevice.h"

template<int Size>
class GPUPipelineStatePermutations
{
public:
    GPUPipelineState* States[Size];

public:
    GPUPipelineStatePermutations()
    {
        Platform::MemoryClear(States, sizeof(States));
    }

    ~GPUPipelineStatePermutations()
    {
        Delete();
    }

public:
    bool IsValid() const
    {
        for (int i = 0; i < Size; i++)
        {
            if (States[i] == nullptr || !States[i]->IsValid())
            {
                return false;
            }
        }

        return true;
    }

    FORCE_INLINE GPUPipelineState* Get(int index) const
    {
        ASSERT_LOW_LAYER(index >= 0 && index < Size);
        return States[index];
    }

    FORCE_INLINE GPUPipelineState*& operator[](int32 index)
    {
        ASSERT_LOW_LAYER(index >= 0 && index < Size);
        return States[index];
    }

public:
    void CreatePipelineStates()
    {
        for (int i = 0; i < Size; i++)
        {
            if (States[i] == nullptr)
            {
                States[i] = GPUDevice::Instance->CreatePipelineState();
            }
        }
    }

    void Release()
    {
        for (int i = 0; i < Size; i++)
        {
            if (States[i])
            {
                States[i]->ReleaseGPU();
            }
        }
    }

    void Delete()
    {
        for (int i = 0; i < Size; i++)
        {
            SAFE_DELETE_GPU_RESOURCE(States[i]);
        }
    }
};

template<int Size>
class GPUPipelineStatePermutationsPs : public GPUPipelineStatePermutations<Size>
{
public:
    typedef GPUPipelineStatePermutations<Size> Base;

public:
    GPUPipelineStatePermutationsPs()
    {
    }

    ~GPUPipelineStatePermutationsPs()
    {
    }

public:
    bool Create(GPUPipelineState::Description& desc, GPUShader* shader, const StringAnsiView& psName)
    {
        for (int i = 0; i < Size; i++)
        {
            ASSERT(Base::States[i]);

            desc.PS = shader->GetPS(psName, i);
            if (Base::States[i]->Init(desc))
                return true;
        }

        return false;
    }
};

template<int Size>
class ComputeShaderPermutation
{
public:
    GPUShaderProgramCS* Shaders[Size];

public:
    ComputeShaderPermutation()
    {
        Platform::MemoryClear(Shaders, sizeof(Shaders));
    }

    ~ComputeShaderPermutation()
    {
    }

public:
    FORCE_INLINE GPUShaderProgramCS* Get(const int index) const
    {
        ASSERT_LOW_LAYER(index >= 0 && index < Size);
        return Shaders[index];
    }

public:
    void Clear()
    {
        Platform::MemoryClear(Shaders, sizeof(Shaders));
    }

    void Get(GPUShader* shader, const StringAnsiView& name)
    {
        for (int i = 0; i < Size; i++)
        {
            Shaders[i] = shader->GetCS(name, i);
        }
    }
};
