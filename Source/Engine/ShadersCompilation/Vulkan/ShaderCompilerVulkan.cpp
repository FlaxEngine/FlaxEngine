// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_VK_SHADER_COMPILER

#include "ShaderCompilerVulkan.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Config.h"
#include "Engine/GraphicsDevice/Vulkan/Types.h"
#include <ThirdParty/LZ4/lz4.h>

// Use glslang for HLSL to SPIR-V translation
// Source: https://github.com/KhronosGroup/glslang
// License: modified BSD
#define NV_EXTENSIONS 1
#define AMD_EXTENSIONS 1
#define ENABLE_HLSL 1
#define ENABLE_OPT 1
#include <ThirdParty/glslang/Public/ShaderLang.h>
#include <ThirdParty/glslang/MachineIndependent/iomapper.h>
#include <ThirdParty/glslang/SPIRV/GlslangToSpv.h>
#include <ThirdParty/spirv-tools/libspirv.hpp>

#if PLATFORM_WINDOWS
// Cooperative-vector shaders are compiled with DXC (HLSL dx::linalg -> SPV_NV_cooperative_vector).
#include "Engine/Core/Types/StringView.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
// COM base types required by dxcapi.h (IncludeWindowsHeaders.h uses WIN32_LEAN_AND_MEAN which omits them).
#include <unknwn.h>     // IUnknown
#include <oaidl.h>      // BSTR
#include <objidl.h>     // IStream
#include <combaseapi.h> // IID_PPV_ARGS
#include "Engine/Platform/Windows/ComPtr.h"
#include <ThirdParty/DirectXShaderCompiler/dxcapi.h>
#endif

#define PRINT_UNIFORMS 0
#define PRINT_DESCRIPTORS 0

namespace
{
    CriticalSection CompileShaderVulkanLocker;
    int32 CompileShaderVulkanInstances = 0;
}

class Includer : public glslang::TShader::Includer
{
private:
    ShaderCompilationContext* _context;

    IncludeResult* include(const char* headerName, const char* includerName, int depth) const
    {
        const char* source;
        int32 sourceLength;
        if (ShaderCompiler::GetIncludedFileSource(_context, includerName, headerName, source, sourceLength))
            return nullptr;
        return New<IncludeResult>(headerName, source, sourceLength, nullptr);
    }

public:
    Includer(ShaderCompilationContext* context)
    {
        _context = context;
    }

public:
    // [glslang::TShader::Include]
    IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override
    {
        return include(headerName, includerName, (int)inclusionDepth);
    }

    IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override
    {
        return include(headerName, includerName, (int)inclusionDepth);
    }

    void releaseInclude(IncludeResult* result) override
    {
        if (result)
            Delete(result);
    }
};

ShaderCompilerVulkan::ShaderCompilerVulkan(ShaderProfile profile)
    : ShaderCompiler(profile)
{
    ScopeLock lock(CompileShaderVulkanLocker);
    if (CompileShaderVulkanInstances == 0)
    {
        glslang::InitializeProcess();
        const auto ver = glslang::GetVersion();
        LOG(Info, "Using glslang {0}.{1}.{2} compiler (SPIR-V version: {3})", ver.major, ver.minor, ver.patch, String(spvSoftwareVersionString()));
    }
    CompileShaderVulkanInstances++;
}

ShaderCompilerVulkan::~ShaderCompilerVulkan()
{
    ScopeLock lock(CompileShaderVulkanLocker);
#if PLATFORM_WINDOWS
    if (_dxcCompiler)
        ((IUnknown*)_dxcCompiler)->Release();
    if (_dxcLibrary)
        ((IUnknown*)_dxcLibrary)->Release();
    if (_dxcModule)
        Platform::FreeLibrary(_dxcModule);
#endif
    CompileShaderVulkanInstances--;
    if (CompileShaderVulkanInstances == 0)
    {
        glslang::FinalizeProcess();
    }
}

// @formatter:off
const TBuiltInResource DefaultTBuiltInResource =
{
    /* .MaxLights = */ 0,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ GPU_MAX_CS_DISPATCH_THREAD_GROUPS,
    /* .MaxComputeWorkGroupCountY = */ GPU_MAX_CS_DISPATCH_THREAD_GROUPS,
    /* .MaxComputeWorkGroupCountZ = */ GPU_MAX_CS_DISPATCH_THREAD_GROUPS,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,
    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }
};
// @formatter:on

struct Descriptor
{
    int32 Slot;
    int32 Binding;
    int32 Size;
    int32 Count;
    SpirvShaderResourceBindingType BindingType;
    VkDescriptorType DescriptorType;
    SpirvShaderResourceType ResourceType;
    PixelFormat ResourceFormat;
    std::string Name;
};

SpirvShaderResourceType GetTextureType(const glslang::TSampler& sampler)
{
    switch (sampler.dim)
    {
    case glslang::Esd1D:
        return sampler.isArrayed() ? SpirvShaderResourceType::Texture1DArray : SpirvShaderResourceType::Texture1D;
    case glslang::Esd2D:
        return sampler.isArrayed() ? SpirvShaderResourceType::Texture2DArray : SpirvShaderResourceType::Texture2D;
    case glslang::Esd3D:
        return SpirvShaderResourceType::Texture3D;
    case glslang::EsdCube:
        return SpirvShaderResourceType::TextureCube;
    default:
        CRASH;
        return SpirvShaderResourceType::Unknown;
    }
}

PixelFormat GetResourceFormat(glslang::TBasicType basicType, uint32 vectorSize)
{
    switch (basicType)
    {
    case glslang::EbtVoid:
        return PixelFormat::Unknown;
    case glslang::EbtFloat:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R32_Float;
        case 2:
            return PixelFormat::R32G32_Float;
        case 3:
            return PixelFormat::R32G32B32_Float;
        case 4:
            return PixelFormat::R32G32B32A32_Float;
        }
        break;
    case glslang::EbtFloat16:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R16_Float;
        case 2:
            return PixelFormat::R16G16_Float;
        case 4:
            return PixelFormat::R16G16B16A16_Float;
        }
        break;
    case glslang::EbtUint:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R32_UInt;
        case 2:
            return PixelFormat::R32G32_UInt;
        case 3:
            return PixelFormat::R32G32B32_UInt;
        case 4:
            return PixelFormat::R32G32B32A32_UInt;
        }
        break;
    case glslang::EbtInt:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R32_SInt;
        case 2:
            return PixelFormat::R32G32_SInt;
        case 3:
            return PixelFormat::R32G32B32_SInt;
        case 4:
            return PixelFormat::R32G32B32A32_SInt;
        }
        break;
    case glslang::EbtUint8:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R8_UInt;
        case 2:
            return PixelFormat::R8G8_UInt;
        case 4:
            return PixelFormat::R8G8B8A8_UInt;
        }
        break;
    case glslang::EbtInt8:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R8_SInt;
        case 2:
            return PixelFormat::R8G8_SInt;
        case 4:
            return PixelFormat::R8G8B8A8_SInt;
        }
        break;
    case glslang::EbtUint16:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R16_UInt;
        case 2:
            return PixelFormat::R16G16_UInt;
        case 4:
            return PixelFormat::R16G16B16A16_UInt;
        }
        break;
    case glslang::EbtInt16:
        switch (vectorSize)
        {
        case 1:
            return PixelFormat::R16_SInt;
        case 2:
            return PixelFormat::R16G16_SInt;
        case 4:
            return PixelFormat::R16G16B16A16_SInt;
        }
        break;
    default:
        break;
    }
    return PixelFormat::Unknown;
}

PixelFormat GetResourceFormat(const glslang::TSampler& sampler)
{
    return GetResourceFormat(sampler.type, sampler.vectorSize);
}

PixelFormat GetResourceFormat(const glslang::TType& type)
{
    return GetResourceFormat(type.getBasicType(), type.getVectorSize());
}

bool IsUavType(const glslang::TType& type)
{
    if (type.getQualifier().isReadOnly())
        return false;
    return (type.getBasicType() == glslang::EbtSampler && type.getSampler().isImage()) || (type.getQualifier().storage == glslang::EvqBuffer);
}

class DescriptorsCollector
{
public:
    int32 Images = 0;
    int32 Buffers = 0;
    int32 TexelBuffers = 0;
    int32 DescriptorsCount = 0;
    Descriptor Descriptors[SpirvShaderDescriptorInfo::MaxDescriptors];

public:
    Descriptor* Add(glslang::TVarEntryInfo& ent)
    {
        const glslang::TType& type = ent.symbol->getType();
        const char* name = ent.symbol->getName().c_str();
        auto& qualifier = type.getQualifier();
        if (DescriptorsCount == SpirvShaderDescriptorInfo::MaxDescriptors)
        {
            // Prevent too many descriptors
            LOG(Warning, "Too many descriptors in use.");
            return nullptr;
        }

        // Guess the descriptor type based on reflection information
        VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        SpirvShaderResourceType resourceType = SpirvShaderResourceType::Unknown;
        SpirvShaderResourceBindingType resourceBindingType = SpirvShaderResourceBindingType::INVALID;
        if (type.getBasicType() == glslang::EbtSampler)
        {
            if (!qualifier.hasBinding())
            {
                // Each resource must have binding specified (from HLSL shaders that do it explicitly)
                LOG(Warning, "Found an uniform \'{0}\' without a binding qualifier. Each uniform must have an explicitly defined binding number.", String(name));
                return nullptr;
            }

            if (type.getSampler().isCombined())
            {
                // Texture + Sampler combined is not supported
                LOG(Warning, "Combined sampler \'{0}\' from glsl language is not supported.", String(name));
                return nullptr;
            }

            if (type.getSampler().isPureSampler())
            {
                // Sampler
                descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                resourceType = SpirvShaderResourceType::Sampler;
                resourceBindingType = SpirvShaderResourceBindingType::SAMPLER;
            }
            else if (type.getSampler().dim == glslang::EsdBuffer)
            {
                if (IsUavType(type))
                {
                    // Buffer UAV
                    descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                    resourceType = SpirvShaderResourceType::Buffer;
                    resourceBindingType = SpirvShaderResourceBindingType::UAV;
                }
                else
                {
                    // Buffer SRV
                    descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                    resourceType = SpirvShaderResourceType::Buffer;
                    resourceBindingType = SpirvShaderResourceBindingType::SRV;
                }
            }
            else if (type.isTexture())
            {
                // Texture SRV
                descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                resourceType = GetTextureType(type.getSampler());
                resourceBindingType = SpirvShaderResourceBindingType::SRV;
            }
            else if (type.isImage())
            {
                if (type.getSampler().dim == glslang::EsdBuffer)
                {
                    // Buffer UAV
                    descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    resourceType = SpirvShaderResourceType::Buffer;
                }
                else
                {
                    // Texture UAV
                    descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    resourceType = GetTextureType(type.getSampler());
                }
                resourceBindingType = SpirvShaderResourceBindingType::UAV;
            }
        }
        else if (qualifier.storage == glslang::EvqUniform)
        {
            if (type.getBasicType() != glslang::EbtBlock)
            {
                // Skip uniforms that are not contained inside structures
                LOG(Warning, "Invalid uniform \'{1} {0}\'. Shader uniforms that are not constant buffer blocks are not supported.", String(name), String(type.getBasicTypeString().c_str()));
                return nullptr;
            }

            // CB
            descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            resourceType = SpirvShaderResourceType::ConstantBuffer;
            name = type.getTypeName().c_str();
            resourceBindingType = SpirvShaderResourceBindingType::CB;
        }
        else if (qualifier.storage == glslang::EvqBuffer)
        {
            if (qualifier.isReadOnly())
            {
                // Buffer SRV
                descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                resourceType = SpirvShaderResourceType::Buffer;
                resourceBindingType = SpirvShaderResourceBindingType::SRV;
            }
            else
            {
                // Buffer UAV
                descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                resourceType = SpirvShaderResourceType::Buffer;
                resourceBindingType = SpirvShaderResourceBindingType::UAV;
            }
        }

        const auto index = DescriptorsCount++;
        auto& descriptor = Descriptors[index];
        descriptor.Binding = index;
        descriptor.Slot = qualifier.layoutBinding;
        descriptor.Size = -1;
        descriptor.BindingType = resourceBindingType;
        descriptor.DescriptorType = descriptorType;
        descriptor.ResourceType = resourceType;
        descriptor.ResourceFormat = GetResourceFormat(type.getSampler());
        descriptor.Name = name;
        descriptor.Count = type.isSizedArray() ? type.getCumulativeArraySize() : 1;

        // Get the output info about shader uniforms usage
        switch (descriptorType)
        {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            Images += descriptor.Count;
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            Buffers += descriptor.Count;
            break;
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            TexelBuffers += descriptor.Count;
            break;
        default:
            LOG(Warning, "Invalid descriptor type {0} for symbol {1}.", (int32)descriptorType, String(name));
            return nullptr;
        }

        return &descriptor;
    }
};

class MyIoMapResolver : public glslang::TDefaultIoResolverBase
{
private:
    int32 _set;
    DescriptorsCollector* _collector;

public:
    MyIoMapResolver(int32 set, DescriptorsCollector* collector, const glslang::TIntermediate& intermediate)
        : TDefaultIoResolverBase(intermediate)
        , _set(set)
        , _collector(collector)
    {
    }

public:
    // [glslang::TDefaultIoResolverBase]
    bool validateBinding(EShLanguage stage, glslang::TVarEntryInfo& ent) override
    {
        return true;
    }

    glslang::TResourceType getResourceType(const glslang::TType& type) override
    {
        if (isUavType(type))
            return glslang::EResUav;
        if (isSrvType(type))
            return glslang::EResTexture;
        if (isSamplerType(type))
            return glslang::EResSampler;
        if (isUboType(type))
            return glslang::EResUbo;
        return glslang::EResCount;
    }

    int resolveBinding(EShLanguage stage, glslang::TVarEntryInfo& ent) override
    {
        // Skip unused things
        if (!ent.live)
            return -1;

        // Add resource
        const auto descriptor = _collector->Add(ent);
        if (descriptor)
        {
            return ent.newBinding = reserveSlot(_set, descriptor->Binding, descriptor->Count);
        }
        return ent.newBinding;
    }

    int resolveSet(EShLanguage stage, glslang::TVarEntryInfo& ent) override
    {
        // Skip unused things
        if (!ent.live)
            return -1;

        // Use different slot per-stage
        return ent.newSet = _set;
    }
};

#if PLATFORM_WINDOWS

// ---------------------------------------------------------------------------
// Cooperative-vector (NVIDIA Neural Shading) shaders are compiled with DXC to SPIR-V because glslang
// cannot translate the dx::linalg intrinsics. DXC emits resources in descriptor set 0 with bindings
// derived from the HLSL register (offset by the -fvk-*-shift values below). We reflect the module to
// build Flax's SpirvShaderDescriptorInfo and rewrite each resource decoration to the engine convention
// (set = pipeline stage set, binding = sequential index), matching what the glslang path produces.
// ---------------------------------------------------------------------------

namespace SpvConst
{
    enum { MagicNumber = 0x07230203, HeaderWords = 5 };
    enum Op
    {
        OpName = 5,
        OpMemoryModel = 14,
        OpTypeImage = 25,
        OpTypeSampler = 26,
        OpTypeArray = 28,
        OpTypeRuntimeArray = 29,
        OpTypeStruct = 30,
        OpTypePointer = 32,
        OpConstant = 43,
        OpVariable = 59,
        OpDecorate = 71,
    };
    enum Decoration
    {
        DecorationBlock = 2,
        DecorationBufferBlock = 3,
        DecorationLocation = 30,
        DecorationBinding = 33,
        DecorationDescriptorSet = 34,
    };
    enum StorageClass
    {
        StorageClassUniformConstant = 0,
        StorageClassInput = 1,
        StorageClassUniform = 2,
        StorageClassOutput = 3,
        StorageClassStorageBuffer = 12,
    };
    enum Dim
    {
        Dim1D = 0,
        Dim3D = 2,
        DimCube = 3,
        DimBuffer = 5,
    };
}

struct SpvIdInfo
{
    uint16 Op = 0;
    int32 Set = -1;
    int32 Binding = -1;
    int32 SetWordIndex = -1;
    int32 BindingWordIndex = -1;
    uint32 StorageClass = 0xFFFFFFFF;
    uint32 TypeRef = 0;
    uint32 ElementType = 0;
    uint32 ArrayLen = 1;
    uint32 ConstValue = 0;
    uint32 ImgDim = 0;
    uint32 ImgSampled = 0;
    bool HasLocation = false;
    bool IsStruct = false;
    bool IsImage = false;
    bool IsSampler = false;
    bool IsArray = false;
    bool IsRuntimeArray = false;
    bool IsBlock = false;
    bool IsBufferBlock = false;
};

// The SPIR-V CooperativeVectorNV capability requires the Vulkan memory model, which in turn
// requires OpMemoryModel to select the Vulkan memory model (DXC emits GLSL450 by default).
// Patch the memory-model operand in place (the addressing model is left untouched).
static void PatchSpirvVulkanMemoryModel(std::vector<unsigned>& spirv)
{
    using namespace SpvConst;
    if (spirv.size() < HeaderWords || spirv[0] != MagicNumber)
        return;
    enum { MemoryModelVulkan = 3 };
    size_t i = HeaderWords;
    while (i < spirv.size())
    {
        const uint32 word0 = spirv[i];
        const uint16 op = (uint16)(word0 & 0xFFFFu);
        const uint16 wordCount = (uint16)(word0 >> 16);
        if (wordCount == 0 || i + wordCount > spirv.size())
            return;
        if (op == OpMemoryModel && wordCount >= 3)
        {
            spirv[i + 2] = MemoryModelVulkan;
            return;
        }
        i += wordCount;
    }
}

static bool ReflectAndRemapCoopVecSpirv(std::vector<unsigned>& spirv, int32 stageSet, SpirvShaderHeader& header, ShaderBindings& bindings)
{
    using namespace SpvConst;
    if (spirv.size() < HeaderWords || spirv[0] != MagicNumber)
    {
        LOG(Warning, "CoopVec reflect: bad SPIR-V header (size={0} words, magic=0x{1:x}).", (int32)spirv.size(), spirv.empty() ? 0u : spirv[0]);
        return false;
    }
    const uint32 bound = spirv[3]; // SPIR-V header: [0]=magic [1]=version [2]=generator [3]=bound [4]=schema
    if (bound == 0)
    {
        LOG(Warning, "CoopVec reflect: SPIR-V id bound is 0.");
        return false;
    }
    std::vector<SpvIdInfo> ids((size_t)bound);

    // Pass 1: scan the module
    size_t i = HeaderWords;
    while (i < spirv.size())
    {
        const uint32 word0 = spirv[i];
        const uint16 op = (uint16)(word0 & 0xFFFFu);
        const uint16 wordCount = (uint16)(word0 >> 16);
        if (wordCount == 0 || i + wordCount > spirv.size())
        {
            LOG(Warning, "CoopVec reflect: malformed instruction at word {0} (op={1}, wordCount={2}, total={3}).", (int32)i, (int32)op, (int32)wordCount, (int32)spirv.size());
            return false;
        }
        switch (op)
        {
        case OpDecorate:
            if (wordCount >= 3)
            {
                const uint32 target = spirv[i + 1];
                const uint32 deco = spirv[i + 2];
                if (target < bound)
                {
                    auto& d = ids[(int32)target];
                    if (deco == DecorationBinding && wordCount >= 4)
                    {
                        d.Binding = (int32)spirv[i + 3];
                        d.BindingWordIndex = (int32)(i + 3);
                    }
                    else if (deco == DecorationDescriptorSet && wordCount >= 4)
                    {
                        d.Set = (int32)spirv[i + 3];
                        d.SetWordIndex = (int32)(i + 3);
                    }
                    else if (deco == DecorationLocation)
                        d.HasLocation = true;
                    else if (deco == DecorationBlock)
                        d.IsBlock = true;
                    else if (deco == DecorationBufferBlock)
                        d.IsBufferBlock = true;
                }
            }
            break;
        case OpTypePointer:
            if (wordCount >= 4)
            {
                const uint32 res = spirv[i + 1];
                if (res < bound)
                {
                    auto& d = ids[(int32)res];
                    d.Op = OpTypePointer;
                    d.StorageClass = spirv[i + 2];
                    d.TypeRef = spirv[i + 3];
                }
            }
            break;
        case OpTypeStruct:
            if (wordCount >= 2)
            {
                const uint32 res = spirv[i + 1];
                if (res < bound)
                    ids[(int32)res].IsStruct = true;
            }
            break;
        case OpTypeImage:
            if (wordCount >= 8)
            {
                const uint32 res = spirv[i + 1];
                if (res < bound)
                {
                    auto& d = ids[(int32)res];
                    d.IsImage = true;
                    d.ImgDim = spirv[i + 3];
                    d.ImgSampled = spirv[i + 7];
                }
            }
            break;
        case OpTypeSampler:
            if (wordCount >= 2)
            {
                const uint32 res = spirv[i + 1];
                if (res < bound)
                    ids[(int32)res].IsSampler = true;
            }
            break;
        case OpTypeArray:
            if (wordCount >= 4)
            {
                const uint32 res = spirv[i + 1];
                if (res < bound)
                {
                    auto& d = ids[(int32)res];
                    d.IsArray = true;
                    d.ElementType = spirv[i + 2];
                    const uint32 lenId = spirv[i + 3];
                    d.ArrayLen = (lenId < bound && ids[(int32)lenId].ConstValue > 0) ? ids[(int32)lenId].ConstValue : 1;
                }
            }
            break;
        case OpTypeRuntimeArray:
            if (wordCount >= 3)
            {
                const uint32 res = spirv[i + 1];
                if (res < bound)
                {
                    auto& d = ids[(int32)res];
                    d.IsRuntimeArray = true;
                    d.ElementType = spirv[i + 2];
                }
            }
            break;
        case OpConstant:
            if (wordCount >= 4)
            {
                const uint32 res = spirv[i + 2];
                if (res < bound)
                    ids[(int32)res].ConstValue = spirv[i + 3];
            }
            break;
        case OpVariable:
            if (wordCount >= 4)
            {
                const uint32 res = spirv[i + 2];
                if (res < bound)
                {
                    auto& d = ids[(int32)res];
                    d.Op = OpVariable;
                    d.TypeRef = spirv[i + 1];
                    d.StorageClass = spirv[i + 3];
                }
            }
            break;
        default:
            break;
        }
        i += wordCount;
    }

    // Pass 2: build descriptors and rewrite bindings
    auto& info = header.DescriptorInfo;
    int32 inputs = 0, outputs = 0;
    for (uint32 id = 0; id < bound; id++)
    {
        const SpvIdInfo& v = ids[(int32)id];
        if (v.Op != OpVariable)
            continue;
        if (v.StorageClass == StorageClassInput)
        {
            if (v.HasLocation)
                inputs++;
            continue;
        }
        if (v.StorageClass == StorageClassOutput)
        {
            if (v.HasLocation)
                outputs++;
            continue;
        }
        if (v.StorageClass != StorageClassUniform && v.StorageClass != StorageClassStorageBuffer && v.StorageClass != StorageClassUniformConstant)
            continue;
        if (v.Binding < 0 || v.BindingWordIndex < 0 || v.TypeRef >= bound)
            continue;

        // Resolve the variable's pointee type, unwrapping resource arrays
        uint32 baseId = ids[(int32)v.TypeRef].TypeRef;
        uint32 count = 1;
        while (baseId < bound && (ids[(int32)baseId].IsArray || ids[(int32)baseId].IsRuntimeArray))
        {
            const SpvIdInfo& a = ids[(int32)baseId];
            if (a.IsArray)
                count *= a.ArrayLen;
            baseId = a.ElementType;
        }
        if (baseId >= bound)
            continue;
        const SpvIdInfo& base = ids[(int32)baseId];

        const int32 btype = v.Binding / 100; // 0=b,1=t,2=u,3=s (from -fvk-*-shift)
        const int32 reg = v.Binding % 100;

        SpirvShaderResourceBindingType bindingType = SpirvShaderResourceBindingType::INVALID;
        VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        SpirvShaderResourceType resourceType = SpirvShaderResourceType::Unknown;
        if (base.IsStruct)
        {
            if (v.StorageClass == StorageClassUniform && base.IsBlock && !base.IsBufferBlock)
            {
                bindingType = SpirvShaderResourceBindingType::CB;
                descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                resourceType = SpirvShaderResourceType::ConstantBuffer;
            }
            else
            {
                bindingType = (btype == 2) ? SpirvShaderResourceBindingType::UAV : SpirvShaderResourceBindingType::SRV;
                descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                resourceType = SpirvShaderResourceType::Buffer;
            }
        }
        else if (base.IsImage)
        {
            const bool isStorage = base.ImgSampled == 2 || btype == 2;
            bindingType = isStorage ? SpirvShaderResourceBindingType::UAV : SpirvShaderResourceBindingType::SRV;
            if (base.ImgDim == DimBuffer)
            {
                resourceType = SpirvShaderResourceType::Buffer;
                descriptorType = isStorage ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            }
            else
            {
                descriptorType = isStorage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                switch (base.ImgDim)
                {
                case Dim1D: resourceType = SpirvShaderResourceType::Texture1D; break;
                case Dim3D: resourceType = SpirvShaderResourceType::Texture3D; break;
                case DimCube: resourceType = SpirvShaderResourceType::TextureCube; break;
                default: resourceType = SpirvShaderResourceType::Texture2D; break;
                }
            }
        }
        else if (base.IsSampler)
        {
            bindingType = SpirvShaderResourceBindingType::SAMPLER;
            descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            resourceType = SpirvShaderResourceType::Sampler;
        }
        else
        {
            continue;
        }

        if (info.DescriptorTypesCount >= SpirvShaderDescriptorInfo::MaxDescriptors)
        {
            LOG(Warning, "Too many descriptors in cooperative-vector shader.");
            return false;
        }
        const int32 newBinding = (int32)info.DescriptorTypesCount;
        auto& d = info.DescriptorTypes[info.DescriptorTypesCount++];
        d.Binding = (byte)newBinding;
        d.Set = (byte)stageSet;
        d.Slot = (byte)reg;
        d.BindingType = bindingType;
        d.DescriptorType = descriptorType;
        d.ResourceType = resourceType;
        d.ResourceFormat = PixelFormat::Unknown;
        d.Count = count;

        // Rewrite SPIR-V decorations to match the engine convention
        spirv[v.BindingWordIndex] = (uint32)newBinding;
        if (v.SetWordIndex >= 0)
            spirv[v.SetWordIndex] = (uint32)stageSet;

        switch (descriptorType)
        {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            info.ImageInfosCount += (uint16)count;
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            info.BufferInfosCount += (uint16)count;
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            info.TexelBufferViewsCount += count;
            break;
        default:
            break;
        }
        switch (bindingType)
        {
        case SpirvShaderResourceBindingType::SAMPLER: bindings.UsedSamplersMask |= 1 << reg; break;
        case SpirvShaderResourceBindingType::CB: bindings.UsedCBsMask |= 1 << reg; break;
        case SpirvShaderResourceBindingType::SRV: bindings.UsedSRsMask |= 1 << reg; break;
        case SpirvShaderResourceBindingType::UAV: bindings.UsedUAsMask |= 1 << reg; break;
        default: break;
        }
    }

    bindings.InputsCount = inputs;
    bindings.OutputsCount = outputs;
    return true;
}

// DXC include handler that resolves engine shader includes (mirrors the DirectX backend handler).
class CoopVecIncludeDX : public IDxcIncludeHandler
{
private:
    ShaderCompilationContext* _context;
    IDxcLibrary* _library;

public:
    CoopVecIncludeDX(ShaderCompilationContext* context, IDxcLibrary* library)
        : _context(context)
        , _library(library)
    {
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof(IDxcIncludeHandler) || riid == __uuidof(IUnknown))
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
    {
        *ppIncludeSource = nullptr;
        const char* source;
        int32 sourceLength;
        StringAnsi filename(pFilename);
        filename.Replace('\\', '/');
        if (ShaderCompiler::GetIncludedFileSource(_context, "", filename.Get(), source, sourceLength))
            return E_FAIL;
        IDxcBlobEncoding* textBlob;
        if (FAILED(_library->CreateBlobWithEncodingFromPinned((LPBYTE)source, sourceLength, CP_UTF8, &textBlob)))
            return E_FAIL;
        *ppIncludeSource = textBlob;
        return S_OK;
    }
};

bool ShaderCompilerVulkan::InitDXC()
{
    if (_dxcInitDone)
        return _dxcCompiler != nullptr && _dxcLibrary != nullptr;
    _dxcInitDone = true;

    // Cooperative vectors need a SPIR-V-enabled DXC. The stock dxcompiler.dll deployed next to the
    // executable (from the Windows SDK) is built WITHOUT SPIR-V codegen and fails with
    // "SPIR-V CodeGen not available", so prefer the SPIR-V-enabled build vendored in the engine's
    // ThirdParty deps folder (where the README has the user drop the preview DXC). Platform::LoadLibrary
    // also adds that folder to the DLL search path so the sibling dxil.dll resolves.
    const String depsDxc = Globals::StartupFolder / TEXT("Source/Platforms/Windows/Binaries/ThirdParty/x64/dxcompiler.dll");
    if (FileSystem::FileExists(depsDxc))
        _dxcModule = Platform::LoadLibrary(depsDxc.Get());
    if (!_dxcModule)
        _dxcModule = (void*)LoadLibraryW(L"dxcompiler.dll"); // fall back to the copy next to the executable
    if (!_dxcModule)
    {
        LOG(Warning, "Failed to load dxcompiler.dll for cooperative-vector SPIR-V compilation.");
        return false;
    }
    const auto createInstance = (DxcCreateInstanceProc)Platform::GetProcAddress(_dxcModule, "DxcCreateInstance");
    if (!createInstance)
    {
        LOG(Warning, "dxcompiler.dll is missing DxcCreateInstance.");
        return false;
    }
    IDxcCompiler3* compiler = nullptr;
    IDxcLibrary* library = nullptr;
    if (FAILED(createInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&compiler)) ||
        FAILED(createInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&library)))
    {
        LOG(Warning, "DxcCreateInstance failed for cooperative-vector SPIR-V compilation.");
        return false;
    }
    _dxcCompiler = compiler;
    _dxcLibrary = library;
    return true;
}

int32 ShaderCompilerVulkan::CompileShaderCoopVec(ShaderFunctionMeta& meta, WritePermutationData customDataWrite)
{
    auto options = _context->Options;
    auto compiler = (IDxcCompiler3*)_dxcCompiler;
    auto library = (IDxcLibrary*)_dxcLibrary;
    const auto type = meta.GetStage();
    CoopVecIncludeDX include(_context, library);

    const Char* targetProfile = type == ShaderStage::Compute ? TEXT("cs_6_9") : TEXT("ps_6_9");
    const int32 stageSet = type == ShaderStage::Compute ? 0 : 1;

    ComPtr<IDxcBlobEncoding> textBlob;
    if (FAILED(library->CreateBlobWithEncodingFromPinned((LPBYTE)options->Source, options->SourceLength, CP_UTF8, &textBlob)))
        return 2;
    DxcBuffer textBuffer;
    textBuffer.Ptr = textBlob->GetBufferPointer();
    textBuffer.Size = textBlob->GetBufferSize();
    textBuffer.Encoding = DXC_CP_ACP;
    const StringAsUTF16<> entryPoint(meta.Name.Get(), meta.Name.Length());
    // Force the SPIR-V entry point name to match the engine convention (the Vulkan runtime binds the
    // stage with pName = shader program name), as DXC may otherwise rename the entry to "main".
    String entryNameArg(TEXT("-fspv-entrypoint-name="));
    entryNameArg += entryPoint.Get();

    // Phase 1: compile and reflect every permutation. This is all-or-nothing: if DXC cannot emit
    // cooperative-vector SPIR-V (old compiler, unsupported intrinsics, ...), bail out before writing
    // anything so the caller can fall back to the glslang fp32 path.
    struct CompiledPermutation
    {
        std::vector<unsigned> Spirv;
        SpirvShaderHeader Header;
        ShaderBindings Bindings;
    };
    std::vector<CompiledPermutation> compiled((size_t)meta.Permutations.Count());
    for (int32 permutationIndex = 0; permutationIndex < meta.Permutations.Count(); permutationIndex++)
    {
        _macros.Clear();
        meta.GetDefinitionsForPermutation(permutationIndex, _macros);
        GetDefineForFunction(meta, _macros);
        _macros.Add(_context->Options->Macros);
        _macros.Add(_globalMacros);

        // Convert defines to "NAME[=VALUE]" UTF-16 strings
        const int32 macrosCount = _macros.Count() - 1;
        Array<String> definesStrings;
        definesStrings.Resize(macrosCount);
        for (int32 m = 0; m < macrosCount; m++)
        {
            auto& macro = _macros[m];
            auto& define = definesStrings[m];
            define = macro.Name;
            if (macro.Definition && *macro.Definition)
            {
                define += TEXT("=");
                define += macro.Definition;
            }
        }

        // Build DXC arguments
        Array<const Char*> args;
        args.Add(options->NoOptimize ? DXC_ARG_SKIP_OPTIMIZATIONS : DXC_ARG_OPTIMIZATION_LEVEL3);
        args.Add(TEXT("-enable-16bit-types"));
        args.Add(TEXT("-spirv"));
        // SPV_NV_cooperative_vector requires SPIR-V 1.6 (Vulkan 1.3) and the Vulkan memory model.
        args.Add(TEXT("-fspv-target-env=vulkan1.3"));
        // Skip DXC's bundled spirv-val: its SPV_NV_cooperative_vector support is incomplete and it
        // rejects spec-legal constructs (e.g. single-scalar OpCompositeConstruct splat of a cooperative
        // vector). The NVIDIA driver validates/compiles the cooperative-vector module at pipeline creation.
        args.Add(TEXT("-Vd"));
        // Do not pass -fspv-extension: that flag *restricts* the allowed extensions and some DXC builds
        // do not recognize the 'SPV_NV_cooperative_vector' name even when their codegen can emit it.
        // Omitting it lets DXC auto-enable whatever extensions the cooperative-vector lowering needs.
        args.Add(TEXT("-fvk-use-dx-layout"));
        // Map b/t/u/s register classes into disjoint binding ranges so the reflector can recover the
        // HLSL register index from the SPIR-V binding (binding = register + 100 * classIndex).
        args.Add(TEXT("-fvk-b-shift")); args.Add(TEXT("0")); args.Add(TEXT("0"));
        args.Add(TEXT("-fvk-t-shift")); args.Add(TEXT("100")); args.Add(TEXT("0"));
        args.Add(TEXT("-fvk-u-shift")); args.Add(TEXT("200")); args.Add(TEXT("0"));
        args.Add(TEXT("-fvk-s-shift")); args.Add(TEXT("300")); args.Add(TEXT("0"));
        args.Add(TEXT("-T")); args.Add(targetProfile);
        args.Add(TEXT("-E")); args.Add(entryPoint.Get());
        args.Add(*entryNameArg);
        args.Add(options->TargetName.Get());
        for (auto& define : definesStrings)
        {
            args.Add(TEXT("-D"));
            args.Add(*define);
        }

        // Compile
        ComPtr<IDxcResult> results;
        HRESULT result = compiler->Compile(&textBuffer, (LPCWSTR*)args.Get(), args.Count(), &include, IID_PPV_ARGS(&results));
        if (SUCCEEDED(result) && results)
            results->GetStatus(&result);
        if (FAILED(result))
        {
            if (results)
            {
                ComPtr<IDxcBlobEncoding> error;
                results->GetErrorBuffer(&error);
                if (error && error->GetBufferSize() > 0)
                {
                    ComPtr<IDxcBlobEncoding> errorUtf8;
                    library->GetBlobAsUtf8(error, &errorUtf8);
                    if (errorUtf8)
                        LOG(Warning, "DXC cooperative-vector SPIR-V compile failed for '{0}': {1}", String(meta.Name), String((const char*)errorUtf8->GetBufferPointer()));
                }
            }
            return 2;
        }

        // Get the SPIR-V output
        ComPtr<IDxcBlob> shaderBuffer;
        if (FAILED(results->GetResult(&shaderBuffer)) || !shaderBuffer || shaderBuffer->GetBufferSize() < SpvConst::HeaderWords * sizeof(unsigned))
        {
            LOG(Warning, "DXC produced no SPIR-V for cooperative-vector shader '{0}'.", String(meta.Name));
            return 2;
        }
        CompiledPermutation& cp = compiled[permutationIndex];
        const int32 spirvWords = (int32)(shaderBuffer->GetBufferSize() / sizeof(unsigned));
        cp.Spirv.resize((size_t)spirvWords);
        Platform::MemoryCopy(cp.Spirv.data(), shaderBuffer->GetBufferPointer(), (uint64)spirvWords * sizeof(unsigned));

        // Cooperative vectors require the Vulkan memory model to be declared in the module.
        PatchSpirvVulkanMemoryModel(cp.Spirv);

        // Reflect and remap the bindings to the engine layout
        Platform::MemoryClear(&cp.Header, sizeof(cp.Header));
        cp.Bindings = {};
        if (!ReflectAndRemapCoopVecSpirv(cp.Spirv, stageSet, cp.Header, cp.Bindings))
        {
            LOG(Warning, "Failed to reflect cooperative-vector SPIR-V for shader '{0}'.", String(meta.Name));
            return 2;
        }
    }

    // Phase 2: everything compiled - commit the results to the shader cache
    for (int32 permutationIndex = 0; permutationIndex < (int32)compiled.size(); permutationIndex++)
    {
        CompiledPermutation& cp = compiled[(size_t)permutationIndex];
        if (Write(_context, meta, permutationIndex, cp.Bindings, cp.Header, cp.Spirv))
        {
            LOG(Error, "Failed to write cooperative-vector shader '{0}'.", String(meta.Name));
            return 1; // Hard failure mid-write; do not fall back (cache is already partially written)
        }
        if (customDataWrite)
        {
            // Rebuild the permutation macros for the custom data writer (matches the glslang path)
            _macros.Clear();
            meta.GetDefinitionsForPermutation(permutationIndex, _macros);
            GetDefineForFunction(meta, _macros);
            _macros.Add(_context->Options->Macros);
            _macros.Add(_globalMacros);
            if (customDataWrite(_context, meta, permutationIndex, _macros, nullptr))
                return 1;
        }
    }

    if (WriteShaderFunctionEnd(_context, meta))
        return 1;
    LOG(Info, "Compiled cooperative-vector SPIR-V (SPV_NV_cooperative_vector) for shader '{0}' via DXC.", String(meta.Name));
    return 0;
}

#endif

bool ShaderCompilerVulkan::CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite)
{
    // TODO: test without locking
    ScopeLock lock(CompileShaderVulkanLocker);
    Includer includer(_context);

    // Prepare
    if (WriteShaderFunctionBegin(_context, meta))
        return true;
    auto type = meta.GetStage();

#if PLATFORM_WINDOWS
    // Cooperative-vector shaders (NVIDIA Neural Shading) use cooperative-vector intrinsics that glslang
    // cannot translate, so they are routed through DXC to emit SPV_NV_cooperative_vector SPIR-V. There
    // is no fp32 fallback: a coopvec-flagged shader requires DXC and must compile, otherwise it is a
    // hard error (the HLSL has no non-coopvec body for glslang to compile).
    if (EnumHasAnyFlags(meta.Flags, ShaderFlags::CooperativeVector) && (type == ShaderStage::Pixel || type == ShaderStage::Compute))
    {
        if (!InitDXC())
        {
            LOG(Error, "Cooperative-vector shader '{0}' requires DXC, but dxcompiler could not be initialized.", String(meta.Name));
            return true;
        }
        const int32 r = CompileShaderCoopVec(meta, customDataWrite);
        if (r == 0)
            return false; // Compiled real cooperative-vector SPIR-V (success)
        LOG(Error, "Cooperative-vector shader '{0}' failed to compile via DXC (no fp32 fallback).", String(meta.Name));
        return true;
    }
#endif

    // Prepare
    EShLanguage lang = EShLanguage::EShLangCount;
    switch (type)
    {
    case ShaderStage::Vertex:
        lang = EShLanguage::EShLangVertex;
        break;
    case ShaderStage::Hull:
        lang = EShLanguage::EShLangTessControl;
        break;
    case ShaderStage::Domain:
        lang = EShLanguage::EShLangTessEvaluation;
        break;
    case ShaderStage::Geometry:
        lang = EShLanguage::EShLangGeometry;
        break;
    case ShaderStage::Pixel:
        lang = EShLanguage::EShLangFragment;
        break;
    case ShaderStage::Compute:
        lang = EShLanguage::EShLangCompute;
        break;
    default:
        LOG(Error, "Unknown shader type.");
        return true;
    }
    EShMessages messages = (EShMessages)(EShMsgReadHlsl | EShMsgSpvRules | EShMsgVulkanRules);

    // Compile all shader function permutations
    AdditionalDataVS additionalDataVS;
    for (int32 permutationIndex = 0; permutationIndex < meta.Permutations.Count(); permutationIndex++)
    {
#if PRINT_DESCRIPTORS
        LOG(Warning, "VULKAN SHADER {0}: {1}[{2}]", _context->Options->TargetName, String(meta.Name), permutationIndex);
#endif
        _macros.Clear();

        // Get function permutation macros
        meta.GetDefinitionsForPermutation(permutationIndex, _macros);

        // Add additional define for compiled function name
        GetDefineForFunction(meta, _macros);

        // Add custom and global macros (global last because contain null define to indicate ending)
        _macros.Add(_context->Options->Macros);
        _macros.Add(_globalMacros);

        // Offset inputs for some pipeline stages to match the descriptors sets layout
        int32 stageSet;
        switch (type)
        {
        case ShaderStage::Vertex:
        case ShaderStage::Compute:
            stageSet = 0;
            break;
        case ShaderStage::Pixel:
            stageSet = 1;
            break;
        case ShaderStage::Geometry:
            stageSet = 2;
            break;
        case ShaderStage::Hull:
            stageSet = 3;
            break;
        case ShaderStage::Domain:
            stageSet = 4;
            break;
        default:
            LOG(Error, "Unknown shader type.");
            return true;
        }

        // Parse HLSL shader using glslang
        glslang::TShader shader(lang);
        glslang::TProgram program;
        shader.setEntryPoint(meta.Name.Get());
        shader.setSourceEntryPoint(meta.Name.Get());
        int lengths = _context->Options->SourceLength - 1;
        const char* names = _context->TargetNameAnsi;
        shader.setStringsWithLengthsAndNames(&_context->Options->Source, &lengths, &names, 1);
        const int defaultVersion = 450;
        std::string preamble;
        for (int32 i = 0; i < _macros.Count() - 1; i++)
        {
            auto& macro = _macros[i];
            preamble.append("#define ");
            preamble.append(macro.Name);
            if (macro.Definition)
            {
                preamble.append(" ");
                preamble.append(macro.Definition);
            }
            preamble.append("\n");
        }
        shader.setPreamble(preamble.c_str());
        shader.setEnvInput(glslang::EShSourceHlsl, lang, glslang::EShClientVulkan, defaultVersion);
        InitParsing(_context, shader);
        if (!shader.parse(&DefaultTBuiltInResource, defaultVersion, false, messages, includer))
        {
            const auto msg = shader.getInfoLog();
            _context->OnError(msg);
            return true;
        }
        program.addShader(&shader);

        // Generate reflection information
        if (!program.link(messages))
        {
            const auto msg = program.getInfoLog();
            _context->OnError(msg);
            return true;
        }
        if (!program.getIntermediate(lang))
        {
            const auto msg = program.getInfoLog();
            _context->OnError(msg);
            return true;
        }
        DescriptorsCollector descriptorsCollector;
        MyIoMapResolver resolver(stageSet, &descriptorsCollector, *program.getIntermediate(lang));
        if (!program.mapIO(&resolver))
        {
            const auto msg = program.getInfoLog();
            _context->OnError(msg);
            return true;
        }
        if (!program.buildReflection())
        {
            const auto msg = program.getInfoLog();
            _context->OnError(msg);
            return true;
        }

        // Process shader reflection data
        void* additionalData = nullptr;
        SpirvShaderHeader header;
        Platform::MemoryClear(&header, sizeof(header));
        ShaderBindings bindings = {};
        bindings.InputsCount = program.getNumPipeInputs();
        bindings.OutputsCount = program.getNumPipeOutputs();
        if (type == ShaderStage::Vertex)
        {
            additionalData = &additionalDataVS;
            additionalDataVS.Inputs.Clear();
            for (int inputIndex = 0; inputIndex < program.getNumPipeInputs(); inputIndex++)
            {
                const glslang::TObjectReflection& input = program.getPipeInput(inputIndex);
                if (!input.getType() || input.getType()->containsBuiltIn())
                    continue;
                additionalDataVS.Inputs.Add({ ParseVertexElementType(input.getType()->getQualifier().semanticName), 0, 0, 0, GetResourceFormat(*input.getType()) });
            }
        }
        for (int blockIndex = 0; blockIndex < program.getNumLiveUniformBlocks(); blockIndex++)
        {
            auto size = program.getUniformBlockSize(blockIndex);
            auto uniform = program.getUniformBlockTType(blockIndex);
            auto& qualifier = uniform->getQualifier();
            auto binding = (int32)qualifier.layoutBinding;

            if (!qualifier.hasBinding())
            {
                // Each uniform must have a valid binding
                //LOG(Warning, "Found a uniform block \'{0}\' without a binding qualifier. Each uniform block must have an explicitly defined binding number.", String(uniform->getTypeName().c_str()));
                continue;
            }

            // Shared storage buffer
            if (qualifier.storage == glslang::EvqBuffer)
            {
                // RWBuffer
            }
            else
            {
                // Uniform buffer
                bool found = false;
                for (int32 i = 0; i < descriptorsCollector.DescriptorsCount; i++)
                {
                    auto& descriptor = descriptorsCollector.Descriptors[i];
                    if (descriptor.BindingType == SpirvShaderResourceBindingType::CB && descriptor.Binding == binding)
                    {
                        found = true;
                        descriptor.Size = size;
                        break;
                    }
                }
                if (!found)
                {
                    LOG(Warning, "Failed to find descriptor for the uniform block \'{0}\' of size {1} (bytes), binding: {2}.", String(uniform->getTypeName().c_str()), size, binding);
                }
            }
        }

#if PRINT_UNIFORMS
        // Debug printing all uniforms
        for (int32 index = 0; index < program.getNumLiveUniformVariables(); index++)
        {
            auto uniform = program.getUniformTType(index);
            auto qualifier = uniform->getQualifier();
            if (!uniform->isArray())
                LOG(Warning, "Shader {0}:{1} - uniform: {2} {3} at binding {4}",
                _context->TargetNameAnsi,
                String(meta.Name),
                uniform->getCompleteString().c_str(),
                program.getUniformName(index),
                qualifier.layoutBinding
            );
        }
#endif

        // Process all descriptors
        header.DescriptorInfo.ImageInfosCount = descriptorsCollector.Images;
        header.DescriptorInfo.BufferInfosCount = descriptorsCollector.Buffers;
        header.DescriptorInfo.TexelBufferViewsCount = descriptorsCollector.TexelBuffers;
        for (int32 i = 0; i < descriptorsCollector.DescriptorsCount; i++)
        {
            auto& descriptor = descriptorsCollector.Descriptors[i];

            // Skip cases (eg. AppendStructuredBuffer counter buffer)
            if (descriptor.Slot == MAX_uint16)
                continue;

            auto& d = header.DescriptorInfo.DescriptorTypes[header.DescriptorInfo.DescriptorTypesCount++];
            d.Binding = descriptor.Binding;
            d.Set = stageSet;
            d.Slot = descriptor.Slot;
            d.BindingType = descriptor.BindingType;
            d.DescriptorType = descriptor.DescriptorType;
            d.ResourceType = descriptor.ResourceType;
            d.ResourceFormat = descriptor.ResourceFormat;
            d.Count = descriptor.Count;

            switch (descriptor.BindingType)
            {
            case SpirvShaderResourceBindingType::SAMPLER:
                ASSERT_LOW_LAYER(descriptor.Slot >= 0 && descriptor.Slot < GPU_MAX_SAMPLER_BINDED);
                bindings.UsedSamplersMask |= 1 << descriptor.Slot;
                break;
            case SpirvShaderResourceBindingType::CB:
                ASSERT_LOW_LAYER(descriptor.Slot >= 0 && descriptor.Slot < GPU_MAX_CB_BINDED);
                bindings.UsedCBsMask |= 1 << descriptor.Slot;
                break;
            case SpirvShaderResourceBindingType::SRV:
                ASSERT_LOW_LAYER(descriptor.Slot >= 0 && descriptor.Slot < GPU_MAX_SR_BINDED);
                bindings.UsedSRsMask |= 1 << descriptor.Slot;
                break;
            case SpirvShaderResourceBindingType::UAV:
                ASSERT_LOW_LAYER(descriptor.Slot >= 0 && descriptor.Slot < GPU_MAX_UA_BINDED);
                bindings.UsedUAsMask |= 1 << descriptor.Slot;
                break;
            }

            if (descriptor.BindingType == SpirvShaderResourceBindingType::CB)
            {
                if (descriptor.Size == -1)
                {
                    // Skip unused constant buffers
                    continue;
                }
                if (descriptor.Size == 0)
                {
                    LOG(Warning, "Found constant buffer \'{1}\' at slot {0} but it's not used or has no valid size.", descriptor.Slot, String(descriptor.Name.c_str()));
                    continue;
                }

                for (int32 b = 0; b < _constantBuffers.Count(); b++)
                {
                    auto& cc = _constantBuffers[b];
                    if (cc.Slot == descriptor.Slot)
                    {
                        // Mark as used and cache some data
                        cc.IsUsed = true;
                        cc.Size = descriptor.Size;
                        break;
                    }
                }
            }

#if PRINT_DESCRIPTORS
            String type;
            switch (descriptor.BindingType)
            {
            case SpirvShaderResourceBindingType::INVALID:
                type = TEXT("INVALID");
                break;
            case SpirvShaderResourceBindingType::CB:
                type = TEXT("CB");
                break;
            case SpirvShaderResourceBindingType::SAMPLER:
                type = TEXT("SAMPLER");
                break;
            case SpirvShaderResourceBindingType::SRV:
                type = TEXT("SRV");
                break;
            case SpirvShaderResourceBindingType::UAV:
                type = TEXT("UAV");
                break;
            default:
                type = TEXT("?");
            }
            LOG(Warning, "VULKAN SHADER RESOURCE: slot: {1}, binding: {2}, name: {0}, type: {3}", String(descriptor.Name.c_str()), descriptor.Slot, descriptor.Binding, type);
#endif
        }

        // Generate SPIR-V (optimize it at the same time)
        std::vector<unsigned> spirv;
        spv::SpvBuildLogger logger;
        glslang::SpvOptions spvOptions;
        InitCodegen(_context, spvOptions);
        glslang::GlslangToSpv(*program.getIntermediate(lang), spirv, &logger, &spvOptions);
        const std::string spirvLogOutput = logger.getAllMessages();
        if (!spirvLogOutput.empty())
        {
            LOG(Warning, "SPIR-V generator log:\n{0}", String(spirvLogOutput.c_str()));
        }
        if (spirv.empty())
        {
            LOG(Warning, "SPIR-V generator failed");
            return true;
        }

#if 0
        // Dump SPIR-V as text for debugging
        {
            spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_0);
            std::string spirvText;
            tools.Disassemble(spirv, &spirvText);
            _context->OnCollectDebugInfo(meta, permutationIndex, spirvText.c_str(), (int32)spirvText.size());
        }
#endif


        if (Write(_context, meta, permutationIndex, bindings, header, spirv))
            return true;

        if (customDataWrite && customDataWrite(_context, meta, permutationIndex, _macros, additionalData))
            return true;
    }

    return WriteShaderFunctionEnd(_context, meta);
}

bool ShaderCompilerVulkan::OnCompileBegin()
{
    if (ShaderCompiler::OnCompileBegin())
        return true;

    //_globalMacros.Add({ "VULKAN", "1" }); // glslang compiler adds VULKAN define if EShMsgVulkanRules flag is specified

    return false;
}

void ShaderCompilerVulkan::InitParsing(ShaderCompilationContext* context, glslang::TShader& shader)
{
    // Pick Vulkan version based on target platform
    // Based on: https://docs.vulkan.org/guide/latest/versions.html#_spir_v
    glslang::EShTargetClientVersion targetVulkan;
    glslang::EShTargetLanguageVersion targetLang;
    switch (context->Options->Platform)
    {
    case PlatformType::Windows:
        // TODO: update glslang and try Vulkan 1.2 with SPIR-V 1.5
        targetVulkan = glslang::EShTargetVulkan_1_1;
        targetLang = glslang::EShTargetSpv_1_2;
        break;
    case PlatformType::Android:
        targetVulkan = glslang::EShTargetVulkan_1_1;
        targetLang = glslang::EShTargetSpv_1_2;
        break;
    case PlatformType::Switch:
        // TODO: update glslang and try Vulkan 1.3 with SPIR-V 1.6
        targetVulkan = glslang::EShTargetVulkan_1_1;
        targetLang = glslang::EShTargetSpv_1_2;
        break;
    case PlatformType::Mac:
    case PlatformType::iOS:
        // TODO: update glslang and try Vulkan 1.4 with SPIR-V 1.6
        targetVulkan = glslang::EShTargetVulkan_1_1;
        targetLang = glslang::EShTargetSpv_1_2;
        break;
    default:
        targetVulkan = glslang::EShTargetVulkan_1_0;
        targetLang = glslang::EShTargetSpv_1_0;
        break;
    }

    shader.setInvertY(true);
    //shader.setAutoMapLocations(true);
    //shader.setAutoMapBindings(true);
    //shader.setShiftBinding(glslang::TResourceType::EResUav, 500);
    shader.setHlslIoMapping(true);
    shader.setEnvClient(glslang::EShClientVulkan, targetVulkan);
    shader.setEnvTarget(glslang::EShTargetSpv, targetLang);
}

void ShaderCompilerVulkan::InitCodegen(ShaderCompilationContext* context, glslang::SpvOptions& spvOptions)
{
    spvOptions.generateDebugInfo = false;
    spvOptions.disassemble = false;
    spvOptions.disableOptimizer = context->Options->NoOptimize;
    spvOptions.optimizeSize = !context->Options->NoOptimize;
    spvOptions.stripDebugInfo = !context->Options->GenerateDebugData;
    spvOptions.validate = BUILD_DEBUG;
}

bool ShaderCompilerVulkan::Write(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, struct SpirvShaderHeader& header, std::vector<unsigned int>& spirv)
{
    // Compress
    const int32 srcSize = (int32)spirv.size() * sizeof(unsigned);
    const int32 maxSize = LZ4_compressBound(srcSize);
    Array<byte> spirvCompressed;
    spirvCompressed.Resize(maxSize + sizeof(int32));
    const int32 dstSize = LZ4_compress_default((const char*)&spirv[0], (char*)spirvCompressed.Get() + sizeof(int32), srcSize, maxSize);
    if (dstSize > 0 && dstSize < (int32)(srcSize * 0.8f)) // Expect 20% or more compression ratio to use it (to avoid decompressing if the gain is not big enough)
    {
        spirvCompressed.Resize(dstSize + sizeof(int32));
        *(int32*)spirvCompressed.Get() = srcSize; // Store original size in the beginning to decompress it
        header.Type = SpirvShaderHeader::Types::SPIRV_LZ4;
        return WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, &header, sizeof(header), spirvCompressed.Get(), spirvCompressed.Count());
    }

    header.Type = SpirvShaderHeader::Types::SPIRV;
    return WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, &header, sizeof(header), &spirv[0], srcSize);
}

#endif
