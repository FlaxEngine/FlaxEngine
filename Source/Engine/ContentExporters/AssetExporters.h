// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_ASSETS_EXPORTER

#include "Types.h"

class AssetExporters
{
public:
    static ExportAssetResult ExportTexture(ExportAssetContext& context);
    static ExportAssetResult ExportCubeTexture(ExportAssetContext& context);
    static ExportAssetResult ExportAudioClip(ExportAssetContext& context);
    static ExportAssetResult ExportModel(ExportAssetContext& context);
    static ExportAssetResult ExportSkinnedModel(ExportAssetContext& context);
};

#endif
