// Copyright (c) Wojciech Figat. All rights reserved.

#include "CollectAssetsStep.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Cache/AssetsCache.h"

bool CollectAssetsStep::Perform(CookingData& data)
{
    LOG(Info, "Searching for assets to include in a build. Using {0} root assets.", data.RootAssets.Count());
    data.StepProgress(TEXT("Collecting assets"), 0);

    // Initialize assets queue
    Array<Guid> assetsQueue;
    assetsQueue.Clear();
    assetsQueue.EnsureCapacity(1024);
    for (auto i = data.RootAssets.Begin(); i.IsNotEnd(); ++i)
        assetsQueue.Add(i->Item);

    // Iterate through the assets graph
    AssetInfo assetInfo;
    Array<Guid> references;
    Array<String> files;
    while (assetsQueue.HasItems())
    {
        BUILD_STEP_CANCEL_CHECK;
        const Guid assetId = assetsQueue.Dequeue();

        // Skip already processed or invalid assets
        if (!assetId.IsValid()
            || !Content::GetRegistry()->FindAsset(assetId, assetInfo)
            || data.Assets.Contains(assetId))
            continue;

        // Skip some assets (with no refs and not required to load)
        if (assetInfo.TypeName == Texture::TypeName ||
            assetInfo.TypeName == CubeTexture::TypeName ||
            assetInfo.TypeName == Shader::TypeName)
        {
            LOG_STR(Info, assetInfo.Path);
            data.Assets.Add(assetId);
            continue;
        }

        // Load asset
        AssetReference<Asset> asset = Content::LoadAsync<Asset>(assetId);
        if (asset == nullptr)
            continue;
        LOG_STR(Info, asset->GetPath());
        data.Assets.Add(assetId);

        // Skip virtual/temporary assets
        if (asset->IsVirtual())
            continue;

        // Asset should have loaded data
        if (asset->WaitForLoaded())
            continue;

        // Gather asset references
        references.Clear();
        asset->Locker.Lock();
        asset->GetReferences(references, files);
        asset->Locker.Unlock();
        assetsQueue.Add(references);
        for (String& file : files)
        {
            if (file.HasChars())
                data.Files.Add(MoveTemp(file));
        }
    }

    data.Stats.TotalAssets = data.Assets.Count();
    LOG(Info, "Found {0} assets to deploy!", data.Assets.Count());

    return false;
}
