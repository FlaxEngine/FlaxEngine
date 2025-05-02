// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Carve tab related to terrain painting. Allows to modify terrain splatmap using brush.
    /// </summary>
    /// <seealso cref="Tab" />
    [HideInEditor]
    public class PaintTab : Tab
    {
        /// <summary>
        /// The object for paint mode settings adjusting via Custom Editor.
        /// </summary>
        private sealed class ProxyObject
        {
            private readonly PaintTerrainGizmoMode _mode;
            private object _currentMode, _currentBrush;

            public ProxyObject(PaintTerrainGizmoMode mode)
            {
                _mode = mode;
                SyncData();
            }

            public void SyncData()
            {
                _currentMode = _mode.CurrentMode;
                _currentBrush = _mode.CurrentBrush;
            }

            [EditorOrder(0), EditorDisplay("Tool"), Tooltip("Paint tool mode to use.")]
            public PaintTerrainGizmoMode.ModeTypes ToolMode
            {
                get => _mode.ToolModeType;
                set => _mode.ToolModeType = value;
            }

            [EditorOrder(100), EditorDisplay("Tool", EditorDisplayAttribute.InlineStyle)]
            public object Mode
            {
                get => _currentMode;
                set { }
            }

            [EditorOrder(1000), EditorDisplay("Brush"), Tooltip("Paint brush type to use.")]
            public PaintTerrainGizmoMode.BrushTypes BrushTypeType
            {
                get => _mode.ToolBrushType;
                set => _mode.ToolBrushType = value;
            }

            [EditorOrder(1100), EditorDisplay("Brush", EditorDisplayAttribute.InlineStyle)]
            public object Brush
            {
                get => _currentBrush;
                set { }
            }
        }

        private readonly ProxyObject _proxy;
        private readonly CustomEditorPresenter _presenter;

        /// <summary>
        /// The parent carve tab.
        /// </summary>
        public readonly CarveTab CarveTab;

        /// <summary>
        /// The related sculp terrain gizmo.
        /// </summary>
        public readonly PaintTerrainGizmoMode Gizmo;

        /// <summary>
        /// Initializes a new instance of the <see cref="PaintTab"/> class.
        /// </summary>
        /// <param name="tab">The parent tab.</param>
        /// <param name="gizmo">The related gizmo.</param>
        public PaintTab(CarveTab tab, PaintTerrainGizmoMode gizmo)
        : base("Paint")
        {
            CarveTab = tab;
            Gizmo = gizmo;
            Gizmo.ToolModeChanged += OnToolModeChanged;
            _proxy = new ProxyObject(gizmo);

            // Main panel
            var panel = new Panel(ScrollBars.Both)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };

            // Options editor
            // TODO: use editor undo for changing brush options
            var editor = new CustomEditorPresenter(null);
            editor.Panel.Parent = panel;
            editor.Select(_proxy);
            _presenter = editor;
        }

        private void OnToolModeChanged()
        {
            _presenter.BuildLayoutOnUpdate();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (_presenter.BuildOnUpdate)
            {
                _proxy.SyncData();
            }

            base.Update(deltaTime);
        }
    }
}
