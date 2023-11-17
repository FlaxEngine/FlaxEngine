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
    public Type[] RequiredTypes;

    /// <summary>
    /// Initializes a new instance of the  <see cref="RequireScriptAttribute"/> class.
    /// </summary>
    /// <param name="type">The required type.</param>
    public RequireScriptAttribute(Type type)
    {
        RequiredTypes = new[] { type };
    }

    /// <summary>
    /// Initializes a new instance of the  &lt;see cref="RequireScriptAttribute"/&gt; class.
    /// </summary>
    /// <param name="types">The required types.</param>
    public RequireScriptAttribute(Type[] types)
    {
        RequiredTypes = types;
    }
}
