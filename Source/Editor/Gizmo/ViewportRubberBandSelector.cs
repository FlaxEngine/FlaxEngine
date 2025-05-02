// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
using FlaxEngine.GUI;

namespace FlaxEngine.Gizmo;

/// <summary>
/// Class for adding viewport rubber band selection.
/// </summary>
public sealed class ViewportRubberBandSelector
{
    private bool _isMosueCaptured;
    private bool _isRubberBandSpanning;
    private bool _tryStartRubberBand;
    private Float2 _cachedStartingMousePosition;
    private Rectangle _rubberBandRect;
    private Rectangle _lastRubberBandRect;
    private List<ActorNode> _nodesCache;
    private List<SceneGraphNode> _hitsCache;
    private IGizmoOwner _owner;

    /// <summary>
    /// Constructs a rubber band selector with a designated gizmo owner.
    /// </summary>
    /// <param name="owner">The gizmo owner.</param>
    public ViewportRubberBandSelector(IGizmoOwner owner)
    {
        _owner = owner;
    }

    /// <summary>
    /// Triggers the start of a rubber band selection.
    /// </summary>
    /// <returns>True if selection started, otherwise false.</returns>
    public bool TryStartingRubberBandSelection()
    {
        if (!_isRubberBandSpanning && _owner.Gizmos.Active != null && !_owner.Gizmos.Active.IsControllingMouse && !_owner.IsRightMouseButtonDown)
        {
            _tryStartRubberBand = true;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Release the rubber band selection.
    /// </summary>
    /// <returns>Returns true if rubber band is currently spanning</returns>
    public bool ReleaseRubberBandSelection()
    {
        if (_isMosueCaptured)
        {
            _isMosueCaptured = false;
            _owner.Viewport.EndMouseCapture();
        }
        if (_tryStartRubberBand)
        {
            _tryStartRubberBand = false;
        }
        if (_isRubberBandSpanning)
        {
            _isRubberBandSpanning = false;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Tries to create a rubber band selection.
    /// </summary>
    /// <param name="canStart">Whether the creation can start.</param>
    /// <param name="mousePosition">The current mouse position.</param>
    public void TryCreateRubberBand(bool canStart, Float2 mousePosition)
    {
        if (_isRubberBandSpanning && !canStart)
        {
            _isRubberBandSpanning = false;
            return;
        }

        if (_tryStartRubberBand && (Mathf.Abs(_owner.MouseDelta.X) > 0.1f || Mathf.Abs(_owner.MouseDelta.Y) > 0.1f) && canStart)
        {
            _isRubberBandSpanning = true;
            _cachedStartingMousePosition = mousePosition;
            _rubberBandRect = new Rectangle(_cachedStartingMousePosition, Float2.Zero);
            _tryStartRubberBand = false;
        }
        else if (_isRubberBandSpanning && _owner.Gizmos.Active != null && !_owner.Gizmos.Active.IsControllingMouse && !_owner.IsRightMouseButtonDown)
        {
            _rubberBandRect.Width = mousePosition.X - _cachedStartingMousePosition.X;
            _rubberBandRect.Height = mousePosition.Y - _cachedStartingMousePosition.Y;
            if (_lastRubberBandRect != _rubberBandRect)
            {
                if (!_isMosueCaptured)
                {
                    _isMosueCaptured = true;
                    _owner.Viewport.StartMouseCapture();
                }
                UpdateRubberBand();
            }
        }
    }

    private struct ViewportProjection
    {
        private Matrix _viewProjection;
        private BoundingFrustum _frustum;
        private Viewport _viewport;
        private Vector3 _origin;

        public void Init(EditorViewport editorViewport)
        {
            // Inline EditorViewport.ProjectPoint to save on calculation for large set of points
            _viewport = new Viewport(0, 0, editorViewport.Width, editorViewport.Height);
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

        public ContainmentType FrustumCull(ref BoundingBox bounds)
        {
            bounds.Minimum -= _origin;
            bounds.Maximum -= _origin;
            return _frustum.Contains(ref bounds);
        }
    }

    private void UpdateRubberBand()
    {
        Profiler.BeginEvent("UpdateRubberBand");

        // Select rubberbanded rect actor nodes
        var adjustedRect = _rubberBandRect;
        _lastRubberBandRect = _rubberBandRect;
        if (adjustedRect.Width < 0 || adjustedRect.Height < 0)
        {
            // Make sure we have a well-formed rectangle i.e. size is positive and X/Y is upper left corner
            var size = adjustedRect.Size;
            adjustedRect.X = Mathf.Min(adjustedRect.X, adjustedRect.X + adjustedRect.Width);
            adjustedRect.Y = Mathf.Min(adjustedRect.Y, adjustedRect.Y + adjustedRect.Height);
            size.X = Mathf.Abs(size.X);
            size.Y = Mathf.Abs(size.Y);
            adjustedRect.Size = size;
        }

        // Get hits from graph nodes
        if (_nodesCache == null)
            _nodesCache = new List<ActorNode>();
        else
            _nodesCache.Clear();
        var nodes = _nodesCache;
        _owner.SceneGraphRoot.GetAllChildActorNodes(nodes);
        if (_hitsCache == null)
            _hitsCache = new List<SceneGraphNode>();
        else
            _hitsCache.Clear();
        var hits = _hitsCache;

        // Process all nodes
        var projection = new ViewportProjection();
        projection.Init(_owner.Viewport);
        foreach (var node in nodes)
        {
            // Skip actors that cannot be selected
            if (!node.CanSelectInViewport)
                continue;
            var a = node.Actor;

            // Skip actor if outside of view frustum
            var actorBox = a.EditorBox;
            if (projection.FrustumCull(ref actorBox) == ContainmentType.Disjoint)
                continue;

            // Get valid selection points
            var points = node.GetActorSelectionPoints();
            if (LoopOverPoints(points, ref adjustedRect, ref projection))
            {
                if (a.HasPrefabLink && _owner is not PrefabWindowViewport)
                    hits.Add(_owner.SceneGraphRoot.Find(a.GetPrefabRoot()));
                else
                    hits.Add(node);
            }
        }

        // Process selection
        if (_owner.IsControlDown)
        {
            var newSelection = new List<SceneGraphNode>();
            var currentSelection = new List<SceneGraphNode>(_owner.SceneGraphRoot.SceneContext.Selection);
            newSelection.AddRange(currentSelection);
            foreach (var hit in hits)
            {
                if (currentSelection.Contains(hit))
                    newSelection.Remove(hit);
                else
                    newSelection.Add(hit);
            }
            _owner.Select(newSelection);
        }
        else if (Input.GetKey(KeyboardKeys.Shift))
        {
            var newSelection = new List<SceneGraphNode>();
            var currentSelection = new List<SceneGraphNode>(_owner.SceneGraphRoot.SceneContext.Selection);
            newSelection.AddRange(hits);
            newSelection.AddRange(currentSelection);
            _owner.Select(newSelection);
        }
        else
        {
            _owner.Select(hits);
        }

        Profiler.EndEvent();
    }

    private bool LoopOverPoints(Vector3[] points, ref Rectangle adjustedRect, ref ViewportProjection projection)
    {
        Profiler.BeginEvent("LoopOverPoints");
        bool containsAllPoints = points.Length != 0;
        for (int i = 0; i < points.Length; i++)
        {
            projection.ProjectPoint(points[i], out var loc);
            if (!adjustedRect.Contains(loc))
            {
                containsAllPoints = false;
                break;
            }
        }
        Profiler.EndEvent();
        return containsAllPoints;
    }

    /// <summary>
    /// Draws the ruber band during owner viewport UI drawing.
    /// </summary>
    public void Draw()
    {
        if (!_isRubberBandSpanning)
            return;
        var style = Style.Current;
        Render2D.FillRectangle(_rubberBandRect, style.Selection);
        Render2D.DrawRectangle(_rubberBandRect, style.SelectionBorder);
    }

    /// <summary>
    /// Immediately stops the rubber band.
    /// </summary>
    /// <returns>True if rubber band was active before stopping.</returns>
    public bool StopRubberBand()
    {
        if (_isMosueCaptured)
        {
            _isMosueCaptured = false;
            _owner.Viewport.EndMouseCapture();
        }
        var result = _tryStartRubberBand;
        _isRubberBandSpanning = false;
        _tryStartRubberBand = false;
        return result;
    }
}
