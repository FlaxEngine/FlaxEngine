namespace FlaxEditor.Utilities;

public class Units
{
    /// <summary>
    /// Factor of units per meter.
    /// </summary>
    public static readonly float Meters2Units = 100f;

    // the next two bools could be cached values in the user preferences

    /// <summary>
    /// Set it to false to always show game units without any postfix
    /// </summary>
    public static bool UseUnitsFormatting = true;

    /// <summary>
    /// If set to true, the distance unit is chosen on the magnitude, otherwise it's meters
    /// </summary>
    public static bool AutomaticUnitsFormatting = true;
}
