// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

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
    public sealed class AnimationGraphWindow : VisjectSurfaceWindow<AnimationGraph, AnimGraphSurface, AnimatedModelPreview>
    {
        internal static Guid BaseModelId = new Guid(1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        private readonly ScriptType[] _newParameterTypes =
        {
            new ScriptType(typeof(float)),
            new ScriptType(typeof(bool)),
            new ScriptType(typeof(int)),
            new ScriptType(typeof(string)),
            new ScriptType(typeof(Vector2)),
            new ScriptType(typeof(Vector3)),
            new ScriptType(typeof(Vector4)),
            new ScriptType(typeof(Color)),
            new ScriptType(typeof(Quaternion)),
            new ScriptType(typeof(Transform)),
            new ScriptType(typeof(Matrix)),
        };

        private sealed class AnimationGraphPreview : AnimatedModelPreview
        {
            private readonly AnimationGraphWindow _window;
            private ContextMenuButton _showFloorButton;
            private StaticModel _floorModel;

            public AnimationGraphPreview(AnimationGraphWindow window)
            : base(true)
            {
                _window = window;

                // Show floor widget
                _showFloorButton = ViewWidgetShowMenu.AddButton("Floor", OnShowFloorModelClicked);
                _showFloorButton.Icon = Style.Current.CheckBoxTick;
                _showFloorButton.IndexInParent = 1;

                // Floor model
                _floorModel = new StaticModel
                {
                    Position = new Vector3(0, -25, 0),
                    Scale = new Vector3(5, 0.5f, 5),
                    Model = FlaxEngine.Content.LoadAsync<Model>(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Primitives/Cube.flax"))
                };
                Task.AddCustomActor(_floorModel);

                // Enable shadows
                PreviewLight.ShadowsMode = ShadowsCastingMode.All;
                PreviewLight.CascadeCount = 2;
                PreviewLight.ShadowsDistance = 1000.0f;
                Task.ViewFlags |= ViewFlags.Shadows;
            }

            private void OnShowFloorModelClicked(ContextMenuButton obj)
            {
                _floorModel.IsActive = !_floorModel.IsActive;
                _showFloorButton.Icon = _floorModel.IsActive ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                if (_window.Asset == null || !_window.Asset.IsLoaded)
                {
                    Render2D.DrawText(style.FontLarge, "Loading...", new Rectangle(Vector2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                }
                if (_window._properties.BaseModel == null)
                {
                    Render2D.DrawText(style.FontLarge, "Missing Base Model", new Rectangle(Vector2.Zero, Size), Color.Red, TextAlignment.Center, TextAlignment.Center, TextWrapping.WrapWords);
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                FlaxEngine.Object.Destroy(ref _floorModel);
                _showFloorButton = null;

                base.OnDestroy();
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
                }
            }
        }

        private FlaxObjectRefPickerControl _debugPicker;
        private NavigationBar _navigationBar;
        private PropertiesProxy _properties;
        private Tab _previewTab;
        private readonly List<Editor.AnimGraphDebugFlowInfo> _debugFlows = new List<Editor.AnimGraphDebugFlowInfo>();

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
            _propertiesEditor.Select(_properties);

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
            _toolstrip.AddButton(editor.Icons.Bone32, () => _preview.ShowNodes = !_preview.ShowNodes).SetAutoCheck(true).LinkTooltip("Show animated model nodes debug view");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs32, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/anim-graph/index.html")).LinkTooltip("See documentation to learn more");

            // Debug picker
            var debugPickerContainer = new ContainerControl();
            var debugPickerLabel = new Label
            {
                AnchorPreset = AnchorPresets.VerticalStretchLeft,
                VerticalAlignment = TextAlignment.Center,
                HorizontalAlignment = TextAlignment.Far,
                Parent = debugPickerContainer,
                Size = new Vector2(50.0f, _toolstrip.Height),
                Text = "Debug:",
                TooltipText = "The current animated model actor to preview. Pick the player to debug it's playback. Leave empty to debug the model from the preview panel.",
            };
            _debugPicker = new FlaxObjectRefPickerControl
            {
                Location = new Vector2(debugPickerLabel.Right + 4.0f, 8.0f),
                Width = 120.0f,
                Type = new ScriptType(typeof(AnimatedModel)),
                Parent = debugPickerContainer,
            };
            debugPickerContainer.Width = _debugPicker.Right + 2.0f;
            debugPickerContainer.Size = new Vector2(_debugPicker.Right + 2.0f, _toolstrip.Height);
            debugPickerContainer.Parent = _toolstrip;
            _debugPicker.CheckValid = OnCheckValid;

            // Navigation bar
            _navigationBar = new NavigationBar
            {
                Parent = this
            };

            Editor.AnimGraphDebugFlow += OnDebugFlow;
        }

        private void OnSurfaceContextChanged(VisjectSurfaceContext context)
        {
            _surface.UpdateNavigationBar(_navigationBar, _toolstrip);
        }

        private bool OnCheckValid(FlaxEngine.Object obj, ScriptType type)
        {
            return obj is AnimatedModel player && player.AnimationGraph == OriginalAsset;
        }

        private void OnDebugFlow(Editor.AnimGraphDebugFlowInfo flowInfo)
        {
            // Filter the flow
            if (_debugPicker.Value != null)
            {
                if (flowInfo.Asset != OriginalAsset || _debugPicker.Value != flowInfo.Object)
                    return;
            }
            else
            {
                if (flowInfo.Asset != Asset || _preview.PreviewActor != flowInfo.Object)
                    return;
            }

            // Register flow to show it in UI on a surface
            lock (_debugFlows)
            {
                _debugFlows.Add(flowInfo);
            }
        }

        /// <inheritdoc />
        public override IEnumerable<ScriptType> NewParameterTypes => _newParameterTypes;

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
                    // Error
                    Editor.LogError("Failed to save animation graph surface");
                    return;
                }

                // Save data to the temporary asset
                if (_asset.SaveSurface(value))
                {
                    // Error
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save animation graph surface data");
                    return;
                }
                _asset.Reload();

                // Reset any root motion
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
                // Error
                Editor.LogError("Failed to load animation graph surface.");
                return true;
            }

            // Init properties and parameters proxy
            _properties.OnLoad(this);
            _previewTab.Presenter.BuildLayoutOnUpdate();

            // Update navbar
            _surface.UpdateNavigationBar(_navigationBar, _toolstrip);

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
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            if (_navigationBar != null && _debugPicker?.Parent != null && Parent != null)
            {
                _navigationBar.Bounds = new Rectangle
                (
                 new Vector2(_debugPicker.Parent.Right + 8.0f, 0),
                 new Vector2(Parent.Width - X - 8.0f, 32)
                );
            }
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            base.OnUpdate();

            // Update graph execution flow debugging visualization
            lock (_debugFlows)
            {
                foreach (var debugFlow in _debugFlows)
                {
                    var node = Surface.Context.FindNode(debugFlow.NodeId);
                    var box = node?.GetBox(debugFlow.BoxId);
                    box?.HighlightConnections();
                }
                _debugFlows.Clear();
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Editor.AnimGraphDebugFlow -= OnDebugFlow;

            _properties = null;
            _navigationBar = null;
            _debugPicker = null;

            base.OnDestroy();
        }
    }
}
