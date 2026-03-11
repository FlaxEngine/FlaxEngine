namespace FlaxEngine;

/// <summary>
/// Specifies the probability of a condition evaluating to true.
/// </summary>
public enum Chance : byte
{
    /// <summary>The condition will always be <see langword="false"/>.</summary>
    /// <remarks>0% probability of being <see langword="true"/>.</remarks>
    Impossible,
    /// <summary>The condition is likely to be <see langword="false"/>.</summary>
    /// <remarks>1% probability of being <see langword="true"/>.</remarks>
    Remote,
    /// <summary>The condition is commonly <see langword="false"/> and occasionally <see langword="true"/>.</summary>
    /// <remarks>25% probability of being <see langword="true"/>.</remarks>
    Unlikely,
    /// <summary>The condition is <see langword="true"/> as often as it is <see langword="false"/>.</summary>
    /// <remarks>50% probability of being <see langword="true"/>.</remarks>
    Even,
    /// <summary>The condition is commonly <see langword="true"/> and occasionally <see langword="false"/>.</summary>
    /// <remarks>75% probability of being <see langword="true"/>.</remarks>
    Likely,
    /// <summary>The condition is likely to be <see langword="true"/>.</summary>
    /// <remarks>99% probability of being <see langword="true"/>.</remarks>
    Expected,
    /// <summary>The condition will always be <see langword="true"/>.</summary>
    /// <remarks>100% probability of being <see langword="true"/>.</remarks>
    Certain
}
