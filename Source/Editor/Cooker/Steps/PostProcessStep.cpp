// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "PostProcessStep.h"
#include "Editor/Cooker/PlatformTools.h"

bool PostProcessStep::Perform(CookingData& data)
{
    return data.Tools->OnPostProcess(data);
}
