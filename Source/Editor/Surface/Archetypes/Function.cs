// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Function group.
    /// </summary>
    [HideInEditor]
    public static class Function
    {
        /// <summary>
        /// The interface for Visject surfaces that can contain function nodes (eg. material functions).
        /// </summary>
        public interface IFunctionSurface
        {
            /// <summary>
            /// Gets the allowed types for function inputs/outputs.
            /// </summary>
            Type[] FunctionTypes { get; }
        }

        internal abstract class FunctionNode : SurfaceNode
        {
            private AssetPicker _assetPicker;
            private Box[] _inputs;
            private Box[] _outputs;
            private Asset _asset; // Keep reference to the asset to keep it loaded and handle function signature changes reload event
            private bool _isRegistered;

            public static int MaxInputs = 16;
            public static int MaxOutputs = 16;

            protected FunctionNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            protected abstract Asset LoadSignature(Guid id, out string[] types, out string[] names);

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                FlaxEngine.Content.AssetReloading += OnAssetReloading;
                FlaxEngine.Content.AssetDisposing += OnContentAssetDisposing;
                _isRegistered = true;

                _assetPicker = GetChild<AssetPicker>();
                _assetPicker.Bounds = new Rectangle(4, 32.0f, Width - 8, 48.0f);
                UpdateUI();
                _assetPicker.SelectedItemChanged += OnAssetPickerSelectedItemChanged;
            }

            private void OnContentAssetDisposing(Asset asset)
            {
                // Ensure to clear reference if need to
                if (asset == _asset)
                    _asset = null;
            }

            private void OnAssetReloading(Asset asset)
            {
                // Update when used function gets modified (signature might be modified)
                if (_asset == asset)
                {
                    UpdateUI();
                    Surface.MarkAsEdited();
                }
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateUI();
            }

            private void OnAssetPickerSelectedItemChanged()
            {
                SetValue(0, _assetPicker.SelectedID);
            }

            private void TryRestoreConnections(Box box, Box[] prevBoxes, ref NodeElementArchetype arch)
            {
                if (prevBoxes == null)
                    return;

                for (int j = 0; j < prevBoxes.Length; j++)
                {
                    var prevBox = prevBoxes[j];
                    if (prevBox != null &&
                        prevBox.HasAnyConnection &&
                        prevBox.Archetype.Text == arch.Text &&
                        box.CanUseType(prevBox.Connections[0].CurrentType))
                    {
                        box.Connections.AddRange(prevBox.Connections);
                        prevBox.Connections.Clear();
                        foreach (var connection in box.Connections)
                        {
                            connection.Connections.Remove(prevBox);
                            connection.Connections.Add(box);
                        }
                        box.ConnectionTick();
                        prevBox.ConnectionTick();
                        foreach (var connection in box.Connections)
                            connection.ConnectionTick();
                        break;
                    }
                }
            }

            private void UpdateUI()
            {
                var prevInputs = _inputs;
                var prevOutputs = _outputs;

                // Extract function signature parameters (inputs and outputs packed)
                _asset = LoadSignature(_assetPicker.SelectedID, out var typeNames, out var names);
                if (typeNames != null && names != null)
                {
                    var types = new Type[typeNames.Length];
                    for (int i = 0; i < typeNames.Length; i++)
                        types[i] = TypeUtils.GetType(typeNames[i]).Type;

                    // Count inputs and outputs
                    int inputsCount = 0;
                    for (var i = 0; i < MaxInputs; i++)
                    {
                        if (!string.IsNullOrEmpty(names[i]))
                            inputsCount++;
                    }
                    int outputsCount = 0;
                    for (var i = MaxInputs; i < MaxInputs + MaxOutputs; i++)
                    {
                        if (!string.IsNullOrEmpty(names[i]))
                            outputsCount++;
                    }

                    // Inputs
                    _inputs = new Box[inputsCount];
                    for (var i = 0; i < inputsCount; i++)
                    {
                        var arch = NodeElementArchetype.Factory.Input(i + 3, names[i], true, types[i], i);
                        var box = new InputBox(this, arch);
                        TryRestoreConnections(box, prevInputs, ref arch);
                        _inputs[i] = box;
                    }

                    // Outputs
                    _outputs = new Box[outputsCount];
                    for (var i = 0; i < outputsCount; i++)
                    {
                        var arch = NodeElementArchetype.Factory.Output(i + 3, names[i + MaxInputs], types[i + MaxInputs], i + MaxInputs);
                        var box = new OutputBox(this, arch);
                        TryRestoreConnections(box, prevOutputs, ref arch);
                        _outputs[i] = box;
                    }

                    Title = _assetPicker.SelectedItem.ShortName;
                }
                else
                {
                    _inputs = null;
                    _outputs = null;
                    Title = Archetype.Title;
                }

                // Remove previous boxes
                if (prevInputs != null)
                {
                    for (int i = 0; i < prevInputs.Length; i++)
                        RemoveElement(prevInputs[i]);
                }
                if (prevOutputs != null)
                {
                    for (int i = 0; i < prevOutputs.Length; i++)
                        RemoveElement(prevOutputs[i]);
                }

                // Add new boxes
                if (_inputs != null)
                {
                    for (int i = 0; i < _inputs.Length; i++)
                        AddElement(_inputs[i]);
                }
                if (_outputs != null)
                {
                    for (int i = 0; i < _outputs.Length; i++)
                        AddElement(_outputs[i]);
                }

                ResizeAuto();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _assetPicker = null;
                _asset = null;
                if (_isRegistered)
                {
                    _isRegistered = false;
                    FlaxEngine.Content.AssetReloading -= OnAssetReloading;
                    FlaxEngine.Content.AssetDisposing -= OnContentAssetDisposing;
                }

                base.OnDestroy();
            }
        }

        private class FunctionInputOutputNode : SurfaceNode
        {
            protected Label _nameField;

            protected ScriptType SignatureType
            {
                get
                {
                    // Convert from old GraphConnectionType
                    if (Values[0] is int asInt)
                        return new ScriptType(VisjectSurfaceContext.GetGraphConnectionType((VisjectSurfaceContext.GraphConnectionType_Deprecated)asInt));
                    if (Values[0] is long asLong)
                        return new ScriptType(VisjectSurfaceContext.GetGraphConnectionType((VisjectSurfaceContext.GraphConnectionType_Deprecated)asLong));
                    if (Values[0] is string asString)
                        return TypeUtils.GetType(asString);
                    throw new Exception(string.Format("Unknown function node signature type {0}.", Values[0] != null ? Values[0].GetType().FullName : "<null>"));
                }
            }

            /// <summary>
            /// Gets or sets the function input/output name.
            /// </summary>
            public string SignatureName
            {
                get => (string)Values[1];
                set
                {
                    if (!string.Equals(value, (string)Values[1], StringComparison.Ordinal))
                    {
                        SetValue(1, value);
                        _nameField.Text = value;
                    }
                }
            }

            /// <inheritdoc />
            public FunctionInputOutputNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _nameField = new Label
                {
                    Width = 140.0f,
                    TextColorHighlighted = Style.Current.ForegroundGrey,
                    HorizontalAlignment = TextAlignment.Near,
                    Parent = this,
                };
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                _nameField.Text = SignatureName;
            }

            /// <inheritdoc />
            public override void OnSpawned()
            {
                base.OnSpawned();

                // Ensure to have unique name
                var name = SignatureName;
                var value = name;
                int count = 1;
                while (!OnRenameValidate(null, value))
                {
                    value = name + " " + count++;
                }
                Values[1] = value;
                _nameField.Text = value;

                // Let user pick a name
                StartRenaming();
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                if (_nameField.Bounds.Contains(ref location) && Surface.CanEdit)
                {
                    StartRenaming();
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Vector2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                menu.AddButton("Rename", StartRenaming).Enabled = Surface.CanEdit;
            }

            /// <summary>
            /// Starts the function input/output parameter renaming by showing a rename popup to the user.
            /// </summary>
            private void StartRenaming()
            {
                Surface.Select(this);
                var dialog = RenamePopup.Show(this, _nameField.Bounds, SignatureName, false);
                dialog.Validate += OnRenameValidate;
                dialog.Renamed += OnRenamed;
            }

            private bool OnRenameValidate(RenamePopup popup, string value)
            {
                if (string.IsNullOrEmpty(value))
                    return false;
                return Context.Nodes.All(node =>
                {
                    if (node != this && node is FunctionInputOutputNode inputOutputNode)
                        return inputOutputNode.SignatureName != value;
                    return true;
                });
            }

            private void OnRenamed(RenamePopup renamePopup)
            {
                SignatureName = renamePopup.Text;
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _nameField.Text = SignatureName;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _nameField = null;

                base.OnDestroy();
            }
        }

        private sealed class FunctionInputNode : FunctionInputOutputNode
        {
            private Type[] _types;
            private ComboBox _typePicker;
            private Box _outputBox;
            private Box _defaultValueBox;

            /// <inheritdoc />
            public FunctionInputNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _types = ((IFunctionSurface)Surface).FunctionTypes;
                _typePicker = new ComboBox
                {
                    Location = new Vector2(4, 32),
                    Width = 80.0f,
                    Parent = this,
                };
                for (int i = 0; i < _types.Length; i++)
                    _typePicker.AddItem(Surface.GetTypeName(new ScriptType(_types[i])));
                _nameField.Location = new Vector2(_typePicker.Right + 2.0f, _typePicker.Y);
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                _outputBox = GetBox(0);
                _outputBox.CurrentType = SignatureType;
                _defaultValueBox = GetBox(1);
                _defaultValueBox.CurrentType = _outputBox.CurrentType;
                _typePicker.SelectedIndex = Array.IndexOf(_types, _outputBox.CurrentType.Type);
                _typePicker.SelectedIndexChanged += OnTypePickerSelectedIndexChanged;
            }

            private void OnTypePickerSelectedIndexChanged(ComboBox picker)
            {
                SetValue(0, _types[picker.SelectedIndex].FullName);
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _outputBox.CurrentType = SignatureType;
                _defaultValueBox.CurrentType = _outputBox.CurrentType;
                _typePicker.SelectedIndex = Array.IndexOf(_types, _outputBox.CurrentType.Type);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _types = null;
                _typePicker = null;
                _outputBox = null;
                _defaultValueBox = null;

                base.OnDestroy();
            }
        }

        private sealed class FunctionOutputNode : FunctionInputOutputNode
        {
            private Type[] _types;
            private ComboBox _typePicker;
            private Box _inputBox;

            /// <inheritdoc />
            public FunctionOutputNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _types = ((IFunctionSurface)Surface).FunctionTypes;
                _typePicker = new ComboBox
                {
                    Location = new Vector2(24, 32),
                    Width = 80.0f,
                    Parent = this,
                };
                for (int i = 0; i < _types.Length; i++)
                    _typePicker.AddItem(Surface.GetTypeName(new ScriptType(_types[i])));
                _nameField.Location = new Vector2(_typePicker.Right + 2.0f, _typePicker.Y);
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                _inputBox = GetBox(0);
                _inputBox.CurrentType = SignatureType;
                _typePicker.SelectedIndex = Array.IndexOf(_types, _inputBox.CurrentType.Type);
                _typePicker.SelectedIndexChanged += OnTypePickerSelectedIndexChanged;
            }

            private void OnTypePickerSelectedIndexChanged(ComboBox picker)
            {
                SetValue(0, _types[picker.SelectedIndex].FullName);
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _inputBox.CurrentType = SignatureType;
                _typePicker.SelectedIndex = Array.IndexOf(_types, _inputBox.CurrentType.Type);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _types = null;
                _typePicker = null;
                _inputBox = null;

                base.OnDestroy();
            }
        }

        internal sealed class MethodOverrideNode : SurfaceNode
        {
            private ScriptMemberInfo.Parameter[] _parameters;
            private bool _isTypesChangedEventRegistered;

            /// <inheritdoc />
            public MethodOverrideNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private ScriptMemberInfo GetMethod()
            {
                _parameters = null;
                var methodInfo = ScriptMemberInfo.Null;
                var surface = (VisualScriptSurface)Surface;
                if (surface.Script && !surface.Script.WaitForLoaded(100))
                {
                    var scriptMeta = surface.Script.Meta;
                    var scriptType = TypeUtils.GetType(scriptMeta.BaseTypename);
                    var members = scriptType.GetMembers((string)Values[0], MemberTypes.Method, BindingFlags.Public | BindingFlags.Instance);
                    foreach (var member in members)
                    {
                        if (SurfaceUtils.IsValidVisualScriptOverrideMethod(member, out _parameters) && _parameters.Length == (int)Values[1])
                        {
                            methodInfo = member;
                            break;
                        }
                    }
                }
                if (!_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = true;
                    Editor.Instance.CodeEditing.TypesChanged += UpdateSignature;
                }
                return methodInfo;
            }

            private void UpdateSignature()
            {
                // Find method and it's signature
                var methodInfo = GetMethod();

                // Try to restore the signature
                if (methodInfo)
                {
                    // Update tooltip
                    var attributes = methodInfo.GetAttributes(true);
                    var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                    if (tooltipAttribute != null)
                        TooltipText = tooltipAttribute.Text;

                    // Generate signature from the method info
                    MakeBox(0, string.Empty, typeof(void), true);
                    var parametersCount = _parameters.Length;
                    for (int i = 0; i < parametersCount; i++)
                    {
                        ref var param = ref _parameters[i];
                        MakeBox(i + 1, param.Name, null);
                    }

                    // Remove any not used boxes
                    for (int i = parametersCount + 1; i < 32; i++)
                    {
                        var box = GetBox(i);
                        if (box == null)
                            break;
                        RemoveElement(box);
                    }

                    // Save method signature info so it can be validated/preserved when node gets loaded by the used script method info is missing or has been modified
                    if (_parameters.Length == 0 && methodInfo.ValueType.IsVoid)
                    {
                        // Skip allocations if method is void and parameter-less
                        Values[2] = Utils.GetEmptyArray<byte>();
                    }
                    else
                    {
                        using (var stream = new MemoryStream())
                        using (var writer = new BinaryWriter(stream))
                        {
                            writer.Write((byte)1); // Version
                            writer.Write(_parameters.Length); // Parameters count
                            for (int i = 0; i < _parameters.Length; i++)
                            {
                                writer.Write(_parameters[i].Name); // Parameter name
                                Utilities.Utils.WriteVariantType(writer, TypeUtils.GetType(_parameters[i].Type)); // Box type
                            }
                            SetValue(2, stream.ToArray());
                        }
                    }
                }
                else if (Values[2] is byte[] data && data.Length != 0 && data[0] == 1)
                {
                    // Recreate node boxes based on the saved signature to preserve the connections
                    using (var stream = new MemoryStream(data))
                    using (var reader = new BinaryReader(stream))
                    {
                        reader.ReadByte(); // Version
                        int parametersCount = reader.ReadInt32(); // Parameters count
                        MakeBox(0, string.Empty, typeof(void), true);
                        for (int i = 0; i < parametersCount; i++)
                        {
                            var parameterName = reader.ReadString(); // Parameter name
                            var boxType = Utilities.Utils.ReadVariantType(reader); // Box type
                            MakeBox(i + 1, parameterName, boxType);
                        }
                    }
                }
                else
                {
                    // Parameter-less, void method
                    MakeBox(0, string.Empty, typeof(void), true);
                }

                // Update size
                Resize(Style.Current.FontLarge.MeasureText(Title).X + 30, 20 + (_parameters?.Length * 20 ?? 0));
            }

            private void MakeBox(int id, string text, Type type, bool single = false)
            {
                var box = (OutputBox)GetBox(id);
                if (box == null)
                {
                    box = new OutputBox(this, NodeElementArchetype.Factory.Output(id, text, type, id, single));
                    AddElement(box);
                }
            }

            /// <inheritdoc />
            protected override Color FooterColor => new Color(200, 11, 112);

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                Title = SurfaceUtils.GetMethodDisplayName((string)Values[0]);
                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                // Update the boxes connection types to match the signature
                // Do it after surface load so connections can receive update on type changes of the method parameter
                if (_parameters != null)
                {
                    for (int i = 0; i < _parameters.Length; i++)
                    {
                        var box = GetBox(i + 1);
                        if (box == null)
                            break;
                        box.CurrentType = _parameters[i].Type;
                    }
                    _parameters = null;
                }
            }

            /// <inheritdoc />
            public override void OnSpawned()
            {
                base.OnSpawned();

                var method = GetMethod();
                _parameters = null;
                if (method && !method.ValueType.IsVoid)
                    OnAddReturnNode();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Vector2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                var method = GetMethod();
                _parameters = null;
                menu.AddSeparator();
                menu.AddButton("Add return node", OnAddReturnNode).Enabled = method && Surface.CanEdit;
                menu.AddButton("Add base method call", OnAddBaseMethodCallNode).Enabled = method && !method.IsAbstract && Surface.CanEdit;
                // TODO: add Go to Base option to navigate to the base impl of the overriden method
            }

            private void OnAddReturnNode()
            {
                var impulseBox = GetBox(0);
                var method = GetMethod();
                _parameters = null;
                var node = Context.SpawnNode(16, 5, UpperRight + new Vector2(40, 0), new object[] { method.ValueType.TypeName });
                if (node != null && !impulseBox.HasAnyConnection)
                    impulseBox.Connect(node.GetBox(0));
            }

            private void OnAddBaseMethodCallNode()
            {
                var impulseBox = GetBox(0);
                var method = GetMethod();
                _parameters = null;
                var parametersCount = method.ParametersCount;
                var node = Context.SpawnNode(16, 4, UpperRight + new Vector2(40, 0), new object[]
                {
                    method.DeclaringType.TypeName, // Script typename
                    method.Name, // Method name
                    parametersCount, // Method parameters count
                    false, // Is Pure?
                    Utils.GetEmptyArray<byte>(), // Cached function signature data
                    // Default value for parameters (27 entries)
                    // @formatter:off
                    null,null,null,null,null,null,null,null,null,null,
                    null,null,null,null,null,null,null,null,null,null,
                    null,null,null,null,null,null,null,
                    // @formatter:on
                });
                if (node != null && !impulseBox.HasAnyConnection)
                {
                    impulseBox.Connect(node.GetBox(0));
                    for (int i = 0; i < parametersCount; i++)
                        GetBox(i + 1).Connect(node.GetBox(4 + i));
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = false;
                    Editor.Instance.CodeEditing.TypesChanged -= UpdateSignature;
                }
                _parameters = null;

                base.OnDestroy();
            }
        }

        private sealed class InvokeMethodNode : SurfaceNode
        {
            private struct SignatureParamInfo
            {
                public string Name;
                public ScriptType Type;
                public bool IsOut;
            }

            private struct SignatureInfo
            {
                public bool IsStatic;
                public ScriptType ReturnType;
                public SignatureParamInfo[] Params;
            }

            private bool _isPure;
            private bool _isTypesChangedEventRegistered;

            public InvokeMethodNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                if (nodeArch.Tag is ScriptMemberInfo memberInfo)
                {
                    Values[4] = GetSignatureData(memberInfo, memberInfo.GetParameters());
                }
            }

            private SignatureInfo LoadSignature()
            {
                var signature = new SignatureInfo();
                var data = Values[4] as byte[];
                if (data == null || data.Length == 0)
                    return signature;
                if (data[0] == 4)
                {
                    using (var stream = new MemoryStream(data))
                    using (var reader = new BinaryReader(stream))
                    {
                        reader.ReadByte(); // Version
                        signature.IsStatic = reader.ReadBoolean(); // Is Static
                        signature.ReturnType = Utilities.Utils.ReadVariantScriptType(reader); // Return type
                        var parametersCount = reader.ReadInt32(); // Parameters count
                        signature.Params = parametersCount != 0 ? new SignatureParamInfo[parametersCount] : Utils.GetEmptyArray<SignatureParamInfo>();
                        for (int i = 0; i < parametersCount; i++)
                        {
                            ref var param = ref signature.Params[i];
                            param.Name = Utilities.Utils.ReadStr(reader, 11); // Parameter name
                            param.Type = Utilities.Utils.ReadVariantScriptType(reader); // Parameter type
                            param.IsOut = reader.ReadByte() != 0; // Is parameter out
                        }
                    }
                    if (signature.Params.Length != (int)Values[2])
                        signature = new SignatureInfo();
                }
                else if (data[0] == 3)
                {
                    using (var stream = new MemoryStream(data))
                    using (var reader = new BinaryReader(stream))
                    {
                        reader.ReadByte(); // Version
                        signature.IsStatic = reader.ReadBoolean(); // Is Static
                        signature.ReturnType = Utilities.Utils.ReadVariantScriptType(reader); // Return type
                        var parametersCount = reader.ReadInt32(); // Parameters count
                        signature.Params = parametersCount != 0 ? new SignatureParamInfo[parametersCount] : Utils.GetEmptyArray<SignatureParamInfo>();
                        for (int i = 0; i < parametersCount; i++)
                        {
                            ref var param = ref signature.Params[i];
                            param.Name = reader.ReadString(); // Parameter name
                            param.Type = Utilities.Utils.ReadVariantScriptType(reader); // Parameter type
                            param.IsOut = reader.ReadByte() != 0; // Is parameter out
                        }
                    }
                    if (signature.Params.Length != (int)Values[2])
                        signature = new SignatureInfo();
                }
                return signature;
            }

            private byte[] GetSignatureData(ScriptMemberInfo methodInfo, ScriptMemberInfo.Parameter[] parameters)
            {
                using (var stream = new MemoryStream())
                using (var writer = new BinaryWriter(stream))
                {
                    writer.Write((byte)4); // Version
                    writer.Write(methodInfo.IsStatic); // Is Static
                    Utilities.Utils.WriteVariantType(writer, methodInfo.ValueType); // Return type
                    writer.Write(parameters.Length); // Parameters count
                    for (int i = 0; i < parameters.Length; i++)
                    {
                        ref var param = ref parameters[i];
                        Utilities.Utils.WriteStr(writer, param.Name, 11); // Parameter name
                        Utilities.Utils.WriteVariantType(writer, param.Type); // Parameter type
                        writer.Write((byte)(param.IsOut ? 1 : 0)); // Is parameter out
                    }
                    return stream.ToArray();
                }
            }

            private ScriptMemberInfo GetMethod(out ScriptType scriptType, out SignatureInfo signature, out ScriptMemberInfo.Parameter[] parameters)
            {
                // Load cached signature from data
                signature = LoadSignature();

                // Find method
                var methodInfo = ScriptMemberInfo.Null;
                parameters = null;
                scriptType = TypeUtils.GetType((string)Values[0]);
                if (scriptType != ScriptType.Null)
                {
                    var members = scriptType.GetMembers((string)Values[1], MemberTypes.Method, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    foreach (var member in members)
                    {
                        if (SurfaceUtils.IsValidVisualScriptInvokeMethod(member, out var memberParameters) && memberParameters.Length == (int)Values[2])
                        {
                            if (signature.Params == null)
                            {
                                // Extract signature
                                signature.Params = new SignatureParamInfo[memberParameters.Length];
                                for (int i = 0; i < signature.Params.Length; i++)
                                {
                                    ref var param = ref signature.Params[i];
                                    param.Type = memberParameters[i].Type;
                                    param.IsOut = memberParameters[i].IsOut;
                                }
                            }
                            else
                            {
                                // Check if this method has matching signature compared to the cached data
                                bool isInvalid = false;
                                for (int i = 0; i < signature.Params.Length; i++)
                                {
                                    ref var param = ref signature.Params[i];
                                    ref var paramMember = ref memberParameters[i];
                                    if (param.Type != paramMember.Type || param.IsOut != paramMember.IsOut)
                                    {
                                        // Special case: param.Type is serialized as just a type while paramMember.Type might be a reference for output parameters (eg. `out Int32` vs `out Int32&`)
                                        var paramMemberTypeName = paramMember.Type.TypeName;
                                        if (param.IsOut && param.IsOut == paramMember.IsOut && paramMember.Type.IsReference && !param.Type.IsReference &&
                                            paramMemberTypeName.Substring(0, paramMemberTypeName.Length - 1) == param.Type.TypeName)
                                            continue;

                                        isInvalid = true;
                                        break;
                                    }
                                }
                                if (isInvalid)
                                    continue;
                            }

                            // Update signature to match the method
                            signature.IsStatic = member.IsStatic;
                            signature.ReturnType = member.ValueType;
                            for (int i = 0; i < signature.Params.Length; i++)
                            {
                                ref var param = ref signature.Params[i];
                                param.Name = memberParameters[i].Name;
                            }

                            methodInfo = member;
                            parameters = memberParameters;
                            break;
                        }
                    }
                }
                if (!_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = true;
                    Editor.Instance.CodeEditing.TypesChanged += UpdateSignature;
                }
                return methodInfo;
            }

            private void UpdateSignature()
            {
                // Invoke Method node boxes:
                // 0 - call impulse
                // 1 - instance (not used for static methods)
                // 2 - returned impulse
                // 3 - returned value
                // 4... - method parameters

                var method = GetMethod(out var scriptType, out var signature, out var parameters);
                var isPure = (bool)Values[3];
                _isPure = isPure;
                if (signature.Params != null)
                {
                    if (!isPure)
                        AddBox(false, 0, 0, string.Empty, new ScriptType(typeof(void)), false);
                    if (!signature.IsStatic)
                        AddBox(false, 1, isPure ? 0 : 1, "Instance", scriptType, true);
                    if (!isPure)
                        AddBox(true, 2, 0, string.Empty, new ScriptType(typeof(void)), true);
                    var isVoid = signature.ReturnType.IsVoid;
                    if (!isVoid)
                        AddBox(true, 3, isPure ? 0 : 1, "Return", signature.ReturnType, false);
                    int inputY = (signature.IsStatic ? 0 : 1) + (isPure ? 0 : 1);
                    int outputY = (isVoid ? 0 : 1) + (isPure ? 0 : 1);
                    if (parameters != null)
                    {
                        for (int i = 0; i < parameters.Length; i++)
                        {
                            ref var param = ref parameters[i];
                            var idx = param.IsOut ? outputY : inputY;
                            var box = AddBox(param.IsOut, i + 4, idx, param.Name, param.Type, !param.IsOut, 5 + i);
                            box.Attributes = param.Attributes;
                            if (param.IsOut)
                                outputY++;
                            else
                                inputY++;
                        }
                        for (int i = parameters.Length + 1; i < 32; i++)
                        {
                            var box = GetBox(i + 4);
                            if (box == null)
                                break;
                            RemoveElement(box);
                        }
                    }
                    else
                    {
                        for (int i = 0; i < signature.Params.Length; i++)
                        {
                            ref var param = ref signature.Params[i];
                            var idx = param.IsOut ? outputY : inputY;
                            AddBox(param.IsOut, i + 4, idx, param.Name, param.Type, !param.IsOut, 5 + i);
                            if (param.IsOut)
                                outputY++;
                            else
                                inputY++;
                        }
                        for (int i = signature.Params.Length + 1; i < 32; i++)
                        {
                            var box = GetBox(i + 4);
                            if (box == null)
                                break;
                            RemoveElement(box);
                        }
                    }
                }
                if (method)
                {
                    TooltipText = SurfaceUtils.GetVisualScriptMemberInfoDescription(method);
                    SetValue(4, GetSignatureData(method, parameters));
                }

                ResizeAuto();
            }

            /// <inheritdoc />
            public override void OnSpawned()
            {
                base.OnSpawned();

                var method = GetMethod(out _, out _, out var parameters);
                if (method && parameters != null)
                {
                    // Setup default values for method parameters
                    for (int i = 0; i < parameters.Length; i++)
                    {
                        ref var param = ref parameters[i];
                        if (param.HasDefaultValue)
                        {
                            Values[i + 5] = param.DefaultValue;
                            if (GetBox(i + 4) is InputBox box)
                                box.UpdateDefaultValue();
                        }
                    }

                    // Convert into pure method if method is typical value getter
                    if (parameters.Length == 0 && !method.ValueType.IsVoid)
                    {
                        Values[3] = true;
                        OnValuesChanged();
                    }
                }
            }

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                Title = SurfaceUtils.GetMethodDisplayName((string)Values[1]);
                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                // Update the boxes connection types to match the signature
                // Do it after surface load so connections can receive update on type changes of the method parameter
                GetMethod(out _, out _, out var parameters);
                if (parameters != null)
                {
                    for (int i = 0; i < parameters.Length; i++)
                    {
                        var box = GetBox(i + 4);
                        if (box == null)
                            break;
                        box.CurrentType = parameters[i].Type;
                    }
                }
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                if (_isPure != (bool)Values[3])
                {
                    for (int i = 0; i < 4; i++)
                    {
                        var box = GetBox(i);
                        if (box != null)
                            RemoveElement(box);
                    }
                }

                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Vector2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                var method = GetMethod(out _, out _, out _);
                if (method && !method.ValueType.IsVoid)
                {
                    menu.AddSeparator();
                    menu.AddButton((bool)Values[3] ? "Convert to method call" : "Convert to pure node", () => SetValue(3, !(bool)Values[3])).Enabled = Surface.CanEdit;
                }
                // TODO: add Find References option to search Visual Script function usages in the whole project
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _headerRect.Contains(ref location))
                {
                    // Open function content item if exists
                    var method = GetMethod(out var scriptType, out _, out _);
                    var item = scriptType.ContentItem;
                    if (item != null)
                    {
                        var window = Editor.Instance.ContentEditing.Open(item);

                        // Focus method
                        if (window is VisualScriptWindow vsWindow && method)
                        {
                            foreach (var node in vsWindow.Surface.Nodes)
                            {
                                if (node is VisualScriptFunctionNode functionNode &&
                                    method.Name == functionNode._signature.Name &&
                                    ((method.ParametersCount == 0 && functionNode._signature.Parameters == null) || method.ParametersCount == functionNode._signature.Parameters.Length) &&
                                    method.IsVirtual == functionNode._signature.IsVirtual &&
                                    method.IsStatic == functionNode._signature.IsStatic &&
                                    method.ValueType == functionNode._signature.ReturnType)
                                {
                                    vsWindow.ShowNode(node);
                                    break;
                                }
                            }
                        }
                    }
                    return true;
                }

                return base.OnMouseDoubleClick(location, button);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = false;
                    Editor.Instance.CodeEditing.TypesChanged -= UpdateSignature;
                }

                base.OnDestroy();
            }
        }

        private sealed class ReturnNode : SurfaceNode
        {
            /// <inheritdoc />
            public ReturnNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private void UpdateSignature()
            {
                var valueTypeName = (string)Values[0];
                var valueType = TypeUtils.GetType(valueTypeName);
                var box = GetBox(1);
                if (valueType)
                {
                    box.Visible = !valueType.IsVoid;
                    box.CurrentType = new ScriptType(valueType.Type);
                    box.Text = string.Empty;
                }
                else
                {
                    box.Visible = valueTypeName.Length != 0;
                    box.CurrentType = ScriptType.Null;
                    box.Text = valueTypeName;
                    box.RemoveConnections();
                }
            }

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateSignature();
            }
        }

        internal sealed class VisualScriptFunctionNode : SurfaceNode
        {
            [Flags]
            private enum Flags
            {
                None = 0,
                Static = 1,
                Virtual = 2,
                Override = 4,
            }

            internal struct Parameter : ICloneable
            {
                [EditorOrder(0), Tooltip("The name of the parameter."), ExpandGroups]
                public string Name;

                [EditorOrder(1), Tooltip("The type for the parameter value.")]
                [TypeReference(typeof(object), nameof(IsTypeValid))]
                public ScriptType Type;

                private static bool IsTypeValid(ScriptType type)
                {
                    return SurfaceUtils.IsValidVisualScriptFunctionType(type) && !type.IsVoid;
                }

                /// <inheritdoc />
                public object Clone()
                {
                    return new Parameter
                    {
                        Name = Name,
                        Type = Type,
                    };
                }
            }

            internal struct Signature : ICloneable
            {
                [HideInEditor]
                public VisualScriptFunctionNode Node;

                [EditorOrder(0), EditorDisplay("Signature"), Tooltip("The name of the method.")]
                public string Name;

                [EditorOrder(1), EditorDisplay("Signature"), Tooltip("If checked, the function will be static and can be called without script instance, otherwise it's member function (can use This Instance node).")]
                public bool IsStatic;

                [EditorOrder(2), EditorDisplay("Signature"), Tooltip("If checked, the function will be virtual and could be inherited in other Visual Scripts.")]
                [VisibleIf("IsStatic", true)]
                public bool IsVirtual;

                [EditorOrder(3), EditorDisplay("Signature"), Tooltip("The value that this function returns. Use Void to not return anything.")]
                [TypeReference(typeof(object), nameof(IsTypeValid))]
                public ScriptType ReturnType;

                [EditorOrder(4), EditorDisplay("Signature"), Tooltip("The list of function parameters.")]
                public Parameter[] Parameters;

                private static bool IsTypeValid(ScriptType type)
                {
                    return SurfaceUtils.IsValidVisualScriptFunctionType(type);
                }

                /// <inheritdoc />
                public override string ToString()
                {
                    if (string.IsNullOrEmpty(Name))
                        return string.Empty;

                    var sb = new StringBuilder();

                    // Signature
                    if (IsStatic)
                        sb.Append("static ");
                    else if (IsVirtual)
                        sb.Append("virtual ");
                    sb.Append(ReturnType.Name);
                    sb.Append(' ');
                    sb.Append(Name);

                    // Parameters
                    sb.Append('(');
                    if (Parameters != null)
                    {
                        for (int i = 0; i < Parameters.Length; i++)
                        {
                            if (i != 0)
                                sb.Append(", ");
                            ref var param = ref Parameters[i];
                            if (param.Type.Type == typeof(float))
                                sb.Append("Float");
                            else if (param.Type.Type == typeof(int))
                                sb.Append("Int");
                            else if (param.Type.Type == typeof(uint))
                                sb.Append("Uint");
                            else if (param.Type.Type == typeof(bool))
                                sb.Append("Bool");
                            else
                                sb.Append(param.Type.Name);
                            sb.Append(' ');
                            sb.Append(param.Name);
                        }
                    }
                    sb.Append(')');

                    // Tooltip
                    var attributes = Node.Meta.GetAttributes();
                    var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                    if (tooltipAttribute != null)
                        sb.Append("\n").Append(tooltipAttribute.Text);

                    return sb.ToString();
                }

                /// <inheritdoc />
                public object Clone()
                {
                    return new Signature
                    {
                        Node = Node,
                        Name = (string)Name?.Clone(),
                        IsStatic = IsStatic,
                        IsVirtual = IsVirtual,
                        ReturnType = ReturnType,
                        Parameters = (Parameter[])Parameters?.Clone(),
                    };
                }
            }

            class SignatureEditor : ContextMenuBase
            {
                private CustomEditorPresenter _presenter;

                public Action<Signature> Edited;

                public SignatureEditor(ref Signature signature)
                {
                    // Context menu dimensions
                    const float width = 340.0f;
                    const float height = 370.0f;
                    Size = new Vector2(width, height);

                    // Title
                    var title = new Label(2, 2, width - 4, 23.0f)
                    {
                        Font = new FontReference(Style.Current.FontLarge),
                        Text = "Edit function signature",
                        Parent = this
                    };

                    // Buttons
                    float buttonsWidth = (width - 16.0f) * 0.5f;
                    float buttonsHeight = 20.0f;
                    var cancelButton = new Button(4.0f, title.Bottom + 4.0f, buttonsWidth, buttonsHeight)
                    {
                        Text = "Cancel",
                        Parent = this
                    };
                    cancelButton.Clicked += Hide;
                    var okButton = new Button(cancelButton.Right + 4.0f, cancelButton.Y, buttonsWidth, buttonsHeight)
                    {
                        Text = "OK",
                        Parent = this
                    };
                    okButton.Clicked += OnOkButtonClicked;

                    // Actual panel
                    var panel1 = new Panel(ScrollBars.Vertical)
                    {
                        Bounds = new Rectangle(0, okButton.Bottom + 4.0f, width, height - okButton.Bottom - 2.0f),
                        Parent = this
                    };
                    var editor = new CustomEditorPresenter(null);
                    editor.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
                    editor.Panel.IsScrollable = true;
                    editor.Panel.Parent = panel1;
                    _presenter = editor;
                    editor.Select(signature.Clone());
                }

                private void OnOkButtonClicked()
                {
                    Edited((Signature)_presenter.Selection[0]);
                    Hide();
                }

                /// <inheritdoc />
                protected override void OnShow()
                {
                    Focus();

                    base.OnShow();
                }

                /// <inheritdoc />
                public override void Hide()
                {
                    if (!Visible)
                        return;

                    Focus(null);

                    base.Hide();
                }

                /// <inheritdoc />
                public override bool OnKeyDown(KeyboardKeys key)
                {
                    if (key == KeyboardKeys.Escape)
                    {
                        Hide();
                        return true;
                    }

                    return base.OnKeyDown(key);
                }

                /// <inheritdoc />
                public override void OnDestroy()
                {
                    _presenter = null;
                    Edited = null;

                    base.OnDestroy();
                }
            }

            internal Signature _signature;

            /// <inheritdoc />
            public VisualScriptFunctionNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private void SaveSignature()
            {
                using (var stream = new MemoryStream())
                using (var writer = new BinaryWriter(stream))
                {
                    writer.Write((byte)1); // Version
                    Utilities.Utils.WriteStrAnsi(writer, _signature.Name, 71); // Name
                    var flags = Flags.None;
                    if (_signature.IsStatic)
                        flags |= Flags.Static;
                    if (_signature.IsVirtual)
                        flags |= Flags.Virtual;
                    writer.Write((byte)flags); // Flags
                    Utilities.Utils.WriteVariantType(writer, _signature.ReturnType); // Return Type
                    var parametersCount = _signature.Parameters?.Length ?? 0;
                    writer.Write(parametersCount); // Parameters count
                    for (int i = 0; i < parametersCount; i++)
                    {
                        ref var param = ref _signature.Parameters[i];
                        Utilities.Utils.WriteStrAnsi(writer, param.Name, 13); // Parameter name
                        Utilities.Utils.WriteVariantType(writer, param.Type); // Parameter type
                        writer.Write((byte)0); // Is parameter out
                        writer.Write((byte)0); // Has default value
                    }
                    SetValue(0, stream.ToArray());
                }
            }

            private void LoadSignature()
            {
                _signature = new Signature();
                _signature.Node = this;
                if (Values[0] is byte[] data && data.Length != 0)
                {
                    using (var stream = new MemoryStream(data))
                    using (var reader = new BinaryReader(stream))
                    {
                        var version = reader.ReadByte(); // Version
                        switch (version)
                        {
                        case 1:
                        {
                            _signature.Name = Utilities.Utils.ReadStrAnsi(reader, 71); // Name
                            var flags = (Flags)reader.ReadByte(); // Flags
                            _signature.IsStatic = (flags & Flags.Static) == Flags.Static;
                            _signature.IsVirtual = (flags & Flags.Virtual) == Flags.Virtual;
                            _signature.ReturnType = Utilities.Utils.ReadVariantScriptType(reader); // Return Type
                            var parametersCount = reader.ReadInt32(); // Parameters count
                            _signature.Parameters = new Parameter[parametersCount];
                            for (int i = 0; i < parametersCount; i++)
                            {
                                var paramName = Utilities.Utils.ReadStrAnsi(reader, 13); // Parameter name
                                var paramType = Utilities.Utils.ReadVariantScriptType(reader); // Parameter type
                                var isOut = reader.ReadByte() != 0; // Is parameter out
                                var hasDefaultValue = reader.ReadByte() != 0; // Has default value
                                _signature.Parameters[i] = new Parameter
                                {
                                    Name = paramName,
                                    Type = paramType,
                                };
                            }
                            break;
                        }
                        default:
                            _signature = new Signature();
                            break;
                        }
                    }
                }

                UpdateUI();
            }

            private void UpdateUI()
            {
                // Function node boxes:
                // 0 - impulse
                // 1... - parameters

                // Setup function boxes
                AddBox(true, 0, 0, string.Empty, new ScriptType(typeof(void)), true);
                var parametersCount = _signature.Parameters?.Length ?? 0;
                for (int i = 0; i < parametersCount; i++)
                {
                    ref var param = ref _signature.Parameters[i];
                    AddBox(true, i + 1, i + 1, param.Name, param.Type, false);
                }

                // Remove any not used parameter boxes
                for (int i = parametersCount; i < 32; i++)
                {
                    var box = GetBox(i + 1);
                    if (box == null)
                        break;
                    RemoveElement(box);
                }

                // Update
                Title = _signature.Name;
                TooltipText = _signature.ToString();
                ResizeAuto();
            }

            /// <inheritdoc />
            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Vector2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                menu.AddSeparator();
                menu.AddButton("Add return node", OnAddReturnNode).Enabled = _signature.ReturnType != ScriptType.Null && Surface.CanEdit;
                menu.AddButton("Edit signature...", OnEditSignature).Enabled = Surface.CanEdit;
                menu.AddButton("Edit attributes...", OnEditAttributes).Enabled = Surface.CanEdit;
                // TODO: add Find References option to search Visual Script function usages in the whole project
            }

            private void OnAddReturnNode()
            {
                var impulseBox = GetBox(0);
                var node = Context.SpawnNode(16, 5, UpperRight + new Vector2(40, 0), new object[] { _signature.ReturnType.TypeName });
                if (node != null && !impulseBox.HasAnyConnection)
                    impulseBox.Connect(node.GetBox(0));
            }

            private void OnEditSignature()
            {
                var editor = new SignatureEditor(ref _signature);
                editor.Edited += newValue =>
                {
                    // Check new signature
                    CheckFunctionName(ref newValue.Name);
                    if (newValue.ReturnType == ScriptType.Null)
                        newValue.ReturnType = new ScriptType(typeof(void));
                    if (newValue.Parameters != null)
                    {
                        for (int i = 0; i < newValue.Parameters.Length; i++)
                        {
                            ref var param = ref newValue.Parameters[i];
                            if (param.Type == ScriptType.Null || param.Type.Type == typeof(void))
                                param.Type = new ScriptType(typeof(float));
                            if (string.IsNullOrEmpty(param.Name))
                                param.Name = "Parameter " + (i + 1);
                        }
                    }

                    // Change signature
                    var prevReturnType = _signature.ReturnType;
                    _signature = newValue;
                    SaveSignature();

                    // Check if return type has been changed
                    if (_signature.ReturnType != prevReturnType)
                    {
                        // Update all return nodes used by this function to match the new type
                        var usedNodes = DepthFirstTraversal(false);
                        var hasAnyReturnNode = false;
                        foreach (var node in usedNodes)
                        {
                            if (node is ReturnNode returnNode)
                            {
                                hasAnyReturnNode = true;
                                returnNode.SetValue(0, _signature.ReturnType.TypeName);
                            }
                        }

                        // Spawn return node if none was used but method is not void
                        if (!hasAnyReturnNode && !_signature.ReturnType.IsVoid)
                        {
                            OnAddReturnNode();
                        }
                    }

                    // Update node interface
                    UpdateUI();
                };
                editor.Show(this, Vector2.Zero);
            }

            private void OnEditAttributes()
            {
                var attributes = Meta.GetAttributes();
                var editor = new AttributesEditor(attributes, NodeFactory.FunctionAttributeTypes);
                editor.Edited += newValue =>
                {
                    var action = new EditNodeAttributesAction
                    {
                        Window = (IVisjectSurfaceWindow)Surface.Owner,
                        ID = ID,
                        Before = Meta.GetAttributes(),
                        After = newValue,
                    };
                    action.Window.Undo?.AddAction(action);
                    action.Do();
                };
                editor.Show(this, Vector2.Zero);
            }

            private void CheckFunctionName(ref string name)
            {
                if (string.IsNullOrEmpty(name))
                    name = "Function";

                var originalValue = name;
                var value = originalValue;
                int count = 1;
                while (Context.Nodes.Any(node => node != this && node is VisualScriptFunctionNode other && other._signature.Name == value))
                    value = originalValue + " " + count++;
                name = value;
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _headerRect.Contains(ref location))
                {
                    OnEditSignature();
                    return true;
                }

                return base.OnMouseDoubleClick(location, button);
            }

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                LoadSignature();
            }

            /// <inheritdoc />
            public override void OnSpawned()
            {
                base.OnSpawned();

                // Setup initial signature
                CheckFunctionName(ref _signature.Name);
                if (_signature.ReturnType == ScriptType.Null)
                    _signature.ReturnType = new ScriptType(typeof(void));
                _signature.Node = this;
                SaveSignature();
                UpdateUI();

                // Start editing
                OnEditSignature();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                LoadSignature();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _signature = new Signature();

                base.OnDestroy();
            }
        }

        private sealed class GetFieldNode : SurfaceNode
        {
            private bool _isTypesChangedEventRegistered;

            /// <inheritdoc />
            public GetFieldNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private void UpdateSignature()
            {
                var fieldInfo = ScriptMemberInfo.Null;
                var scriptType = TypeUtils.GetType((string)Values[0]);
                if (scriptType != ScriptType.Null)
                {
                    var members = scriptType.GetMembers((string)Values[1], MemberTypes.Field, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    foreach (var member in members)
                    {
                        if (SurfaceUtils.IsValidVisualScriptField(member))
                        {
                            fieldInfo = member;
                            break;
                        }
                    }
                }
                if (!_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = true;
                    Editor.Instance.CodeEditing.TypesChanged += UpdateSignature;
                }
                var typeName = (string)Values[2];
                var isStatic = (bool)Values[3];
                ScriptType type;
                if (fieldInfo)
                {
                    type = fieldInfo.ValueType;
                    SetValue(2, type.TypeName);
                    SetValue(3, type.IsStatic);
                }
                else
                {
                    type = TypeUtils.GetType(typeName);
                }
                if (type)
                {
                    AddBox(true, 0, 0, string.Empty, type, false);
                    if (!isStatic)
                        AddBox(false, 1, 0, "Instance", scriptType, true);
                    else
                        RemoveElement(GetBox(1));
                }
                else
                {
                    RemoveElement(GetBox(0));
                    RemoveElement(GetBox(1));
                }
                ResizeAuto();
            }

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                Title = "Get " + SurfaceUtils.GetMethodDisplayName((string)Values[1]);
                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = false;
                    Editor.Instance.CodeEditing.TypesChanged -= UpdateSignature;
                }

                base.OnDestroy();
            }
        }

        private sealed class SetFieldNode : SurfaceNode
        {
            private bool _isTypesChangedEventRegistered;

            /// <inheritdoc />
            public SetFieldNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private void UpdateSignature()
            {
                var fieldInfo = ScriptMemberInfo.Null;
                var scriptType = TypeUtils.GetType((string)Values[0]);
                if (scriptType != ScriptType.Null)
                {
                    var members = scriptType.GetMembers((string)Values[1], MemberTypes.Field, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    foreach (var member in members)
                    {
                        if (SurfaceUtils.IsValidVisualScriptField(member))
                        {
                            fieldInfo = member;
                            break;
                        }
                    }
                }
                if (!_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = true;
                    Editor.Instance.CodeEditing.TypesChanged += UpdateSignature;
                }
                var typeName = (string)Values[2];
                var isStatic = (bool)Values[3];
                ScriptType type;
                if (fieldInfo)
                {
                    type = fieldInfo.ValueType;
                    SetValue(2, type.TypeName);
                    SetValue(3, type.IsStatic);
                }
                else
                {
                    type = TypeUtils.GetType(typeName);
                }
                AddBox(false, 2, 0, string.Empty, new ScriptType(typeof(void)), false);
                AddBox(true, 3, 0, string.Empty, new ScriptType(typeof(void)), true);
                if (type)
                {
                    AddBox(false, 0, 1, string.Empty, type, true, 4);
                    if (!isStatic)
                        AddBox(false, 1, 2, "Instance", scriptType, true);
                    else
                        RemoveElement(GetBox(1));
                }
                else
                {
                    RemoveElement(GetBox(0));
                    RemoveElement(GetBox(1));
                }
                ResizeAuto();
            }

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                Title = "Set " + SurfaceUtils.GetMethodDisplayName((string)Values[1]);
                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateSignature();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (_isTypesChangedEventRegistered)
                {
                    _isTypesChangedEventRegistered = false;
                    Editor.Instance.CodeEditing.TypesChanged -= UpdateSignature;
                }

                base.OnDestroy();
            }
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            new NodeArchetype
            {
                TypeID = 1,
                Create = (id, context, arch, groupArch) => new FunctionInputNode(id, context, arch, groupArch),
                Title = "Function Input",
                Description = "The graph function input data",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph | NodeFlags.NoSpawnViaPaste,
                Size = new Vector2(240, 60),
                DefaultValues = new object[]
                {
                    typeof(float).FullName,
                    "Input",
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                    NodeElementArchetype.Factory.Input(1.5f, "Default Value", true, typeof(float), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Create = (id, context, arch, groupArch) => new FunctionOutputNode(id, context, arch, groupArch),
                Title = "Function Output",
                Description = "The graph function output data",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph | NodeFlags.NoSpawnViaPaste,
                Size = new Vector2(240, 60),
                DefaultValues = new object[]
                {
                    typeof(float).FullName,
                    "Output",
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Create = (id, context, arch, groupArch) => new MethodOverrideNode(id, context, arch, groupArch),
                Title = string.Empty,
                Description = "Overrides the base class method with custom implementation",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.NoSpawnViaGUI | NodeFlags.NoSpawnViaPaste,
                Size = new Vector2(240, 60),
                DefaultValues = new object[]
                {
                    string.Empty, // Overriden method name
                    0, // Overriden method parameters count
                    Utils.GetEmptyArray<byte>(), // Cached function signature data
                },
            },
            new NodeArchetype
            {
                TypeID = 4,
                Create = (id, context, arch, groupArch) => new InvokeMethodNode(id, context, arch, groupArch),
                Title = string.Empty,
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.NoSpawnViaGUI,
                Size = new Vector2(240, 60),
                DefaultValues = new object[]
                {
                    string.Empty, // Script typename
                    string.Empty, // Method name
                    0, // Method parameters count
                    false, // Is Pure?
                    Utils.GetEmptyArray<byte>(), // Cached function signature data

                    // Default value for parameters (27 entries)
                    // @formatter:off
                    null,null,null,null,null,null,null,null,null,null,
                    null,null,null,null,null,null,null,null,null,null,
                    null,null,null,null,null,null,null,
                    // @formatter:on
                },
            },
            new NodeArchetype
            {
                TypeID = 5,
                Create = (id, context, arch, groupArch) => new ReturnNode(id, context, arch, groupArch),
                Title = "Return",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.NoSpawnViaGUI,
                Size = new Vector2(100, 40),
                DefaultValues = new object[]
                {
                    string.Empty, // Value TypeName
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Result", true, null, 1),
                },
            },
            new NodeArchetype
            {
                TypeID = 6,
                Create = (id, context, arch, groupArch) => new VisualScriptFunctionNode(id, context, arch, groupArch),
                Title = "New Function",
                Description = "Adds a new function to the script",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Vector2(240, 20),
                DefaultValues = new object[]
                {
                    Utils.GetEmptyArray<byte>(), // Function signature data
                },
            },
            new NodeArchetype
            {
                TypeID = 7,
                Create = (id, context, arch, groupArch) => new GetFieldNode(id, context, arch, groupArch),
                Title = string.Empty,
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.NoSpawnViaGUI,
                Size = new Vector2(240, 60),
                DefaultValues = new object[]
                {
                    string.Empty, // Script typename
                    string.Empty, // Field name
                    string.Empty, // Field typename
                    false, // Is Static
                },
            },
            new NodeArchetype
            {
                TypeID = 8,
                Create = (id, context, arch, groupArch) => new SetFieldNode(id, context, arch, groupArch),
                Title = string.Empty,
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.NoSpawnViaGUI,
                Size = new Vector2(240, 60),
                DefaultValues = new object[]
                {
                    string.Empty, // Script typename
                    string.Empty, // Field name
                    string.Empty, // Field typename
                    false, // Is Static
                    null, // Default value
                },
            },
        };
    }
}
