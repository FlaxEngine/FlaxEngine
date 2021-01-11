// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Power supply status.
/// </summary>
API_ENUM() enum class ACLineStatus : byte
{
    /// <summary>
    /// Power supply is not connected.
    /// </summary>
    Offline = 0,
    /// <summary>
    /// Power supply is connected.
    /// </summary>
    Online = 1,
    /// <summary>
    /// Unknown status.
    /// </summary>
    Unknown = 255
};

/// <summary>
/// Contains information about power supply (Battery).
/// </summary>
API_STRUCT() struct BatteryInfo
{
DECLARE_SCRIPTING_TYPE_MINIMAL(BatteryInfo);

    /// <summary>
    /// Power supply status.
    /// </summary>
    API_FIELD() ACLineStatus ACLineStatus;

    /// <summary>
    /// Battery percentage left.
    /// </summary>
    API_FIELD() byte BatteryLifePercent;

    /// <summary>
    /// Remaining battery life time in second.
    /// </summary>
    API_FIELD() uint32 BatteryLifeTime;
};
