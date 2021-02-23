// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Scripting;
using FlaxEditor.Tools;
using FlaxEditor.Tools.Foliage;
using FlaxEditor.Tools.Terrain;
using FlaxEditor.Utilities;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Objects spawning tab. Supports searching actor types and prefabs for spawning them into the level.
    /// </summary>
    /// <seealso cref="Tab" />
    public class SpawnTab : Tab
    {
        private class Item : TreeNode
        {
            private DragData _dragData;
            private List<Rectangle> _highlights;

            public Item(string text, DragData dragData = null)
            : this(text, dragData, SpriteHandle.Invalid)
            {
            }

            private Item(string text, DragData dragData, SpriteHandle icon)
            : base(false, icon, icon)
            {
                Text = text;
                Height = 20;
                TextMargin = new Margin(-5.0f, 2.0f, 2.0f, 2.0f);
                _dragData = dragData;
            }

            public void SetHighlights(List<Rectangle> highlights)
            {
                _highlights = highlights;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Draw all highlights
                if (_highlights != null)
                {
                    var style = Style.Current;
                    var color = style.ProgressNormal * 0.6f;
                    for (int i = 0; i < _highlights.Count; i++)
                        Render2D.FillRectangle(_highlights[i], color);
                }
            }

            /// <inheritdoc />
            protected override void DoDragDrop()
            {
                if (_dragData != null)
                    DoDragDrop(_dragData);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _dragData = null;
                _highlights = null;

                base.OnDestroy();
            }
        }

        private TextBox _searchBox;
        private ContainerControl _groupSearch;

        /// <summary>
        /// The editor instance.
        /// </summary>
        public readonly Editor Editor;

        /// <summary>
        /// Initializes a new instance of the <see cref="SpawnTab"/> class.
        /// </summary>
        /// <param name="icon">The icon.</param>
        /// <param name="editor">The editor instance.</param>
        public SpawnTab(SpriteHandle icon, Editor editor)
        : base(string.Empty, icon)
        {
            Editor = editor;
            Selected += tab => Editor.Windows.EditWin.Viewport.SetActiveMode<TransformGizmoMode>();
            ScriptsBuilder.ScriptsReload += OnScriptsReload;

            var actorGroups = new Tabs
            {
                Orientation = Orientation.Vertical,
                UseScroll = true,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Vector2(120, 32),
                Parent = this,
            };

            _groupSearch = CreateGroupWithList(actorGroups, "Search", 26);
            _searchBox = new TextBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                WatermarkText = "Search...",
                Parent = _groupSearch.Parent.Parent,
                Bounds = new Rectangle(4, 4, actorGroups.Width - 8, 18),
            };
            _searchBox.TextChanged += OnSearchBoxTextChanged;

            var groupBasicModels = CreateGroupWithList(actorGroups, "Basic Models");
            groupBasicModels.AddChild(CreateEditorAssetItem("Cube", "Primitives/Cube.flax"));
            groupBasicModels.AddChild(CreateEditorAssetItem("Sphere", "Primitives/Sphere.flax"));
            groupBasicModels.AddChild(CreateEditorAssetItem("Plane", "Primitives/Plane.flax"));
            groupBasicModels.AddChild(CreateEditorAssetItem("Cylinder", "Primitives/Cylinder.flax"));
            groupBasicModels.AddChild(CreateEditorAssetItem("Cone", "Primitives/Cone.flax"));
            groupBasicModels.AddChild(CreateEditorAssetItem("Capsule", "Primitives/Capsule.flax"));

            var groupLights = CreateGroupWithList(actorGroups, "Lights");
            groupLights.AddChild(CreateActorItem("Directional Light", typeof(DirectionalLight)));
            groupLights.AddChild(CreateActorItem("Point Light", typeof(PointLight)));
            groupLights.AddChild(CreateActorItem("Spot Light", typeof(SpotLight)));
            groupLights.AddChild(CreateActorItem("Sky Light", typeof(SkyLight)));

            var groupVisuals = CreateGroupWithList(actorGroups, "Visuals");
            groupVisuals.AddChild(CreateActorItem("Camera", typeof(Camera)));
            groupVisuals.AddChild(CreateActorItem("Environment Probe", typeof(EnvironmentProbe)));
            groupVisuals.AddChild(CreateActorItem("Skybox", typeof(Skybox)));
            groupVisuals.AddChild(CreateActorItem("Sky", typeof(Sky)));
            groupVisuals.AddChild(CreateActorItem("Exponential Height Fog", typeof(ExponentialHeightFog)));
            groupVisuals.AddChild(CreateActorItem("PostFx Volume", typeof(PostFxVolume)));
            groupVisuals.AddChild(CreateActorItem("Decal", typeof(Decal)));
            groupVisuals.AddChild(CreateActorItem("Particle Effect", typeof(ParticleEffect)));

            var groupPhysics = CreateGroupWithList(actorGroups, "Physics");
            groupPhysics.AddChild(CreateActorItem("Rigid Body", typeof(RigidBody)));
            groupPhysics.AddChild(CreateActorItem("Character Controller", typeof(CharacterController)));
            groupPhysics.AddChild(CreateActorItem("Box Collider", typeof(BoxCollider)));
            groupPhysics.AddChild(CreateActorItem("Sphere Collider", typeof(SphereCollider)));
            groupPhysics.AddChild(CreateActorItem("Capsule Collider", typeof(CapsuleCollider)));
            groupPhysics.AddChild(CreateActorItem("Mesh Collider", typeof(MeshCollider)));
            groupPhysics.AddChild(CreateActorItem("Fixed Joint", typeof(FixedJoint)));
            groupPhysics.AddChild(CreateActorItem("Distance Joint", typeof(DistanceJoint)));
            groupPhysics.AddChild(CreateActorItem("Slider Joint", typeof(SliderJoint)));
            groupPhysics.AddChild(CreateActorItem("Spherical Joint", typeof(SphericalJoint)));
            groupPhysics.AddChild(CreateActorItem("Hinge Joint", typeof(HingeJoint)));
            groupPhysics.AddChild(CreateActorItem("D6 Joint", typeof(D6Joint)));

            var groupOther = CreateGroupWithList(actorGroups, "Other");
            groupOther.AddChild(CreateActorItem("Animated Model", typeof(AnimatedModel)));
            groupOther.AddChild(CreateActorItem("Bone Socket", typeof(BoneSocket)));
            groupOther.AddChild(CreateActorItem("CSG Box Brush", typeof(BoxBrush)));
            groupOther.AddChild(CreateActorItem("Audio Source", typeof(AudioSource)));
            groupOther.AddChild(CreateActorItem("Audio Listener", typeof(AudioListener)));
            groupOther.AddChild(CreateActorItem("Empty Actor", typeof(EmptyActor)));
            groupOther.AddChild(CreateActorItem("Scene Animation", typeof(SceneAnimationPlayer)));
            groupOther.AddChild(CreateActorItem("Nav Mesh Bounds Volume", typeof(NavMeshBoundsVolume)));
            groupOther.AddChild(CreateActorItem("Nav Link", typeof(NavLink)));
            groupOther.AddChild(CreateActorItem("Nav Modifier Volume", typeof(NavModifierVolume)));
            groupOther.AddChild(CreateActorItem("Spline", typeof(Spline)));

            var groupGui = CreateGroupWithList(actorGroups, "GUI");
            groupGui.AddChild(CreateActorItem("UI Control", typeof(UIControl)));
            groupGui.AddChild(CreateActorItem("UI Canvas", typeof(UICanvas)));
            groupGui.AddChild(CreateActorItem("Text Render", typeof(TextRender)));
            groupGui.AddChild(CreateActorItem("Sprite Render", typeof(SpriteRender)));

            actorGroups.SelectedTabIndex = 1;
        }

        private void OnScriptsReload()
        {
            // Prevent any references to actor types from the game assemblies that will be reloaded
            _searchBox.Clear();
            _groupSearch.DisposeChildren();
            _groupSearch.PerformLayout();
        }

        private void OnSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            var filterText = _searchBox.Text;
            _groupSearch.LockChildrenRecursive();
            _groupSearch.DisposeChildren();

            foreach (var actorType in Editor.CodeEditing.Actors.Get())
            {
                var text = actorType.Name;

                if (!QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
                    continue;

                var item = _groupSearch.AddChild(CreateActorItem(CustomEditors.CustomEditorsUtil.GetPropertyNameUI(text), actorType));
                item.TooltipText = actorType.TypeName;
                var attributes = actorType.GetAttributes(false);
                var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                if (tooltipAttribute != null)
                {
                    item.TooltipText += '\n';
                    item.TooltipText += tooltipAttribute.Text;
                }

                var highlights = new List<Rectangle>(ranges.Length);
                var style = Style.Current;
                var font = style.FontSmall;
                var textRect = item.TextRect;
                for (int i = 0; i < ranges.Length; i++)
                {
                    var start = font.GetCharPosition(text, ranges[i].StartIndex);
                    var end = font.GetCharPosition(text, ranges[i].EndIndex);
                    highlights.Add(new Rectangle(start.X + textRect.X, textRect.Y, end.X - start.X, textRect.Height));
                }
                item.SetHighlights(highlights);
            }

            _groupSearch.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        private Item CreateEditorAssetItem(string name, string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor", path);
            return new Item(name, GUI.Drag.DragItems.GetDragData(path));
        }

        private Item CreateActorItem(string name, Type type)
        {
            return new Item(name, GUI.Drag.DragActorType.GetDragData(type));
        }

        private Item CreateActorItem(string name, ScriptType type)
        {
            return new Item(name, GUI.Drag.DragActorType.GetDragData(type));
        }

        private ContainerControl CreateGroupWithList(Tabs parentTabs, string title, float topOffset = 0)
        {
            var tab = parentTabs.AddTab(new Tab(title));
            var panel = new Panel(ScrollBars.Both)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, topOffset, 0),
                Parent = tab
            };
            var tree = new Tree(false)
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Parent = panel
            };
            return tree;
        }
    }

    /// <summary>
    /// A helper utility window with bunch of tools used during scene editing.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public class ToolboxWindow : EditorWindow
    {
        /// <summary>
        /// Gets the tabs control used by this window. Can be used to add custom toolbox modes.
        /// </summary>
        public Tabs TabsControl { get; private set; }

        /// <summary>
        /// The objects spawning tab.
        /// </summary>
        public SpawnTab Spawn;

        /// <summary>
        /// The vertex painting tab.
        /// </summary>
        public VertexPaintingTab VertexPaint;

        /// <summary>
        /// The foliage editing tab.
        /// </summary>
        public FoliageTab Foliage;

        /// <summary>
        /// The terrain editing tab.
        /// </summary>
        public CarveTab Carve;

        /// <summary>
        /// Initializes a new instance of the <see cref="ToolboxWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ToolboxWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Toolbox";
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            float tabSize = 48 * Editor.Options.Options.Interface.IconsScale;
            TabsControl = new Tabs
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Vector2(tabSize, tabSize),
                Parent = this
            };

            TabsControl.AddTab(Spawn = new SpawnTab(Editor.Icons.Add48, Editor));
            TabsControl.AddTab(VertexPaint = new VertexPaintingTab(Editor.Icons.Paint48, Editor));
            TabsControl.AddTab(Foliage = new FoliageTab(Editor.Icons.Foliage48, Editor));
            TabsControl.AddTab(Carve = new CarveTab(Editor.Icons.Mountain48, Editor));

            TabsControl.SelectedTabIndex = 0;
        }
    }
}
