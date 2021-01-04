// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native event information for bindings generator.
    /// </summary>
    public class EventInfo : MemberInfo
    {
        public TypeInfo Type;

        public override string ToString()
        {
            var result = string.Empty;
            if (IsStatic)
                result += "static ";
            result += Type + " " + Name;
            return result;
        }
    }
}
