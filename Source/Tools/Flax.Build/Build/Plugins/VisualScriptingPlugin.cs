// Copyright (c) Wojciech Figat. All rights reserved.

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
            BindingsGenerator.ScriptingLangInfos.Add(new BindingsGenerator.ScriptingLangInfo
            {
                Enabled = true,
                VirtualWrapperMethodsPostfix = "_VisualScriptWrapper",
            });
        }

        private void OnGenerateCppScriptWrapperFunction(Builder.BuildData buildData, VirtualClassInfo classInfo, FunctionInfo functionInfo, int scriptVTableSize, int scriptVTableIndex, StringBuilder contents)
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
            string scriptVTableOffset;
            if (classInfo.IsInterface)
            {
                contents.AppendLine($"        auto object = ScriptingObject::FromInterface(this, {classInfo.NativeName}::TypeInitializer);");
                contents.AppendLine("        if (object == nullptr)");
                contents.AppendLine("        {");
                contents.AppendLine($"            LOG(Error, \"Failed to cast interface {{0}} to scripting object\", TEXT(\"{classInfo.Name}\"));");
                BindingsGenerator.GenerateCppReturn(buildData, contents, "            ", functionInfo.ReturnType);
                contents.AppendLine("        }");
                contents.AppendLine($"        const int32 scriptVTableOffset = {scriptVTableIndex} + object->GetType().GetInterface({classInfo.NativeName}::TypeInitializer)->ScriptVTableOffset;");
                scriptVTableOffset = "scriptVTableOffset";
            }
            else
            {
                contents.AppendLine($"        auto object = ({classInfo.NativeName}*)this;");
                scriptVTableOffset = scriptVTableIndex.ToString();
            }
            contents.AppendLine("        static THREADLOCAL void* WrapperCallInstance = nullptr;");
            contents.AppendLine("        if (WrapperCallInstance == object)");
            contents.AppendLine("        {");
            BindingsGenerator.GenerateCppVirtualWrapperCallBaseMethod(buildData, contents, classInfo, functionInfo, "object->GetType().Script.ScriptVTableBase", scriptVTableOffset);
            contents.AppendLine("        }");
            contents.AppendLine("        auto scriptVTable = (VisualScript::Method**)object->GetType().Script.ScriptVTable;");
            contents.AppendLine($"        ASSERT(scriptVTable && scriptVTable[{scriptVTableOffset}]);");

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

            contents.AppendLine("        auto prevWrapperCallInstance = WrapperCallInstance;");
            contents.AppendLine("        WrapperCallInstance = object;");
            contents.AppendLine($"        auto __result = VisualScripting::Invoke(scriptVTable[{scriptVTableOffset}], object, Span<Variant>(parameters, {functionInfo.Parameters.Count}));");
            contents.AppendLine("        WrapperCallInstance = prevWrapperCallInstance;");

            if (!functionInfo.ReturnType.IsVoid)
            {
                contents.AppendLine($"        return {BindingsGenerator.GenerateCppWrapperVariantToNative(buildData, functionInfo.ReturnType, classInfo, "__result")};");
            }

            contents.AppendLine("    }");
            contents.AppendLine();
        }
    }
}
