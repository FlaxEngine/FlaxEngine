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
        /// Gets or sets the pivot color for the UI Gizmo.
        /// </summary>
        [DefaultValue(typeof(Color), "0.0,0.5725,0.8,0.5")]
        [EditorDisplay("UI Gizmo", "Pivot Color"), EditorOrder(103), Tooltip("The color of the pivot for the UI Gizmo.")]
        public Color UIPivotColor { get; set; } = new Color(0.0f, 0.5725f, 0.8f, 0.5f);

        /// <summary>
        /// Gets or sets the anchor color for the UI Gizmo.
        /// </summary>
        [DefaultValue(typeof(Color), "0.8392,0.8471,0.8706,0.5")]
        [EditorDisplay("UI Gizmo", "Anchor Color"), EditorOrder(103), Tooltip("The color of the anchors for the UI Gizmo.")]
        public Color UIAnchorColor { get; set; } = new Color(0.8392f, 0.8471f, 0.8706f, 0.5f);

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
        /// Gets or set a value indicating how bright the transform gizmo is. Value over 1 will result in the gizmo emitting light.
        /// </summary>
        [DefaultValue(1f), Range(0f, 5f)]
        [EditorDisplay("Transform Gizmo", "Gizmo Brightness"), EditorOrder(210)]
        public float TransformGizmoBrightness { get; set; } = 1f;

        /// <summary>
        /// Gets or set a value indicating the opacity of the transform gizmo.
        /// </summary>
        [DefaultValue(1f), Range(0f, 1f)]
        [EditorDisplay("Transform Gizmo", "Gizmo Opacity"), EditorOrder(211)]
        public float TransformGizmoOpacity { get; set; } = 1f;

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
