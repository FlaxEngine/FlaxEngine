// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="GameplayGlobals"/> asset.
    /// </summary>
    /// <seealso cref="GameplayGlobals" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class GameplayGlobalsWindow : AssetEditorWindowBase<GameplayGlobals>
    {
        private class AddRemoveParamAction : IUndoAction
        {
            public PropertiesProxy Proxy;
            public bool IsAdd;
            public string Name;
            public object DefaultValue;

            /// <inheritdoc />
            public string ActionString => IsAdd ? "Add parameter" : "Remove parameter";

            /// <inheritdoc />
            public void Do()
            {
                if (IsAdd)
                    Add();
                else
                    Remove();
            }

            /// <inheritdoc />
            public void Undo()
            {
                if (IsAdd)
                    Remove();
                else
                    Add();
            }

            private void Add()
            {
                Proxy.DefaultValues[Name] = DefaultValue;

                Proxy.Window._propertiesEditor.BuildLayoutOnUpdate();
            }

            private void Remove()
            {
                DefaultValue = Proxy.DefaultValues[Name];

                Proxy.DefaultValues.Remove(Name);

                Proxy.Window._propertiesEditor.BuildLayoutOnUpdate();
            }

            /// <inheritdoc />
            public void Dispose()
            {
                DefaultValue = null;
                Proxy = null;
            }
        }

        private class RenameParamAction : IUndoAction
        {
            public PropertiesProxy Proxy;
            public string Before;
            public string After;

            /// <inheritdoc />
            public string ActionString => "Rename parameter";

            /// <inheritdoc />
            public void Do()
            {
                Rename(Before, After);
            }

            /// <inheritdoc />
            public void Undo()
            {
                Rename(After, Before);
            }

            private void Rename(string from, string to)
            {
                var defaultValue = Proxy.DefaultValues[from];

                Proxy.DefaultValues.Remove(from);
                Proxy.DefaultValues[to] = defaultValue;

                Proxy.Window._propertiesEditor.BuildLayoutOnUpdate();
            }

            /// <inheritdoc />
            public void Dispose()
            {
                Before = null;
                After = null;
            }
        }

        [CustomEditor(typeof(PropertiesProxyEditor))]
        private sealed class PropertiesProxy
        {
            [NoSerialize]
            public GameplayGlobalsWindow Window;

            [NoSerialize]
            public GameplayGlobals Asset;

            public Dictionary<string, object> DefaultValues;

            public void Init(GameplayGlobalsWindow window)
            {
                Window = window;
                Asset = window.Asset;
                DefaultValues = Asset.DefaultValues;
            }
        }

        private sealed class VariableValueContainer : ValueContainer
        {
            private readonly PropertiesProxy _proxy;
            private readonly string _name;
            private readonly bool _isDefault;

            public VariableValueContainer(PropertiesProxy proxy, string name, object value, bool isDefault)
            : base(ScriptMemberInfo.Null, new ScriptType(value.GetType()))
            {
                _proxy = proxy;
                _name = name;
                _isDefault = isDefault;

                Add(value);
            }

            private object Getter(object instance, int index)
            {
                if (_isDefault)
                    return _proxy.DefaultValues[_name];
                return _proxy.Asset.GetValue(_name);
            }

            private void Setter(object instance, int index, object value)
            {
                if (_isDefault)
                    _proxy.DefaultValues[_name] = value;
                else
                    _proxy.Asset.SetValue(_name, value);
            }

            /// <inheritdoc />
            public override void Refresh(ValueContainer instanceValues)
            {
                if (instanceValues == null || instanceValues.Count != Count)
                    throw new ArgumentException();

                for (int i = 0; i < Count; i++)
                {
                    var v = instanceValues[i];
                    this[i] = Getter(v, i);
                }
            }

            /// <inheritdoc />
            public override void Set(ValueContainer instanceValues, object value)
            {
                if (instanceValues == null || instanceValues.Count != Count)
                    throw new ArgumentException();

                for (int i = 0; i < Count; i++)
                {
                    var v = instanceValues[i];
                    Setter(v, i, value);
                    this[i] = value;
                }
            }

            /// <inheritdoc />
            public override void Set(ValueContainer instanceValues, ValueContainer values)
            {
                /*if (instanceValues == null || instanceValues.Count != Count)
                    throw new ArgumentException();
                if (values == null || values.Count != Count)
                    throw new ArgumentException();

                for (int i = 0; i < Count; i++)
                {
                    var v = instanceValues[i];
                    var value = ((CustomValueContainer)values)[i];
                    Setter(v, i, value);
                    this[i] = value;
                }*/
            }

            /// <inheritdoc />
            public override void Set(ValueContainer instanceValues)
            {
                if (instanceValues == null || instanceValues.Count != Count)
                    throw new ArgumentException();

                for (int i = 0; i < Count; i++)
                {
                    var v = instanceValues[i];
                    Setter(v, i, Getter(v, i));
                }
            }

            /// <inheritdoc />
            public override void RefreshReferenceValue(object instanceValue)
            {
                // Not supported
            }
        }

        private sealed class PropertiesProxyEditor : CustomEditor
        {
            private PropertiesProxy _proxy;
            private ComboBox _addParamType;

            private static readonly Type[] AllowedTypes =
            {
                typeof(float),
                typeof(bool),
                typeof(int),
                typeof(Float2),
                typeof(Float3),
                typeof(Float4),
                typeof(Double2),
                typeof(Double3),
                typeof(Double4),
                typeof(Color),
                typeof(Quaternion),
                typeof(Transform),
                typeof(BoundingBox),
                typeof(BoundingSphere),
                typeof(Rectangle),
                typeof(Matrix),
                typeof(string),
            };

            public override void Initialize(LayoutElementsContainer layout)
            {
                _proxy = (PropertiesProxy)Values[0];
                if (_proxy?.DefaultValues == null)
                {
                    layout.Label("Loading...", TextAlignment.Center);
                    return;
                }

                var isPlayModeActive = _proxy.Window.Editor.StateMachine.IsPlayMode;
                if (isPlayModeActive)
                {
                    layout.Label("Play mode is active. Editing runtime values.", TextAlignment.Center);
                    layout.Space(10);

                    foreach (var e in _proxy.DefaultValues)
                    {
                        var name = e.Key;
                        var value = _proxy.Asset.GetValue(name);
                        var valueContainer = new VariableValueContainer(_proxy, name, value, false);
                        var propertyLabel = new PropertyNameLabel(name)
                        {
                            Tag = name,
                        };
                        string tooltip = null;
                        if (_proxy.DefaultValues.TryGetValue(name, out var defaultValue))
                            tooltip = "Default value: " + defaultValue;
                        layout.Object(propertyLabel, valueContainer, null, tooltip);
                    }
                }
                else
                {
                    foreach (var e in _proxy.DefaultValues)
                    {
                        var name = e.Key;
                        var value = e.Value;
                        var valueContainer = new VariableValueContainer(_proxy, name, value, true);
                        var propertyLabel = new ClickablePropertyNameLabel(name)
                        {
                            Tag = name,
                        };
                        propertyLabel.MouseLeftDoubleClick += (label, location) => StartParameterRenaming(name, label);
                        propertyLabel.SetupContextMenu += OnPropertyLabelSetupContextMenu;
                        layout.Object(propertyLabel, valueContainer, null, "Type: " + CustomEditorsUtil.GetTypeNameUI(value.GetType()));
                    }

                    // TODO: improve the UI
                    layout.Space(40);
                    var addParamType = layout.ComboBox().ComboBox;
                    object lastValue = null;
                    foreach (var e in _proxy.DefaultValues)
                        lastValue = e.Value;

                    var allowedTypes = AllowedTypes.Select(CustomEditorsUtil.GetTypeNameUI).ToList();
                    int index = 0;
                    if (lastValue != null)
                        index = allowedTypes.FindIndex(x => x.Equals(CustomEditorsUtil.GetTypeNameUI(lastValue.GetType()), StringComparison.Ordinal));

                    addParamType.Items = allowedTypes;
                    addParamType.SelectedIndex = index;
                    _addParamType = addParamType;
                    var addParamButton = layout.Button("Add").Button;
                    addParamButton.Clicked += OnAddParamButtonClicked;
                }
            }

            private void OnAddParamButtonClicked()
            {
                AddParameter(AllowedTypes[_addParamType.SelectedIndex]);
            }

            private void OnPropertyLabelSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
            {
                var name = (string)label.Tag;
                menu.AddSeparator();
                menu.AddButton("Rename", () => StartParameterRenaming(name, label));
                menu.AddButton("Delete", () => DeleteParameter(name));
            }

            private void AddParameter(Type type)
            {
                var asset = _proxy?.Asset;
                if (asset == null || asset.WaitForLoaded())
                    return;
                var action = new AddRemoveParamAction
                {
                    Proxy = _proxy,
                    IsAdd = true,
                    Name = Utilities.Utils.IncrementNameNumber("New parameter", x => OnParameterRenameValidate(null, x)),
                    DefaultValue = TypeUtils.GetDefaultValue(new ScriptType(type)),
                };
                _proxy.Window.Undo.AddAction(action);
                action.Do();
            }

            private void StartParameterRenaming(string name, Control label)
            {
                var dialog = RenamePopup.Show(label, new Rectangle(0, 0, label.Width - 2, label.Height), name, false);
                dialog.Tag = name;
                dialog.Validate += OnParameterRenameValidate;
                dialog.Renamed += OnParameterRenamed;
            }

            private bool OnParameterRenameValidate(RenamePopup popup, string value)
            {
                return !string.IsNullOrWhiteSpace(value) && !_proxy.DefaultValues.ContainsKey(value);
            }

            private void OnParameterRenamed(RenamePopup renamePopup)
            {
                var name = (string)renamePopup.Tag;
                var action = new RenameParamAction
                {
                    Proxy = _proxy,
                    Before = name,
                    After = renamePopup.Text,
                };
                _proxy.Window.Undo.AddAction(action);
                action.Do();
            }

            private void DeleteParameter(string name)
            {
                var action = new AddRemoveParamAction
                {
                    Proxy = _proxy,
                    IsAdd = false,
                    Name = name,
                };
                _proxy.Window.Undo.AddAction(action);
                action.Do();
            }
        }

        private CustomEditorPresenter _propertiesEditor;
        private PropertiesProxy _proxy;
        private ToolStripButton _saveButton;
        private ToolStripButton _undoButton;
        private ToolStripButton _redoButton;
        private ToolStripButton _resetButton;
        private Undo _undo;

        /// <summary>
        /// Gets the undo for asset editing actions.
        /// </summary>
        public Undo Undo => _undo;

        /// <inheritdoc />
        public GameplayGlobalsWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            _undo = new Undo();
            _undo.ActionDone += OnUndo;
            _undo.UndoDone += OnUndo;
            _undo.RedoDone += OnUndo;
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this,
            };
            _propertiesEditor = new CustomEditorPresenter(_undo);
            _propertiesEditor.Panel.Parent = panel;
            _propertiesEditor.Modified += OnPropertiesEditorModified;
            _proxy = new PropertiesProxy();
            _propertiesEditor.Select(_proxy);

            _saveButton = _toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _undoButton = _toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _redoButton = _toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _resetButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Rotate32, Reset).LinkTooltip("Resets the variables values to the default values");

            InputActions.Add(options => options.Save, Save);
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnPropertiesEditorModified()
        {
            if (_proxy.Window.Editor.StateMachine.IsPlayMode)
                return;

            MarkAsEdited();
        }

        private void OnUndo(IUndoAction action)
        {
            if (_proxy.Window.Editor.StateMachine.IsPlayMode)
                return;

            UpdateToolstrip();
            MarkAsEdited();
        }

        private void Reset()
        {
            _asset.ResetValues();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _undo.Clear();
            _proxy.Init(this);
            _propertiesEditor.BuildLayoutOnUpdate();
            UpdateToolstrip();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _undo.Dispose();

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;
            _undoButton.Enabled = _undo.CanUndo;
            _redoButton.Enabled = _undo.CanRedo;
            _resetButton.Enabled = _asset != null;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            base.OnPlayBegin();

            if (IsEdited)
            {
                if (MessageBox.Show("Gameplay Globals asset has been modified. Save it before entering the play mode?", "Save gameplay globals?", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    Save();
                }
            }

            ClearEditedFlag();
            _undo.Enabled = false;
            _undo.Clear();
            _propertiesEditor.BuildLayoutOnUpdate();
            UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            base.OnPlayEnd();

            _undo.Enabled = true;
            _undo.Clear();
            _propertiesEditor.BuildLayoutOnUpdate();
            UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            Asset.DefaultValues = _proxy.DefaultValues;
            if (Asset.Save())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();

            _undo = null;
            _propertiesEditor = null;
            _proxy = null;
            _saveButton = null;
            _undoButton = null;
            _redoButton = null;
            _resetButton = null;
        }
    }
}
