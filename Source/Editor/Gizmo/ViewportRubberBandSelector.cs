// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
using FlaxEngine.GUI;

namespace FlaxEngine.Gizmo;

/// <summary>
/// Class for adding viewport rubber band selection.
/// </summary>
public class ViewportRubberBandSelector
{
    private bool _isRubberBandSpanning;
    private bool _tryStartRubberBand;
    private Float2 _cachedStartingMousePosition;
    private Rectangle _rubberBandRect;
    private Rectangle _lastRubberBandRect;

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
    public void TryStartingRubberBandSelection()
    {
        if (!_isRubberBandSpanning && !_owner.Gizmos.Active.IsControllingMouse && !_owner.IsRightMouseButtonDown)
        {
            _tryStartRubberBand = true;
        }
    }

    /// <summary>
    /// Release the rubber band selection.
    /// </summary>
    /// <returns>Returns true if rubber band is currently spanning</returns>
    public bool ReleaseRubberBandSelection()
    {
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
    /// <param name="viewFrustum">The view frustum.</param>
    public void TryCreateRubberBand(bool canStart, Float2 mousePosition, BoundingFrustum viewFrustum)
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
        else if (_isRubberBandSpanning && !_owner.Gizmos.Active.IsControllingMouse && !_owner.IsRightMouseButtonDown)
        {
            _rubberBandRect.Width = mousePosition.X - _cachedStartingMousePosition.X;
            _rubberBandRect.Height = mousePosition.Y - _cachedStartingMousePosition.Y;

            if (_lastRubberBandRect != _rubberBandRect)
            {
                // Select rubberbanded rect actor nodes
                var adjustedRect = _rubberBandRect;
                _lastRubberBandRect = _rubberBandRect;
                if (adjustedRect.Width < 0 || adjustedRect.Height < 0)
                {
                    // make sure we have a well-formed rectangle i.e. size is positive and X/Y is upper left corner
                    var size = adjustedRect.Size;
                    adjustedRect.X = Mathf.Min(adjustedRect.X, adjustedRect.X + adjustedRect.Width);
                    adjustedRect.Y = Mathf.Min(adjustedRect.Y, adjustedRect.Y + adjustedRect.Height);
                    size.X = Mathf.Abs(size.X);
                    size.Y = Mathf.Abs(size.Y);
                    adjustedRect.Size = size;
                }

                // Get hits from graph nodes.
                List<SceneGraphNode> hits = new List<SceneGraphNode>();
                var nodes = _owner.SceneGraphRoot.GetAllChildActorNodes();
                foreach (var node in nodes)
                {
                    // Check for custom can select code
                    if (!node.CanSelectActorNodeWithSelector())
                        continue;
 
                    var a = node.Actor;
                    // Skip actor if outside of view frustum
                    var actorBox = a.EditorBox;
                    if (viewFrustum.Contains(actorBox) == ContainmentType.Disjoint)
                        continue;

                    // Get valid selection points
                    var points = node.GetActorSelectionPoints();
                    bool containsAllPoints = points.Length != 0;
                    foreach (var point in points)
                    {
                        _owner.Viewport.ProjectPoint(point, out var loc);
                        if (!adjustedRect.Contains(loc))
                        {
                            containsAllPoints = false;
                            break;
                        }
                    }
                    if (containsAllPoints)
                    {
                        if (a.HasPrefabLink)
                            hits.Add(_owner.SceneGraphRoot.Find(a.GetPrefabRoot()));
                        else
                            hits.Add(node);
                    }
                }

                var editor = Editor.Instance;
                if (_owner.IsControlDown)
                {
                    var newSelection = new List<SceneGraphNode>();
                    var currentSelection = editor.SceneEditing.Selection;
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
                    var currentSelection = editor.SceneEditing.Selection;
                    newSelection.AddRange(hits);
                    newSelection.AddRange(currentSelection);
                    _owner.Select(newSelection);
                }
                else
                {
                    _owner.Select(hits);
                }
            }
            
        }
    }

    /// <summary>
    /// Used to draw the rubber band. Begins render 2D.
    /// </summary>
    /// <param name="context">The GPU Context.</param>
    /// <param name="target">The GPU texture target.</param>
    /// <param name="targetDepth">The GPU texture target depth.</param>
    public void Draw(GPUContext context, GPUTexture target, GPUTexture targetDepth)
    {
        // Draw RubberBand for rect selection
        if (!_isRubberBandSpanning)
            return;
        Render2D.Begin(context, target, targetDepth);
        Draw2D();
        Render2D.End();
    }

    /// <summary>
    /// Used to draw the rubber band. Use if already rendering 2D context.
    /// </summary>
    public void Draw2D()
    {
        if (!_isRubberBandSpanning)
            return;
        Render2D.FillRectangle(_rubberBandRect, Style.Current.Selection);
        Render2D.DrawRectangle(_rubberBandRect, Style.Current.SelectionBorder);
    }

    /// <summary>
    /// Immediately stops the rubber band.
    /// </summary>
    public void StopRubberBand()
    {
        _isRubberBandSpanning = false;
        _tryStartRubberBand = false;
    }
}
