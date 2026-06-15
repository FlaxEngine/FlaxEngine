// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_VK_SHADER_COMPILER && COMPILE_WITH_VK_DXC_SPIRV

#include "ShaderCompilerVulkan.h"
#include "Engine/Core/Log.h"
#include "Engine/GraphicsDevice/Vulkan/Types.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "Engine/Platform/Windows/ComPtr.h"
#include <d3d12shader.h>
#include <ThirdParty/DirectXShaderCompiler/dxcapi.h>
#include <vector>

namespace
{
    IDxcCompiler3* DxcCompiler = nullptr;
    IDxcLibrary* DxcLibrary = nullptr;
    IDxcUtils* DxcUtils = nullptr;

    bool EnsureDxc()
    {
        if (DxcCompiler)
            return true;
        if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(DxcCompiler), reinterpret_cast<void**>(&DxcCompiler))) ||
            FAILED(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(DxcLibrary), reinterpret_cast<void**>(&DxcLibrary))) ||
            FAILED(DxcCreateInstance(CLSID_DxcUtils, __uuidof(DxcUtils), reinterpret_cast<void**>(&DxcUtils))))
        {
            LOG(Error, "DxcCreateInstance failed for Vulkan ray tracing shader compilation.");
            return false;
        }
        return true;
    }

    bool ShaderUsesRayQuery(const char* source, int32 length)
    {
        const char* pattern = "RayQuery";
        const int32 patternLength = 8;
        for (int32 i = 0; i + patternLength <= length; i++)
        {
            if (Platform::MemoryCompare(source + i, pattern, patternLength) == 0)
                return true;
        }
        return false;
    }

    // glslang (used for every non-ray-tracing Vulkan shader) rewrites each resource so that the SPIR-V
    // DescriptorSet decoration is the per-stage set index (DescriptorSet::Stage: Vertex=0, Pixel=1, ...) and
    // the Binding decoration is the resource's sequential index within the stage. DXC instead emits
    // set=register-space (0) and binding=register-number. The Vulkan runtime builds the descriptor set
    // layout and descriptor writes from those exact (stage-set, sequential-binding) values, so the DXC
    // output has to be patched to the same convention or the shader's resources never get bound.
    // 'bindingRemap[dxcBinding] = sequentialBinding' (-1 = leave unchanged).
    void RewriteSpirvDescriptors(std::vector<unsigned>& spirv, uint32 stageSet, const Array<int32>& bindingRemap)
    {
        const size_t count = spirv.size();
        if (count < 5)
            return;
        const uint32 OpDecorate = 71;
        const uint32 DecorationBinding = 33;
        const uint32 DecorationDescriptorSet = 34;
        size_t i = 5; // Skip the 5-word module header
        while (i < count)
        {
            const uint32 word = spirv[i];
            const uint16 wordCount = (uint16)(word >> 16);
            const uint16 opcode = (uint16)(word & 0xFFFF);
            if (wordCount == 0)
                break; // Malformed - avoid an infinite loop
            if (opcode == OpDecorate && wordCount >= 4 && i + 3 < count)
            {
                const uint32 decoration = spirv[i + 2];
                if (decoration == DecorationDescriptorSet)
                {
                    spirv[i + 3] = stageSet;
                }
                else if (decoration == DecorationBinding)
                {
                    const uint32 oldBinding = spirv[i + 3];
                    if ((int32)oldBinding < bindingRemap.Count() && bindingRemap[oldBinding] >= 0)
                        spirv[i + 3] = (uint32)bindingRemap[oldBinding];
                }
            }
            i += wordCount;
        }
    }

    SpirvShaderResourceType MapD3DTextureResourceType(D3D_SRV_DIMENSION dimension)
    {
        switch (dimension)
        {
        case D3D_SRV_DIMENSION_TEXTURE1D:
            return SpirvShaderResourceType::Texture1D;
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
            return SpirvShaderResourceType::Texture1DArray;
        case D3D_SRV_DIMENSION_TEXTURE2D:
            return SpirvShaderResourceType::Texture2D;
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
            return SpirvShaderResourceType::Texture2DArray;
        case D3D_SRV_DIMENSION_TEXTURE3D:
            return SpirvShaderResourceType::Texture3D;
        case D3D_SRV_DIMENSION_TEXTURECUBE:
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
            return SpirvShaderResourceType::TextureCube;
        default:
            return SpirvShaderResourceType::Unknown;
        }
    }

    class IncludeDxcSpirv : public IDxcIncludeHandler
    {
    private:
        ShaderCompilationContext* _context;
        IDxcLibrary* _library;

    public:
        IncludeDxcSpirv(ShaderCompilationContext* context, IDxcLibrary* library)
            : _context(context)
            , _library(library)
        {
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return 1;
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            return 1;
        }

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

        HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
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
}

bool ShaderCompilerVulkan::CompileRayTracingPermutation(ShaderFunctionMeta& meta, WritePermutationData customDataWrite, int32 permutationIndex, int32 stageSet, ShaderStage type)
{
    if (!EnsureDxc())
        return true;

    auto options = _context->Options;
    IncludeDxcSpirv include(_context, DxcLibrary);

    String targetProfile;
    switch (type)
    {
    case ShaderStage::Vertex:
        targetProfile = TEXT("vs_6_5");
        break;
    case ShaderStage::Hull:
        targetProfile = TEXT("hs_6_5");
        break;
    case ShaderStage::Domain:
        targetProfile = TEXT("ds_6_5");
        break;
    case ShaderStage::Geometry:
        targetProfile = TEXT("gs_6_5");
        break;
    case ShaderStage::Pixel:
        targetProfile = TEXT("ps_6_5");
        break;
    case ShaderStage::Compute:
        targetProfile = TEXT("cs_6_5");
        break;
    default:
        _context->OnError("Unknown shader type for Vulkan ray tracing compilation.");
        return true;
    }

    ComPtr<IDxcBlobEncoding> textBlob;
    if (FAILED(DxcLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)options->Source, options->SourceLength, CP_UTF8, &textBlob)))
        return true;
    DxcBuffer textBuffer;
    textBuffer.Ptr = textBlob->GetBufferPointer();
    textBuffer.Size = textBlob->GetBufferSize();
    textBuffer.Encoding = DXC_CP_ACP;

    const StringAsUTF16<> entryPoint(meta.Name.Get(), meta.Name.Length());

    // Arguments shared by both compiles (SPIR-V shader object + DXIL reflection).
    Array<const Char*, FixedAllocation<16>> commonArgs;
    if (options->NoOptimize)
        commonArgs.Add(DXC_ARG_SKIP_OPTIMIZATIONS);
    else
        commonArgs.Add(DXC_ARG_OPTIMIZATION_LEVEL3);
    if (options->TreatWarningsAsErrors)
        commonArgs.Add(DXC_ARG_WARNINGS_ARE_ERRORS);
    if (options->GenerateDebugData)
        commonArgs.Add(DXC_ARG_DEBUG);
    commonArgs.Add(TEXT("-T"));
    commonArgs.Add(targetProfile.Get());
    commonArgs.Add(TEXT("-E"));
    commonArgs.Add(entryPoint.Get());
    commonArgs.Add(options->TargetName.Get());

    Array<String> definesStrings;
    const int32 macrosCount = _macros.Count() - 1;
    definesStrings.Resize(macrosCount);
    for (int32 i = 0; i < macrosCount; i++)
    {
        auto& macro = _macros[i];
        auto& define = definesStrings[i];
        define = macro.Name;
        if (macro.Definition && *macro.Definition)
        {
            define += TEXT("=");
            define += macro.Definition;
        }
    }

    // Runs DXC over the source. With 'spirv' the produced object is SPIR-V (the shader Vulkan runs); otherwise
    // it is DXIL, compiled only to obtain reflection (DXC does not emit reflection data for SPIR-V targets).
    const auto runDxc = [&](bool spirv, ComPtr<IDxcResult>& outResults) -> bool
    {
        Array<const Char*, InlinedAllocation<250>> argsFull;
        if (spirv)
        {
            argsFull.Add(TEXT("-spirv"));
            argsFull.Add(TEXT("-fspv-target-env=vulkan1.2"));
        }
        else
        {
            // Reflection only - skip DXIL validation so dxil.dll (the signing validator) is not required.
            argsFull.Add(DXC_ARG_SKIP_VALIDATION);
        }
        for (auto& e : commonArgs)
            argsFull.Add(e);
        for (auto& d : definesStrings)
        {
            argsFull.Add(TEXT("-D"));
            argsFull.Add(*d);
        }
        ComPtr<IDxcResult> results;
        HRESULT result = DxcCompiler->Compile(
            &textBuffer,
            (LPCWSTR*)argsFull.Get(),
            argsFull.Count(),
            &include,
            IID_PPV_ARGS(results.GetAddressOf()));
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
                    DxcLibrary->GetBlobAsUtf8(error, &errorUtf8);
                    if (errorUtf8)
                        _context->OnError((const char*)errorUtf8->GetBufferPointer());
                }
            }
            return true;
        }
        outResults = results;
        return false;
    };

    // 1) Compile to SPIR-V - the actual shader bytecode consumed by Vulkan.
    ComPtr<IDxcResult> spirvResults;
    if (runDxc(true, spirvResults))
        return true;
    ComPtr<IDxcBlob> spirvBlob;
    if (FAILED(spirvResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(spirvBlob.GetAddressOf()), nullptr)))
    {
        _context->OnError("DXC SPIR-V compile failed to produce shader object.");
        return true;
    }

    // 2) Compile the same source to DXIL purely to obtain reflection. The HLSL register bindings are identical
    // to the SPIR-V output, so this reflection is reused to build and patch the SPIR-V descriptor bindings.
    ComPtr<IDxcResult> reflectionResults;
    if (runDxc(false, reflectionResults))
        return true;
    ComPtr<IDxcBlob> reflectionBlob;
    if (FAILED(reflectionResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionBlob.GetAddressOf()), nullptr)))
    {
        _context->OnError("DXC failed to produce reflection data.");
        return true;
    }

    DxcBuffer reflectionBuffer;
    reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
    reflectionBuffer.Size = reflectionBlob->GetBufferSize();
    reflectionBuffer.Encoding = DXC_CP_ACP;
    ComPtr<ID3D12ShaderReflection> shaderReflection;
    if (FAILED(DxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(shaderReflection.GetAddressOf()))))
    {
        _context->OnError("DXC CreateReflection failed.");
        return true;
    }

    D3D12_SHADER_DESC desc;
    Platform::MemoryClear(&desc, sizeof(desc));
    shaderReflection->GetDesc(&desc);

    SpirvShaderHeader header;
    Platform::MemoryClear(&header, sizeof(header));
    ShaderBindings bindings = { desc.InstructionCount };
    bindings.InputsCount = desc.InputParameters;
    bindings.OutputsCount = desc.OutputParameters;

    // Maps DXC's emitted binding (= HLSL register number) to the sequential binding the Vulkan runtime
    // expects. Built alongside the descriptor metadata below, then applied to the SPIR-V at the end.
    Array<int32> bindingRemap;
    const auto setBindingRemap = [&bindingRemap](uint32 dxcBinding, int32 sequentialBinding)
    {
        if ((int32)dxcBinding >= bindingRemap.Count())
        {
            const int32 oldCount = bindingRemap.Count();
            bindingRemap.Resize((int32)dxcBinding + 1);
            for (int32 j = oldCount; j < bindingRemap.Count(); j++)
                bindingRemap[j] = -1;
        }
        bindingRemap[(int32)dxcBinding] = sequentialBinding;
    };

    for (uint32 cbIndex = 0; cbIndex < desc.ConstantBuffers; cbIndex++)
    {
        auto cb = shaderReflection->GetConstantBufferByIndex(cbIndex);
        D3D12_SHADER_BUFFER_DESC cbDesc;
        cb->GetDesc(&cbDesc);
        if (cbDesc.Type != D3D_CT_CBUFFER)
            continue;

        int32 slot = INVALID_INDEX;
        for (uint32 b = 0; b < desc.BoundResources; b++)
        {
            D3D12_SHADER_INPUT_BIND_DESC bDesc;
            shaderReflection->GetResourceBindingDesc(b, &bDesc);
            if (StringUtils::Compare(bDesc.Name, cbDesc.Name) == 0)
            {
                slot = bDesc.BindPoint;
                break;
            }
        }
        if (slot == INVALID_INDEX)
        {
            _context->OnError("Missing bound constant buffer resource.");
            return true;
        }

        if (header.DescriptorInfo.DescriptorTypesCount >= SpirvShaderDescriptorInfo::MaxDescriptors)
        {
            _context->OnError("Too many descriptors in Vulkan ray tracing shader.");
            return true;
        }

        auto& d = header.DescriptorInfo.DescriptorTypes[header.DescriptorInfo.DescriptorTypesCount++];
        d.Binding = header.DescriptorInfo.DescriptorTypesCount - 1;
        d.Set = (byte)stageSet;
        d.Slot = (byte)slot;
        d.BindingType = SpirvShaderResourceBindingType::CB;
        d.DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        d.ResourceType = SpirvShaderResourceType::ConstantBuffer;
        d.ResourceFormat = PixelFormat::Unknown;
        d.Count = 1;
        header.DescriptorInfo.BufferInfosCount++;

        bindings.UsedCBsMask |= 1 << slot;
        for (int32 b = 0; b < _constantBuffers.Count(); b++)
        {
            auto& cc = _constantBuffers[b];
            if (cc.Slot == slot)
            {
                cc.IsUsed = true;
                cc.Size = cbDesc.Size;
                break;
            }
        }
    }

    for (uint32 i = 0; i < desc.BoundResources; i++)
    {
        D3D12_SHADER_INPUT_BIND_DESC resDesc;
        shaderReflection->GetResourceBindingDesc(i, &resDesc);
        switch (resDesc.Type)
        {
        case D3D_SIT_CBUFFER:
        case D3D_SIT_TBUFFER:
            continue;
        case D3D_SIT_SAMPLER:
        {
            if (header.DescriptorInfo.DescriptorTypesCount >= SpirvShaderDescriptorInfo::MaxDescriptors)
                return true;
            auto& d = header.DescriptorInfo.DescriptorTypes[header.DescriptorInfo.DescriptorTypesCount++];
            d.Binding = header.DescriptorInfo.DescriptorTypesCount - 1;
            d.Set = (byte)stageSet;
            d.Slot = (byte)resDesc.BindPoint;
            d.BindingType = SpirvShaderResourceBindingType::SAMPLER;
            d.DescriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            d.ResourceType = SpirvShaderResourceType::Sampler;
            d.ResourceFormat = PixelFormat::Unknown;
            d.Count = resDesc.BindCount;
            header.DescriptorInfo.ImageInfosCount += d.Count;
            bindings.UsedSamplersMask |= 1 << resDesc.BindPoint;
            break;
        }
        case D3D_SIT_TEXTURE:
        {
            if (header.DescriptorInfo.DescriptorTypesCount >= SpirvShaderDescriptorInfo::MaxDescriptors)
                return true;
            auto& d = header.DescriptorInfo.DescriptorTypes[header.DescriptorInfo.DescriptorTypesCount++];
            d.Binding = header.DescriptorInfo.DescriptorTypesCount - 1;
            d.Set = (byte)stageSet;
            d.Slot = (byte)resDesc.BindPoint;
            d.BindingType = SpirvShaderResourceBindingType::SRV;
            d.DescriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            d.ResourceType = MapD3DTextureResourceType(resDesc.Dimension);
            d.ResourceFormat = PixelFormat::Unknown;
            d.Count = resDesc.BindCount;
            header.DescriptorInfo.ImageInfosCount += d.Count;
            for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                bindings.UsedSRsMask |= 1 << (resDesc.BindPoint + shift);
            break;
        }
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        {
            if (header.DescriptorInfo.DescriptorTypesCount >= SpirvShaderDescriptorInfo::MaxDescriptors)
                return true;
            auto& d = header.DescriptorInfo.DescriptorTypes[header.DescriptorInfo.DescriptorTypesCount++];
            d.Binding = header.DescriptorInfo.DescriptorTypesCount - 1;
            d.Set = (byte)stageSet;
            d.Slot = (byte)resDesc.BindPoint;
            d.BindingType = SpirvShaderResourceBindingType::SRV;
            d.DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            d.ResourceType = SpirvShaderResourceType::Buffer;
            d.ResourceFormat = PixelFormat::Unknown;
            d.Count = resDesc.BindCount;
            header.DescriptorInfo.BufferInfosCount += d.Count;
            for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                bindings.UsedSRsMask |= 1 << (resDesc.BindPoint + shift);
            break;
        }
        case D3D_SIT_RTACCELERATIONSTRUCTURE:
        {
            if (header.DescriptorInfo.DescriptorTypesCount >= SpirvShaderDescriptorInfo::MaxDescriptors)
                return true;
            auto& d = header.DescriptorInfo.DescriptorTypes[header.DescriptorInfo.DescriptorTypesCount++];
            d.Binding = header.DescriptorInfo.DescriptorTypesCount - 1;
            d.Set = (byte)stageSet;
            d.Slot = (byte)resDesc.BindPoint;
            d.BindingType = SpirvShaderResourceBindingType::SRV;
            d.DescriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            d.ResourceType = SpirvShaderResourceType::Unknown;
            d.ResourceFormat = PixelFormat::Unknown;
            d.Count = resDesc.BindCount;
            header.DescriptorInfo.AccelerationStructureInfosCount += (uint16)d.Count;
            for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                bindings.UsedSRsMask |= 1 << (resDesc.BindPoint + shift);
            break;
        }
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        {
            if (header.DescriptorInfo.DescriptorTypesCount >= SpirvShaderDescriptorInfo::MaxDescriptors)
                return true;
            auto& d = header.DescriptorInfo.DescriptorTypes[header.DescriptorInfo.DescriptorTypesCount++];
            d.Binding = header.DescriptorInfo.DescriptorTypesCount - 1;
            d.Set = (byte)stageSet;
            d.Slot = (byte)resDesc.BindPoint;
            d.BindingType = SpirvShaderResourceBindingType::UAV;
            d.DescriptorType = resDesc.Dimension == D3D_SRV_DIMENSION_BUFFER ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            d.ResourceType = resDesc.Dimension == D3D_SRV_DIMENSION_BUFFER ? SpirvShaderResourceType::Buffer : MapD3DTextureResourceType(resDesc.Dimension);
            d.ResourceFormat = PixelFormat::Unknown;
            d.Count = resDesc.BindCount;
            if (d.DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                header.DescriptorInfo.BufferInfosCount += d.Count;
            else
                header.DescriptorInfo.ImageInfosCount += d.Count;
            for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                bindings.UsedUAsMask |= 1 << (resDesc.BindPoint + shift);
            break;
        }
        default:
            break;
        }
    }

    // Each descriptor stored its HLSL register number in Slot (== the binding DXC emitted, since the shader
    // uses register space 0). Map register -> the descriptor's sequential index, which is the binding the
    // Vulkan runtime uses when building the layout and writing descriptors.
    for (uint16 di = 0; di < header.DescriptorInfo.DescriptorTypesCount; di++)
        setBindingRemap(header.DescriptorInfo.DescriptorTypes[di].Slot, (int32)di);

    const uint32* spirvWords = (const uint32*)spirvBlob->GetBufferPointer();
    const size_t spirvWordCount = spirvBlob->GetBufferSize() / sizeof(uint32);
    std::vector<unsigned> spirv(spirvWords, spirvWords + spirvWordCount);
    if (spirv.empty())
    {
        _context->OnError("DXC produced empty SPIR-V output.");
        return true;
    }

    // Patch the DXC SPIR-V to use Flax's per-stage descriptor set and sequential bindings (see notes above).
    RewriteSpirvDescriptors(spirv, (uint32)stageSet, bindingRemap);

    if (Write(_context, meta, permutationIndex, bindings, header, spirv))
        return true;

    if (customDataWrite && customDataWrite(_context, meta, permutationIndex, _macros, nullptr))
        return true;

    return false;
}

bool ShaderCompilerVulkan_UsesRayQuery(const char* source, int32 length)
{
    return ShaderUsesRayQuery(source, length);
}

#endif
