// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_WEBGPU_SHADER_COMPILER

#include "ShaderCompilerWebGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Engine/Globals.h"
#include "Engine/GraphicsDevice/Vulkan/Types.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include <ThirdParty/glslang/SPIRV/SpvTools.h>
#include <ThirdParty/spirv-tools/libspirv.hpp>
#include <ThirdParty/LZ4/lz4.h>

ShaderCompilerWebGPU::ShaderCompilerWebGPU(ShaderProfile profile)
    : ShaderCompilerVulkan(profile)
{
}

bool ShaderCompilerWebGPU::OnCompileBegin()
{
    _globalMacros.Add({ "WGSL", "1" });
    return ShaderCompilerVulkan::OnCompileBegin();
}

void ShaderCompilerWebGPU::InitParsing(ShaderCompilationContext* context, glslang::TShader& shader)
{
    ShaderCompilerVulkan::InitParsing(context, shader);

    // Don't flip Y like Vulkan does
    shader.setInvertY(false);

    // Use newer SPIR-V
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
}

void ShaderCompilerWebGPU::InitCodegen(ShaderCompilationContext* context, glslang::SpvOptions& spvOptions)
{
    ShaderCompilerVulkan::InitCodegen(context, spvOptions);

    // Always optimize SPIR-V
    spvOptions.disableOptimizer = false;
    spvOptions.optimizeSize = true;
}

bool ShaderCompilerWebGPU::Write(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, struct SpirvShaderHeader& header, std::vector<unsigned int>& spirv)
{
    auto id = Guid::New().ToString(Guid::FormatType::N);
    auto folder = Globals::ProjectCacheFolder / TEXT("Shaders");
    auto inputFile = folder / id + TEXT(".spvasm");
    auto outputFile = folder / id + TEXT(".wgsl");

    // Convert SPIR-V to WGSL
    // Tint compiler from: https://github.com/google/dawn/releases
    // License: Source/ThirdParty/tint-license.txt (BSD 3-Clause)
    File::WriteAllBytes(inputFile, &spirv[0], (int32)spirv.size() * sizeof(unsigned int));
    CreateProcessSettings procSettings;
    procSettings.Arguments = String::Format(TEXT("\"{}\" -if spirv -o \"{}\""), inputFile, outputFile);
    if (!context->Options->NoOptimize)
        procSettings.Arguments += TEXT(" --minify");
    procSettings.Arguments += TEXT(" --allow-non-uniform-derivatives"); // Fix sampling texture within non-uniform control flow
#if PLATFORM_WINDOWS
    procSettings.FileName = Globals::BinariesFolder / TEXT("tint.exe");
#else
    procSettings.FileName = Globals::BinariesFolder / TEXT("tint");
#endif
    int32 result = Platform::CreateProcess(procSettings);
    StringAnsi wgsl;
    File::ReadAllText(outputFile, wgsl);
    if (result != 0 || wgsl.IsEmpty())
    {
        LOG(Error, "Failed to compile shader '{}' function '{}' (permutation {}) from SPIR-V into WGSL with result {}", context->Options->TargetName, String(meta.Name), permutationIndex, result);
#if 1
        // Convert SPIR-V bytecode to text with assembly
        spvtools::SpirvTools tools(SPV_ENV_UNIVERSAL_1_3);
        std::string spirvText;
        tools.Disassemble(spirv, &spirvText);
        File::WriteAllBytes(folder / id + TEXT(".txt"), &spirvText[0], (int32)spirvText.size());
#endif
#if 1
        // Dump source code
        File::WriteAllBytes(folder / id + TEXT(".hlsl"), context->Options->Source, context->Options->SourceLength);
#endif
        return true;
    }

    // Cleanup files
    FileSystem::DeleteFile(inputFile);
    FileSystem::DeleteFile(outputFile);

    // Compress
    const int32 srcSize = wgsl.Length() + 1;
    const int32 maxSize = LZ4_compressBound(srcSize);
    Array<byte> wgslCompressed;
    wgslCompressed.Resize(maxSize + sizeof(int32));
    const int32 dstSize = LZ4_compress_default(wgsl.Get(), (char*)wgslCompressed.Get() + sizeof(int32), srcSize, maxSize);
    if (dstSize > 0)
    {
        wgslCompressed.Resize(dstSize + sizeof(int32));
        *(int32*)wgslCompressed.Get() = srcSize; // Store original size in the beginning to decompress it
        header.Type = SpirvShaderHeader::Types::WGSL_LZ4;
        return WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, &header, sizeof(header), wgslCompressed.Get(), wgslCompressed.Count());
    }


    header.Type = SpirvShaderHeader::Types::WGSL;
    return WriteShaderFunctionPermutation(_context, meta, permutationIndex, bindings, &header, sizeof(header), wgsl.Get(), wgsl.Length() + 1);
}

#endif
