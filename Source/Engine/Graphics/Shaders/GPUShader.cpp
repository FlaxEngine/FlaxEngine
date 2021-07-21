// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GPUShader.h"
#include "GPUConstantBuffer.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Serialization/MemoryReadStream.h"

GPUShaderProgramsContainer::GPUShaderProgramsContainer()
    : _shaders(64)
{
    // TODO: test different values for _shaders capacity, test performance impact (less hash collisions but more memory?)
}

GPUShaderProgramsContainer::~GPUShaderProgramsContainer()
{
    // Remember to delete all programs
    _shaders.ClearDelete();
}

void GPUShaderProgramsContainer::Add(GPUShaderProgram* shader, int32 permutationIndex)
{
    // Validate input
    ASSERT(shader && Math::IsInRange(permutationIndex, 0, SHADER_PERMUTATIONS_MAX_COUNT - 1));
#if ENABLE_ASSERTION
    if ((Get(shader->GetName(), permutationIndex) != nullptr))
    {
        CRASH;
    }
#endif

    // Store shader
    const int32 hash = CalculateHash(shader->GetName(), permutationIndex);
    _shaders.Add(hash, shader);
}

GPUShaderProgram* GPUShaderProgramsContainer::Get(const StringAnsiView& name, int32 permutationIndex) const
{
    // Validate input
    ASSERT(name.Length() > 0 && Math::IsInRange(permutationIndex, 0, SHADER_PERMUTATIONS_MAX_COUNT - 1));

    // Find shader
    GPUShaderProgram* result = nullptr;
    const int32 hash = CalculateHash(name, permutationIndex);
    _shaders.TryGet(hash, result);

    return result;
}

void GPUShaderProgramsContainer::Clear()
{
    _shaders.ClearDelete();
}

uint32 GPUShaderProgramsContainer::CalculateHash(const StringAnsiView& name, int32 permutationIndex)
{
    return GetHash(name) * 37 + permutationIndex;
}

GPUShader::GPUShader()
{
    Platform::MemoryClear(_constantBuffers, sizeof(_constantBuffers));
}

bool GPUShader::Create(MemoryReadStream& stream)
{
    ReleaseGPU();

    // Version
    int32 version;
    stream.ReadInt32(&version);
    if (version != GPU_SHADER_CACHE_VERSION)
    {
        LOG(Warning, "Unsupported shader version {0}. The supported version is {1}.", version, GPU_SHADER_CACHE_VERSION);
        return true;
    }

    // Additional data start
    int32 additionalDataStart;
    stream.ReadInt32(&additionalDataStart);

    // Shaders count
    int32 shadersCount;
    stream.ReadInt32(&shadersCount);
    GPUShaderProgramInitializer initializer;
#if !BUILD_RELEASE
    initializer.Owner = this;
#endif
    for (int32 i = 0; i < shadersCount; i++)
    {
        const ShaderStage type = static_cast<ShaderStage>(stream.ReadByte());
        const int32 permutationsCount = stream.ReadByte();
        ASSERT(Math::IsInRange(permutationsCount, 1, SHADER_PERMUTATIONS_MAX_COUNT));

        // Load shader name
        stream.ReadStringAnsi(&initializer.Name, 11);
        ASSERT(initializer.Name.HasChars());

        // Load shader flags
        stream.ReadUint32((uint32*)&initializer.Flags);

        for (int32 permutationIndex = 0; permutationIndex < permutationsCount; permutationIndex++)
        {
            // Load cache
            uint32 cacheSize;
            stream.ReadUint32(&cacheSize);
            if (cacheSize > stream.GetLength() - stream.GetPosition())
            {
                LOG(Warning, "Invalid shader cache size.");
                return true;
            }
            byte* cache = stream.Read<byte>(cacheSize);

            // Read bindings
            stream.ReadBytes(&initializer.Bindings, sizeof(ShaderBindings));

            // Create shader program
            GPUShaderProgram* shader = CreateGPUShaderProgram(type, initializer, cache, cacheSize, stream);
            if (shader == nullptr)
            {
                LOG(Warning, "Failed to create shader program.");
                return true;
            }

            // Add to collection
            _shaders.Add(shader, permutationIndex);
        }
    }

    // Constant Buffers
    const byte constantBuffersCount = stream.ReadByte();
    const byte maximumConstantBufferSlot = stream.ReadByte();
    if (constantBuffersCount > 0)
    {
        ASSERT(maximumConstantBufferSlot < MAX_CONSTANT_BUFFER_SLOTS);

        for (int32 i = 0; i < constantBuffersCount; i++)
        {
            // Load info
            const byte slotIndex = stream.ReadByte();
            uint32 size;
            stream.ReadUint32(&size);

            // Create CB
#if GPU_ENABLE_RESOURCE_NAMING
            String name = ToString() + TEXT(".CB") + i;
#else
			String name;
#endif
            ASSERT(_constantBuffers[slotIndex] == nullptr);
            const auto cb = CreateCB(name, size, stream);
            if (cb == nullptr)
            {
                LOG(Warning, "Failed to create shader constant buffer.");
                return true;
            }
            _constantBuffers[slotIndex] = cb;
        }
    }

    // Don't read additional data

    _memoryUsage = 1;
    return false;
}

GPUShaderProgram* GPUShader::GetShader(ShaderStage stage, const StringAnsiView& name, int32 permutationIndex) const
{
    const auto shader = _shaders.Get(name, permutationIndex);

#if BUILD_RELEASE

	// Release build is more critical on that
	ASSERT(shader != nullptr && shader->GetStage() == stage);

#else

    if (shader == nullptr)
    {
        LOG(Error, "Missing {0} shader \'{1}\'[{2}]. Object: {3}.", ::ToString(stage), String(name), permutationIndex, ToString());
    }
    else if (shader->GetStage() != stage)
    {
        LOG(Error, "Invalid shader stage \'{1}\'[{2}]. Expected: {0}. Actual: {4}. Object: {3}.", ::ToString(stage), String(name), permutationIndex, ToString(), ::ToString(shader->GetStage()));
    }

#endif

    return shader;
}

GPUResource::ResourceType GPUShader::GetResourceType() const
{
    return ResourceType::Shader;
}

void GPUShader::OnReleaseGPU()
{
    _memoryUsage = 0;
    _shaders.Clear();
    Platform::MemoryClear(_constantBuffers, sizeof(_constantBuffers));
}
