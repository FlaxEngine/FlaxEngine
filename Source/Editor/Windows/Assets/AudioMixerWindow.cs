// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using FlaxEditor.Content;
using FlaxEditor.Content.Settings;
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
    /// Editor window to view/modify <see cref="AudioMixer"/> asset.
    /// </summary>
    /// <seealso cref="AudioMixer" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class AudioMixerWindow : AssetEditorWindowBase<AudioMixer>
    {
        private class AddRemoveParamAction : IUndoAction
        {
            public PropertiesProxy Proxy;
            public bool IsAdd;
            public string Name;
            public float DefaultValue;

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

                // Proxy.Window._propertiesEditor.BuildLayoutOnUpdate();
            }

            private void Remove()
            {
                DefaultValue = Proxy.DefaultValues[Name];

                Proxy.DefaultValues.Remove(Name);

                // Proxy.Window._propertiesEditor.BuildLayoutOnUpdate();
            }

            /// <inheritdoc />
            public void Dispose()
            {
                DefaultValue = 0.0f;
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

                // Proxy.Window._propertiesEditor.BuildLayoutOnUpdate();
            }

            /// <inheritdoc />
            public void Dispose()
            {
                Before = null;
                After = null;
            }
        }

        // [CustomEditor(typeof(PropertiesProxyEditor))]
        private sealed class PropertiesProxy
        {
            [NoSerialize]
            public AudioMixerWindow Window;

            [NoSerialize]
            public AudioMixer Asset;

            public Dictionary<string, float> DefaultValues;

            public void Init(AudioMixerWindow window)
            {
                Window = window;
                Asset = window.Asset;
                var groups = GameSettings.Load<AudioSettings>();
                if (groups?.AudioMixerGroups is not null) Asset.MixerInit(groups.AudioMixerGroups);
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

            private float Getter(object instance, int index)
            {
                if (_isDefault)
                    return _proxy.DefaultValues[_name];
                return _proxy.Asset.GetMixerVolumeValue(_name);
            }

            private void Setter(object instance, int index, float value)
            {
                if (_isDefault)
                    _proxy.DefaultValues[_name] = value;
                else
                    _proxy.Asset.SetMixerVolumeValue(_name, value);
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
                    Setter(v, i, (float)value);
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

            public override void Initialize(LayoutElementsContainer layout)
            {
                _proxy = (PropertiesProxy)Values[0];

                if (_proxy?.DefaultValues is null)
                {
                    layout.Label("Loading...", TextAlignment.Center);
                    return;
                }

                var isPlayModeActive = _proxy.Window.Editor.StateMachine.IsPlayMode;

                if (isPlayModeActive)
                {
                    layout.Label("Play mode is active. Editing runtime values.", TextAlignment.Center);
                    layout.Space(10);

                    foreach(var e in _proxy.DefaultValues)
                    {
                        var name = e.Key;
                        var value = _proxy.Asset.GetMixerVolumeValue(name);
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

                    return;
                }

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
                    layout.Object(propertyLabel,valueContainer, null, "Type: " + CustomEditorsUtil.GetTypeNameUI(value.GetType()));
                }

            }

            private void OnPropertyLabelSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
            {
                var name = (string)label.Tag;
                menu.AddSeparator();
                menu.AddButton("Rename", () => StartParameterRenaming(name, label));
                menu.AddButton("Delete", () => DeleteParameter(name));
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
                // _proxy.Window.Undo.AddAction(action);
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
                // _proxy.Window.Undo.AddAction(action);
                action.Do();
            }
        }

        public AudioMixerWindow(Editor editor, AssetItem item) : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;
        }
    }
}
