// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEditor.Utilities;

/// <summary>
/// Units display utilities for Editor.
/// </summary>
public static class Units
{
    /// <summary>
    /// Factor of units per meter.
    /// </summary>
    public static readonly float Meters2Units = 100f;

    /// <summary>
    /// False to always show game units without any postfix.
    /// </summary>
    public static bool UseUnitsFormatting = true;

    /// <summary>
    /// Add a space between numbers and units for readability.
    /// </summary>
    public static bool SeparateValueAndUnit = true;

    /// <summary>
    /// If set to true, the distance unit is chosen on the magnitude, otherwise it's meters.
    /// </summary>
    public static bool AutomaticUnitsFormatting = true;

    /// <summary>
    /// Return the unit according to user settings. 
    /// </summary>
    /// <param name="unit">The unit name.</param>
    /// <returns>The formatted text.</returns>
    public static string Unit(string unit)
    {
        if (SeparateValueAndUnit)
            return $" {unit}";
        return unit;
    }
}
