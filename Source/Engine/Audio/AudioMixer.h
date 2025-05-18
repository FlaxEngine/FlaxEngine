// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/Variant.h"
#include "Audio.h"

API_CLASS(NoSpawn) class FLAXENGINE_API AudioMixer : public BinaryAsset 
{
    DECLARE_BINARY_ASSET_HEADER(AudioMixer,2);
};
