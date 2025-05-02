// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Text;
using System.Collections.Generic;
using Flax.Build.Bindings;

namespace Flax.Build
{
    /// <summary>
    /// The utilities for C++ typename mangling.
    /// </summary>
    internal static class CppNameMangling
    {
        public static string MangleFunctionName(Builder.BuildData buildData, string name, string outerName, TypeInfo returnType, TypeInfo parameter0 = null, List<FunctionInfo.ParameterInfo> parameters1 = null, List<FunctionInfo.ParameterInfo> parameters2 = null)
        {
            List<FunctionInfo.ParameterInfo> parameters = null;
            if (parameter0 == null)
            {
                if (parameters1 == null || parameters1.Count == 0)
                    parameters = parameters2;
                else if (parameters2 == null || parameters2.Count == 0)
                    parameters = parameters1;
            }
            if (parameters == null)
            {
                parameters = new List<FunctionInfo.ParameterInfo>();
                if (parameter0 != null)
                    parameters.Add(new FunctionInfo.ParameterInfo() { Type = parameter0 });
                if (parameters1 != null)
                    parameters.AddRange(parameters1);
                if (parameters2 != null)
                    parameters.AddRange(parameters2);
            }
            return MangleFunctionName(buildData, name, outerName, returnType, parameters);
        }

        public static string MangleFunctionName(Builder.BuildData buildData, string name, string outerName, TypeInfo returnType, List<FunctionInfo.ParameterInfo> parameters)
        {
            if (name.Contains(":"))
                throw new NotImplementedException("No nested types mangling support.");
            if (buildData.Toolchain == null)
                return name; // Ignore when building C# bindings only without native toolchain
            var sb = BindingsGenerator.GetStringBuilder();
            switch (buildData.Toolchain.Compiler)
            {
            case TargetCompiler.MSVC:
                // References:
                // https://learn.microsoft.com/en-us/cpp/build/reference/decorated-names
                // https://mearie.org/documents/mscmangle/
                sb.Append('?');
                sb.Append(name);
                sb.Append('@');
                sb.Append(outerName);
                sb.Append('@');
                // TODO: mangle parameters
                break;
            case TargetCompiler.Clang:
                // References:
                // http://web.mit.edu/tibbetts/Public/inside-c/www/mangling.html
                // https://en.wikipedia.org/wiki/Name_mangling
                sb.Append("_ZN");
                //if (!returnType.IsVoid)
                //    MangleTypeClang(sb, returnType);
                sb.Append(outerName.Length);
                sb.Append(outerName);
                sb.Append(name.Length);
                sb.Append(name);
                sb.Append('E');
                if (parameters == null || parameters.Count == 0)
                {
                    sb.Append('v');
                }
                else
                {
                    foreach (var e in parameters)
                    {
                        MangleTypeClang(sb, e.Type);
                    }
                }
                break;
            default: throw new InvalidPlatformException(buildData.Platform.Target);
            }
            var result = sb.ToString();
            BindingsGenerator.PutStringBuilder(sb);
            return result;
        }

        private static void MangleTypeClang(StringBuilder sb, TypeInfo type)
        {
            if (type.IsPtr)
                sb.Append('P');
            if (type.IsRef)
                sb.Append('R');
            if (type.IsConst)
                sb.Append('K');
            switch (type.Type)
            {
            // TODO: use proper typedef extraction from TypedefInfo
            case "Float2":
                sb.Append("11Vector2BaseIfE");
                break;
            case "Float3":
                sb.Append("11Vector3BaseIfE");
                break;
            case "Float4":
                sb.Append("11Vector4BaseIfE");
                break;

            case "bool":
                sb.Append('b');
                break;
            case "char":
            case "int8":
                sb.Append('c');
                break;
            case "int":
            case "int32":
                sb.Append('i');
                break;
            case "float":
                sb.Append('f');
                break;
            default:
                sb.Append(type.Type.Length);
                sb.Append(type.Type);
                if (type.GenericArgs != null)
                {
                    sb.Append('I');
                    foreach (var e in type.GenericArgs)
                        MangleTypeClang(sb, e);
                    sb.Append('E');
                }
                break;
            }
        }
    }
}
