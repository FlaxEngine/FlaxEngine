// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_VK_SHADER_COMPILER

#include "ShaderCompilerVulkan.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Config.h"
#include "Engine/GraphicsDevice/Vulkan/Types.h"

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
    CompileShaderVulkanInstances--;
    if (CompileShaderVulkanInstances == 0)
    {
        glslang::FinalizeProcess();
    }
}

// @formatter:off
const TBuiltInResource DefaultTBuiltInResource =
{
    /* .MaxLights = */ 32,
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

bool ShaderCompilerVulkan::CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite)
{
    // TODO: test without locking
    ScopeLock lock(CompileShaderVulkanLocker);
    Includer includer(_context);

    // Prepare
    if (WriteShaderFunctionBegin(_context, meta))
        return true;
    auto options = _context->Options;
    auto type = meta.GetStage();

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
        int lengths = options->SourceLength - 1;
        const char* names = _context->TargetNameAnsi;
        shader.setStringsWithLengthsAndNames(&options->Source, &lengths, &names, 1);
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
        shader.setInvertY(true);
        //shader.setAutoMapLocations(true);
        //shader.setAutoMapBindings(true);
        //shader.setShiftBinding(glslang::TResourceType::EResUav, 500);
        shader.setHlslIoMapping(true);
        shader.setEnvInput(glslang::EShSourceHlsl, lang, glslang::EShClientVulkan, defaultVersion);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
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
        ShaderBindings bindings = { 0, 0, 0, 0 };
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
        spvOptions.generateDebugInfo = false;
        spvOptions.disassemble = false;
        spvOptions.disableOptimizer = options->NoOptimize;
        spvOptions.optimizeSize = !options->NoOptimize;
        spvOptions.stripDebugInfo = !options->GenerateDebugData;
#if BUILD_DEBUG
        spvOptions.validate = true;
#else
        spvOptions.validate = false;
#endif
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

        int32 spirvBytesCount = (int32)spirv.size() * sizeof(unsigned);
        header.Type = SpirvShaderHeader::Types::Raw;

        if (WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, &header, sizeof(header), &spirv[0], spirvBytesCount))
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

    // TODO: handle options->TreatWarningsAsErrors

    return false;
}

#endif
