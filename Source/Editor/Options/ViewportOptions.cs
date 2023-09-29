// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEngine;

namespace FlaxEditor.Options
{
    /// <summary>
    /// Editor viewport options data container.
    /// </summary>
    [CustomEditor(typeof(Editor<ViewportOptions>))]
    public class ViewportOptions
    {
        /// <summary>
        /// Gets or sets the mouse movement sensitivity scale applied when using the viewport camera.
        /// </summary>
        [DefaultValue(1.0f), Limit(0.01f, 100.0f)]
        [EditorDisplay("General"), EditorOrder(100), Tooltip("The mouse movement sensitivity scale applied when using the viewport camera.")]
        public float MouseSensitivity { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the mouse wheel sensitivity applied to zoom in orthographic mode.
        /// </summary>
        [DefaultValue(1.0f), Limit(0.01f, 100.0f)]
        [EditorDisplay("General"), EditorOrder(101), Tooltip("The mouse wheel sensitivity applied to zoom in orthographic mode.")]
        public float MouseWheelSensitivity { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the default movement speed for the viewport camera (must match the dropdown menu values in the viewport).
        /// </summary>
        [DefaultValue(1.0f), Limit(0.01f, 100.0f)]
        [EditorDisplay("Defaults"), EditorOrder(110), Tooltip("The default movement speed for the viewport camera (must match the dropdown menu values in the viewport).")]
        public float DefaultMovementSpeed { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the default near clipping plane distance for the viewport camera.
        /// </summary>
        [DefaultValue(10.0f), Limit(0.001f, 1000.0f)]
        [EditorDisplay("Defaults"), EditorOrder(120), Tooltip("The default near clipping plane distance for the viewport camera.")]
        public float DefaultNearPlane { get; set; } = 10.0f;

        /// <summary>
        /// Gets or sets the default far clipping plane distance for the viewport camera.
        /// </summary>
        [DefaultValue(40000.0f), Limit(10.0f)]
        [EditorDisplay("Defaults"), EditorOrder(130), Tooltip("The default far clipping plane distance for the viewport camera.")]
        public float DefaultFarPlane { get; set; } = 40000.0f;

        /// <summary>
        /// Gets or sets the default field of view angle (in degrees) for the viewport camera.
        /// </summary>
        [DefaultValue(60.0f), Limit(35.0f, 160.0f, 0.1f)]
        [EditorDisplay("Defaults", "Default Field Of View"), EditorOrder(140), Tooltip("The default field of view angle (in degrees) for the viewport camera.")]
        public float DefaultFieldOfView { get; set; } = 60.0f;

        /// <summary>
        /// Gets or sets if the panning direction is inverted for the viewport camera.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Defaults"), EditorOrder(150), Tooltip("Invert the panning direction for the viewport camera.")]
        public bool DefaultInvertPanning { get; set; } = false;

        /// <summary>
        /// Scales editor viewport grid.
        /// </summary>
        [DefaultValue(50.0f), Limit(25.0f, 500.0f, 5.0f)]
        [EditorDisplay("Defaults"), EditorOrder(160), Tooltip("Scales editor viewport grid.")]
        public float ViewportGridScale { get; set; } = 50.0f;

        /// <summary>
        /// Spawn new actors on front of EditGame viewport when enabled if the Actor don' have a parent.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Defaults"), EditorOrder(170)]
        [Tooltip("If enabled, when create an Actor without parent it set actor position to front of your view, but when disable create actor on scene position.")]
        public bool CreateActorsOnFrontOfView { get; set; } = true;
    }
}
