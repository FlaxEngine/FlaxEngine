// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "ImportShader.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/Log.h"
#include "Engine/Platform/File.h"
#include "Engine/Graphics/Shaders/Cache/ShaderStorage.h"
#include "Engine/Graphics/Shaders/Cache/ShaderCacheManager.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Content/Assets/Shader.h"

CreateAssetResult ImportShader::Import(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(Shader, 21);
    const int32 SourceCodeChunk = 15;
    context.SkipMetadata = true;

    // Read text (handles any Unicode convert into ANSI)
    StringAnsi sourceCodeText;
    if (File::ReadAllText(context.InputPath, sourceCodeText))
        return CreateAssetResult::InvalidPath;

    // Load source code
    if (context.AllocateChunk(SourceCodeChunk))
        return CreateAssetResult::CannotAllocateChunk;
    const auto sourceCodeSize = sourceCodeText.Length();
    if (sourceCodeSize < 10)
    {
        LOG(Warning, "Empty shader source file.");
        return CreateAssetResult::Error;
    }
    context.Data.Header.Chunks[SourceCodeChunk]->Data.Allocate(sourceCodeSize + 1);
    const auto sourceCode = context.Data.Header.Chunks[SourceCodeChunk]->Get();
    Platform::MemoryCopy(sourceCode, sourceCodeText.Get(), sourceCodeSize);

    // Encrypt source code
    Encryption::EncryptBytes(sourceCode, sourceCodeSize);

    // Set Custom Data with Header
    ShaderStorage::Header21 shaderHeader;
    Platform::MemoryClear(&shaderHeader, sizeof(shaderHeader));
    context.Data.CustomData.Copy(&shaderHeader);

#if COMPILE_WITH_SHADER_CACHE_MANAGER
    // Invalidate shader cache
    ShaderCacheManager::RemoveCache(context.Data.Header.ID);
#endif

    return CreateAssetResult::Ok;
}

#endif
