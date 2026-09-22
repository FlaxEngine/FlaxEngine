namespace FlaxEngine;

/// <summary>
/// Specifies the probability of a condition evaluating to true.
/// </summary>
public enum Chance : uint
{
    /// <summary>The condition will always be <see langword="false"/>.</summary>
    /// <remarks>0% probability of being <see langword="true"/>.</remarks>
    Impossible = uint.MinValue,
    /// <summary>The condition is likely to be <see langword="false"/>.</summary>
    /// <remarks>1% probability of being <see langword="true"/>.</remarks>
    Remote = uint.MaxValue / 100,
    /// <summary>The condition is commonly <see langword="false"/> and occasionally <see langword="true"/>.</summary>
    /// <remarks>25% probability of being <see langword="true"/>.</remarks>
    Unlikely = uint.MaxValue / 4,
    /// <summary>The condition is <see langword="true"/> as often as it is <see langword="false"/>.</summary>
    /// <remarks>50% probability of being <see langword="true"/>.</remarks>
    Even = uint.MaxValue / 2,
    /// <summary>The condition is commonly <see langword="true"/> and occasionally <see langword="false"/>.</summary>
    /// <remarks>75% probability of being <see langword="true"/>.</remarks>
    Likely = uint.MaxValue / 4 * 3,
    /// <summary>The condition is likely to be <see langword="true"/>.</summary>
    /// <remarks>99% probability of being <see langword="true"/>.</remarks>
    Expected = uint.MaxValue / 100 * 99,
    /// <summary>The condition will always be <see langword="true"/>.</summary>
    /// <remarks>100% probability of being <see langword="true"/>.</remarks>
    Certain = uint.MaxValue
}
