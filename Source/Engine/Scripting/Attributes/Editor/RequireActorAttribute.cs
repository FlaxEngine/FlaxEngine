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
    /// Whether to include inherited types.
    /// </summary>
    public bool IncludeInheritedTypes;

    /// <summary>
    /// Initializes a new instance of the  <see cref="RequireActorAttribute"/> class.
    /// </summary>
    /// <param name="type">The required type.</param>
    /// <param name="includeInheritedTypes">Whether to include inherited types.</param>
    public RequireActorAttribute(Type type, bool includeInheritedTypes = false)
    {
        RequiredType = type;
        IncludeInheritedTypes = includeInheritedTypes;
    }
}
