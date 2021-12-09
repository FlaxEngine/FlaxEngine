// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.Content;
using FlaxEditor.GUI.Tree;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.States;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Windows used to present loaded scenes collection and whole scene graph.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.SceneEditorWindow" />
    public partial class SceneTreeWindow : SceneEditorWindow
    {
        /// <summary>
        /// The spawnable actors group.
        /// </summary>
        public struct ActorsGroup
        {
            /// <summary>
            /// The group name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The types to spawn (name and type).
            /// </summary>
            public KeyValuePair<string, Type>[] Types;
        }

        /// <summary>
        /// The Spawnable actors (groups with single entry are inlined without a child menu)
        /// </summary>
        public static readonly ActorsGroup[] SpawnActorsGroups =
        {
            new ActorsGroup
            {
                Types = new[] { new KeyValuePair<string, Type>("Actor", typeof(EmptyActor)) }
            },
            new ActorsGroup
            {
                Types = new[] { new KeyValuePair<string, Type>("Model", typeof(StaticModel)) }
            },
            new ActorsGroup
            {
                Types = new[] { new KeyValuePair<string, Type>("Camera", typeof(Camera)) }
            },
            new ActorsGroup
            {
                Name = "Lights",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Directional Light", typeof(DirectionalLight)),
                    new KeyValuePair<string, Type>("Point Light", typeof(PointLight)),
                    new KeyValuePair<string, Type>("Spot Light", typeof(SpotLight)),
                    new KeyValuePair<string, Type>("Sky Light", typeof(SkyLight)),
                }
            },
            new ActorsGroup
            {
                Name = "Visuals",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Environment Probe", typeof(EnvironmentProbe)),
                    new KeyValuePair<string, Type>("Sky", typeof(Sky)),
                    new KeyValuePair<string, Type>("Skybox", typeof(Skybox)),
                    new KeyValuePair<string, Type>("Exponential Height Fog", typeof(ExponentialHeightFog)),
                    new KeyValuePair<string, Type>("PostFx Volume", typeof(PostFxVolume)),
                    new KeyValuePair<string, Type>("Decal", typeof(Decal)),
                    new KeyValuePair<string, Type>("Particle Effect", typeof(ParticleEffect)),
                }
            },
            new ActorsGroup
            {
                Name = "Physics",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Rigid Body", typeof(RigidBody)),
                    new KeyValuePair<string, Type>("Character Controller", typeof(CharacterController)),
                    new KeyValuePair<string, Type>("Box Collider", typeof(BoxCollider)),
                    new KeyValuePair<string, Type>("Sphere Collider", typeof(SphereCollider)),
                    new KeyValuePair<string, Type>("Capsule Collider", typeof(CapsuleCollider)),
                    new KeyValuePair<string, Type>("Mesh Collider", typeof(MeshCollider)),
                    new KeyValuePair<string, Type>("Fixed Joint", typeof(FixedJoint)),
                    new KeyValuePair<string, Type>("Distance Joint", typeof(DistanceJoint)),
                    new KeyValuePair<string, Type>("Slider Joint", typeof(SliderJoint)),
                    new KeyValuePair<string, Type>("Spherical Joint", typeof(SphericalJoint)),
                    new KeyValuePair<string, Type>("Hinge Joint", typeof(HingeJoint)),
                    new KeyValuePair<string, Type>("D6 Joint", typeof(D6Joint)),
                }
            },
            new ActorsGroup
            {
                Name = "Other",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Animated Model", typeof(AnimatedModel)),
                    new KeyValuePair<string, Type>("Bone Socket", typeof(BoneSocket)),
                    new KeyValuePair<string, Type>("CSG Box Brush", typeof(BoxBrush)),
                    new KeyValuePair<string, Type>("Audio Source", typeof(AudioSource)),
                    new KeyValuePair<string, Type>("Audio Listener", typeof(AudioListener)),
                    new KeyValuePair<string, Type>("Scene Animation", typeof(SceneAnimationPlayer)),
                    new KeyValuePair<string, Type>("Nav Mesh Bounds Volume", typeof(NavMeshBoundsVolume)),
                    new KeyValuePair<string, Type>("Nav Link", typeof(NavLink)),
                    new KeyValuePair<string, Type>("Nav Modifier Volume", typeof(NavModifierVolume)),
                    new KeyValuePair<string, Type>("Spline", typeof(Spline)),
                }
            },
            new ActorsGroup
            {
                Name = "GUI",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("UI Control", typeof(UIControl)),
                    new KeyValuePair<string, Type>("UI Canvas", typeof(UICanvas)),
                    new KeyValuePair<string, Type>("Text Render", typeof(TextRender)),
                    new KeyValuePair<string, Type>("Sprite Render", typeof(SpriteRender)),
                }
            },
        };

        private TextBox _searchBox;
        private Tree _tree;
        private bool _isUpdatingSelection;
        private bool _isMouseDown;

        private DragAssets _dragAssets;
        private DragActorType _dragActorType;
        private DragHandlers _dragHandlers;

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneTreeWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public SceneTreeWindow(Editor editor)
        : base(editor, true, ScrollBars.Both)
        {
            Title = "Scene";
            ScrollMargin = new Margin(0, 0, 0, 100.0f);

            // Scene searching query input box
            var headerPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Offsets = new Margin(0, 0, 0, 18 + 6),
                Parent = this,
            };
            _searchBox = new TextBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                WatermarkText = "Search...",
                Parent = headerPanel,
                Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18),
            };
            _searchBox.TextChanged += OnSearchBoxTextChanged;

            // Create scene structure tree
            var root = editor.Scene.Root;
            root.TreeNode.ChildrenIndent = 0;
            root.TreeNode.Expand();
            _tree = new Tree(true)
            {
                Y = headerPanel.Bottom,
                Margin = new Margin(0.0f, 0.0f, -16.0f, 0.0f), // Hide root node
            };
            _tree.AddChild(root.TreeNode);
            _tree.SelectedChanged += Tree_OnSelectedChanged;
            _tree.RightClick += Tree_OnRightClick;
            _tree.Parent = this;

            // Setup input actions
            InputActions.Add(options => options.TranslateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate);
            InputActions.Add(options => options.RotateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate);
            InputActions.Add(options => options.ScaleMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale);
            InputActions.Add(options => options.FocusSelection, () => Editor.Windows.EditWin.Viewport.FocusSelection());
            InputActions.Add(options => options.Rename, Rename);
        }

        private void OnSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            var root = Editor.Scene.Root;
            root.TreeNode.LockChildrenRecursive();

            // Update tree
            var query = _searchBox.Text;
            root.TreeNode.UpdateFilter(query);

            root.TreeNode.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        private void Rename()
        {
            var selection = Editor.SceneEditing.Selection;
            if (selection.Count != 0 && selection[0] is ActorNode actor)
            {
                if (selection.Count != 0)
                    Editor.SceneEditing.Select(actor);
                actor.TreeNode.StartRenaming();
            }
        }

        private void Spawn(Type type)
        {
            // Create actor
            Actor actor = (Actor)FlaxEngine.Object.New(type);
            Actor parentActor = null;
            if (Editor.SceneEditing.HasSthSelected && Editor.SceneEditing.Selection[0] is ActorNode actorNode)
            {
                parentActor = actorNode.Actor;
                actorNode.TreeNode.Expand();
            }
            if (parentActor == null)
            {
                var scenes = Level.Scenes;
                if (scenes.Length > 0)
                    parentActor = scenes[scenes.Length - 1];
            }
            if (parentActor != null)
            {
                // Use the same location
                actor.Transform = parentActor.Transform;

                // Rename actor to identify it easily
                actor.Name = StringUtils.IncrementNameNumber(type.Name, x => parentActor.GetChild(x) == null);
            }

            // Spawn it
            Editor.SceneEditing.Spawn(actor, parentActor);
        }

        /// <summary>
        /// Focuses search box.
        /// </summary>
        public void Search()
        {
            _searchBox.Focus();
        }

        private void Tree_OnSelectedChanged(List<TreeNode> before, List<TreeNode> after)
        {
            // Check if lock events
            if (_isUpdatingSelection)
                return;

            if (after.Count > 0)
            {
                // Get actors from nodes
                var actors = new List<SceneGraphNode>(after.Count);
                for (int i = 0; i < after.Count; i++)
                {
                    if (after[i] is ActorTreeNode node && node.Actor)
                        actors.Add(node.ActorNode);
                }

                // Select
                Editor.SceneEditing.Select(actors);
            }
            else
            {
                // Deselect
                Editor.SceneEditing.Deselect();
            }
        }

        private void Tree_OnRightClick(TreeNode node, Vector2 location)
        {
            if (!Editor.StateMachine.CurrentState.CanEditScene)
                return;

            ShowContextMenu(node, location);
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Editor.SceneEditing.SelectionChanged += OnSelectionChanged;
        }

        private void OnSelectionChanged()
        {
            _isUpdatingSelection = true;

            var selection = Editor.SceneEditing.Selection;
            if (selection.Count == 0)
            {
                _tree.Deselect();
            }
            else
            {
                // Find nodes to select
                var nodes = new List<TreeNode>(selection.Count);
                for (int i = 0; i < selection.Count; i++)
                {
                    if (selection[i] is ActorNode node)
                    {
                        nodes.Add(node.TreeNode);
                    }
                }

                // Select nodes
                _tree.Select(nodes);

                // For single node selected scroll view so user can see it
                if (nodes.Count == 1)
                {
                    nodes[0].ExpandAllParents(true);
                    ScrollViewTo(nodes[0]);
                }
            }

            _isUpdatingSelection = false;
        }
        
        private bool ValidateDragAsset(AssetItem assetItem)
        {
            return assetItem.OnEditorDrag(this);
        }

        private static bool ValidateDragActorType(ScriptType actorType)
        {
            return true;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;

            // Draw overlay
            string overlayText = null;
            var state = Editor.StateMachine.CurrentState;
            var textWrap = TextWrapping.NoWrap;
            if (state is LoadingState)
            {
                overlayText = "Loading...";
            }
            else if (state is ChangingScenesState)
            {
                overlayText = "Loading scene...";
            }
            else if (((ContainerControl)_tree.GetChild(0)).ChildrenCount == 0)
            {
                overlayText = "No scene\nOpen one from the content window";
                textWrap = TextWrapping.WrapWords;
            }
            if (overlayText != null)
            {
                Render2D.DrawText(style.FontLarge, overlayText, GetClientArea(), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center, textWrap);
            }

            base.Draw();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton buttons)
        {
            if (base.OnMouseDown(location, buttons))
                return true;

            if (buttons == MouseButton.Right)
            {
                _isMouseDown = true;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton buttons)
        {
            if (base.OnMouseUp(location, buttons))
                return true;

            if (_isMouseDown && buttons == MouseButton.Right)
            {
                _isMouseDown = false;

                if (Editor.StateMachine.CurrentState.CanEditScene)
                {
                    // Show context menu
                    Editor.SceneEditing.Deselect();
                    ShowContextMenu(Parent, location + _searchBox.BottomLeft);
                }

                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            _isMouseDown = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (result == DragDropEffect.None && Editor.StateMachine.CurrentState.CanEditScene)
            {
                if (_dragHandlers == null)
                    _dragHandlers = new DragHandlers();
                if (_dragAssets == null)
                {
                    _dragAssets = new DragAssets(ValidateDragAsset);
                    _dragHandlers.Add(_dragAssets);
                }
                if (_dragAssets.OnDragEnter(data))
                    return _dragAssets.Effect;
                if (_dragActorType == null)
                {
                    _dragActorType = new DragActorType(ValidateDragActorType);
                    _dragHandlers.Add(_dragActorType);
                }
                if (_dragActorType.OnDragEnter(data))
                    return _dragActorType.Effect;
            }
            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);
            if (result == DragDropEffect.None && Editor.StateMachine.CurrentState.CanEditScene && _dragHandlers != null)
            {
                result = _dragHandlers.Effect;
            }
            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            base.OnDragLeave();

            _dragHandlers?.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result == DragDropEffect.None)
            {
                // Drag assets
                if (_dragAssets != null && _dragAssets.HasValidDrag)
                {
                    for (int i = 0; i < _dragAssets.Objects.Count; i++)
                    {
                        var item = _dragAssets.Objects[i];
                        var actor = item.OnEditorDrop(this);
                        actor.Name = item.ShortName;
                        Level.SpawnActor(actor);
                    }
                    result = DragDropEffect.Move;
                }
                // Drag actor type
                else if (_dragActorType != null && _dragActorType.HasValidDrag)
                {
                    for (int i = 0; i < _dragActorType.Objects.Count; i++)
                    {
                        var item = _dragActorType.Objects[i];
                        var actor = item.CreateInstance() as Actor;
                        if (actor == null)
                        {
                            Editor.LogWarning("Failed to spawn actor of type " + item.TypeName);
                            continue;
                        }
                        actor.Name = item.Name;
                        Level.SpawnActor(actor);
                    }
                    result = DragDropEffect.Move;
                }

                _dragHandlers.OnDragDrop(null);
            }
            return result;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _dragAssets = null;
            _dragActorType = null;
            _dragHandlers?.Clear();
            _dragHandlers = null;
            _tree = null;
            _searchBox = null;

            base.OnDestroy();
        }
    }
}
