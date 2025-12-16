using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.Viewport;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Gizmo;

public class DirectionGizmo
{
    private IGizmoOwner _owner;
    private ViewportProjection _viewportProjection;
    private EditorViewport _viewport;
    private Vector3 _gizmoCenter;
    private float _axisLength = 70.0f;
    private float _textAxisLength = 85.0f;

    private AxisData _xAxisData;
    private AxisData _yAxisData;
    private AxisData _zAxisData;
    private AxisData _negXAxisData;
    private AxisData _negYAxisData;
    private AxisData _negZAxisData;

    private List<AxisData> _axisData = new List<AxisData>();

    private SpriteHandle _posHandle;
    private SpriteHandle _negHandle;

    private FontReference _fontReference;
    
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
        
        _xAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "X", AxisColor = new Color(1.0f, 0.0f, 0.02745f, 1.0f), Negative = false };
        _yAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "Y", AxisColor = new Color(0.239215f, 1.0f, 0.047058f, 1.0f), Negative = false };
        _zAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "Z", AxisColor = new Color(0.0f, 0.0235294f, 1.0f, 1.0f), Negative = false };
        
        _negXAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-X", AxisColor = new Color(1.0f, 0.0f, 0.02745f, 1.0f), Negative = true };
        _negYAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-Y", AxisColor = new Color(0.239215f, 1.0f, 0.047058f, 1.0f), Negative = true };
        _negZAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-Z", AxisColor = new Color(0.0f, 0.0235294f, 1.0f, 1.0f), Negative = true };
        _axisData.EnsureCapacity(6);

        _posHandle = Editor.Instance.Icons.VisjectBoxClosed32;
        _negHandle = Editor.Instance.Icons.VisjectBoxOpen32;
        
        _fontReference = new FontReference(Style.Current.FontSmall);
        _fontReference.Size = 8;
    }
    
    /// <summary>
    /// Used to Draw the gizmo.
    /// </summary>
    public void Draw()
    {
        _viewportProjection.Init(_owner.Viewport);
        _gizmoCenter = _viewport.Task.View.WorldPosition + _viewport.Task.View.Direction * 1500;
        _viewportProjection.ProjectPoint(_gizmoCenter, out var gizmoCenterScreen);
        var bounds = _viewport.Bounds;
        var screenLocation = bounds.Location - new Float2(-bounds.Width * 0.5f + 50, bounds.Height * 0.5f - 50);
        var relativeCenter = gizmoCenterScreen - screenLocation;

        // Project unit vectors
        _viewportProjection.ProjectPoint(_gizmoCenter + Vector3.Right, out var xProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter + Vector3.Up, out var yProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter + Vector3.Forward, out var zProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter - Vector3.Right, out var negXProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter - Vector3.Up, out var negYProjected);
        _viewportProjection.ProjectPoint(_gizmoCenter - Vector3.Forward, out var negZProjected);

        // Normalize by viewport height to keep size independent of FOV and viewport dimensions
        float heightNormalization = _viewport.Height / 720.0f; // 720 = reference height

        Float2 xDelta = (xProjected - gizmoCenterScreen) / heightNormalization;
        Float2 yDelta = (yProjected - gizmoCenterScreen) / heightNormalization;
        Float2 zDelta = (zProjected - gizmoCenterScreen) / heightNormalization;
        Float2 negXDelta = (negXProjected - gizmoCenterScreen) / heightNormalization;
        Float2 negYDelta = (negYProjected - gizmoCenterScreen) / heightNormalization;
        Float2 negZDelta = (negZProjected - gizmoCenterScreen) / heightNormalization;

        // Calculate distances from camera to determine draw order
        Vector3 cameraPosition = _viewport.Task.View.Position;
        float xDistance = Vector3.Distance(cameraPosition, _gizmoCenter + Vector3.Right);
        float yDistance = Vector3.Distance(cameraPosition, _gizmoCenter + Vector3.Up);
        float zDistance = Vector3.Distance(cameraPosition, _gizmoCenter + Vector3.Forward);
        float negXDistance = Vector3.Distance(cameraPosition, _gizmoCenter - Vector3.Right);
        float negYDistance = Vector3.Distance(cameraPosition, _gizmoCenter - Vector3.Up);
        float negZDistance = Vector3.Distance(cameraPosition, _gizmoCenter - Vector3.Forward);

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
        _axisData.Sort( (a, b) => -a.Distance.CompareTo(b.Distance));
        
        // Draw in order from farthest to closest
        foreach (var axis in _axisData)
        {
            Float2 tipScreen = relativeCenter + axis.Delta * _axisLength;
            Float2 tipTextScreen = relativeCenter + axis.Delta * _textAxisLength;

            if (!axis.Negative)
            {
                Render2D.DrawLine(relativeCenter, tipScreen, axis.AxisColor, 2.0f);
                Render2D.DrawSprite(_posHandle, new Rectangle(tipTextScreen - new Float2(12, 12), new Float2(24, 24)), axis.AxisColor);
                var font = _fontReference.GetFont();
                Render2D.DrawText(font, axis.Label, Color.Black, tipTextScreen - font.MeasureText(axis.Label) * 0.5f);
            }
            else
            {
                Render2D.DrawSprite(_posHandle, new Rectangle(tipTextScreen - new Float2(12, 12), new Float2(24, 24)), axis.AxisColor.RGBMultiplied(0.65f));
                Render2D.DrawSprite(_negHandle, new Rectangle(tipTextScreen - new Float2(12, 12), new Float2(24, 24)), axis.AxisColor);
            }
        }
    }
}
