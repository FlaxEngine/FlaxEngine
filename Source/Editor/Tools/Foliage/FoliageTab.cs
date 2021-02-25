// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.Modules;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Foliage editing tab. Supports different modes for foliage editing including spawning, removing, and managing tools.
    /// </summary>
    /// <seealso cref="Tab" />
    [HideInEditor]
    public class FoliageTab : Tab
    {
        private readonly Tabs _modes;
        private readonly ContainerControl _noFoliagePanel;
        private int _selectedFoliageTypeIndex = -1;
        private Button _createNewFoliage;

        /// <summary>
        /// The editor instance.
        /// </summary>
        public readonly Editor Editor;

        /// <summary>
        /// The cached selected foliage. It's synchronized with <see cref="SceneEditingModule.Selection"/>.
        /// </summary>
        public FlaxEngine.Foliage SelectedFoliage;

        /// <summary>
        /// Occurs when selected foliage gets changed (to a different value).
        /// </summary>
        public event Action SelectedFoliageChanged;

        /// <summary>
        /// Delegate signature for selected foliage index change.
        /// </summary>
        /// <param name="previousIndex">The index of the previous foliage type.</param>
        /// <param name="currentIndex">The index of the current foliage type.</param>
        public delegate void SelectedFoliageTypeIndexChangedDelegate(int previousIndex, int currentIndex);

        /// <summary>
        /// Occurs when selected foliage type index gets changed.
        /// </summary>
        public event SelectedFoliageTypeIndexChangedDelegate SelectedFoliageTypeIndexChanged;

        /// <summary>
        /// Occurs when selected foliage actors gets modification for foliage types collection (item added or removed). UI uses it to update the layout without manually tracking the collection.
        /// </summary>
        public event Action SelectedFoliageTypesChanged;

        /// <summary>
        /// Gets or sets the index of the selected foliage type.
        /// </summary>
        public int SelectedFoliageTypeIndex
        {
            get => _selectedFoliageTypeIndex;
            set
            {
                var prev = _selectedFoliageTypeIndex;
                if (value == prev)
                    return;

                _selectedFoliageTypeIndex = value;
                SelectedFoliageTypeIndexChanged?.Invoke(prev, value);
            }
        }

        /// <summary>
        /// The foliage types tab;
        /// </summary>
        public FoliageTypesTab FoliageTypes;

        /// <summary>
        /// The paint tab;
        /// </summary>
        public PaintTab Paint;

        /// <summary>
        /// The edit tab;
        /// </summary>
        public EditTab Edit;

        /// <summary>
        /// The foliage type model asset IDs checked to paint with them by default.
        /// </summary>
        public readonly Dictionary<Guid, bool> FoliageTypeModelIdsToPaint = new Dictionary<Guid, bool>();

        /// <summary>
        /// Initializes a new instance of the <see cref="FoliageTab"/> class.
        /// </summary>
        /// <param name="icon">The icon.</param>
        /// <param name="editor">The editor instance.</param>
        public FoliageTab(SpriteHandle icon, Editor editor)
        : base(string.Empty, icon)
        {
            Level.SceneLoaded += OnSceneLoaded;
            Editor = editor;
            Editor.SceneEditing.SelectionChanged += OnSelectionChanged;

            Selected += OnSelected;

            _modes = new Tabs
            {
                Orientation = Orientation.Vertical,
                UseScroll = true,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Vector2(50, 32),
                Parent = this
            };

            // Init tool modes
            InitSculptMode();
            InitPaintMode();
            InitEditMode();

            _modes.SelectedTabIndex = 0;

            _noFoliagePanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                BackgroundColor = Style.Current.Background,
                Parent = this
            };
            var noFoliageLabel = new Label
            {
                Text = "Select foliage to edit\nor\n\n\n\n",
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _noFoliagePanel
            };
            _createNewFoliage = new Button
            {
                Text = "Create new foliage",
                AnchorPreset = AnchorPresets.MiddleCenter,
                Offsets = new Margin(-60, 120, -12, 24),
                Parent = _noFoliagePanel,
                Enabled = false
            };
            _createNewFoliage.Clicked += OnCreateNewFoliageClicked;
        }

        private void OnSceneLoaded(Scene arg1, Guid arg2)
        {
            _createNewFoliage.Enabled = true;

            Level.SceneUnloaded += OnSceneUnloaded;
            Level.SceneLoaded -= OnSceneLoaded;
        }

        private void OnSceneUnloaded(Scene arg1, Guid arg2)
        {
            _createNewFoliage.Enabled = false;

            Level.SceneLoaded += OnSceneLoaded;
            Level.SceneUnloaded -= OnSceneUnloaded;
        }

        private void OnSelected(Tab tab)
        {
            // Auto select first foliage actor to make usage easier
            if (Editor.SceneEditing.SelectionCount == 1 && Editor.SceneEditing.Selection[0] is SceneGraph.ActorNode actorNode && actorNode.Actor is FlaxEngine.Foliage)
                return;
            var actor = Level.FindActor<FlaxEngine.Foliage>();
            if (actor)
            {
                Editor.SceneEditing.Select(actor);
            }
        }

        private void OnCreateNewFoliageClicked()
        {
            // Create
            var actor = new FlaxEngine.Foliage
            {
                StaticFlags = StaticFlags.FullyStatic,
                Name = "Foliage"
            };

            // Spawn
            Editor.SceneEditing.Spawn(actor);

            // Select
            Editor.SceneEditing.Select(actor);
        }

        private void OnSelectionChanged()
        {
            var node = Editor.SceneEditing.SelectionCount > 0 ? Editor.SceneEditing.Selection[0] as FoliageNode : null;
            var foliage = node?.Actor as FlaxEngine.Foliage;
            if (foliage != SelectedFoliage)
            {
                SelectedFoliageTypeIndex = -1;
                SelectedFoliage = foliage;
                SelectedFoliageChanged?.Invoke();
            }

            _noFoliagePanel.Visible = foliage == null;
        }

        private void InitSculptMode()
        {
            var tab = _modes.AddTab(FoliageTypes = new FoliageTypesTab(this));
            tab.Selected += OnTabSelected;
        }

        private void InitPaintMode()
        {
            var tab = _modes.AddTab(Paint = new PaintTab(this, Editor.Windows.EditWin.Viewport.PaintFoliageGizmo));
            tab.Selected += OnTabSelected;
        }

        private void InitEditMode()
        {
            var tab = _modes.AddTab(Edit = new EditTab(this, Editor.Windows.EditWin.Viewport.EditFoliageGizmo));
            tab.Selected += OnTabSelected;
        }

        /// <inheritdoc />
        public override void OnSelected()
        {
            base.OnSelected();

            UpdateGizmoMode();
        }

        private void OnTabSelected(Tab tab)
        {
            UpdateGizmoMode();
        }

        /// <summary>
        /// Updates the active viewport gizmo mode based on the current mode.
        /// </summary>
        private void UpdateGizmoMode()
        {
            switch (_modes.SelectedTabIndex)
            {
            case 0:
                Editor.Windows.EditWin.Viewport.SetActiveMode<NoGizmoMode>();
                break;
            case 1:
                Editor.Windows.EditWin.Viewport.SetActiveMode<PaintFoliageGizmoMode>();
                break;
            case 2:
                Editor.Windows.EditWin.Viewport.SetActiveMode<EditFoliageGizmoMode>();
                break;
            default: throw new IndexOutOfRangeException("Invalid foliage tab mode.");
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            FoliageTypes.CheckFoliageTypesCount();

            base.Update(deltaTime);
        }

        internal void OnSelectedFoliageTypesChanged()
        {
            SelectedFoliageTypesChanged?.Invoke();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_createNewFoliage.Enabled)
                Level.SceneUnloaded -= OnSceneUnloaded;
            else
                Level.SceneLoaded -= OnSceneLoaded;

            base.OnDestroy();
        }
    }
}
