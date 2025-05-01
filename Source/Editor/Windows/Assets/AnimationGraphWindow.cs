// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Windows.Search;
using Object = FlaxEngine.Object;

// ReSharper disable UnusedMember.Local
// ReSharper disable UnusedMember.Global
// ReSharper disable MemberCanBePrivate.Local

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Animation Graph window allows to view and edit <see cref="AnimationGraph"/> asset.
    /// </summary>
    /// <seealso cref="AnimationGraph" />
    /// <seealso cref="AnimGraphSurface" />
    /// <seealso cref="AnimatedModelPreview" />
    public sealed class AnimationGraphWindow : VisjectSurfaceWindow<AnimationGraph, AnimGraphSurface, AnimatedModelPreview>, ISearchWindow
    {
        internal static Guid BaseModelId = new Guid(1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        private sealed class AnimationGraphPreview : AnimationPreview
        {
            private readonly AnimationGraphWindow _window;

            public AnimationGraphPreview(AnimationGraphWindow window)
            : base(true)
            {
                _window = window;
                ShowFloor = true;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                if (_window.Asset == null || !_window.Asset.IsLoaded)
                {
                    Render2D.DrawText(style.FontLarge, "Loading...", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                }
            }
        }

        /// <summary>
        /// The graph properties proxy object.
        /// </summary>
        private sealed class PropertiesProxy
        {
            private SkinnedModel _baseModel;

            [EditorOrder(10), EditorDisplay("General"), Tooltip("The base model used to preview the animation and prepare the graph (skeleton bones structure must match in instanced AnimationModel actors)")]
            public SkinnedModel BaseModel
            {
                get => _baseModel;
                set
                {
                    if (_baseModel != value)
                    {
                        _baseModel = value;
                        if (Window != null)
                            Window.PreviewActor.SkinnedModel = _baseModel;
                    }
                }
            }

            [EditorOrder(1000), EditorDisplay("Parameters"), CustomEditor(typeof(ParametersEditor)), NoSerialize]
            // ReSharper disable once UnusedAutoPropertyAccessor.Local
            public AnimationGraphWindow Window { get; set; }

            [HideInEditor, Serialize]
            // ReSharper disable once UnusedMember.Local
            public List<SurfaceParameter> Parameters
            {
                get => Window.Surface.Parameters;
                set => throw new Exception("No setter.");
            }

            /// <summary>
            /// Gathers parameters from the graph.
            /// </summary>
            /// <param name="window">The graph window.</param>
            public void OnLoad(AnimationGraphWindow window)
            {
                Window = window;
                var surfaceParam = window.Surface.GetParameter(BaseModelId);
                if (surfaceParam != null)
                    BaseModel = FlaxEngine.Content.LoadAsync<SkinnedModel>((Guid)surfaceParam.Value);
                else
                    BaseModel = window.PreviewActor.GetParameterValue(BaseModelId) as SkinnedModel;
            }

            /// <summary>
            /// Saves the properties to the graph.
            /// </summary>
            /// <param name="window">The graph window.</param>
            public void OnSave(AnimationGraphWindow window)
            {
                var model = window.PreviewActor;
                model.SetParameterValue(BaseModelId, BaseModel);
                var surfaceParam = window.Surface.GetParameter(BaseModelId);
                if (surfaceParam != null)
                    surfaceParam.Value = BaseModel?.ID ?? Guid.Empty;
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
            public AnimationGraphWindow Window;

            private class Editor : CustomEditor
            {
                public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    var window = (AnimationGraphWindow)Values[0];
                    var parameters = window.PreviewActor.Parameters;
                    var data = SurfaceUtils.InitGraphParameters(parameters);
                    SurfaceUtils.DisplayGraphParameters(layout, data,
                                                        (instance, parameter, tag) => ((AnimationGraphWindow)instance).PreviewActor.GetParameterValue(parameter.Identifier),
                                                        (instance, value, parameter, tag) => ((AnimationGraphWindow)instance).PreviewActor.SetParameterValue(parameter.Identifier, value),
                                                        Values);

                    // Parameters will always have one element
                    if (parameters.Length < 2)
                        layout.Label("No parameters", TextAlignment.Center);
                }
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private unsafe struct AnimGraphDebugFlowInfo
        {
            public uint NodeId;
            public int BoxId;
            public fixed uint NodePath[8];
        }

        private FlaxObjectRefPickerControl _debugPicker;
        private NavigationBar _navigationBar;
        private PropertiesProxy _properties;
        private ToolStripButton _showNodesButton;
        private Tab _previewTab;
        private readonly List<AnimGraphDebugFlowInfo> _debugFlows = new List<AnimGraphDebugFlowInfo>();

        /// <summary>
        /// Gets the animated model actor used for the animation preview.
        /// </summary>
        public AnimatedModel PreviewActor => _preview.PreviewActor;

        /// <inheritdoc />
        public AnimationGraphWindow(Editor editor, AssetItem item)
        : base(editor, item, true)
        {
            // Asset preview
            _preview = new AnimationGraphPreview(this)
            {
                ViewportCamera = new FPSCamera(),
                ScaleToFit = false,
                PlayAnimation = true,
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
            _surface = new AnimGraphSurface(this, Save, _undo)
            {
                Parent = _split1.Panel1,
                Enabled = false
            };
            _surface.ContextChanged += OnSurfaceContextChanged;

            // Toolstrip
            SurfaceUtils.PerformCommonSetup(this, _toolstrip, _surface, out _saveButton, out _undoButton, out _redoButton);
            _showNodesButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Bone64, () => _preview.ShowNodes = !_preview.ShowNodes).LinkTooltip("Show animated model nodes debug view");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/anim-graph/index.html")).LinkTooltip("See documentation to learn more");

            // Debug picker
            var debugPickerContainer = new ContainerControl();
            var debugPickerLabel = new Label
            {
                AnchorPreset = AnchorPresets.VerticalStretchLeft,
                VerticalAlignment = TextAlignment.Center,
                HorizontalAlignment = TextAlignment.Far,
                Parent = debugPickerContainer,
                Size = new Float2(50.0f, _toolstrip.Height),
                Text = "Debug:",
                TooltipText = "The current animated model actor to preview. Pick the player to debug it's playback. Leave empty to debug the model from the preview panel.",
            };
            _debugPicker = new FlaxObjectRefPickerControl
            {
                Location = new Float2(debugPickerLabel.Right + 4.0f, 8.0f),
                Width = 120.0f,
                Type = new ScriptType(typeof(AnimatedModel)),
                Parent = debugPickerContainer,
            };
            debugPickerContainer.Width = _debugPicker.Right + 2.0f;
            debugPickerContainer.Size = new Float2(_debugPicker.Right + 2.0f, _toolstrip.Height);
            debugPickerContainer.Parent = _toolstrip;
            _debugPicker.CheckValid = OnCheckValid;

            // Navigation bar
            _navigationBar = new NavigationBar
            {
                Parent = this
            };

            Animations.DebugFlow += OnDebugFlow;
        }

        private void OnSurfaceContextChanged(VisjectSurfaceContext context)
        {
            _surface.UpdateNavigationBar(_navigationBar, _toolstrip);
        }

        private bool OnCheckValid(Object obj, ScriptType type)
        {
            return obj is AnimatedModel player && player.AnimationGraph == OriginalAsset;
        }

        private unsafe void OnDebugFlow(Animations.DebugFlowInfo flowInfo)
        {
            // Filter the flow
            if (_debugPicker.Value != null)
            {
                if (flowInfo.Asset != OriginalAsset || _debugPicker.Value != flowInfo.Instance)
                    return;
            }
            else
            {
                if (flowInfo.Asset != Asset || _preview.PreviewActor != flowInfo.Instance)
                    return;
            }

            // Register flow to show it in UI on a surface
            var flow = new AnimGraphDebugFlowInfo { NodeId = flowInfo.NodeId, BoxId = (int)flowInfo.BoxId };
            Utils.MemoryCopy(new IntPtr(flow.NodePath), new IntPtr(flowInfo.NodePath0), sizeof(uint) * 8ul);
            lock (_debugFlows)
            {
                _debugFlows.Add(flow);
            }
        }

        /// <inheritdoc />
        public override IEnumerable<ScriptType> NewParameterTypes => Editor.CodeEditing.VisualScriptPropertyTypes.Get();

        /// <inheritdoc />
        public override void SetParameter(int index, object value)
        {
            try
            {
                var param = Surface.Parameters[index];
                PreviewActor.SetParameterValue(param.ID, value);
            }
            catch
            {
                // Ignored
            }

            base.SetParameter(index, value);
        }

        /// <summary>
        /// Sets the base model of the animation graph this window is editing.
        /// </summary>
        /// <param name="baseModel">The new base model.</param>
        public void SetBaseModel(SkinnedModel baseModel)
        {
            _properties.BaseModel = baseModel;
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            if (PreviewActor != null)
                PreviewActor.AnimationGraph = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            PreviewActor.AnimationGraph = _asset;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override string SurfaceName => "Anim Graph";

        /// <inheritdoc />
        public override byte[] SurfaceData
        {
            get => _asset.LoadSurface();
            set
            {
                if (value == null)
                {
                    Editor.LogError("Failed to save surface data");
                    return;
                }
                if (_asset.SaveSurface(value))
                {
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save surface data");
                    return;
                }
                _asset.Reload();
                _asset.WaitForLoaded();
                _preview.PreviewActor.ResetLocalTransform();
                _previewTab.Presenter.BuildLayoutOnUpdate();
            }
        }

        /// <inheritdoc />
        protected override bool LoadSurface()
        {
            // Load surface graph
            if (_surface.Load())
            {
                Editor.LogError("Failed to load animation graph surface.");
                return true;
            }

            // Init properties and parameters proxy
            _properties.OnLoad(this);
            _previewTab.Presenter.BuildLayoutOnUpdate();

            // Update navbar
            _surface.UpdateNavigationBar(_navigationBar, _toolstrip);

            // Show whole model
            _preview.ResetCamera();

            return false;
        }

        /// <inheritdoc />
        protected override bool SaveSurface()
        {
            // Sync edited parameters
            _properties.OnSave(this);

            // Save surface (will call SurfaceData setter)
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
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            if (_navigationBar != null && _debugPicker?.Parent != null && Parent != null)
            {
                _navigationBar.Bounds = new Rectangle
                (
                 new Float2(_debugPicker.Parent.Right + 8.0f, 0),
                 new Float2(Parent.Width - X - 8.0f, 32)
                );
            }
        }

        /// <inheritdoc />
        public override unsafe void OnUpdate()
        {
            // Auto-attach preview
            var debugActor = _debugPicker.Value as AnimatedModel;
            if (!debugActor && Editor.IsPlayMode && Editor.Options.Options.General.AutoAttachDebugPreviewActor)
            {
                var animationGraph = OriginalAsset;
                var animatedModels = Level.GetActors<AnimatedModel>();
                foreach (var animatedModel in animatedModels)
                {
                    if (animatedModel.AnimationGraph == animationGraph &&
                        animatedModel.IsActiveInHierarchy)
                    {
                        _debugPicker.Value = animatedModel;
                        break;
                    }
                }
            }

            // Extract animations playback state from the events tracing
            if (debugActor == null)
                debugActor = _preview.PreviewActor;
            if (debugActor != null)
            {
                debugActor.EnableTracing = true;
                Surface.LastTraceEvents = debugActor.TraceEvents;
            }

            base.OnUpdate();

            // Update graph execution flow debugging visualization
            lock (_debugFlows)
            {
                foreach (var debugFlow in _debugFlows)
                {
                    var context = Surface.FindContext(new Span<uint>(debugFlow.NodePath, 8));
                    var node = context?.FindNode(debugFlow.NodeId);
                    var box = node?.GetBox(debugFlow.BoxId);
                    box?.HighlightConnections();
                }
                _debugFlows.Clear();
            }

            // Update preview values when debugging specific instance
            if (debugActor != null && debugActor != _preview.PreviewActor)
            {
                var parameters = debugActor.Parameters;
                foreach (var p in parameters)
                {
                    _preview.PreviewActor.SetParameterValue(p.Identifier, p.Value);
                }
            }

            _showNodesButton.Checked = _preview.ShowNodes;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            base.OnDestroy();
            Animations.DebugFlow -= OnDebugFlow;

            _properties = null;
            _navigationBar = null;
            _debugPicker = null;
            _showNodesButton = null;
            _previewTab = null;
        }

        /// <inheritdoc />
        public SearchAssetTypes AssetType => SearchAssetTypes.AnimGraph;
    }
}
