// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.Options;
using FlaxEditor.Scripting;
using FlaxEngine.Utilities;
using FlaxEngine;
using FlaxEditor.GUI;

namespace FlaxEditor.Surface
{
    internal static class SurfaceUtils
    {
        internal struct GraphParameterData
        {
            public GraphParameter Parameter;
            public bool IsPublic;
            public Type Type;
            public object Tag;
            public Attribute[] Attributes;
            public EditorOrderAttribute Order;
            public EditorDisplayAttribute Display;
            public TooltipAttribute Tooltip;
            public SpaceAttribute Space;
            public HeaderAttribute Header;
            public string DisplayName;

            public GraphParameterData(GraphParameter parameter, object tag = null)
            : this(parameter, null, parameter.IsPublic, parameter.Type, SurfaceMeta.GetAttributes(parameter), tag)
            {
            }

            public GraphParameterData(GraphParameter parameter, string name, bool isPublic, Type type, Attribute[] attributes, object tag)
            {
                Parameter = parameter;
                IsPublic = isPublic;
                Type = type;
                Tag = tag;
                Attributes = attributes;
                Order = (EditorOrderAttribute)Attributes.FirstOrDefault(x => x is EditorOrderAttribute);
                Display = (EditorDisplayAttribute)Attributes.FirstOrDefault(x => x is EditorDisplayAttribute);
                Tooltip = (TooltipAttribute)Attributes.FirstOrDefault(x => x is TooltipAttribute);
                Space = (SpaceAttribute)Attributes.FirstOrDefault(x => x is SpaceAttribute);
                Header = (HeaderAttribute)Attributes.FirstOrDefault(x => x is HeaderAttribute);
                DisplayName = Display?.Name ?? name ?? parameter.Name;
            }

            public static int Compare(GraphParameterData x, GraphParameterData y)
            {
                // By order
                if (x.Order != null)
                {
                    if (y.Order != null)
                        return x.Order.Order - y.Order.Order;
                    return -1;
                }
                if (y.Order != null)
                    return 1;

                // By group name
                if (x.Display?.Group != null)
                {
                    if (y.Display?.Group != null)
                        return string.Compare(x.Display.Group, y.Display.Group, StringComparison.InvariantCulture);
                }

                // By name
                if (Editor.Instance.Options.Options.General.ScriptMembersOrder == GeneralOptions.MembersOrder.Alphabetical)
                    return string.Compare(x.DisplayName, y.DisplayName, StringComparison.InvariantCulture);

                // Keep same order
                return 0;
            }
        }

        private static Type ToType(MaterialParameterType type)
        {
            switch (type)
            {
            case MaterialParameterType.Bool: return typeof(bool);
            case MaterialParameterType.Integer: return typeof(int);
            case MaterialParameterType.Float: return typeof(float);
            case MaterialParameterType.Vector2: return typeof(Vector2);
            case MaterialParameterType.Vector3: return typeof(Vector3);
            case MaterialParameterType.Vector4: return typeof(Vector4);
            case MaterialParameterType.Color: return typeof(Color);
            case MaterialParameterType.Texture: return typeof(Texture);
            case MaterialParameterType.CubeTexture: return typeof(CubeTexture);
            case MaterialParameterType.NormalMap: return typeof(Texture);
            case MaterialParameterType.SceneTexture: return typeof(MaterialSceneTextures);
            case MaterialParameterType.GPUTexture: return typeof(GPUTexture);
            case MaterialParameterType.Matrix: return typeof(Matrix);
            case MaterialParameterType.ChannelMask: return typeof(ChannelMask);
            case MaterialParameterType.GameplayGlobal: return typeof(GameplayGlobals);
            case MaterialParameterType.TextureGroupSampler: return typeof(int);
            case MaterialParameterType.GlobalSDF: return typeof(object);
            default: throw new ArgumentOutOfRangeException(nameof(type), type, null);
            }
        }

        internal static GroupElement InitGraphParametersGroup(LayoutElementsContainer layout)
        {
            return CustomEditors.Editors.GenericEditor.OnGroup(layout, "Parameters");
        }

        private sealed class DummyMaterialSurfaceOwner : IVisjectSurfaceOwner
        {
            public Asset SurfaceAsset => null;
            public string SurfaceName => null;
            public FlaxEditor.Undo Undo => null;
            public byte[] SurfaceData { get; set; }
            public VisjectSurfaceContext ParentContext => null;

            public void OnContextCreated(VisjectSurfaceContext context)
            {
            }

            public void OnSurfaceEditedChanged()
            {
            }

            public void OnSurfaceGraphEdited()
            {
            }

            public void OnSurfaceClose()
            {
            }
        }

        private static void FindGraphParameters(Material material, List<SurfaceParameter> surfaceParameters)
        {
            if (material == null || material.WaitForLoaded())
                return;
            var surfaceData = material.LoadSurface(false);
            if (surfaceData != null && surfaceData.Length > 0)
            {
                var surfaceOwner = new DummyMaterialSurfaceOwner { SurfaceData = surfaceData };
                var surface = new MaterialSurface(surfaceOwner);
                if (!surface.Load())
                {
                    surfaceParameters.AddRange(surface.Parameters);

                    // Search for any nested parameters (eg. via Sample Layer)
                    foreach (var node in surface.Nodes)
                    {
                        if (node.GroupArchetype.GroupID == 8 && node.Archetype.TypeID == 1) // Sample Layer
                        {
                            if (node.Values != null && node.Values.Length > 0 && node.Values[0] is Guid layerId)
                            {
                                var layer = FlaxEngine.Content.Load<MaterialBase>(layerId);
                                if (layer)
                                {
                                    FindGraphParameters(layer, surfaceParameters);
                                }
                            }
                        }
                    }
                }
            }
        }

        private static void FindGraphParameters(MaterialBase materialBase, List<SurfaceParameter> surfaceParameters)
        {
            while (materialBase != null && !materialBase.WaitForLoaded())
            {
                if (materialBase is MaterialInstance materialInstance)
                {
                    materialBase = materialInstance.BaseMaterial;
                }
                else if (materialBase is Material material)
                {
                    FindGraphParameters(material, surfaceParameters);
                    break;
                }
            }
        }

        internal static GraphParameterData[] InitGraphParameters(IEnumerable<MaterialParameter> parameters, Material material)
        {
            int count = parameters.Count();
            var data = new GraphParameterData[count];
            int i = 0;

            // Load material surface parameters meta to use it for material instance parameters editing
            var surfaceParameters = new List<SurfaceParameter>();
            try
            {
                Profiler.BeginEvent("Init Material Parameters UI Data");
                FindGraphParameters(material, surfaceParameters);
            }
            catch (Exception ex)
            {
                Editor.LogError("Failed to get material parameters metadata.");
                Editor.LogWarning(ex);
            }
            finally
            {
                Profiler.EndEvent();
            }

            foreach (var parameter in parameters)
            {
                var parameterId = parameter.ParameterID;
                var surfaceParameter = surfaceParameters.FirstOrDefault(x => x.ID == parameterId);
                if (surfaceParameter == null)
                {
                    // Permutate original parameter ID to reflect logic in MaterialGenerator::prepareLayer used for nested layers
                    unsafe
                    {
                        var raw = parameterId;
                        var interop = *(FlaxEngine.Json.JsonSerializer.GuidInterop*)&raw;
                        interop.A -= (uint)(i * 17 + 13);
                        parameterId = *(Guid*)&interop;
                    }
                    surfaceParameter = surfaceParameters.FirstOrDefault(x => x.ID == parameterId);
                }
                if (surfaceParameter != null)
                {
                    // Reorder so it won't be picked by other parameter that uses the same ID (eg. params from duplicated materials used as layers in other material)
                    surfaceParameters.Remove(surfaceParameter);
                    surfaceParameters.Add(surfaceParameter);
                }
                var attributes = surfaceParameter?.Meta.GetAttributes() ?? FlaxEngine.Utils.GetEmptyArray<Attribute>();
                data[i] = new GraphParameterData(null, parameter.Name, parameter.IsPublic, ToType(parameter.ParameterType), attributes, parameter);
                i++;
            }
            Array.Sort(data, GraphParameterData.Compare);
            return data;
        }

        internal static GraphParameterData[] InitGraphParameters(IEnumerable<ParticleEffectParameter> parameters)
        {
            int count = parameters.Count();
            var data = new GraphParameterData[count];
            int i = 0;
            foreach (var parameter in parameters)
            {
                data[i] = new GraphParameterData(parameter.EmitterParameter, parameter);
                i++;
            }
            Array.Sort(data, GraphParameterData.Compare);
            return data;
        }

        internal static GraphParameterData[] InitGraphParameters(IEnumerable<GraphParameter> parameters)
        {
            int count = parameters.Count();
            var data = new GraphParameterData[count];
            int i = 0;
            foreach (var parameter in parameters)
            {
                data[i] = new GraphParameterData(parameter);
                i++;
            }
            Array.Sort(data, GraphParameterData.Compare);
            return data;
        }

        internal delegate object GetGraphParameterDelegate(object instance, GraphParameter parameter, object parameterTag);

        internal delegate void SetGraphParameterDelegate(object instance, object value, GraphParameter parameter, object parameterTag);

        internal delegate void CustomPropertySpawnDelegate(LayoutElementsContainer itemLayout, ValueContainer valueContainer, ref GraphParameterData e);

        internal static void DisplayGraphParameters(LayoutElementsContainer layout, GraphParameterData[] data, GetGraphParameterDelegate getter, SetGraphParameterDelegate setter, ValueContainer values, GetGraphParameterDelegate defaultValueGetter = null, CustomPropertySpawnDelegate propertySpawn = null)
        {
            CustomEditors.Editors.GenericEditor.OnGroupsBegin();
            for (int i = 0; i < data.Length; i++)
            {
                ref var e = ref data[i];
                if (!e.IsPublic)
                    continue;
                var tag = e.Tag;
                var parameter = e.Parameter;

                // Editor Display
                var itemLayout = CustomEditors.Editors.GenericEditor.OnGroup(layout, e.Display);
                if (itemLayout is GroupElement groupElement)
                    groupElement.Panel.Open(false);

                // Space
                if (e.Space != null)
                    itemLayout.Space(e.Space.Height);

                // Header
                if (e.Header != null)
                    itemLayout.Header(e.Header);

                // Values container
                var valueType = new ScriptType(e.Type);
                var valueContainer = new CustomValueContainer(valueType, (instance, index) => getter(values[index], parameter, tag), (instance, index, value) => setter(values[index], value, parameter, tag), e.Attributes);
                for (int j = 0; j < values.Count; j++)
                    valueContainer.Add(getter(values[j], parameter, tag));

                // Default value
                if (defaultValueGetter != null)
                    valueContainer.SetDefaultValue(defaultValueGetter(values[0], parameter, tag));

                // Prefab value
                if (values.HasReferenceValue)
                {
                    var referenceValue = getter(values.ReferenceValue, parameter, tag);
                    if (referenceValue != null)
                    {
                        valueContainer.SetReferenceValue(referenceValue);
                    }
                }

                if (propertySpawn == null)
                    itemLayout.Property(e.DisplayName, valueContainer, null, e.Tooltip?.Text);
                else
                    propertySpawn(itemLayout, valueContainer, ref e);
            }
            CustomEditors.Editors.GenericEditor.OnGroupsEnd();
        }

        internal static string GetMethodDisplayName(string methodName)
        {
            if (methodName.StartsWith("get_"))
                return "Get " + methodName.Substring(4);
            if (methodName.StartsWith("set_"))
                return "Set " + methodName.Substring(4);
            if (methodName.StartsWith("op_"))
                return "Operator " + methodName.Substring(3);
            return methodName;
        }

        internal static bool IsValidVisualScriptInvokeMethod(ScriptMemberInfo member, out ScriptMemberInfo.Parameter[] parameters)
        {
            parameters = null;
            if (member.IsMethod &&
                IsValidVisualScriptType(member.ValueType) &&
                !member.HasAttribute(typeof(HideInEditorAttribute), true))
            {
                parameters = member.GetParameters();
                if (parameters.Length <= 27)
                {
                    for (int i = 0; i < parameters.Length; i++)
                    {
                        var parameter = parameters[i];
                        if (!IsValidVisualScriptType(parameter.Type))
                            return false;
                    }
                    return true;
                }
            }
            return false;
        }

        internal static bool IsValidVisualScriptOverrideMethod(ScriptMemberInfo member, out ScriptMemberInfo.Parameter[] parameters)
        {
            if (member.IsMethod &&
                member.IsVirtual &&
                IsValidVisualScriptType(member.ValueType) &&
                member.HasAttribute(typeof(UnmanagedAttribute), true) &&
                !member.HasAttribute(typeof(HideInEditorAttribute), true))
            {
                parameters = member.GetParameters();
                if (parameters.Length < 31)
                {
                    for (int i = 0; i < parameters.Length; i++)
                    {
                        var parameter = parameters[i];
                        if (!IsValidVisualScriptType(parameter.Type) || parameter.IsOut)
                            return false;
                    }
                    return true;
                }
            }
            parameters = null;
            return false;
        }

        internal static bool IsValidVisualScriptField(ScriptMemberInfo member)
        {
            return member.IsField && IsValidVisualScriptType(member.ValueType);
        }

        internal static bool IsValidVisualScriptEvent(ScriptMemberInfo member)
        {
            return member.IsEvent && member.HasAttribute(typeof(UnmanagedAttribute));
        }

        internal static bool IsValidVisualScriptType(ScriptType scriptType)
        {
            if (!scriptType.IsPublic ||
                scriptType.HasAttribute(typeof(HideInEditorAttribute), true) ||
                scriptType.HasAttribute(typeof(System.Runtime.CompilerServices.CompilerGeneratedAttribute), false))
                return false;
            if (scriptType.IsGenericType)
            {
                // Only Dictionary generic type is valid
                return scriptType.GetGenericTypeDefinition() == typeof(Dictionary<,>);
            }
            var managedType = TypeUtils.GetType(scriptType);
            return !TypeUtils.IsDelegate(managedType);
        }

        internal static bool IsValidVisualScriptFunctionType(ScriptType scriptType)
        {
            if (scriptType.IsGenericType || scriptType.IsStatic || !scriptType.IsPublic || scriptType.HasAttribute(typeof(HideInEditorAttribute), true))
                return false;
            var managedType = TypeUtils.GetType(scriptType);
            return !TypeUtils.IsDelegate(managedType);
        }

        internal static string GetVisualScriptTypeDescription(ScriptType type)
        {
            if (type == ScriptType.Null)
                return "null";
            var sb = new StringBuilder();
            if (type.IsStatic)
                sb.Append("static ");
            else if (type.IsAbstract)
                sb.Append("abstract ");
            sb.Append(Editor.Instance.CodeDocs.GetTooltip(type));
            return sb.ToString();
        }

        internal static string GetVisualScriptMemberInfoSignature(ScriptMemberInfo member)
        {
            var name = member.Name;
            var declaringType = member.DeclaringType;
            var valueType = member.ValueType;

            // Getter/setter method of the property - we can return early here
            if (member.IsMethod && (name.StartsWith("get_") || name.StartsWith("set_")))
            {
                var flags = member.IsStatic ? BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly : BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly;
                var property = declaringType.GetMembers(name.Substring(4), MemberTypes.Property, flags);
                if (property != null && property.Length != 0)
                {
                    return GetVisualScriptMemberInfoSignature(property[0]);
                }
            }

            var sb = new StringBuilder();
            if (member.IsStatic)
                sb.Append("static ");
            else if (member.IsVirtual)
                sb.Append("virtual ");
            sb.Append(valueType.Name);
            sb.Append(' ');
            if (member.IsMethod)
                sb.Append(member.DeclaringType.Namespace).Append('.');
            sb.Append(declaringType.Name);
            sb.Append('.');
            sb.Append(name);

            if (member.IsMethod)
            {
                sb.Append('(');
                var parameters = member.GetParameters();
                for (int i = 0; i < parameters.Length; i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    ref var param = ref parameters[i];
                    param.ToString(sb);
                }
                sb.Append(')');
            }
            else if (member.IsProperty)
            {
                sb.Append(' ');
                sb.Append('{');
                if (member.HasGet)
                    sb.Append(" get;");
                if (member.HasSet)
                    sb.Append(" set;");
                sb.Append(' ');
                sb.Append('}');
            }

            return sb.ToString();
        }

        internal static string GetVisualScriptMemberShortDescription(ScriptMemberInfo member)
        {
            var name = member.Name;
            var declaringType = member.DeclaringType;

            // Getter/setter method of the property - we can return early here
            if (member.IsMethod && (name.StartsWith("get_") || name.StartsWith("set_")))
            {
                var flags = member.IsStatic ? BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly : BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly;
                var property = declaringType.GetMembers(name.Substring(4), MemberTypes.Property, flags);
                if (property != null && property.Length != 0)
                {
                    return GetVisualScriptMemberShortDescription(property[0]);
                }
            }

            return Editor.Instance.CodeDocs.GetTooltip(member);
        }

        internal static string GetVisualScriptMemberInfoDescription(ScriptMemberInfo member)
        {
            var signature = GetVisualScriptMemberInfoSignature(member);
            var sb = new StringBuilder(signature);

            // Tooltip
            var tooltip = GetVisualScriptMemberShortDescription(member);
            if (!string.IsNullOrEmpty(tooltip))
                sb.Append("\n").Append(tooltip);

            return sb.ToString();
        }

        internal static Double4 GetDouble4(object v, float w = 0.0f)
        {
            var value = new Double4(0, 0, 0, w);
            if (v is Vector2 vec2)
                value = new Double4(vec2, 0.0f, w);
            else if (v is Vector3 vec3)
                value = new Double4(vec3, w);
            else if (v is Vector4 vec4)
                value = vec4;
            else if (v is Float2 float2)
                value = new Double4(float2, 0.0, w);
            else if (v is Float3 float3)
                value = new Double4(float3, w);
            else if (v is Float4 float4)
                value = float4;
            else if (v is Double2 double2)
                value = new Double4(double2, 0.0, w);
            else if (v is Double3 double3)
                value = new Double4(double3, w);
            else if (v is Double4 double4)
                value = double4;
            else if (v is Color col)
                value = new Double4(col.R, col.G, col.B, col.A);
            else if (v is float f)
                value = new Double4(f);
            else if (v is int i)
                value = new Double4(i);
            return value;
        }

        private static bool AreScriptTypesEqualInner(ScriptType left, ScriptType right)
        {
            // Special case for Vector types that use typedefs and might overlap
            if (left.Type == typeof(Vector2) && (right.Type == typeof(Float2) || right.Type == typeof(Double2)))
                return true;
            if (left.Type == typeof(Vector3) && (right.Type == typeof(Float3) || right.Type == typeof(Double3)))
                return true;
            if (left.Type == typeof(Vector4) && (right.Type == typeof(Float4) || right.Type == typeof(Double4)))
                return true;
            return false;
        }

        internal static bool AreScriptTypesEqual(ScriptType left, ScriptType right)
        {
            if (left == right)
                return true;
            return AreScriptTypesEqualInner(left, right) || AreScriptTypesEqualInner(right, left);
        }

        internal static void PerformCommonSetup(Windows.Assets.AssetEditorWindow window, ToolStrip toolStrip, VisjectSurface surface,
                                                out ToolStripButton saveButton, out ToolStripButton undoButton, out ToolStripButton redoButton)
        {
            var editor = window.Editor;
            var interfaceOptions = editor.Options.Options.Interface;
            var inputOptions = editor.Options.Options.Input;
            var undo = surface.Undo;

            // Toolstrip
            saveButton = toolStrip.AddButton(editor.Icons.Save64, window.Save).LinkTooltip("Save", ref inputOptions.Save);
            toolStrip.AddSeparator();
            undoButton = toolStrip.AddButton(editor.Icons.Undo64, undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            redoButton = toolStrip.AddButton(editor.Icons.Redo64, undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            toolStrip.AddSeparator();
            toolStrip.AddButton(editor.Icons.Search64, editor.ContentFinding.ShowSearch).LinkTooltip("Open content search tool",  ref inputOptions.Search);
            toolStrip.AddButton(editor.Icons.CenterView64, surface.ShowWholeGraph).LinkTooltip("Show whole graph");
            var gridSnapButton = toolStrip.AddButton(editor.Icons.Grid32, surface.ToggleGridSnapping);
            gridSnapButton.LinkTooltip("Toggle grid snapping for nodes.");
            gridSnapButton.AutoCheck = true;
            gridSnapButton.Checked = surface.GridSnappingEnabled = interfaceOptions.SurfaceGridSnapping;
            surface.GridSnappingSize = interfaceOptions.SurfaceGridSnappingSize;

            // Setup input actions
            window.InputActions.Add(options => options.Undo, undo.PerformUndo);
            window.InputActions.Add(options => options.Redo, undo.PerformRedo);
            window.InputActions.Add(options => options.Search, editor.ContentFinding.ShowSearch);
        }
    }
}
