// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Main editor window used to modify scene objects. Provides Gizmos and camera viewport navigation.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.SceneEditorWindow" />
    public sealed class EditGameWindow : SceneEditorWindow
    {
        /// <summary>
        /// Camera preview output control.
        /// </summary>
        public class CameraPreview : RenderOutputControl
        {
            private bool _isPinned;
            private Button _pinButton;

            /// <summary>
            /// Gets or sets a value indicating whether this preview is pinned.
            /// </summary>
            public bool IsPinned
            {
                get => _isPinned;
                set
                {
                    if (_isPinned != value)
                    {
                        _isPinned = value;
                        UpdatePinButton();
                    }
                }
            }

            /// <summary>
            /// Gets or sets the camera.
            /// </summary>
            public Camera Camera
            {
                get => Task.Camera;
                set => Task.Camera = value;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="CameraPreview"/> class.
            /// </summary>
            public CameraPreview()
            : base(FlaxEngine.Object.New<SceneRenderTask>())
            {
                // Don't steal focus
                AutoFocus = false;

                const float PinSize = 12.0f;
                const float PinMargin = 2.0f;

                _pinButton = new Button
                {
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = this,
                    Bounds = new Rectangle(Width - PinSize - PinMargin, PinMargin, PinSize, PinSize),
                };
                _pinButton.Clicked += () => IsPinned = !IsPinned;
                UpdatePinButton();

                Task.Begin += OnTaskBegin;
            }

            private void OnTaskBegin(RenderTask task, GPUContext context)
            {
                var sceneTask = (SceneRenderTask)task;

                // Copy camera view parameters for the scene rendering
                var view = sceneTask.View;
                var viewport = new FlaxEngine.Viewport(Float2.Zero, sceneTask.Buffers.Size);
                view.CopyFrom(Camera, ref viewport);
                sceneTask.View = view;
            }

            private void UpdatePinButton()
            {
                if (_isPinned)
                {
                    _pinButton.Text = "-";
                    _pinButton.TooltipText = "Unpin preview";
                }
                else
                {
                    _pinButton.Text = "+";
                    _pinButton.TooltipText = "Pin preview";
                }
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Draw frame
                Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Color.Black);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                IsPinned = false;
                Camera = null;
                _pinButton = null;

                base.OnDestroy();
            }
        }

        private readonly List<CameraPreview> _previews = new List<CameraPreview>();
        private Actor _pilotActor;
        private BoundingBox _pilotBounds;
        private Transform _pilotStart;
        private ViewportWidgetButton _pilotWidget;

        /// <summary>
        /// The viewport control.
        /// </summary>
        public new readonly MainEditorGizmoViewport Viewport;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditGameWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public EditGameWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Editor";
            Icon = editor.Icons.Grid32;

            // Create viewport
            Viewport = new MainEditorGizmoViewport(editor)
            {
                Parent = this,
                NearPlane = 8.0f,
                FarPlane = 20000.0f
            };
            Viewport.Task.ViewFlags = ViewFlags.DefaultEditor;

            Editor.SceneEditing.SelectionChanged += OnSelectionChanged;
            Editor.Scene.ActorRemoved += SceneOnActorRemoved;
        }

        /// <inheritdoc />
        public override void OnEditorStateChanged()
        {
            base.OnEditorStateChanged();

            UpdateCameraPreview();
        }

        private void OnSelectionChanged()
        {
            UpdateCameraPreview();
        }

        /// <summary>
        /// Gets a value indicating whether actor pilot feature is active and in use.
        /// </summary>
        public bool IsPilotActorActive => _pilotActor;

        /// <summary>
        /// Gets the current actor that is during pilot action.
        /// </summary>
        public Actor ActorToPilot => _pilotActor;

        /// <summary>
        /// Moves viewport to the actor and attaches actor to the viewport to pilot it over the scene.
        /// </summary>
        /// <param name="actor">The actor to pilot.</param>
        public void PilotActor(Actor actor)
        {
            if (_pilotActor)
                EndPilot();

            _pilotActor = actor;
            _pilotStart = actor.Transform;
            _pilotBounds = actor.BoxWithChildren;
            Viewport.ViewTransform = _pilotStart;
            if (_pilotWidget == null)
            {
                var container = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft)
                {
                    Parent = Viewport,
                };
                _pilotWidget = new ViewportWidgetButton(string.Empty, SpriteHandle.Invalid)
                {
                    Parent = container,
                };
                _pilotWidget.Clicked += button => EndPilot();
                container.UnlockChildrenRecursive();
            }
            else
            {
                _pilotWidget.Parent.Visible = true;
            }
            _pilotWidget.Text = string.Format("Piloting actor: {0} (click to stop)", actor.Name);
            _pilotWidget.PerformLayout();
            _pilotWidget.Parent.PerformLayout();
        }

        /// <summary>
        /// Ends the actor piloting mode.
        /// </summary>
        public void EndPilot()
        {
            if (_pilotActor == null)
                return;

            if (Editor.Undo.Enabled)
            {
                ActorNode node = Editor.Scene.GetActorNode(_pilotActor);
                bool navigationDirty = node.AffectsNavigationWithChildren;
                var action = new TransformObjectsAction
                (
                 new List<SceneGraphNode> { node },
                 new List<Transform> { _pilotStart },
                 ref _pilotBounds,
                 navigationDirty
                );
                Editor.Undo.AddAction(action);
            }
            _pilotActor = null;
            _pilotWidget.Parent.Visible = false;
        }

        private void SceneOnActorRemoved(ActorNode actorNode)
        {
            if (actorNode is CameraNode cameraNode)
            {
                // Remove previews using this camera
                HideCameraPreview((Camera)cameraNode.Actor);
            }
            if (actorNode.Actor == _pilotActor)
            {
                EndPilot();
            }
        }

        /// <summary>
        /// Updates the camera previews.
        /// </summary>
        private void UpdateCameraPreview()
        {
            // Disable rendering preview during GI baking
            if (Editor.StateMachine.CurrentState.IsPerformanceHeavy)
            {
                HideAllCameraPreviews();
                return;
            }

            var selection = Editor.SceneEditing.Selection;

            // Hide unpinned previews for which camera being previews is not selected
            for (int i = 0; i < _previews.Count; i++)
            {
                if (_previews[i].IsPinned)
                    continue;
                var camera = _previews[i].Camera;
                var cameraNode = Editor.Scene.GetActorNode(camera);
                if (cameraNode == null || !selection.Contains(cameraNode))
                {
                    // Hide it
                    HideCameraPreview(_previews[i--]);
                }
            }

            if (Editor.Options.Options.Interface.ShowSelectedCameraPreview)
            {
                // Find any selected cameras and create previews for them
                for (int i = 0; i < selection.Count; i++)
                {
                    if (selection[i] is CameraNode cameraNode)
                    {
                        // Check limit for cameras
                        if (_previews.Count >= 8)
                            break;

                        var camera = (Camera)cameraNode.Actor;
                        var preview = _previews.FirstOrDefault(x => x.Camera == camera);
                        if (preview == null)
                        {
                            // Show it
                            preview = new CameraPreview
                            {
                                Camera = camera,
                                Parent = this
                            };
                            _previews.Add(preview);
                        }
                    }
                }
            }

            // Update previews locations
            int count = _previews.Count;
            if (count > 0)
            {
                // Update view dimensions and check if we can show it
                const float aspectRatio = 16.0f / 9.0f;
                const float minHeight = 20;
                const float minWidth = minHeight * aspectRatio;
                const float maxHeight = 150;
                const float maxWidth = maxHeight * aspectRatio;
                const float viewSpaceMaxPercentage = 0.7f;
                const float margin = 10;
                var totalSize = Size * viewSpaceMaxPercentage - margin;
                var singleSize = totalSize / count - count * margin;
                float sizeX = Mathf.Clamp(singleSize.X, minWidth, maxWidth);
                float sizeY = sizeX / aspectRatio;
                singleSize = new Float2(sizeX, sizeY);
                int countPerX = Mathf.FloorToInt(totalSize.X / singleSize.X);
                int countPerY = Mathf.FloorToInt(totalSize.Y / singleSize.Y);
                int index = 0;
                for (int y = 1; y <= countPerY; y++)
                {
                    for (int x = 1; x <= countPerX; x++)
                    {
                        if (index == count)
                            break;

                        var pos = Size - (singleSize + margin) * new Float2(x, y);
                        _previews[index++].Bounds = new Rectangle(pos, singleSize);
                    }

                    if (index == count)
                        break;
                }
            }
        }

        /// <summary>
        /// Hides the camera preview that uses given camera.
        /// </summary>
        /// <param name="camera">The camera to hide.</param>
        private void HideCameraPreview(Camera camera)
        {
            var preview = _previews.FirstOrDefault(x => x.Camera == camera);
            if (preview != null)
                HideCameraPreview(preview);
        }

        /// <summary>
        /// Hides the camera preview.
        /// </summary>
        /// <param name="preview">The preview to hide.</param>
        private void HideCameraPreview(CameraPreview preview)
        {
            _previews.Remove(preview);
            preview.Dispose();
        }

        /// <summary>
        /// Hides all the camera previews.
        /// </summary>
        private void HideAllCameraPreviews()
        {
            while (_previews.Count > 0)
                HideCameraPreview(_previews[0]);
        }

        /// <inheritdoc />
        public override void OnSceneUnloading(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < _previews.Count; i++)
            {
                if (_previews[i].Camera.Scene == scene)
                    HideCameraPreview(_previews[i--]);
            }

            if (_pilotActor && _pilotActor.Scene == scene)
            {
                EndPilot();
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (Root.GetKeyDown(KeyboardKeys.F12))
            {
                Viewport.TakeScreenshot();
            }

            base.Update(deltaTime);

            if (_pilotActor)
            {
                var transform = Viewport.ViewTransform;
                transform.Scale = _pilotActor.Scale;
                _pilotActor.Transform = transform;
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            HideAllCameraPreviews();
            EndPilot();
            _pilotWidget = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("GridEnabled", Viewport.Grid.Enabled.ToString());
            writer.WriteAttributeString("ShowFpsCounter", Viewport.ShowFpsCounter.ToString());
            writer.WriteAttributeString("ShowNavigation", Viewport.ShowNavigation.ToString());
            writer.WriteAttributeString("NearPlane", Viewport.NearPlane.ToString());
            writer.WriteAttributeString("FarPlane", Viewport.FarPlane.ToString());
            writer.WriteAttributeString("FieldOfView", Viewport.FieldOfView.ToString());
            writer.WriteAttributeString("MovementSpeed", Viewport.MovementSpeed.ToString());
            writer.WriteAttributeString("OrthographicScale", Viewport.OrthographicScale.ToString());
            writer.WriteAttributeString("UseOrthographicProjection", Viewport.UseOrthographicProjection.ToString());
            writer.WriteAttributeString("ViewFlags", ((ulong)Viewport.Task.View.Flags).ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (bool.TryParse(node.GetAttribute("GridEnabled"), out bool value1))
                Viewport.Grid.Enabled = value1;

            if (bool.TryParse(node.GetAttribute("ShowFpsCounter"), out value1))
                Viewport.ShowFpsCounter = value1;

            if (bool.TryParse(node.GetAttribute("ShowNavigation"), out value1))
                Viewport.ShowNavigation = value1;

            if (float.TryParse(node.GetAttribute("NearPlane"), out float value2))
                Viewport.NearPlane = value2;

            if (float.TryParse(node.GetAttribute("FarPlane"), out value2))
                Viewport.FarPlane = value2;

            if (float.TryParse(node.GetAttribute("FieldOfView"), out value2))
                Viewport.FieldOfView = value2;

            if (float.TryParse(node.GetAttribute("MovementSpeed"), out value2))
                Viewport.MovementSpeed = value2;

            if (float.TryParse(node.GetAttribute("OrthographicScale"), out value2))
                Viewport.OrthographicScale = value2;

            if (bool.TryParse(node.GetAttribute("UseOrthographicProjection"), out value1))
                Viewport.UseOrthographicProjection = value1;

            if (ulong.TryParse(node.GetAttribute("ViewFlags"), out ulong value3))
                Viewport.Task.ViewFlags = (ViewFlags)value3;

            // Reset view flags if opening with different engine version (ViewFlags enum could be modified)
            if (Editor.LastProjectOpenedEngineBuild != Globals.EngineBuildNumber)
                Viewport.Task.ViewFlags = ViewFlags.DefaultEditor;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            Viewport.Grid.Enabled = true;
            Viewport.NearPlane = 8.0f;
            Viewport.FarPlane = 20000.0f;
            Viewport.FieldOfView = 60.0f;
            Viewport.MovementSpeed = 1.0f;
            Viewport.OrthographicScale = 1.0f;
            Viewport.UseOrthographicProjection = false;
        }
    }
}
