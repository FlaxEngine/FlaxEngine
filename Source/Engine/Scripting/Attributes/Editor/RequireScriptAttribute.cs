using System;

namespace FlaxEngine;

/// <summary>
/// This attribute is used to check for if a script requires another script type.
/// </summary>
[Serializable]
[AttributeUsage(AttributeTargets.Class)]
public class RequireScriptAttribute : Attribute
{
    /// <summary>
    /// The required type.
    /// </summary>
    public Type RequiredType;

    /// <summary>
    /// Initializes a new instance of the  <see cref="RequireScriptAttribute"/> class.
    /// </summary>
    /// <param name="type">The required type.</param>
    public RequireScriptAttribute(Type type)
    {
        RequiredType = type;
    }
}
