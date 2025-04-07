// Copyright (c) Wojciech Figat. All rights reserved.

#include "ImportFont.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/DeleteMe.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Render2D/FontAsset.h"

CreateAssetResult ImportFont::Import(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(FontAsset, 3);

    // Setup header
    FontOptions options;
    options.Hinting = FontHinting::Default;
    options.Flags = FontFlags::AntiAliasing;
    context.Data.CustomData.Copy(&options);

    // Open the file
    auto stream = FileReadStream::Open(context.InputPath);
    if (stream == nullptr)
        return CreateAssetResult::InvalidPath;
    DeleteMe<FileReadStream> deleteStream(stream);

    // Copy font file data
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    const auto size = stream->GetLength();
    context.Data.Header.Chunks[0]->Data.Allocate(size);
    stream->ReadBytes(context.Data.Header.Chunks[0]->Get(), size);

    return CreateAssetResult::Ok;
}

#endif
