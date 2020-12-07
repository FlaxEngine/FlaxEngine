// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native field information for bindings generator.
    /// </summary>
    public class FieldInfo : MemberInfo
    {
        public TypeInfo Type;
        public bool IsReadOnly;
        public bool NoArray;
        public FunctionInfo Getter;
        public FunctionInfo Setter;

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
