// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Contains information about power supply (battery).
/// </summary>
API_STRUCT(NoDefault) struct BatteryInfo
{
DECLARE_SCRIPTING_TYPE_MINIMAL(BatteryInfo);

	/// <summary>
	/// Power supply status.
	/// </summary>
	API_ENUM() enum class States
	{
	    /// <summary>
	    /// Unknown status.
	    /// </summary>
	    Unknown,
		
	    /// <summary>
	    /// Power supply is connected and battery is charging.
	    /// </summary>
	    BatteryCharging,

	    /// <summary>
	    /// Device is running on a battery.
	    /// </summary>
	    BatteryDischarging,
		
	    /// <summary>
	    /// Device is connected to the stable power supply (AC).
	    /// </summary>
	    Connected,
	};

    /// <summary>
    /// Power supply state.
    /// </summary>
    API_FIELD() BatteryInfo::States State = BatteryInfo::States::Unknown;

    /// <summary>
    /// Battery percentage left (normalized to 0-1 range).
    /// </summary>
    API_FIELD() float BatteryLifePercent = 1.0f;
};
