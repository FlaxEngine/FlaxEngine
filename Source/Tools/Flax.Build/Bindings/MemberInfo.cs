// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native member information for bindings generator.
    /// </summary>
    public class MemberInfo
    {
        public string Name;
        public string[] Comment;
        public bool IsStatic;
        public AccessLevel Access;
        public string Attributes;

        public bool HasAttribute(string name)
        {
            return Attributes != null && Attributes.Contains(name);
        }
    }
}
