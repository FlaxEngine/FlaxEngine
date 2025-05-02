// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_DX_SHADER_COMPILER

#include "ShaderCompilerDX.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Graphics/Config.h"
#include "Engine/GraphicsDevice/DirectX/DX12/Types.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "Engine/Platform/Windows/ComPtr.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include <d3d12shader.h>
#include <ThirdParty/DirectXShaderCompiler/dxcapi.h>

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
    IDxcCompiler3* compiler = nullptr;
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

bool ShaderCompilerDX::CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite)
{
    if (WriteShaderFunctionBegin(_context, meta))
        return true;

    // Prepare
    auto options = _context->Options;
    auto compiler = (IDxcCompiler3*)_compiler;
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
    DxcBuffer textBuffer;
    textBuffer.Ptr = textBlob->GetBufferPointer();
    textBuffer.Size = textBlob->GetBufferSize();
    textBuffer.Encoding = DXC_CP_ACP;
    const StringAsUTF16<> entryPoint(meta.Name.Get(), meta.Name.Length());
    Array<String> definesStrings;
    Array<const Char*, FixedAllocation<12>> args;
    if (_context->Options->NoOptimize)
        args.Add(DXC_ARG_SKIP_OPTIMIZATIONS);
    else
        args.Add(DXC_ARG_OPTIMIZATION_LEVEL3);
    if (_context->Options->TreatWarningsAsErrors)
        args.Add(DXC_ARG_WARNINGS_ARE_ERRORS);
    if (_context->Options->GenerateDebugData)
        args.Add(DXC_ARG_DEBUG);
    args.Add(TEXT("-T"));
    args.Add(targetProfile);
    args.Add(TEXT("-E"));
    args.Add(entryPoint.Get());
    args.Add(options->TargetName.Get());
    Array<const Char*, InlinedAllocation<250>> argsFull;
    String debugOutputFolder;
    if (_context->Options->GenerateDebugData)
    {
        debugOutputFolder = Globals::ProjectCacheFolder / TEXT("Shaders") / TEXT("DXC");
        if (!FileSystem::DirectoryExists(debugOutputFolder))
            FileSystem::CreateDirectory(debugOutputFolder);
    }

    // Compile all shader function permutations
    AdditionalDataVS additionalDataVS;
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

        // Build full list of arguments
        argsFull.Clear();
        for (auto& e : args)
            argsFull.Add(e);
        for (auto& d : definesStrings)
        {
            argsFull.Add(TEXT("-D"));
            argsFull.Add(*d);
        }

        // Compile
        ComPtr<IDxcResult> results;
        HRESULT result = compiler->Compile(
            &textBuffer,
            (LPCWSTR*)argsFull.Get(),
            argsFull.Count(),
            &include,
            IID_PPV_ARGS(&results));
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
            ComPtr<IDxcResult> disassembly;
            DxcBuffer shaderDxcBuffer;
            shaderDxcBuffer.Ptr = shaderBuffer->GetBufferPointer();
            shaderDxcBuffer.Size = shaderBuffer->GetBufferSize();
            shaderDxcBuffer.Encoding = DXC_CP_ACP;
            if (FAILED(compiler->Disassemble(&shaderDxcBuffer, IID_PPV_ARGS(&disassembly))))
                return true;
            ComPtr<IDxcBlob> disassemblyBlob;
            ComPtr<IDxcBlobUtf16> disassemblyPath;
            if (FAILED(disassembly->GetOutput(DXC_OUT_DISASSEMBLY, IID_PPV_ARGS(disassemblyBlob.GetAddressOf()), disassemblyPath.GetAddressOf())))
                return true;
            ComPtr<IDxcBlobEncoding> disassemblyUtf8;
            if (FAILED(library->GetBlobAsUtf8(disassemblyBlob, &disassemblyUtf8)))
                return true;

            // Extract debug info
            _context->OnCollectDebugInfo(meta, permutationIndex, (const char*)disassemblyUtf8->GetBufferPointer(), (int32)disassemblyUtf8->GetBufferSize());
        }
#endif
        if (_context->Options->GenerateDebugData)
        {
            ComPtr<IDxcBlob> pdbBlob;
            ComPtr<IDxcBlobUtf16> pdbName;
            if (results->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdbBlob.GetAddressOf()), pdbName.GetAddressOf()) == S_OK)
            {
                File::WriteAllBytes(debugOutputFolder / String(pdbName->GetStringPointer(), (int32)pdbName->GetStringLength()), (byte*)pdbBlob->GetBufferPointer(), (int32)pdbBlob->GetBufferSize());
            }
        }

        // Perform shader reflection
        if (FAILED(containerReflection->Load(shaderBuffer)))
        {
            LOG(Error, "IDxcContainerReflection::Load failed.");
            return true;
        }
        const uint32 dxilPartKind = DXC_PART_DXIL;
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
        void* additionalData = nullptr;
        if (type == ShaderStage::Vertex)
        {
            additionalData = &additionalDataVS;
            additionalDataVS.Inputs.Clear();
            for (UINT inputIdx = 0; inputIdx < desc.InputParameters; inputIdx++)
            {
                D3D12_SIGNATURE_PARAMETER_DESC inputDesc;
                shaderReflection->GetInputParameterDesc(inputIdx, &inputDesc);
                if (inputDesc.SystemValueType != D3D10_NAME_UNDEFINED)
                    continue;
                auto format = PixelFormat::Unknown;
                switch (inputDesc.ComponentType)
                {
                case D3D_REGISTER_COMPONENT_UINT32:
                    if (inputDesc.Mask >= 0b1111)
                        format = PixelFormat::R32G32B32A32_UInt;
                    else if (inputDesc.Mask >= 0b111)
                        format = PixelFormat::R32G32B32_UInt;
                    else if (inputDesc.Mask >= 0b11)
                        format = PixelFormat::R32G32_UInt;
                    else
                        format = PixelFormat::R32_UInt;
                    break;
                case D3D_REGISTER_COMPONENT_SINT32:
                    if (inputDesc.Mask >= 0b1111)
                        format = PixelFormat::R32G32B32A32_SInt;
                    else if (inputDesc.Mask >= 0b111)
                        format = PixelFormat::R32G32B32_SInt;
                    else if (inputDesc.Mask >= 0b11)
                        format = PixelFormat::R32G32_SInt;
                    else
                        format = PixelFormat::R32_SInt;
                    break;
                case D3D_REGISTER_COMPONENT_FLOAT32:
                    if (inputDesc.Mask >= 0b1111)
                        format = PixelFormat::R32G32B32A32_Float;
                    else if (inputDesc.Mask >= 0b111)
                        format = PixelFormat::R32G32B32_Float;
                    else if (inputDesc.Mask >= 0b11)
                        format = PixelFormat::R32G32_Float;
                    else
                        format = PixelFormat::R32_Float;
                    break;
                }
                additionalDataVS.Inputs.Add({ ParseVertexElementType(inputDesc.SemanticName, inputDesc.SemanticIndex), 0, 0, 0, format });
            }
        }
        DxShaderHeader header;
        Platform::MemoryClear(&header, sizeof(header));
        ShaderBindings bindings = { desc.InstructionCount, 0, 0, 0 };
        for (uint32 a = 0; a < desc.ConstantBuffers; a++)
        {
            auto cb = shaderReflection->GetConstantBufferByIndex(a);
            D3D12_SHADER_BUFFER_DESC cbDesc;
            cb->GetDesc(&cbDesc);
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
                    _context->OnError("Missing bound resource.");
                    return true;
                }

                // Set flag
                bindings.UsedCBsMask |= 1 << slot;

                // Try to add CB to the list
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
        }
        for (uint32 i = 0; i < desc.BoundResources; i++)
        {
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
                for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                {
                    bindings.UsedSRsMask |= 1 << (resDesc.BindPoint + shift);
                    header.SrDimensions[resDesc.BindPoint + shift] = resDesc.Dimension;
                }
                break;
            case D3D_SIT_STRUCTURED:
            case D3D_SIT_BYTEADDRESS:
                for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                {
                    bindings.UsedSRsMask |= 1 << (resDesc.BindPoint + shift);
                    header.SrDimensions[resDesc.BindPoint + shift] = D3D_SRV_DIMENSION_BUFFER;
                }
                break;

            // Unordered Access
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                {
                    bindings.UsedUAsMask |= 1 << (resDesc.BindPoint + shift);
                    header.UaDimensions[resDesc.BindPoint + shift] = (byte)resDesc.Dimension; // D3D_SRV_DIMENSION matches D3D12_UAV_DIMENSION
                }
                break;
            }
        }

        if (WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, &header, sizeof(header), shaderBuffer->GetBufferPointer(), (int32)shaderBuffer->GetBufferSize()))
            return true;

        if (customDataWrite && customDataWrite(_context, meta, permutationIndex, _macros, additionalData))
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
