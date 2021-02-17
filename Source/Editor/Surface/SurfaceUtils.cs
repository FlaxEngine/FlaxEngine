// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.Scripting;
using FlaxEditor.Utilities;
using FlaxEngine;

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
                return string.Compare(x.DisplayName, y.DisplayName, StringComparison.InvariantCulture);
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
            default: throw new ArgumentOutOfRangeException(nameof(type), type, null);
            }
        }

        internal static GraphParameterData[] InitGraphParameters(IEnumerable<MaterialParameter> parameters, Material material)
        {
            int count = parameters.Count();
            var data = new GraphParameterData[count];
            int i = 0;

            // Load material surface parameters meta to use it for material instance parameters editing
            SurfaceParameter[] surfaceParameters = null;
            try
            {
                Profiler.BeginEvent("Init Material Parameters UI Data");

                if (material != null && !material.WaitForLoaded())
                {
                    var surfaceData = material.LoadSurface(false);
                    if (surfaceData != null && surfaceData.Length > 0)
                    {
                        using (var memoryStream = new MemoryStream(surfaceData))
                        using (var stream = new BinaryReader(memoryStream))
                        {
                            // IMPORTANT! This must match C++ Graph format

                            // Magic Code
                            int tmp = stream.ReadInt32();
                            if (tmp != 1963542358)
                            {
                                // Error
                                throw new Exception("Invalid Graph format version");
                            }

                            // Version
                            var version = stream.ReadUInt32();
                            var guidBytes = new byte[16];
                            if (version < 7000)
                            {
                                // Time saved (not used anymore to prevent binary diffs after saving unmodified surface)
                                stream.ReadInt64();

                                // Nodes count
                                int nodesCount = stream.ReadInt32();

                                // Parameters count
                                int parametersCount = stream.ReadInt32();

                                // For each node
                                for (int j = 0; j < nodesCount; j++)
                                {
                                    // ID
                                    stream.ReadUInt32();

                                    // Type
                                    stream.ReadUInt16();
                                    stream.ReadUInt16();
                                }

                                // For each param
                                surfaceParameters = new SurfaceParameter[parametersCount];
                                for (int j = 0; j < parametersCount; j++)
                                {
                                    // Create param
                                    var param = new SurfaceParameter();
                                    surfaceParameters[j] = param;

                                    // Properties
                                    param.Type = new ScriptType(VisjectSurfaceContext.GetGraphParameterValueType((VisjectSurfaceContext.GraphParamType_Deprecated)stream.ReadByte()));
                                    stream.Read(guidBytes, 0, 16);
                                    param.ID = new Guid(guidBytes);
                                    param.Name = stream.ReadStr(97);
                                    param.IsPublic = stream.ReadByte() != 0;
                                    var isStatic = stream.ReadByte() != 0;
                                    var isUIVisible = stream.ReadByte() != 0;
                                    var isUIEditable = stream.ReadByte() != 0;

                                    // References [Deprecated]
                                    int refsCount = stream.ReadInt32();
                                    for (int k = 0; k < refsCount; k++)
                                        stream.ReadUInt32();

                                    // Value
                                    stream.ReadCommonValue(ref param.Value);

                                    // Meta
                                    param.Meta.Load(stream);
                                }
                            }
                            else if (version == 7000)
                            {
                                // Nodes count
                                int nodesCount = stream.ReadInt32();

                                // Parameters count
                                int parametersCount = stream.ReadInt32();

                                // For each node
                                for (int j = 0; j < nodesCount; j++)
                                {
                                    // ID
                                    stream.ReadUInt32();

                                    // Type
                                    stream.ReadUInt16();
                                    stream.ReadUInt16();
                                }

                                // For each param
                                surfaceParameters = new SurfaceParameter[parametersCount];
                                for (int j = 0; j < parametersCount; j++)
                                {
                                    // Create param
                                    var param = new SurfaceParameter();
                                    surfaceParameters[j] = param;

                                    // Properties
                                    param.Type = stream.ReadVariantScriptType();
                                    stream.Read(guidBytes, 0, 16);
                                    param.ID = new Guid(guidBytes);
                                    param.Name = stream.ReadStr(97);
                                    param.IsPublic = stream.ReadByte() != 0;

                                    // Value
                                    param.Value = stream.ReadVariant();

                                    // Meta
                                    param.Meta.Load(stream);
                                }
                            }
                        }
                    }
                }
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
                var surfaceParameter = surfaceParameters?.FirstOrDefault(x => x.ID == parameter.ParameterID);
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
            GroupElement lastGroup = null;
            for (int i = 0; i < data.Length; i++)
            {
                ref var e = ref data[i];
                if (!e.IsPublic)
                    continue;
                var tag = e.Tag;
                var parameter = e.Parameter;
                LayoutElementsContainer itemLayout;

                // Editor Display
                if (e.Display?.Group != null)
                {
                    if (lastGroup == null || lastGroup.Panel.HeaderText != e.Display.Group)
                    {
                        lastGroup = layout.Group(e.Display.Group);
                        lastGroup.Panel.Open(false);
                    }
                    itemLayout = lastGroup;
                }
                else
                {
                    lastGroup = null;
                    itemLayout = layout;
                }

                // Space
                if (e.Space != null)
                    itemLayout.Space(e.Space.Height);

                // Header
                if (e.Header != null)
                    itemLayout.Header(e.Header.Text);

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
            if (scriptType.IsGenericType || !scriptType.IsPublic || scriptType.HasAttribute(typeof(HideInEditorAttribute), true))
                return false;
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

        internal static string GetVisualScriptMemberInfoDescription(ScriptMemberInfo member)
        {
            var name = member.Name;
            var declaringType = member.DeclaringType;
            var valueType = member.ValueType;

            var sb = new StringBuilder();
            if (member.IsStatic)
                sb.Append("static ");
            else if (member.IsVirtual)
                sb.Append("virtual ");
            sb.Append(valueType.Name);
            sb.Append(' ');
            sb.Append(declaringType.Name);
            sb.Append('.');
            sb.Append(name);
            if (member.IsMethod)
            {
                // Getter/setter method of the property
                if (name.StartsWith("get_") || name.StartsWith("set_"))
                {
                    var flags = member.IsStatic ? BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly : BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly;
                    var property = declaringType.GetMembers(name.Substring(4), MemberTypes.Property, flags);
                    if (property != null && property.Length != 0)
                    {
                        return GetVisualScriptMemberInfoDescription(property[0]);
                    }
                }
                // Method
                else
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

            // Tooltip
            var attributes = member.GetAttributes();
            var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltipAttribute != null)
                sb.Append("\n").Append(tooltipAttribute.Text);

            return sb.ToString();
        }
    }
}
