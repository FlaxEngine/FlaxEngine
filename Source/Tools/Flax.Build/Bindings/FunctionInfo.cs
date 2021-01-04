// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native method information for bindings generator.
    /// </summary>
    public class FunctionInfo : MemberInfo
    {
        public struct ParameterInfo
        {
            public string Name;
            public TypeInfo Type;
            public string DefaultValue;
            public string Attributes;
            public bool IsRef;
            public bool IsOut;

            public bool HasDefaultValue => !string.IsNullOrEmpty(DefaultValue);

            public bool HasAttribute(string name)
            {
                return Attributes != null && Attributes.Contains(name);
            }

            public override string ToString()
            {
                var result = Type + " " + Name;
                if (HasDefaultValue)
                    result += " = " + DefaultValue;
                return result;
            }
        }

        public struct GlueInfo
        {
            public bool UseReferenceForResult;
            public List<ParameterInfo> CustomParameters;
        }

        public string UniqueName;
        public TypeInfo ReturnType;
        public List<ParameterInfo> Parameters;
        public bool IsVirtual;
        public bool IsConst;
        public bool NoProxy;
        public GlueInfo Glue;

        public override string ToString()
        {
            var result = string.Empty;

            if (IsStatic)
                result += "static ";
            else if (IsVirtual)
                result += "virtual ";

            result += ReturnType + " " + Name + "(";

            for (int i = 0; i < Parameters.Count; i++)
            {
                if (i > 0)
                    result += ", ";
                result += Parameters[i];
            }

            result += ")";
            if (IsConst)
                result += " const";

            return result;
        }
    }
}
