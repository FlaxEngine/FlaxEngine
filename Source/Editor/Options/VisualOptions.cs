// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEngine;

namespace FlaxEditor.Options
{
    /// <summary>
    /// Visual editor options data container.
    /// </summary>
    [CustomEditor(typeof(Editor<VisualOptions>))]
    public sealed class VisualOptions
    {
        /// <summary>
        /// Gets or sets the selection outline enable flag.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Gizmo"), EditorOrder(99), Tooltip("If checked, the selection outline will be visible.")]
        public bool ShowSelectionOutline { get; set; } = true;

        /// <summary>
        /// Gets or sets the first outline color.
        /// </summary>
        [DefaultValue(typeof(Color), "0.039,0.827,0.156")]
        [EditorDisplay("Transform Gizmo"), EditorOrder(100), Tooltip("The first color of the selection outline gradient.")]
        public Color SelectionOutlineColor0 { get; set; } = new Color(0.039f, 0.827f, 0.156f);

        /// <summary>
        /// Gets or sets the second outline color.
        /// </summary>
        [DefaultValue(typeof(Color), "0.019,0.615,0.101")]
        [EditorDisplay("Transform Gizmo"), EditorOrder(101), Tooltip("The second color of the selection outline gradient.")]
        public Color SelectionOutlineColor1 { get; set; } = new Color(0.019f, 0.615f, 0.101f);

        /// <summary>
        /// Gets or sets the selection outline size for UI controls.
        /// </summary>
        [DefaultValue(2.0f)]
        [EditorDisplay("UI Gizmo", "UI Control Outline Size"), EditorOrder(103), Tooltip("The size of the selection outline for UI controls.")]
        public float UISelectionOutlineSize { get; set; } = 2.0f;

        /// <summary>
        /// Gets or sets the transform gizmo size.
        /// </summary>
        [DefaultValue(1.0f), Limit(0.01f, 100.0f, 0.01f)]
        [EditorDisplay("Transform Gizmo"), EditorOrder(110), Tooltip("The transform gizmo size.")]
        public float GizmoSize { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the color used to highlight selected meshes and CSG surfaces.
        /// </summary>
        [DefaultValue(typeof(Color), "0.0,0.533,1.0,1.0")]
        [EditorDisplay("Transform Gizmo"), EditorOrder(200), Tooltip("The color used to highlight selected meshes and CSG surfaces.")]
        public Color HighlightColor { get; set; } = new Color(0.0f, 0.533f, 1.0f, 1.0f);

        /// <summary>
        /// Gets or sets a value indicating whether enable MSAA for DebugDraw primitives rendering. Helps with pixel aliasing but reduces performance.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Quality", "Enable MSAA For Debug Draw"), EditorOrder(500), Tooltip("Determines whether enable MSAA for DebugDraw primitives rendering. Helps with pixel aliasing but reduces performance.")]
        public bool EnableMSAAForDebugDraw { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether show looping particle effects in Editor viewport to simulate in-game look.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Preview"), EditorOrder(1000)]
        public bool EnableParticlesPreview { get; set; } = true;
    }
}
