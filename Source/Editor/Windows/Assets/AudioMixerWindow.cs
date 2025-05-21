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

        }

        public AudioMixerWindow(Editor editor, AssetItem item) : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;
        }
    }
}
