// Copyright (c) Wojciech Figat. All rights reserved.

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
        /// Gets or sets whether to invert the Y rotation of the mouse in the editor viewport.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("General"), EditorOrder(102), Tooltip("Whether to invert the Y rotation of the mouse in the editor viewport.")]
        public bool InvertMouseYAxisRotation { get; set; } = false;

        /// <summary>
        /// Gets or sets the total amount of steps the camera needs to go from minimum to maximum speed.
        /// </summary>
        [DefaultValue(64), Limit(1, 128)]
        [EditorDisplay("Camera"), EditorOrder(110), Tooltip("The total amount of steps the camera needs to go from minimum to maximum speed.")]
        public int TotalCameraSpeedSteps { get; set; } = 64;

        /// <summary>
        /// Gets or sets the degree to which the camera will be eased when using camera flight in the editor window.
        /// </summary>
        [DefaultValue(3.0f), Limit(1.0f, 8.0f)]
        [EditorDisplay("Camera"), EditorOrder(111), Tooltip("The degree to which the camera will be eased when using camera flight in the editor window (ignored if camera easing degree is enabled).")]
        public float CameraEasingDegree { get; set; } = 3.0f;

        /// <summary>
        /// Gets or sets the default movement speed for the viewport camera (must be in range between minimum and maximum movement speed values).
        /// </summary>
        [DefaultValue(1.0f), Limit(0.05f, 32.0f)]
        [EditorDisplay("Defaults"), EditorOrder(120), Tooltip("The default movement speed for the viewport camera (must be in range between minimum and maximum movement speed values).")]
        public float MovementSpeed { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the default minimum camera movement speed.
        /// </summary>
        [DefaultValue(0.05f), Limit(0.05f, 32.0f)]
        [EditorDisplay("Defaults"), EditorOrder(121), Tooltip("The default minimum movement speed for the viewport camera.")]
        public float MinMovementSpeed { get; set; } = 0.05f;

        /// <summary>
        /// Gets or sets the default maximum camera movement speed.
        /// </summary>
        [DefaultValue(32.0f), Limit(16.0f, 1000.0f)]
        [EditorDisplay("Defaults"), EditorOrder(122), Tooltip("The default maximum movement speed for the viewport camera.")]
        public float MaxMovementSpeed { get; set; } = 32f;

        /// <summary>
        /// Gets or sets the default camera easing mode.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Defaults"), EditorOrder(130), Tooltip("The default camera easing mode.")]
        public bool UseCameraEasing { get; set; } = true;

        /// <summary>
        /// Gets or sets the default near clipping plane distance for the viewport camera.
        /// </summary>
        [DefaultValue(10.0f), Limit(0.001f, 1000.0f)]
        [EditorDisplay("Defaults"), EditorOrder(140), Tooltip("The default near clipping plane distance for the viewport camera.")]
        public float NearPlane { get; set; } = 10.0f;

        /// <summary>
        /// Gets or sets the default far clipping plane distance for the viewport camera.
        /// </summary>
        [DefaultValue(40000.0f), Limit(10.0f)]
        [EditorDisplay("Defaults"), EditorOrder(150), Tooltip("The default far clipping plane distance for the viewport camera.")]
        public float FarPlane { get; set; } = 40000.0f;

        /// <summary>
        /// Gets or sets the default field of view angle (in degrees) for the viewport camera.
        /// </summary>
        [DefaultValue(60.0f), Limit(35.0f, 160.0f, 0.1f)]
        [EditorDisplay("Defaults"), EditorOrder(160), Tooltip("The default field of view angle (in degrees) for the viewport camera.")]
        public float FieldOfView { get; set; } = 60.0f;

        /// <summary>
        /// Gets or sets the default camera orthographic mode.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Defaults"), EditorOrder(170), Tooltip("The default camera orthographic mode.")]
        public bool UseOrthographicProjection { get; set; } = false;

        /// <summary>
        /// Gets or sets the default camera orthographic scale (if camera uses orthographic mode).
        /// </summary>
        [DefaultValue(5.0f), Limit(0.001f, 100000.0f, 0.1f)]
        [EditorDisplay("Defaults"), EditorOrder(180), Tooltip("The default camera orthographic scale (if camera uses orthographic mode).")]
        public float OrthographicScale { get; set; } = 5.0f;

        /// <summary>
        /// Gets or sets the default panning direction for the viewport camera.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Defaults"), EditorOrder(190), Tooltip("The default panning direction for the viewport camera.")]
        public bool InvertPanning { get; set; } = false;

        /// <summary>
        /// Gets or sets the default relative panning mode.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Defaults"), EditorOrder(200), Tooltip("The default relative panning mode. Uses distance between camera and target to determine panning speed.")]
        public bool UseRelativePanning { get; set; } = true;

        /// <summary>
        /// Gets or sets the default panning speed (ignored if relative panning is speed enabled).
        /// </summary>
        [DefaultValue(0.8f), Limit(0.01f, 128.0f, 0.1f)]
        [EditorDisplay("Defaults"), EditorOrder(210), Tooltip("The default camera panning speed (ignored if relative panning is enabled).")]
        public float PanningSpeed { get; set; } = 0.8f;

        /// <summary>
        /// Gets or sets the default editor viewport grid scale.
        /// </summary>
        [DefaultValue(50.0f), Limit(25.0f, 500.0f, 5.0f)]
        [EditorDisplay("Defaults"), EditorOrder(220), Tooltip("The default editor viewport grid scale.")]
        public float ViewportGridScale { get; set; } = 50.0f;

        /// <summary>
        /// Gets or sets the view distance you can see the grid.
        /// </summary>
        [DefaultValue(2500.0f)]
        [EditorDisplay("Grid"), EditorOrder(300), Tooltip("The maximum distance you will be able to see the grid.")]
        public float ViewportGridViewDistance { get; set; } = 2500.0f;

        /// <summary>
        /// Gets or sets the grid color.
        /// </summary>
        [DefaultValue(typeof(Color), "0.5,0.5,0.5,1.0")]
        [EditorDisplay("Grid"), EditorOrder(310), Tooltip("The color for the viewport grid.")]
        public Color ViewportGridColor { get; set; } = new Color(0.5f, 0.5f, 0.5f, 1.0f);
    }
}
