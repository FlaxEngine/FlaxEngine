using System;

namespace FlaxEngine;

/// <summary>
/// This attribute is used to check for if a script requires an Actor type.
/// </summary>
[Serializable]
[AttributeUsage(AttributeTargets.Class)]
public class RequireActorAttribute : Attribute
{
    /// <summary>
    /// The required type.
    /// </summary>
    public Type RequiredType;

    /// <summary>
    /// Initializes a new instance of the  <see cref="RequireActorAttribute"/> class.
    /// </summary>
    /// <param name="type">The required type.</param>
    public RequireActorAttribute(Type type)
    {
        RequiredType = type;
    }
}
