// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

API_STRUCT() struct BatteryInfo
{
DECLARE_SCRIPTING_TYPE_MINIMAL(BatteryInfo);

    API_FIELD() byte ACLineStatus;

    API_FIELD() byte BatteryLifePercent;

    API_FIELD() uint32 BatteryLifeTime;
};
