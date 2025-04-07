// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_ASSETS_EXPORTER

#include "AssetExporters.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Tools/TextureTool/TextureTool.h"

ExportAssetResult AssetExporters::ExportTexture(ExportAssetContext& context)
{
    // Prepare
    auto asset = (TextureBase*)context.Asset.Get();
    auto lock = asset->Storage->LockSafe();
    auto path = GET_OUTPUT_PATH(context, "png");

    // Load the top mip data
    const int32 chunkIndex = 0;
    if (asset->LoadChunk(chunkIndex))
        return ExportAssetResult::CannotLoadData;
    BytesContainer data;
    asset->GetMipData(0, data);
    if (data.IsInvalid())
        return ExportAssetResult::Error;

    // Peek image description
    const auto format = asset->Format();
    const int32 width = asset->Width();
    const int32 height = asset->Height();
    uint32 rowPitch, slicePitch;
    RenderTools::ComputePitch(format, width, height, rowPitch, slicePitch);

    // Setup texture data
    TextureData textureData;
    textureData.Width = width;
    textureData.Height = height;
    textureData.Depth = 1;
    textureData.Format = format;
    textureData.Items.Resize(1);
    auto& item = textureData.Items[0];
    item.Mips.Resize(1);
    auto& mip = item.Mips[0];
    mip.RowPitch = rowPitch;
    mip.DepthPitch = slicePitch;
    mip.Lines = height;
    mip.Data.Link(data.Get(), slicePitch);

    // Export to file
    if (TextureTool::ExportTexture(path, textureData))
    {
        return ExportAssetResult::Error;
    }

    return ExportAssetResult::Ok;
}

ExportAssetResult AssetExporters::ExportCubeTexture(ExportAssetContext& context)
{
    // Prepare
    auto asset = (CubeTexture*)context.Asset.Get();
    auto lock = asset->Storage->LockSafe();
    auto path = GET_OUTPUT_PATH(context, "dds");

    // Load the asset data
    if (asset->LoadChunks(ALL_ASSET_CHUNKS))
        return ExportAssetResult::CannotLoadData;

    // Peek image description
    const auto format = asset->Format();
    const int32 width = asset->Width();
    const int32 height = asset->Height();
    const int32 mipLevels = asset->StreamingTexture()->TotalMipLevels();
    const int32 arraySize = 6;

    // Setup texture data
    TextureData textureData;
    textureData.Width = width;
    textureData.Height = height;
    textureData.Depth = 1;
    textureData.Format = format;
    textureData.Items.Resize(arraySize);
    for (int32 arrayIndex = 0; arrayIndex < 6; arrayIndex++)
    {
        auto& item = textureData.Items[arrayIndex];
        item.Mips.Resize(mipLevels);
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            auto& mip = item.Mips[mipIndex];

            const int32 mipWidth = Math::Max(1, width >> mipIndex);
            const int32 mipHeight = Math::Max(1, height >> mipIndex);
            uint32 rowPitch, slicePitch;
            RenderTools::ComputePitch(format, mipWidth, mipHeight, rowPitch, slicePitch);

            mip.RowPitch = rowPitch;
            mip.DepthPitch = slicePitch;
            mip.Lines = mipHeight;

            BytesContainer wholeMipData;
            asset->GetMipData(mipIndex, wholeMipData);
            if (wholeMipData.IsInvalid())
                return ExportAssetResult::Error;

            mip.Data.Link(wholeMipData.Get() + slicePitch * arrayIndex, slicePitch);
        }
    }

    // Export to file
    if (TextureTool::ExportTexture(path, textureData))
    {
        return ExportAssetResult::Error;
    }

    return ExportAssetResult::Ok;
}

#endif
