// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_DX_SHADER_COMPILER

#include "ShaderCompilerDX.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "Engine/Platform/Windows/ComPtr.h"
#include <d3d12shader.h>
#include <ThirdParty/DirectXShaderCompiler/dxcapi.h>

#ifndef DXIL_FOURCC
#define DXIL_FOURCC(ch0, ch1, ch2, ch3) ((uint32)(uint8)(ch0) | (uint32)(uint8)(ch1) << 8 | (uint32)(uint8)(ch2) << 16 | (uint32)(uint8)(ch3) << 24)
#endif

/// <summary>
/// Helper class to include source for DX shaders compiler.
/// </summary>
class IncludeDX : public IDxcIncludeHandler
{
private:

    ShaderCompilationContext* _context;
    IDxcLibrary* _library;

public:

    IncludeDX(ShaderCompilationContext* context, IDxcLibrary* library)
    {
        _context = context;
        _library = library;
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
        const StringAnsi filename(pFilename);
        if (ShaderCompiler::GetIncludedFileSource(_context, "", filename.Get(), source, sourceLength))
            return E_FAIL;
        IDxcBlobEncoding* textBlob;
        if (FAILED(_library->CreateBlobWithEncodingFromPinned((LPBYTE)source, sourceLength, CP_UTF8, &textBlob)))
            return E_FAIL;
        *ppIncludeSource = textBlob;
        return S_OK;
    }
};

ShaderCompilerDX::ShaderCompilerDX(ShaderProfile profile)
    : ShaderCompiler(profile)
{
    IDxcCompiler2* compiler = nullptr;
    IDxcLibrary* library = nullptr;
    IDxcContainerReflection* containerReflection = nullptr;
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(compiler), reinterpret_cast<void**>(&compiler))) ||
        FAILED(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(library), reinterpret_cast<void**>(&library))) ||
        FAILED(DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(containerReflection), reinterpret_cast<void**>(&containerReflection))))
    {
        LOG(Error, "DxcCreateInstance failed");
    }
    _compiler = compiler;
    _library = library;
    _containerReflection = containerReflection;
    static bool PrintVersion = true;
    if (PrintVersion)
    {
        PrintVersion = false;
        IDxcVersionInfo* version = nullptr;
        if (compiler && SUCCEEDED(compiler->QueryInterface(__uuidof(version), reinterpret_cast<void**>(&version))))
        {
            UINT32 major, minor;
            version->GetVersion(&major, &minor);
            LOG(Info, "DXC version {0}.{1}", major, minor);
            version->Release();
        }
    }
}

ShaderCompilerDX::~ShaderCompilerDX()
{
    auto compiler = (IDxcCompiler2*)_compiler;
    if (compiler)
        compiler->Release();
    auto library = (IDxcLibrary*)_library;
    if (library)
        library->Release();
    auto containerReflection = (IDxcContainerReflection*)_containerReflection;
    if (containerReflection)
        containerReflection->Release();
}

namespace
{
    bool ProcessShader(ShaderCompilationContext* context, Array<ShaderCompiler::ShaderResourceBuffer>& constantBuffers, ID3D12ShaderReflection* shaderReflection, D3D12_SHADER_DESC& desc, ShaderBindings& bindings)
    {
        // Extract constant buffers usage information
        for (uint32 a = 0; a < desc.ConstantBuffers; a++)
        {
            // Get CB
            auto cb = shaderReflection->GetConstantBufferByIndex(a);

            // Get CB description
            D3D12_SHADER_BUFFER_DESC cbDesc;
            cb->GetDesc(&cbDesc);

            // Check buffer type
            if (cbDesc.Type == D3D_CT_CBUFFER)
            {
                // Find CB slot index
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
                    context->OnError("Missing bound resource.");
                    return true;
                }

                // Set flag
                bindings.UsedCBsMask |= 1 << slot;

                // Try to add CB to the list
                for (int32 b = 0; b < constantBuffers.Count(); b++)
                {
                    auto& cc = constantBuffers[b];
                    if (cc.Slot == slot)
                    {
                        cc.IsUsed = true;
                        cc.Size = cbDesc.Size;
                        break;
                    }
                }
            }
        }

        // Extract resources usage
        for (uint32 i = 0; i < desc.BoundResources; i++)
        {
            // Get resource description
            D3D12_SHADER_INPUT_BIND_DESC resDesc;
            shaderReflection->GetResourceBindingDesc(i, &resDesc);

            switch (resDesc.Type)
            {
                // Sampler
            case D3D_SIT_SAMPLER:
                break;

                // Constant Buffer
            case D3D_SIT_CBUFFER:
            case D3D_SIT_TBUFFER:
                break;

                // Shader Resource
            case D3D_SIT_TEXTURE:
            case D3D_SIT_STRUCTURED:
            case D3D_SIT_BYTEADDRESS:
                bindings.UsedSRsMask |= 1 << resDesc.BindPoint;
                break;

                // Unordered Access
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                bindings.UsedUAsMask |= 1 << resDesc.BindPoint;
                break;
            }
        }

        return false;
    }
}

bool ShaderCompilerDX::CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite)
{
    if (WriteShaderFunctionBegin(_context, meta))
        return true;

    // Prepare
    auto options = _context->Options;
    auto compiler = (IDxcCompiler2*)_compiler;
    auto library = (IDxcLibrary*)_library;
    auto containerReflection = (IDxcContainerReflection*)_containerReflection;
    auto type = meta.GetStage();
    IncludeDX include(_context, library);
    const Char* targetProfile;
    switch (type)
    {
    case ShaderStage::Vertex:
        targetProfile = TEXT("vs_6_0");
        break;
    case ShaderStage::Hull:
        targetProfile = TEXT("hs_6_0");
        break;
    case ShaderStage::Domain:
        targetProfile = TEXT("ds_6_0");
        break;
    case ShaderStage::Geometry:
        targetProfile = TEXT("gs_6_0");
        break;
    case ShaderStage::Pixel:
        targetProfile = TEXT("ps_6_0");
        break;
    case ShaderStage::Compute:
        targetProfile = TEXT("cs_6_0");
        break;
    default:
        return true;
    }
    ComPtr<IDxcBlobEncoding> textBlob;
    if (FAILED(library->CreateBlobWithEncodingFromPinned((LPBYTE)options->Source, options->SourceLength, CP_UTF8, &textBlob)))
        return true;
    const StringAsUTF16<> entryPoint(meta.Name.Get(), meta.Name.Length());
    Array<String> definesStrings;
    Array<DxcDefine> defines;
    Array<Char*, FixedAllocation<16>> args;
    if (_context->Options->NoOptimize)
        args.Add(TEXT("-Od"));
    else
        args.Add(TEXT("-O3"));
    if (_context->Options->TreatWarningsAsErrors)
        args.Add(TEXT("-WX"));
    if (_context->Options->GenerateDebugData)
        args.Add(TEXT("-Zi"));

    // Compile all shader function permutations
    for (int32 permutationIndex = 0; permutationIndex < meta.Permutations.Count(); permutationIndex++)
    {
        _macros.Clear();

        // Get function permutation macros
        meta.GetDefinitionsForPermutation(permutationIndex, _macros);

        // Add additional define for compiled function name
        GetDefineForFunction(meta, _macros);

        // Add custom and global macros (global last because contain null define to indicate ending)
        _macros.Add(_context->Options->Macros);
        _macros.Add(_globalMacros);

        // Convert defines from char* to Char*
        const int32 macrosCount = _macros.Count() - 1;
        definesStrings.Resize(macrosCount * 2);
        defines.Resize(macrosCount);
        for (int32 i = 0; i < macrosCount; i++)
        {
            auto& macro = _macros[i];
            auto& define = defines[i];
            auto& defineName = definesStrings[i * 2];
            auto& defineValue = definesStrings[i * 2 + 1];
            defineName = macro.Name;
            defineValue = macro.Definition;
            define.Name = defineName.GetText();
            define.Value = defineValue.Get();
        }

        // Compile
        ComPtr<IDxcOperationResult> results;
        HRESULT result = compiler->Compile(
            textBlob.Get(),
            options->TargetName.Get(),
            entryPoint.Get(),
            targetProfile,
            (LPCWSTR*)args.Get(),
            args.Count(),
            defines.Get(),
            defines.Count(),
            &include,
            &results);
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
                    {
                        _context->OnError((const char*)errorUtf8->GetBufferPointer());
                    }
                }
            }
            return true;
        }

        // Get the output
        ComPtr<IDxcBlob> shaderBuffer = nullptr;
        if (FAILED(results->GetResult(&shaderBuffer)))
        {
            LOG(Error, "IDxcOperationResult::GetResult failed.");
            return true;
        }

#ifdef GPU_USE_SHADERS_DEBUG_LAYER
        // Generate debug information
        {
            // Disassemble compiled shader
            ComPtr<IDxcBlobEncoding> disassembly;
            if (FAILED(compiler->Disassemble(shaderBuffer, &disassembly)))
                return true;
            ComPtr<IDxcBlobEncoding> disassemblyUtf8;
            if (FAILED(library->GetBlobAsUtf8(disassembly, &disassemblyUtf8)))
                return true;

            // Extract debug info
            _context->OnCollectDebugInfo(meta, permutationIndex, (const char*)disassemblyUtf8->GetBufferPointer(), (int32)disassemblyUtf8->GetBufferSize());
        }
#endif

        // Perform shader reflection
        if (FAILED(containerReflection->Load(shaderBuffer)))
        {
            LOG(Error, "IDxcContainerReflection::Load failed.");
            return true;
        }
        const uint32 dxilPartKind = DXIL_FOURCC('D', 'X', 'I', 'L');
        uint32 dxilPartIndex = ~0u;
        if (FAILED(containerReflection->FindFirstPartKind(dxilPartKind, &dxilPartIndex)))
        {
            LOG(Error, "IDxcContainerReflection::FindFirstPartKind failed.");
            return true;
        }
        ComPtr<ID3D12ShaderReflection> shaderReflection;
        if (FAILED(containerReflection->GetPartReflection(dxilPartIndex, IID_PPV_ARGS(&shaderReflection))))
        {
            LOG(Error, "IDxcContainerReflection::GetPartReflection failed.");
            return true;
        }

        // Get shader description
        D3D12_SHADER_DESC desc;
        Platform::MemoryClear(&desc, sizeof(desc));
        shaderReflection->GetDesc(&desc);

        // Process shader reflection data
        ShaderBindings bindings = { desc.InstructionCount, 0, 0, 0 };
        if (ProcessShader(_context, _constantBuffers, shaderReflection.Get(), desc, bindings))
            return true;

        if (WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, shaderBuffer->GetBufferPointer(), (int32)shaderBuffer->GetBufferSize()))
            return true;

        if (customDataWrite && customDataWrite(_context, meta, permutationIndex, _macros))
            return true;
    }

    return WriteShaderFunctionEnd(_context, meta);
}

bool ShaderCompilerDX::OnCompileBegin()
{
    if (ShaderCompiler::OnCompileBegin())
        return true;

    _globalMacros.Add({ "DIRECTX", "1" });

    return false;
}

#endif
