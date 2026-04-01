// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Options;
using FlaxEditor.Viewport;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Gizmo;

[HideInEditor]
internal class DirectionGizmo : ContainerControl
{
    public const float DefaultGizmoSize = 112.5f;

    private const float AxisLength = 71.25f;
    private const float SpriteRadius = 10.85f;

    private IGizmoOwner _owner;
    private ViewportProjection _viewportProjection;
    private EditorViewport _viewport;
    private Vector3 _gizmoCenter;
    private float _gizmoBrightness;
    private float _gizmoOpacity;
    private float _backgroundOpacity;
    private float _axisLength;
    private float _spriteRadius;

    private AxisData _xAxisData;
    private AxisData _yAxisData;
    private AxisData _zAxisData;
    private AxisData _negXAxisData;
    private AxisData _negYAxisData;
    private AxisData _negZAxisData;

    private List<AxisData> _axisData = new List<AxisData>();
    private int _hoveredAxisIndex = -1;

    private SpriteHandle _posHandle;
    private SpriteHandle _negHandle;

    private FontReference _fontReference;

    // Store sprite positions for hover detection
    private List<(Float2 position, AxisDirection direction)> _spritePositions = new List<(Float2, AxisDirection)>();

    private struct ViewportProjection
    {
        private Matrix _viewProjection;
        private BoundingFrustum _frustum;
        private FlaxEngine.Viewport _viewport;
        private Vector3 _origin;

        public void Init(EditorViewport editorViewport)
        {
            // Inline EditorViewport.ProjectPoint to save on calculation for large set of points
            _viewport = new FlaxEngine.Viewport(0, 0, editorViewport.Width, editorViewport.Height);
            _frustum = editorViewport.ViewFrustum;
            _viewProjection = _frustum.Matrix;
            _origin = editorViewport.Task.View.Origin;
        }

        public void ProjectPoint(Vector3 worldSpaceLocation, out Float2 viewportSpaceLocation)
        {
            worldSpaceLocation -= _origin;
            _viewport.Project(ref worldSpaceLocation, ref _viewProjection, out var projected);
            viewportSpaceLocation = new Float2((float)projected.X, (float)projected.Y);
        }
    }

    private struct AxisData
    {
        public Float2 Delta;
        public float Distance;
        public string Label;
        public Color AxisColor;
        public bool Negative;
        public AxisDirection Direction;
    }

    private enum AxisDirection
    {
        PosX,
        PosY,
        PosZ,
        NegX,
        NegY,
        NegZ
    }

    /// <summary>
    /// Constructor of the Direction Gizmo
    /// </summary>
    /// <param name="owner">The owner of this object.</param>
    public DirectionGizmo(IGizmoOwner owner)
    {
        _owner = owner;
        _viewport = owner.Viewport;
        _viewportProjection.Init(owner.Viewport);

        _xAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "X", AxisColor = new Color(1.0f, 0.0f, 0.02745f, 1.0f), Negative = false, Direction = AxisDirection.PosX };
        _yAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "Y", AxisColor = new Color(0.239215f, 1.0f, 0.047058f, 1.0f), Negative = false, Direction = AxisDirection.PosY };
        _zAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "Z", AxisColor = new Color(0.0f, 0.3607f, 0.9f, 1.0f), Negative = false, Direction = AxisDirection.PosZ };

        _negXAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-X", AxisColor = new Color(1.0f, 0.0f, 0.02745f, 1.0f), Negative = true, Direction = AxisDirection.NegX };
        _negYAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-Y", AxisColor = new Color(0.239215f, 1.0f, 0.047058f, 1.0f), Negative = true, Direction = AxisDirection.NegY };
        _negZAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-Z", AxisColor = new Color(0.0f, 0.3607f, 0.9f, 1.0f), Negative = true, Direction = AxisDirection.NegZ };
        _axisData.EnsureCapacity(6);
        _spritePositions.EnsureCapacity(6);

        var editor = Editor.Instance;
        _posHandle = editor.Icons.VisjectBoxClosed32;
        _negHandle = editor.Icons.VisjectBoxOpen32;

        _fontReference = new FontReference(Style.Current.FontSmall);

        editor.Options.OptionsChanged += OnEditorOptionsChanged;
        OnEditorOptionsChanged(editor.Options.Options);
    }

    private void OnEditorOptionsChanged(EditorOptions options)
    {
        float gizmoScale = options.Viewport.DirectionGizmoScale;
        _axisLength = AxisLength * gizmoScale;
        _spriteRadius = SpriteRadius * gizmoScale;
        _gizmoBrightness = options.Viewport.DirectionGizmoBrightness;
        _gizmoOpacity = options.Viewport.DirectionGizmoOpacity;
        _backgroundOpacity = options.Viewport.DirectionGizmoBackgroundOpacity;

        _fontReference.Size = 8.25f * gizmoScale;
    }

    private bool IsPointInSprite(Float2 point, Float2 spriteCenter)
    {
        Float2 delta = point - spriteCenter;
        float distanceSq = delta.LengthSquared;
        float radiusSq = _spriteRadius * _spriteRadius;
        return distanceSq <= radiusSq;
    }

    /// <inheritdoc />
    public override void OnMouseMove(Float2 location)
    {
        _hoveredAxisIndex = -1;

        // Check which axis is being hovered - check from closest to farthest for proper layering
        for (int i = _spritePositions.Count - 1; i >= 0; i--)
        {
            if (IsPointInSprite(location, _spritePositions[i].position))
            {
                _hoveredAxisIndex = i;
                break;
            }
        }

        base.OnMouseMove(location);
    }

    /// <inheritdoc />
    public override bool OnMouseUp(Float2 location, MouseButton button)
    {
        if (base.OnMouseUp(location, button))
            return true;

        // Check which axis is being clicked - check from closest to farthest for proper layering
        for (int i = _spritePositions.Count - 1; i >= 0; i--)
        {
            if (IsPointInSprite(location, _spritePositions[i].position))
            {
                OrientViewToAxis(_spritePositions[i].direction);
                return true;
            }
        }

        return false;
    }

    private void OrientViewToAxis(AxisDirection direction)
    {
        Quaternion orientation = direction switch
        {
            AxisDirection.PosX => Quaternion.Euler(0, -90, 0),
            AxisDirection.NegX => Quaternion.Euler(0, 90, 0),
            AxisDirection.PosY => Quaternion.Euler(90, 0, 0),
            AxisDirection.NegY => Quaternion.Euler(-90, 0, 0),
            AxisDirection.PosZ => Quaternion.Euler(0, 180, 0),
            AxisDirection.NegZ => Quaternion.Euler(0, 0, 0),
            _ => Quaternion.Identity
        };
        _viewport.OrientViewport(ref orientation);
    }

    /// <summary>
    /// Used to Draw the gizmo.
    /// </summary>
    public override void DrawSelf()
    {
        base.DrawSelf();

        var features = Render2D.Features;
        Render2D.Features = features & ~Render2D.RenderingFeatures.VertexSnapping;
        _viewportProjection.Init(_owner.Viewport);
        _gizmoCenter = _viewport.Task.View.WorldPosition + _viewport.Task.View.Direction * 1500;
        _viewportProjection.ProjectPoint(_gizmoCenter, out var gizmoCenterScreen);

        var relativeCenter = Size * 0.5f;

        // Project unit vectors
        _viewportProjection.ProjectPoint(_gizmoCenter + Vector3.Right, out var xProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter + Vector3.Up, out var yProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter + Vector3.Forward, out var zProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter - Vector3.Right, out var negXProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter - Vector3.Up, out var negYProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter - Vector3.Forward, out var negZProjected);

        // Normalize by viewport height to keep size independent of FOV and viewport dimensions
        float heightNormalization = _viewport.Height / 720.0f; // 720 = reference height

        // Fix in axes distance no matter FOV/OrthoScale to keep consistent size regardless of zoom level
        if (_owner.Viewport.UseOrthographicProjection)
            heightNormalization /= _owner.Viewport.OrthographicScale * 0.5f;
        else
        {
            // This could be some actual math expression, not that hack
            var fov = _owner.Viewport.FieldOfView / 60.0f;
            float scaleAt30 = 0.1f, scaleAt60 = 1.0f, scaleAt120 = 1.5f, scaleAt180 = 3.0f;
            heightNormalization /= Mathf.Lerp(scaleAt30, scaleAt60, fov);
            heightNormalization /= Mathf.Lerp(scaleAt60, scaleAt120, Mathf.Saturate(fov - 1));
            heightNormalization /= Mathf.Lerp(scaleAt60, scaleAt180, Mathf.Saturate(fov - 2));
        }

        Float2 xDelta = (xProjected - gizmoCenterScreen) / heightNormalization;
        Float2 yDelta = (yProjected - gizmoCenterScreen) / heightNormalization;
        Float2 zDelta = (zProjected - gizmoCenterScreen) / heightNormalization;
        Float2 negXDelta = (negXProjected - gizmoCenterScreen) / heightNormalization;
        Float2 negYDelta = (negYProjected - gizmoCenterScreen) / heightNormalization;
        Float2 negZDelta = (negZProjected - gizmoCenterScreen) / heightNormalization;

        // Calculate distances from camera to determine draw order
        Vector3 cameraPosition = _viewport.Task.View.Position;
        float xDistance = (float)Vector3.Distance(cameraPosition, _gizmoCenter + Vector3.Right);
        float yDistance = (float)Vector3.Distance(cameraPosition, _gizmoCenter + Vector3.Up);
        float zDistance = (float)Vector3.Distance(cameraPosition, _gizmoCenter + Vector3.Forward);
        float negXDistance = (float)Vector3.Distance(cameraPosition, _gizmoCenter - Vector3.Right);
        float negYDistance = (float)Vector3.Distance(cameraPosition, _gizmoCenter - Vector3.Up);
        float negZDistance = (float)Vector3.Distance(cameraPosition, _gizmoCenter - Vector3.Forward);

        _xAxisData.Delta = xDelta;
        _xAxisData.Distance = xDistance;
        _yAxisData.Delta = yDelta;
        _yAxisData.Distance = yDistance;
        _zAxisData.Delta = zDelta;
        _zAxisData.Distance = zDistance;
        _negXAxisData.Delta = negXDelta;
        _negXAxisData.Distance = negXDistance;
        _negYAxisData.Delta = negYDelta;
        _negYAxisData.Distance = negYDistance;
        _negZAxisData.Delta = negZDelta;
        _negZAxisData.Distance = negZDistance;

        // Sort for correct draw order.
        _axisData.Clear();
        _axisData.AddRange([_xAxisData, _yAxisData, _zAxisData, _negXAxisData, _negYAxisData, _negZAxisData]);
        _axisData.Sort((a, b) => -a.Distance.CompareTo(b.Distance));

        // Rebuild sprite positions list for hover detection
        _spritePositions.Clear();

        Render2D.DrawSprite(_posHandle, new Rectangle(0, 0, Size), Color.Black.AlphaMultiplied(_backgroundOpacity));

        // Draw in order from farthest to closest
        for (int i = 0; i < _axisData.Count; i++)
        {
            var axis = _axisData[i];
            Float2 tipScreen = relativeCenter + axis.Delta * _axisLength;
            bool isHovered = _hoveredAxisIndex == i;

            // Store sprite position for hover detection
            _spritePositions.Add((tipScreen, axis.Direction));

            var axisColor = isHovered ? new Color(1.0f, 0.8980392f, 0.039215688f) : axis.AxisColor;
            axisColor = axisColor.RGBMultiplied(_gizmoBrightness).AlphaMultiplied(_gizmoOpacity);
            var font = _fontReference.GetFont();
            if (!axis.Negative)
            {
                Render2D.DrawLine(relativeCenter, tipScreen, axisColor, 1.5f);
                Render2D.DrawSprite(_posHandle, new Rectangle(tipScreen - new Float2(_spriteRadius), new Float2(_spriteRadius * 2)), axisColor);
                Render2D.DrawText(font, axis.Label, Color.Black, tipScreen - font.MeasureText(axis.Label) * 0.5f);
            }
            else
            {
                Render2D.DrawSprite(_posHandle, new Rectangle(tipScreen - new Float2(_spriteRadius), new Float2(_spriteRadius * 2)), axisColor.RGBMultiplied(0.85f).AlphaMultiplied(0.8f));
                if (isHovered)
                    Render2D.DrawText(font, axis.Label, Color.Black, tipScreen - font.MeasureText(axis.Label) * 0.5f);
            }
        }

        Render2D.Features = features;
    }
}
