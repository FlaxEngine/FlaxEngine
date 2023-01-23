// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using Flax.Build.Bindings;

namespace Flax.Build
{
    /// <summary>
    /// The utilities for C++ typename mangling.
    /// </summary>
    internal static class CppNameMangling
    {
        public static string MangleFunctionName(Builder.BuildData buildData, string name, string outerName, List<FunctionInfo.ParameterInfo> parameters1, List<FunctionInfo.ParameterInfo> parameters2)
        {
            if (parameters1 == null || parameters1.Count == 0)
                return MangleFunctionName(buildData, name, outerName, parameters2);
            if (parameters2 == null || parameters2.Count == 0)
                return MangleFunctionName(buildData, name, outerName, parameters1);
            var parameters = new List<FunctionInfo.ParameterInfo>();
            parameters.AddRange(parameters1);
            parameters.AddRange(parameters2);
            return MangleFunctionName(buildData, name, outerName, parameters);
        }

        public static string MangleFunctionName(Builder.BuildData buildData, string name, string outerName, List<FunctionInfo.ParameterInfo> parameters)
        {
            if (name.Contains(":"))
                throw new NotImplementedException("No nested types mangling support.");
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
                sb.Append("todo");
                break;
            default:
                throw new InvalidPlatformException(buildData.Platform.Target);
            }
            var result = sb.ToString();
            BindingsGenerator.PutStringBuilder(sb);
            return result;
        }
    }
}
