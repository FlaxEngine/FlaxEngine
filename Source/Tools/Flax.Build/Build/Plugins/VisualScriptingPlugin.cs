// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Text;
using Flax.Build.Bindings;

namespace Flax.Build.Plugins
{
    /// <summary>
    /// Flax.Build plugin for Visual Scripting support. Generates required bindings glue code for overriding native methods with Visual Scripts. 
    /// </summary>
    /// <seealso cref="Flax.Build.Plugin" />
    sealed class VisualScriptingPlugin : Plugin
    {
        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            BindingsGenerator.GenerateCppScriptWrapperFunction += OnGenerateCppScriptWrapperFunction;
            BindingsGenerator.CppScriptObjectVirtualWrapperMethodsPostfixes.Add("_VisualScriptWrapper");
        }

        private void OnGenerateCppScriptWrapperFunction(Builder.BuildData buildData, ClassInfo classInfo, FunctionInfo functionInfo, int scriptVTableSize, int scriptVTableIndex, StringBuilder contents)
        {
            // Generate C++ wrapper function to invoke Visual Script instead of overridden native function (with support for base method callback)

            BindingsGenerator.CppIncludeFiles.Add("Engine/Content/Assets/VisualScript.h");

            contents.AppendFormat("    {0} {1}_VisualScriptWrapper(", functionInfo.ReturnType, functionInfo.UniqueName);
            var separator = false;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;

                contents.Append(parameterInfo.Type);
                contents.Append(' ');
                contents.Append(parameterInfo.Name);
            }

            contents.Append(')');
            contents.AppendLine();
            contents.AppendLine("    {");
            contents.AppendLine($"        auto object = ({classInfo.NativeName}*)this;");
            contents.AppendLine("        static THREADLOCAL bool IsDuringWrapperCall = false;");
            contents.AppendLine("        if (IsDuringWrapperCall)");
            contents.AppendLine("        {");
            contents.AppendLine("            // Prevent stack overflow by calling base method");
            contents.AppendLine("            const auto scriptVTableBase = object->GetType().Script.ScriptVTableBase;");
            contents.Append($"            return (object->**({functionInfo.UniqueName}_Signature*)&scriptVTableBase[{scriptVTableIndex} + 2])(");
            separator = false;
            for (var i = 0; i < functionInfo.Parameters.Count; i++)
            {
                var parameterInfo = functionInfo.Parameters[i];
                if (separator)
                    contents.Append(", ");
                separator = true;
                contents.Append(parameterInfo.Name);
            }
            contents.AppendLine(");");
            contents.AppendLine("        }");
            contents.AppendLine("        auto scriptVTable = (VisualScript::Method**)object->GetType().Script.ScriptVTable;");
            contents.AppendLine($"        ASSERT(scriptVTable && scriptVTable[{scriptVTableIndex}]);");

            if (functionInfo.Parameters.Count != 0)
            {
                contents.AppendLine($"        Variant parameters[{functionInfo.Parameters.Count}];");
                for (var i = 0; i < functionInfo.Parameters.Count; i++)
                {
                    var parameterInfo = functionInfo.Parameters[i];
                    contents.AppendLine($"        parameters[{i}] = {BindingsGenerator.GenerateCppWrapperNativeToVariant(buildData, parameterInfo.Type, classInfo, parameterInfo.Name)};");
                }
            }
            else
            {
                contents.AppendLine("        Variant* parameters = nullptr;");
            }

            contents.AppendLine("        IsDuringWrapperCall = true;");
            contents.AppendLine($"        auto __result = VisualScripting::Invoke(scriptVTable[{scriptVTableIndex}], object, Span<Variant>(parameters, {functionInfo.Parameters.Count}));");
            contents.AppendLine("        IsDuringWrapperCall = false;");

            if (!functionInfo.ReturnType.IsVoid)
            {
                contents.AppendLine($"        return {BindingsGenerator.GenerateCppWrapperVariantToNative(buildData, functionInfo.ReturnType, classInfo, "__result")};");
            }

            contents.AppendLine("    }");
            contents.AppendLine();
        }
    }
}
