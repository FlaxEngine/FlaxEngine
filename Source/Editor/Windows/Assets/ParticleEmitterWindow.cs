// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.Windows.Search;

// ReSharper disable UnusedMember.Local
// ReSharper disable UnusedMember.Global
// ReSharper disable MemberCanBePrivate.Local

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Particle Emitter window allows to view and edit <see cref="ParticleEmitter"/> asset.
    /// </summary>
    /// <seealso cref="ParticleEmitter" />
    /// <seealso cref="ParticleEmitterSurface" />
    /// <seealso cref="ParticleEmitterPreview" />
    public sealed class ParticleEmitterWindow : VisjectSurfaceWindow<ParticleEmitter, ParticleEmitterSurface, ParticleEmitterPreview>, ISearchWindow
    {
        private readonly ScriptType[] _newParameterTypes =
        {
            new ScriptType(typeof(float)),
            new ScriptType(typeof(Texture)),
            new ScriptType(typeof(CubeTexture)),
            new ScriptType(typeof(GPUTexture)),
            new ScriptType(typeof(ChannelMask)),
            new ScriptType(typeof(bool)),
            new ScriptType(typeof(int)),
            new ScriptType(typeof(Float2)),
            new ScriptType(typeof(Float3)),
            new ScriptType(typeof(Float4)),
            new ScriptType(typeof(Vector2)),
            new ScriptType(typeof(Vector3)),
            new ScriptType(typeof(Vector4)),
            new ScriptType(typeof(Color)),
            new ScriptType(typeof(Quaternion)),
            new ScriptType(typeof(Transform)),
            new ScriptType(typeof(Matrix)),
        };

        /// <summary>
        /// The properties proxy object.
        /// </summary>
        private sealed class PropertiesProxy
        {
            [EditorOrder(1000), EditorDisplay("Parameters"), CustomEditor(typeof(ParametersEditor)), NoSerialize]
            // ReSharper disable once UnusedAutoPropertyAccessor.Local
            public ParticleEmitterWindow Window { get; set; }

            [HideInEditor, Serialize]
            // ReSharper disable once UnusedMember.Local
            public List<SurfaceParameter> Parameters
            {
                get => Window.Surface.Parameters;
                set => throw new Exception("No setter.");
            }

            /// <summary>
            /// Gathers parameters from the specified ParticleEmitter.
            /// </summary>
            /// <param name="particleEmitterWin">The ParticleEmitter window.</param>
            public void OnLoad(ParticleEmitterWindow particleEmitterWin)
            {
                // Link
                Window = particleEmitterWin;
            }

            /// <summary>
            /// Clears temporary data.
            /// </summary>
            public void OnClean()
            {
                // Unlink
                Window = null;
            }
        }

        /// <summary>
        /// The graph parameters preview proxy object.
        /// </summary>
        private sealed class PreviewProxy
        {
            [EditorDisplay("Parameters"), CustomEditor(typeof(Editor)), NoSerialize]
            // ReSharper disable once UnusedAutoPropertyAccessor.Local
            public ParticleEmitterWindow Window;

            private class Editor : CustomEditor
            {
                public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    var window = (ParticleEmitterWindow)Values[0];
                    var parameters = window.Preview.PreviewActor.Parameters;
                    var data = SurfaceUtils.InitGraphParameters(parameters);
                    SurfaceUtils.DisplayGraphParameters(layout, data,
                                                        (instance, parameter, tag) => ((ParticleEmitterWindow)instance).Preview.PreviewActor.GetParameterValue(string.Empty, parameter.Name),
                                                        (instance, value, parameter, tag) => ((ParticleEmitterWindow)instance).Preview.PreviewActor.SetParameterValue(string.Empty, parameter.Name, value),
                                                        Values);

                    if (!parameters.Any())
                        layout.Label("No parameters", TextAlignment.Center);
                }
            }
        }

        private readonly PropertiesProxy _properties;
        private Tab _previewTab;
        private ToolStripButton _showSourceCodeButton;

        /// <inheritdoc />
        public ParticleEmitterWindow(Editor editor, AssetItem item)
        : base(editor, item, true)
        {
            // Asset preview
            _preview = new ParticleEmitterPreview(true)
            {
                PlaySimulation = true,
                Parent = _split2.Panel1
            };

            // Asset properties proxy
            _properties = new PropertiesProxy();

            // Preview properties editor
            _previewTab = new Tab("Preview");
            _previewTab.Presenter.Select(new PreviewProxy
            {
                Window = this,
            });
            _tabs.AddTab(_previewTab);

            // Surface
            _surface = new ParticleEmitterSurface(this, Save, _undo)
            {
                Parent = _split1.Panel1,
                Enabled = false
            };

            // Toolstrip
            SurfaceUtils.PerformCommonSetup(this, _toolstrip, _surface, out _saveButton, out _undoButton, out _redoButton);
            _showSourceCodeButton = _toolstrip.AddButton(editor.Icons.Code64, ShowSourceCode);
            _showSourceCodeButton.LinkTooltip("Show generated shader source code");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/particles/index.html")).LinkTooltip("See documentation to learn more");
        }

        private void ShowSourceCode()
        {
            var source = Editor.GetShaderSourceCode(_asset);
            Utilities.Utils.ShowSourceCodeWindow(source, "Particle Emitter GPU Simulation Source", RootWindow.Window);
        }

        /// <inheritdoc />
        public override void OnParamRenameUndo()
        {
            base.OnParamRenameUndo();

            _refreshPropertiesOnLoad = true;
        }

        /// <inheritdoc />
        public override void OnParamAddUndo()
        {
            base.OnParamAddUndo();

            _refreshPropertiesOnLoad = true;
        }

        /// <inheritdoc />
        public override void OnParamRemoveUndo()
        {
            base.OnParamRemoveUndo();

            _refreshPropertiesOnLoad = true;
        }

        /// <inheritdoc />
        public override IEnumerable<ScriptType> NewParameterTypes => _newParameterTypes;

        /// <inheritdoc />
        public override void SetParameter(int index, object value)
        {
            try
            {
                Preview.PreviewActor.Parameters[index].Value = value;
            }
            catch
            {
                // Ignored
            }

            base.SetParameter(index, value);
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            _preview.Emitter = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Emitter = _asset;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override string SurfaceName => "Particle Emitter";

        /// <inheritdoc />
        public override byte[] SurfaceData
        {
            get => _asset.LoadSurface(true);
            set
            {
                if (_asset.SaveSurface(value))
                {
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save surface data");
                }
                _asset.Reload();
                _asset.WaitForLoaded();
                _preview.PreviewActor.ResetSimulation();
                _previewTab.Presenter.BuildLayoutOnUpdate();
            }
        }

        /// <inheritdoc />
        protected override bool LoadSurface()
        {
            // Load surface graph
            if (_surface.Load())
            {
                Editor.LogError("Failed to load Particle Emitter surface.");
                return true;
            }

            // Init asset properties and parameters proxy
            _properties.OnLoad(this);
            _previewTab.Presenter.BuildLayoutOnUpdate();

            return false;
        }

        /// <inheritdoc />
        protected override bool SaveSurface()
        {
            _surface.Save();
            return false;
        }

        /// <inheritdoc />
        protected override void OnSurfaceEditingStart()
        {
            _propertiesEditor.Select(_properties);

            base.OnSurfaceEditingStart();
        }

        /// <inheritdoc />
        protected override bool CanEditSurfaceOnAssetLoadError => true;

        /// <inheritdoc />
        protected override bool SaveToOriginal()
        {
            // Copy shader cache from the temporary Particle Emitter (will skip compilation on Reload - faster)
            Guid dstId = _item.ID;
            Guid srcId = _asset.ID;
            Editor.Internal_CopyCache(ref dstId, ref srcId);

            return base.SaveToOriginal();
        }

        /// <inheritdoc />
        public SearchAssetTypes AssetType => SearchAssetTypes.ParticleEmitter;

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_asset == null)
                return;
            _showSourceCodeButton.Enabled = _asset.HasShaderCode;
        }
    }
}
