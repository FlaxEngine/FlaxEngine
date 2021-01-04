// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native property information for bindings generator.
    /// </summary>
    public class PropertyInfo : MemberInfo
    {
        public TypeInfo Type;
        public FunctionInfo Getter;
        public FunctionInfo Setter;

        public override string ToString()
        {
            return Type + " " + Name;
        }
    }
}
