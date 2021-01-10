// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actors/BoxVolume.h"

/// <summary>
/// A special type of volume that defines the areas of the scene in which navigation meshes are generated.
/// </summary>
API_CLASS() class FLAXENGINE_API NavMeshBoundsVolume : public BoxVolume
{
DECLARE_SCENE_OBJECT(NavMeshBoundsVolume);
protected:

    // [BoxVolume]
    void OnEnable() override;
    void OnDisable() override;
#if USE_EDITOR
    void OnBoundsChanged(const BoundingBox& prevBounds) override;
    Color GetWiresColor() override;
#endif
};
