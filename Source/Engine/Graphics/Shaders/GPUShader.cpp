// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GPUShader.h"
#include "GPUConstantBuffer.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Serialization/MemoryReadStream.h"

static FORCE_INLINE uint32 HashPermutation(const StringAnsiView& name, int32 permutationIndex)
{
    return GetHash(name) * 37 + permutationIndex;
}

GPUShader::GPUShader()
    : GPUResource(SpawnParams(Guid::New(), TypeInitializer))
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
        LOG(Warning, "Unsupported shader version {0}. The current version is {1}.", version, GPU_SHADER_CACHE_VERSION);
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
    const StringView name = GetName();
#else
    const StringView name;
#endif
    const bool hasCompute = GPUDevice::Instance->Limits.HasCompute;
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
            byte* cache = stream.Move<byte>(cacheSize);

            // Read bindings
            stream.ReadBytes(&initializer.Bindings, sizeof(ShaderBindings));

            // Create shader program
            if (type == ShaderStage::Compute && !hasCompute)
            {
                LOG(Warning, "Failed to create {} Shader program '{}' ({}).", ::ToString(type), String(initializer.Name), name);
                continue;
            }
            GPUShaderProgram* shader = CreateGPUShaderProgram(type, initializer, cache, cacheSize, stream);
            if (shader == nullptr)
            {
#if !GPU_ALLOW_TESSELLATION_SHADERS
                if (type == ShaderStage::Hull || type == ShaderStage::Domain)
                    continue;
#endif
#if !GPU_ALLOW_GEOMETRY_SHADERS
                if (type == ShaderStage::Geometry)
                    continue;
#endif
                LOG(Error, "Failed to create {} Shader program '{}' ({}).", ::ToString(type), String(initializer.Name), name);
                return true;
            }

            // Add to the collection
            const uint32 hash = HashPermutation(shader->GetName(), permutationIndex);
            ASSERT_LOW_LAYER(!_shaders.ContainsKey(hash));
            _shaders.Add(hash, shader);
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
            String name = String::Format(TEXT("{}.CB{}"), ToString(), i);
#else
			String name;
#endif
            ASSERT(_constantBuffers[slotIndex] == nullptr);
            const auto cb = GPUDevice::Instance->CreateConstantBuffer(size, name);
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

bool GPUShader::HasShader(const StringAnsiView& name, int32 permutationIndex) const
{
    const uint32 hash = HashPermutation(name, permutationIndex);
    return _shaders.ContainsKey(hash);
}

GPUShaderProgram* GPUShader::GetShader(ShaderStage stage, const StringAnsiView& name, int32 permutationIndex) const
{
    GPUShaderProgram* shader = nullptr;
    const uint32 hash = HashPermutation(name, permutationIndex);
    _shaders.TryGet(hash, shader);
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

GPUResourceType GPUShader::GetResourceType() const
{
    return GPUResourceType::Shader;
}

void GPUShader::OnReleaseGPU()
{
    for (GPUConstantBuffer*& cb : _constantBuffers)
    {
        if (cb)
        {
            SAFE_DELETE_GPU_RESOURCE(cb);
            cb = nullptr;
        }
    }
    _memoryUsage = 0;
    _shaders.ClearDelete();
}
