// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_D3D_SHADER_COMPILER

#include "ShaderCompilerD3D.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Config.h"
#include "Engine/GraphicsDevice/DirectX/IncludeDirectXHeaders.h"
#include "Engine/Platform/Windows/ComPtr.h"
#include <d3dcompiler.h>

/// <summary>
/// Helper class to include source for D3D shaders compiler.
/// </summary>
class IncludeD3D : public ID3DInclude
{
private:
    ShaderCompilationContext* _context;

public:
    IncludeD3D(ShaderCompilationContext* context)
    {
        _context = context;
    }

    HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
    {
        const char* source;
        int32 sourceLength;
        const StringAnsi filename(pFileName);
        if (ShaderCompiler::GetIncludedFileSource(_context, "", filename.Get(), source, sourceLength))
            return E_FAIL;
        *ppData = source;
        *pBytes = sourceLength;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) override
    {
        return S_OK;
    }
};

ShaderCompilerD3D::ShaderCompilerD3D(ShaderProfile profile)
    : ShaderCompiler(profile)
{
}

#ifdef GPU_USE_SHADERS_DEBUG_LAYER

namespace
{
    bool ProcessDebugInfo(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, void* srcData, uint32 srcDataSize)
    {
        // TODO: test with D3D_DISASM_ENABLE_INSTRUCTION_CYCLE option

        ComPtr<ID3DBlob> debug;
        HRESULT result = D3DDisassemble(srcData, srcDataSize, D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS | D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING, nullptr, &debug);
        if (FAILED(result))
        {
            LOG(Warning, "DirectX error: {0} at {1}:{2}", result, TEXT(__FILE__), __LINE__);
            context->OnError("D3DDisassemble failed.");
            return true;
        }

        // Extract debug info
        char* debugData = static_cast<char*>(debug->GetBufferPointer());
        const int32 debugDataSize = static_cast<int32>(debug->GetBufferSize());
        context->OnCollectDebugInfo(meta, permutationIndex, debugData, debugDataSize);

        return false;
    }
}

#endif

namespace
{
    bool ProcessShader(ShaderCompilationContext* context, Array<ShaderCompiler::ShaderResourceBuffer>& constantBuffers, ID3D11ShaderReflection* reflector, D3D11_SHADER_DESC& desc, ShaderBindings& bindings)
    {
        // Extract constant buffers usage information
        for (uint32 a = 0; a < desc.ConstantBuffers; a++)
        {
            auto cb = reflector->GetConstantBufferByIndex(a);
            D3D11_SHADER_BUFFER_DESC cbDesc;
            cb->GetDesc(&cbDesc);
            if (cbDesc.Type == D3D_CT_CBUFFER)
            {
                // Find CB slot index
                int32 slot = INVALID_INDEX;
                for (uint32 b = 0; b < desc.BoundResources; b++)
                {
                    D3D11_SHADER_INPUT_BIND_DESC bDesc;
                    reflector->GetResourceBindingDesc(b, &bDesc);
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
            D3D11_SHADER_INPUT_BIND_DESC resDesc;
            reflector->GetResourceBindingDesc(i, &resDesc);
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
                for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                    bindings.UsedSRsMask |= 1 << (resDesc.BindPoint + shift);
                break;

            // Unordered Access
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                for (UINT shift = 0; shift < resDesc.BindCount; shift++)
                    bindings.UsedUAsMask |= 1 << (resDesc.BindPoint + shift);
                break;
            }
        }

        return false;
    }
}

bool ShaderCompilerD3D::CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite)
{
    if (WriteShaderFunctionBegin(_context, meta))
        return true;

    // Prepare
    auto options = _context->Options;
    auto type = meta.GetStage();
    IncludeD3D include(_context);
    StringAnsi profileName;
    switch (type)
    {
    case ShaderStage::Vertex:
        profileName = "vs";
        break;
    case ShaderStage::Hull:
        profileName = "hs";
        break;
    case ShaderStage::Domain:
        profileName = "ds";
        break;
    case ShaderStage::Geometry:
        profileName = "gs";
        break;
    case ShaderStage::Pixel:
        profileName = "ps";
        break;
    case ShaderStage::Compute:
        profileName = "cs";
        break;
    default:
        return true;
    }
    if (_profile == ShaderProfile::DirectX_SM5)
    {
        profileName += "_5_0";
    }
    else
    {
        profileName += "_4_0";
        if (type == ShaderStage::Domain || type == ShaderStage::Hull)
        {
            _context->OnError("Tessellation is not supported on DirectX 10");
            return true;
        }
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

        // Compile
        ComPtr<ID3DBlob> errors;
        ComPtr<ID3DBlob> shader;
        HRESULT result = D3DCompile2(
            options->Source,
            options->SourceLength,
            _context->TargetNameAnsi,
            reinterpret_cast<const D3D_SHADER_MACRO*>(_macros.Get()),
            &include,
            meta.Name.Get(),
            profileName.Get(),
            _flags,
            0,
            0,
            nullptr,
            0,
            &shader,
            &errors);
        if (FAILED(result))
        {
            const auto msg = static_cast<const char*>(errors->GetBufferPointer());
            _context->OnError(msg);
            return true;
        }
        void* shaderBuffer = shader->GetBufferPointer();
        uint32 shaderBufferSize = static_cast<int32>(shader->GetBufferSize());

        // Perform shader reflection
        ComPtr<ID3D11ShaderReflection> reflector;
        result = D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflector);
        if (FAILED(result))
        {
            LOG(Warning, "DirectX error: {0} at {1}:{2}", result, TEXT(__FILE__), __LINE__);
            _context->OnError("D3DReflect failed.");
            return true;
        }

        // Get shader description
        D3D11_SHADER_DESC desc;
        reflector->GetDesc(&desc);

        // Process shader reflection data
        void* additionalData = nullptr;
        if (type == ShaderStage::Vertex)
        {
            additionalData = &additionalDataVS;
            additionalDataVS.Inputs.Clear();
            for (UINT inputIdx = 0; inputIdx < desc.InputParameters; inputIdx++)
            {
                D3D11_SIGNATURE_PARAMETER_DESC inputDesc;
                reflector->GetInputParameterDesc(inputIdx, &inputDesc);
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
        ShaderBindings bindings = { desc.InstructionCount, 0, 0, 0 };
        if (ProcessShader(_context, _constantBuffers, reflector.Get(), desc, bindings))
            return true;

#ifdef GPU_USE_SHADERS_DEBUG_LAYER
        // Generate debug information
        if (ProcessDebugInfo(_context, meta, permutationIndex, shaderBuffer, shaderBufferSize))
            return true;
#endif

        // Strip shader bytecode for an optimization
        ComPtr<ID3DBlob> shaderStripped;
        if (!options->GenerateDebugData)
        {
            // Strip shader bytes
            result = D3DStripShader(
                shaderBuffer,
                shaderBufferSize,
                D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS,
                &shaderStripped);
            if (FAILED(result))
            {
                LOG(Warning, "Cannot strip shader.");
                return true;
            }

            // Set new buffer
            shaderBuffer = shaderStripped->GetBufferPointer();
            shaderBufferSize = static_cast<int32>(shaderStripped->GetBufferSize());
        }

        if (WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, shaderBuffer, shaderBufferSize))
            return true;

        if (customDataWrite && customDataWrite(_context, meta, permutationIndex, _macros, additionalData))
            return true;
    }

    return WriteShaderFunctionEnd(_context, meta);
}

bool ShaderCompilerD3D::OnCompileBegin()
{
    if (ShaderCompiler::OnCompileBegin())
        return true;

    _globalMacros.Add({ "DIRECTX", "1" });

    _flags = 0;
    if (_context->Options->NoOptimize)
        _flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    else
        _flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    if (_context->Options->GenerateDebugData)
        _flags |= D3DCOMPILE_DEBUG;
    if (_context->Options->TreatWarningsAsErrors)
        _flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if GRAPHICS_API_DIRECTX12
    _flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;
#endif

    return false;
}

#endif
