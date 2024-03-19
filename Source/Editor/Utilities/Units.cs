namespace FlaxEditor.Utilities;

public class Units
{
    /// <summary>
    /// Factor of units per meter.
    /// </summary>
    public static readonly float Meters2Units = 100f;

    /// <summary>
    /// Set it to false to always show game units without any postfix.
    /// </summary>
    public static bool UseUnitsFormatting = true;

    /// <summary>
    /// Add a space between numbers and units for readability.
    /// </summary>
    public static bool SpaceNumberAndUnits = true;

    /// <summary>
    /// If set to true, the distance unit is chosen on the magnitude, otherwise it's meters.
    /// </summary>
    public static bool AutomaticUnitsFormatting = true;

    /// <summary>
    /// Return the unit according to user settings. 
    /// </summary>
    /// <param name="unit"></param>
    /// <returns></returns>
    public static string Unit(string unit)
    {
        if (SpaceNumberAndUnits)
            return $" {unit}";
        return unit;
    }
}
