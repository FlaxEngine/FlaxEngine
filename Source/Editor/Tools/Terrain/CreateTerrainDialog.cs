// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.Dialogs;
using FlaxEngine;
using FlaxEngine.GUI;

// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Local
// ReSharper disable UnusedMember.Global
#pragma warning disable 649
#pragma warning disable 414

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Terrain creator dialog. Allows user to specify initial terrain properties perform proper setup.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Dialogs.Dialog" />
    sealed class CreateTerrainDialog : Dialog
    {
        private enum ChunkSizes
        {
            _31 = 31,
            _63 = 63,
            _127 = 127,
            _254 = 254,
            _255 = 255,
            _511 = 511,
        }

        private class Options
        {
            [EditorOrder(100), EditorDisplay("Layout", "Number Of Patches"), DefaultValue(typeof(Int2), "1,1"), Limit(0, 512), Tooltip("Amount of terrain patches in each direction (X and Z). Each terrain patch contains a grid of 16 chunks. Patches can be later added or removed from terrain using a terrain editor tool.")]
            public Int2 NumberOfPatches = new Int2(1, 1);

            [EditorOrder(110), EditorDisplay("Layout"), DefaultValue(ChunkSizes._127), Tooltip("The size of the chunk (amount of quads per edge for the highest LOD).")]
            public ChunkSizes ChunkSize = ChunkSizes._127;

            [EditorOrder(120), EditorDisplay("Layout", "LOD Count"), DefaultValue(6), Limit(1, FlaxEngine.Terrain.MaxLODs), Tooltip("The maximum Level Of Details count. The actual amount of LODs may be lower due to provided chunk size (each LOD has 4 times less quads).")]
            public int LODCount = 6;

            [EditorOrder(130), EditorDisplay("Layout"), DefaultValue(null), Tooltip("The default material used for terrain rendering (chunks can override this). It must have Domain set to terrain.")]
            public MaterialBase Material;

            [EditorOrder(210), EditorDisplay("Collision", "Collision LOD"), DefaultValue(-1), Limit(-1, 100, 0.1f), Tooltip("Terrain geometry LOD index used for collision.")]
            public int CollisionLOD = -1;

            [EditorOrder(300), EditorDisplay("Import Data"), DefaultValue(null), Tooltip("Custom heightmap texture to import. Used as a source for height field values (from channel Red).")]
            public Texture Heightmap;

            [EditorOrder(310), EditorDisplay("Import Data"), DefaultValue(5000.0f), Tooltip("Custom heightmap texture values scale. Applied to adjust the normalized heightmap values into the world units.")]
            public float HeightmapScale = 5000.0f;

            [EditorOrder(320), EditorDisplay("Import Data"), DefaultValue(null), Tooltip("Custom terrain splat map used as a source of the terrain layers weights. Each channel from RGBA is used as an independent layer weight for terrain layers compositing.")]
            public Texture Splatmap1;

            [EditorOrder(330), EditorDisplay("Import Data"), DefaultValue(null), Tooltip("Custom terrain splat map used as a source of the terrain layers weights. Each channel from RGBA is used as an independent layer weight for terrain layers compositing.")]
            public Texture Splatmap2;

            [EditorOrder(400), EditorDisplay("Transform", "Position"), DefaultValue(typeof(Double3), "0,0,0"), Tooltip("Position of the terrain (importer offset it on the Y axis.)")]
            public Double3 Position = new Double3(0.0f, 0.0f, 0.0f);

            [EditorOrder(410), EditorDisplay("Transform", "Rotation"), DefaultValue(typeof(Quaternion), "0,0,0,1"), Tooltip("Orientation of the terrain")]
            public Quaternion Orientation = Quaternion.Identity;

            [EditorOrder(420), EditorDisplay("Transform", "Scale"), DefaultValue(1.0f), Limit(0.0001f, float.MaxValue, 0.01f), Tooltip("Scale of the terrain")]
            public float Scale = 1.0f;
        }

        private readonly Options _options = new Options();
        private bool _isDone;
        private bool _isWorking;
        private FlaxEngine.Terrain _terrain;
        private CustomEditorPresenter _editor;

        /// <summary>
        /// Initializes a new instance of the <see cref="CreateTerrainDialog"/> class.
        /// </summary>
        public CreateTerrainDialog()
        : base("Create terrain")
        {
            const float TotalWidth = 450;
            const float EditorHeight = 600;
            Width = TotalWidth;

            // Header and help description
            var headerLabel = new Label
            {
                Text = "New Terrain",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, 40),
                Parent = this,
                Font = new FontReference(Style.Current.FontTitle)
            };
            var infoLabel = new Label
            {
                Text = "Specify options for new terrain.\nIt will be added to the first opened scene.\nMany of the following settings can be adjusted later.\nYou can also create terrain at runtime from code.",
                HorizontalAlignment = TextAlignment.Near,
                Margin = new Margin(7),
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(10, -20, 45, 70),
                Parent = this
            };

            // Buttons
            const float ButtonsWidth = 60;
            const float ButtonsHeight = 24;
            const float ButtonsMargin = 8;
            var importButton = new Button
            {
                Text = "Create",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-ButtonsWidth - ButtonsMargin, ButtonsWidth, -ButtonsHeight - ButtonsMargin, ButtonsHeight),
                Parent = this
            };
            importButton.Clicked += OnSubmit;
            var cancelButton = new Button
            {
                Text = "Cancel",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-ButtonsWidth - ButtonsMargin - ButtonsWidth - ButtonsMargin, ButtonsWidth, -ButtonsHeight - ButtonsMargin, ButtonsHeight),
                Parent = this
            };
            cancelButton.Clicked += OnCancel;

            // Settings editor
            var settingsEditor = new CustomEditorPresenter(null);
            settingsEditor.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            settingsEditor.Panel.Offsets = new Margin(2, 2, infoLabel.Bottom + 2, EditorHeight);
            settingsEditor.Panel.Parent = this;
            _editor = settingsEditor;

            _dialogSize = new Float2(TotalWidth, settingsEditor.Panel.Bottom);

            settingsEditor.Select(_options);
        }

        private void OnCreate()
        {
            var scene = Level.GetScene(0);
            if (scene == null)
                throw new InvalidOperationException("No scene found to add terrain to it!");

            // Create terrain object and setup some options
            var terrain = new FlaxEngine.Terrain();
            terrain.Setup(_options.LODCount, (int)_options.ChunkSize);
            terrain.Transform = new Transform(_options.Position, _options.Orientation, new Float3(_options.Scale));
            terrain.Material = _options.Material;
            terrain.CollisionLOD = _options.CollisionLOD;
            if (_options.Heightmap)
                terrain.Position -= new Vector3(0, _options.HeightmapScale * 0.5f, 0);

            // Add to scene (even if generation fails user gets a terrain in the scene)
            terrain.Parent = scene;
            Editor.Instance.Scene.MarkSceneEdited(scene);

            // Show loading label
            var label = new Label
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Text = "Generating terrain...",
                BackgroundColor = Style.Current.ForegroundDisabled,
                Parent = this,
            };

            // Lock UI
            _editor.Panel.Enabled = false;
            _isWorking = true;
            _isDone = false;

            // Start async work
            _terrain = terrain;
            var thread = new System.Threading.Thread(Generate)
            {
                Name = "Terrain Generator"
            };
            thread.Start();
        }

        private void Generate()
        {
            _isWorking = true;
            _isDone = false;

            // Call tool to generate the terrain patches from the input data
            if (TerrainTools.GenerateTerrain(_terrain, ref _options.NumberOfPatches, _options.Heightmap, _options.HeightmapScale, _options.Splatmap1, _options.Splatmap2))
            {
                Editor.LogError("Failed to generate terrain. See log for more info.");
            }

            _isWorking = false;
            _isDone = true;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            if (_isWorking)
                return;

            OnCreate();
        }

        /// <inheritdoc />
        public override void OnCancel()
        {
            if (_isWorking)
                return;

            base.OnCancel();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (_isDone)
            {
                Editor.Instance.SceneEditing.Select(_terrain);

                _terrain = null;
                _isDone = false;
                Close(DialogResult.OK);
                return;
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        protected override bool CanCloseWindow(ClosingReason reason)
        {
            if (_isWorking && reason == ClosingReason.User)
                return false;

            return base.CanCloseWindow(reason);
        }
    }
}
