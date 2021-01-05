// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        public string DefaultValue;

        public bool HasDefaultValue => !string.IsNullOrEmpty(DefaultValue);

        public override string ToString()
        {
            var result = string.Empty;
            if (IsStatic)
                result += "static ";
            result += Type + " " + Name;
            if (HasDefaultValue)
                result += " = " + DefaultValue;
            return result;
        }
    }
}
