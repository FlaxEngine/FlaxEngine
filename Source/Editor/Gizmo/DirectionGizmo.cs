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
    private float _axisLength = 75.0f;
    private float _textAxisLength = 95.0f;
    private float _spriteRadius = 12.0f;

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
        _zAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "Z", AxisColor = new Color(0.0f, 0.0235294f, 1.0f, 1.0f), Negative = false, Direction = AxisDirection.PosZ };
        
        _negXAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-X", AxisColor = new Color(1.0f, 0.0f, 0.02745f, 1.0f), Negative = true, Direction = AxisDirection.NegX };
        _negYAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-Y", AxisColor = new Color(0.239215f, 1.0f, 0.047058f, 1.0f), Negative = true, Direction = AxisDirection.NegY };
        _negZAxisData = new AxisData { Delta = new Float2(0, 0), Distance = 0, Label = "-Z", AxisColor = new Color(0.0f, 0.0235294f, 1.0f, 1.0f), Negative = true, Direction = AxisDirection.NegZ };
        _axisData.EnsureCapacity(6);

        _posHandle = Editor.Instance.Icons.VisjectBoxClosed32;
        _negHandle = Editor.Instance.Icons.VisjectBoxOpen32;
        
        _fontReference = new FontReference(Style.Current.FontSmall);
        _fontReference.Size = 8;
    }

    private bool IsPointInSprite(Float2 point, Float2 spriteCenter)
    {
        Float2 delta = point - spriteCenter;
        float distanceSq = delta.LengthSquared;
        float radiusSq = _spriteRadius * _spriteRadius;
        return distanceSq <= radiusSq;
    }

    /// <summary>
    /// Updates the gizmo for mouse interactions (hover and click detection).
    /// </summary>
    public void Update(Float2 mousePos)
    {
        _hoveredAxisIndex = -1;

        // Check which axis is being hovered - check from end (closest) to start (farthest) for proper layering
        for (int i = _spritePositions.Count - 1; i >= 0; i--)
        {
            if (IsPointInSprite(mousePos, _spritePositions[i].position))
            {
                _hoveredAxisIndex = i;
                break;
            }
        }
    }
    
    /// <summary>
    /// Handles mouse click on direction gizmo sprites.
    /// </summary>
    public bool OnMouseDown(Float2 mousePos)
    {
        // Check which axis is being clicked - check from end (closest) to start (farthest) for proper layering
        for (int i = _spritePositions.Count - 1; i >= 0; i--)
        {
            if (IsPointInSprite(mousePos, _spritePositions[i].position))
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
            AxisDirection.PosX => Quaternion.Euler(0, 90, 0),
            AxisDirection.NegX => Quaternion.Euler(0, -90, 0),
            AxisDirection.PosY => Quaternion.Euler(-90, 0, 0),
            AxisDirection.NegY => Quaternion.Euler(90, 0, 0),
            AxisDirection.PosZ => Quaternion.Euler(0, 0, 0),
            AxisDirection.NegZ => Quaternion.Euler(0, 180, 0),
            _ => Quaternion.Identity
        };

        _viewport.ViewOrientation = orientation;
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

        // Rebuild sprite positions list for hover detection
        _spritePositions.Clear();
        
        // Draw in order from farthest to closest
        for (int i = 0; i < _axisData.Count; i++)
        {
            var axis = _axisData[i];
            Float2 tipScreen = relativeCenter + axis.Delta * _axisLength;
            Float2 tipTextScreen = relativeCenter + axis.Delta * _textAxisLength;
            bool isHovered = _hoveredAxisIndex == i;

            // Store sprite position for hover detection
            _spritePositions.Add((tipTextScreen, axis.Direction));

            if (!axis.Negative)
            {
                Render2D.DrawLine(relativeCenter, tipScreen, axis.AxisColor, 2.0f);
                Render2D.DrawSprite(_posHandle, new Rectangle(tipTextScreen - new Float2(12, 12), new Float2(24, 24)), axis.AxisColor);
                
                var font = _fontReference.GetFont();
                Color textColor = isHovered ? Color.White : Color.Black;
                Render2D.DrawText(font, axis.Label, textColor, tipTextScreen - font.MeasureText(axis.Label) * 0.5f);
            }
            else
            {
                Render2D.DrawSprite(_posHandle, new Rectangle(tipTextScreen - new Float2(12, 12), new Float2(24, 24)), axis.AxisColor.RGBMultiplied(0.65f));
                Render2D.DrawSprite(_negHandle, new Rectangle(tipTextScreen - new Float2(12, 12), new Float2(24, 24)), axis.AxisColor);
                
                // Draw white label text on hover for negative axes
                if (isHovered)
                {
                    var font = _fontReference.GetFont();
                    Render2D.DrawText(font, axis.Label, Color.White, tipTextScreen - font.MeasureText(axis.Label) * 0.5f);
                }
            }
        }
    }
}
