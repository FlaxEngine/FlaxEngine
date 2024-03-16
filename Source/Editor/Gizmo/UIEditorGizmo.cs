// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Viewport.Cameras;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor
{
    /// <summary>
    /// UI editor camera.
    /// </summary>
    [HideInEditor]
    internal sealed class UIEditorCamera : ViewportCamera
    {
        public UIEditorRoot UIEditor;

        public void ShowActors(IEnumerable<Actor> actors)
        {
            // Calculate bounds of all selected objects
            var areaRect = Rectangle.Empty;
            var root = UIEditor.UIRoot;
            foreach (var actor in actors)
            {
                Rectangle bounds;
                if (actor is UIControl uiControl && uiControl.HasControl && uiControl.IsActive)
                {
                    var control = uiControl.Control;
                    bounds = control.EditorBounds;

                    var ul = control.PointToParent(root, bounds.UpperLeft);
                    var ur = control.PointToParent(root, bounds.UpperRight);
                    var bl = control.PointToParent(root, bounds.BottomLeft);
                    var br = control.PointToParent(root, bounds.BottomRight);

                    var min = Float2.Min(Float2.Min(ul, ur), Float2.Min(bl, br));
                    var max = Float2.Max(Float2.Max(ul, ur), Float2.Max(bl, br));
                    bounds = new Rectangle(min, Float2.Max(max - min, Float2.Zero));
                }
                else if (actor is UICanvas uiCanvas && uiCanvas.IsActive && uiCanvas.GUI.Parent == root)
                {
                    bounds = uiCanvas.GUI.Bounds;
                }
                else
                    continue;

                if (areaRect == Rectangle.Empty)
                    areaRect = bounds;
                else
                    areaRect = Rectangle.Union(areaRect, bounds);
            }
            if (areaRect == Rectangle.Empty)
                return;

            // Add margin
            areaRect = areaRect.MakeExpanded(100.0f);

            // Show bounds
            UIEditor.ViewScale = (UIEditor.Size / areaRect.Size).MinValue * 0.95f;
            UIEditor.ViewCenterPosition = areaRect.Center;
        }

        public override void FocusSelection(GizmosCollection gizmos, ref Quaternion orientation)
        {
            ShowActors(gizmos.Get<TransformGizmo>().Selection, ref orientation);
        }

        public override void ShowActor(Actor actor)
        {
            ShowActors(new[] { actor });
        }

        public override void ShowActors(List<SceneGraphNode> selection, ref Quaternion orientation)
        {
            ShowActors(selection.ConvertAll(x => (Actor)x.EditableObject));
        }

        public override void UpdateView(float dt, ref Vector3 moveDelta, ref Float2 mouseDelta, out bool centerMouse)
        {
            centerMouse = false;
        }
    }

    /// <summary>
    /// Root control for UI Controls presentation in the game/prefab viewport.
    /// </summary>
    [HideInEditor]
    internal class UIEditorRoot : InputsPassThrough
    {
        /// <summary>
        /// View for the UI structure to be linked in for camera zoom and panning operations.
        /// </summary>
        private sealed class View : ContainerControl
        {
            public View(UIEditorRoot parent)
            {
                AutoFocus = false;
                ClipChildren = false;
                CullChildren = false;
                Pivot = Float2.Zero;
                Size = new Float2(1920, 1080);
                Parent = parent;
            }

            public override bool RayCast(ref Float2 location, out Control hit)
            {
                // Ignore self
                return RayCastChildren(ref location, out hit);
            }

            public override bool IntersectsContent(ref Float2 locationParent, out Float2 location)
            {
                location = PointFromParent(ref locationParent);
                return true;
            }

            public override void DrawSelf()
            {
                var uiRoot = (UIEditorRoot)Parent;
                if (!uiRoot.EnableBackground)
                    return;

                // Draw canvas area
                var bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, new Color(0, 0, 0, 0.2f));
            }
        }

        private bool _mouseMovesControl, _mouseMovesView;
        private Float2 _mouseMovesPos, _moveSnapDelta;
        private float _mouseMoveSum;
        private UndoMultiBlock _undoBlock;
        private View _view;
        private float[] _gridTickSteps = Utilities.Utils.CurveTickSteps, _gridTickStrengths;

        /// <summary>
        /// True if enable displaying UI editing background and grid elements.
        /// </summary>
        public virtual bool EnableBackground => false;

        /// <summary>
        /// True if enable selecting controls with mouse button.
        /// </summary>
        public virtual bool EnableSelecting => false;

        /// <summary>
        /// True if enable panning and zooming the view.
        /// </summary>
        public bool EnableCamera => _view != null;

        /// <summary>
        /// Transform gizmo to use sync with (selection, snapping, transformation settings).
        /// </summary>
        public virtual TransformGizmo TransformGizmo => null;

        /// <summary>
        /// The root control for controls to be linked in.
        /// </summary>
        public readonly ContainerControl UIRoot;

        internal Float2 ViewPosition
        {
            get => _view.Location / -ViewScale;
            set => _view.Location = value * -ViewScale;
        }

        internal Float2 ViewCenterPosition
        {
            get => (_view.Location - Size * 0.5f) / -ViewScale;
            set => _view.Location = Size * 0.5f + value * -ViewScale;
        }

        internal float ViewScale
        {
            get => _view.Scale.X;
            set
            {
                value = Mathf.Clamp(value, 0.1f, 4.0f);
                _view.Scale = new Float2(value);
            }
        }

        public UIEditorRoot(bool enableCamera = false)
        {
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
            AutoFocus = false;
            UIRoot = this;
            CullChildren = false;
            ClipChildren = true;
            if (enableCamera)
            {
                _view = new View(this);
                UIRoot = _view;
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            var transformGizmo = TransformGizmo;
            var owner = transformGizmo?.Owner;
            if (EnableSelecting && owner != null && !_mouseMovesControl && button == MouseButton.Left)
            {
                // Raycast the control under the mouse
                var mousePos = PointFromWindow(RootWindow.MousePosition);
                if (RayCastControl(ref mousePos, out var hitControl))
                {
                    var uiControlNode = FindUIControlNode(hitControl);
                    if (uiControlNode != null)
                    {
                        // Select node (with additive mode)
                        var selection = new List<SceneGraphNode>();
                        if (Root.GetKey(KeyboardKeys.Control))
                        {
                            // Add/remove from selection
                            selection.AddRange(transformGizmo.Selection);
                            if (transformGizmo.Selection.Contains(uiControlNode))
                                selection.Remove(uiControlNode);
                            else
                                selection.Add(uiControlNode);
                        }
                        else
                        {
                            // Select
                            selection.Add(uiControlNode);
                        }
                        owner.Select(selection);

                        // Initialize control movement
                        _mouseMovesControl = true;
                        _mouseMovesPos = location;
                        _mouseMoveSum = 0.0f;
                        _moveSnapDelta = Float2.Zero;
                        Focus();
                        StartMouseCapture();
                        return true;
                    }
                }
            }
            if (EnableCamera && (button == MouseButton.Right || button == MouseButton.Middle))
            {
                // Initialize surface movement
                _mouseMovesView = true;
                _mouseMovesPos = location;
                _mouseMoveSum = 0.0f;
                Focus();
                StartMouseCapture();
                return true;
            }

            return false;
        }

        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);

            var transformGizmo = TransformGizmo;
            if (_mouseMovesControl && transformGizmo != null)
            {
                // Calculate transform delta
                var delta = location - _mouseMovesPos;
                if (transformGizmo.TranslationSnapEnable || transformGizmo.Owner.UseSnapping)
                {
                    _moveSnapDelta += delta;
                    delta = Float2.SnapToGrid(_moveSnapDelta, new Float2(transformGizmo.TranslationSnapValue * ViewScale));
                    _moveSnapDelta -= delta;
                }

                // Move selected controls
                if (delta.LengthSquared > 0.0f)
                {
                    StartUndo();
                    var moved = false;
                    var moveLocation = _mouseMovesPos + delta;
                    var selection = transformGizmo.Selection;
                    for (var i = 0; i < selection.Count; i++)
                    {
                        if (IsValidControl(selection[i], out var uiControl))
                        {
                            // Move control (handle any control transformations by moving in editor's local-space)
                            var control = uiControl.Control;
                            var localLocation = control.LocalLocation;
                            var pointOrigin = control.Parent ?? control;
                            var startPos = pointOrigin.PointFromParent(this, _mouseMovesPos);
                            var endPos = pointOrigin.PointFromParent(this, moveLocation);
                            var uiControlDelta = endPos - startPos;
                            control.LocalLocation = localLocation + uiControlDelta;

                            // Don't move if layout doesn't allow it
                            if (control.Parent != null)
                                control.Parent.PerformLayout();
                            else
                                control.PerformLayout();

                            // Check if control was moved (parent container could block it)
                            if (localLocation != control.LocalLocation)
                                moved = true;
                        }
                    }
                    _mouseMovesPos = location;
                    _mouseMoveSum += delta.Length;
                    if (moved)
                        Cursor = CursorType.SizeAll;
                }
            }
            if (_mouseMovesView)
            {
                // Move view
                var delta = location - _mouseMovesPos;
                if (delta.LengthSquared > 4.0f)
                {
                    _mouseMovesPos = location;
                    _mouseMoveSum += delta.Length;
                    _view.Location += delta;
                    Cursor = CursorType.SizeAll;
                }
            }
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            EndMovingControls();
            if (_mouseMovesView)
            {
                EndMovingView();
                if (button == MouseButton.Right && _mouseMoveSum < 2.0f)
                    TransformGizmo.Owner.OpenContextMenu();
            }

            return base.OnMouseUp(location, button);
        }

        public override void OnMouseLeave()
        {
            EndMovingControls();
            EndMovingView();

            base.OnMouseLeave();
        }

        public override void OnLostFocus()
        {
            EndMovingControls();
            EndMovingView();

            base.OnLostFocus();
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (base.OnMouseWheel(location, delta))
                return true;

            if (EnableCamera && !_mouseMovesControl)
            {
                // Zoom view
                var nextViewScale = ViewScale + delta * 0.1f;
                if (delta > 0 && !_mouseMovesControl)
                {
                    // Scale towards mouse when zooming in
                    var nextCenterPosition = ViewPosition + location / ViewScale;
                    ViewScale = nextViewScale;
                    ViewPosition = nextCenterPosition - (location / ViewScale);
                }
                else
                {
                    // Scale while keeping center position when zooming out or when dragging view
                    var viewCenter = ViewCenterPosition;
                    ViewScale = nextViewScale;
                    ViewCenterPosition = viewCenter;
                }

                return true;
            }

            return false;
        }

        public override void Draw()
        {
            if (EnableBackground)
            {
                // Draw background
                Surface.VisjectSurface.DrawBackgroundDefault(Editor.Instance.UI.VisjectSurfaceBackground, Width, Height);

                // Draw grid
                var viewRect = GetClientArea();
                var upperLeft = _view.PointFromParent(viewRect.Location);
                var bottomRight = _view.PointFromParent(viewRect.Size);
                var min = Float2.Min(upperLeft, bottomRight);
                var max = Float2.Max(upperLeft, bottomRight);
                var pixelRange = (max - min) * ViewScale;
                Render2D.PushClip(ref viewRect);
                DrawAxis(Float2.UnitX, viewRect, min.X, max.X, pixelRange.X);
                DrawAxis(Float2.UnitY, viewRect, min.Y, max.Y, pixelRange.Y);
                Render2D.PopClip();
            }

            base.Draw();

            bool drawAnySelectedControl = false;
            var transformGizmo = TransformGizmo;
            if (transformGizmo != null)
            {
                // Selected UI controls outline
                var selection = transformGizmo.Selection;
                for (var i = 0; i < selection.Count; i++)
                {
                    if (IsValidControl(selection[i], out var controlActor))
                    {
                        DrawControlBounds(controlActor.Control, true, ref drawAnySelectedControl);
                        // TODO: draw anchors
                    }
                }
            }
            if (EnableSelecting && !_mouseMovesControl && IsMouseOver)
            {
                // Highlight control under mouse for easier selecting (except if already selected)
                var mousePos = PointFromWindow(RootWindow.MousePosition);
                if (RayCastControl(ref mousePos, out var hitControl) &&
                    (transformGizmo == null || !transformGizmo.Selection.Any(x => x.EditableObject is UIControl controlActor && controlActor.Control == hitControl)))
                {
                    DrawControlBounds(hitControl, false, ref drawAnySelectedControl);
                }
            }
            if (drawAnySelectedControl)
                Render2D.PopTransform();

            if (EnableBackground)
            {
                // Draw border
                if (ContainsFocus)
                {
                    Render2D.DrawRectangle(new Rectangle(1, 1, Width - 2, Height - 2), Editor.IsPlayMode ? Color.OrangeRed : Style.Current.BackgroundSelected);
                }
            }
        }

        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            EndMovingControls();
            EndMovingView();

            base.OnDestroy();
        }

        private void DrawAxis(Float2 axis, Rectangle viewRect, float min, float max, float pixelRange)
        {
            var style = Style.Current;
            var linesColor = style.ForegroundDisabled.RGBMultiplied(0.5f);
            var labelsColor = style.ForegroundDisabled;
            var labelsSize = 10.0f;
            Utilities.Utils.DrawCurveTicks((float tick, float strength) =>
            {
                var p = _view.PointToParent(axis * tick);

                // Draw line
                var lineRect = new Rectangle
                (
                 viewRect.Location + (p - 0.5f) * axis,
                 Float2.Lerp(viewRect.Size, Float2.One, axis)
                );
                Render2D.FillRectangle(lineRect, linesColor.AlphaMultiplied(strength));

                // Draw label
                string label = tick.ToString(System.Globalization.CultureInfo.InvariantCulture);
                var labelRect = new Rectangle
                (
                 viewRect.X + 4.0f + (p.X * axis.X),
                 viewRect.Y - labelsSize + (p.Y * axis.Y) + (viewRect.Size.Y * axis.X),
                 50,
                 labelsSize
                );
                Render2D.DrawText(style.FontSmall, label, labelRect, labelsColor.AlphaMultiplied(strength), TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, 0.7f);
            }, _gridTickSteps, ref _gridTickStrengths, min, max, pixelRange);
        }

        private void DrawControlBounds(Control control, bool selection, ref bool drawAnySelectedControl)
        {
            if (!drawAnySelectedControl)
            {
                drawAnySelectedControl = true;
                Render2D.PushTransform(ref _cachedTransform);
            }
            var options = Editor.Instance.Options.Options.Visual;
            var bounds = control.EditorBounds;
            var ul = control.PointToParent(this, bounds.UpperLeft);
            var ur = control.PointToParent(this, bounds.UpperRight);
            var bl = control.PointToParent(this, bounds.BottomLeft);
            var br = control.PointToParent(this, bounds.BottomRight);
            var color = selection ? options.SelectionOutlineColor0 : Style.Current.SelectionBorder;
#if false
            // AABB
            var min = Float2.Min(Float2.Min(ul, ur), Float2.Min(bl, br));
            var max = Float2.Max(Float2.Max(ul, ur), Float2.Max(bl, br));
            bounds = new Rectangle(min, Float2.Max(max - min, Float2.Zero));
            Render2D.DrawRectangle(bounds, color, options.UISelectionOutlineSize);
#else
            // OBB
            Render2D.DrawLine(ul, ur, color, options.UISelectionOutlineSize);
            Render2D.DrawLine(ur, br, color, options.UISelectionOutlineSize);
            Render2D.DrawLine(br, bl, color, options.UISelectionOutlineSize);
            Render2D.DrawLine(bl, ul, color, options.UISelectionOutlineSize);
#endif
        }

        private bool IsValidControl(SceneGraphNode node, out UIControl uiControl)
        {
            uiControl = null;
            if (node.EditableObject is UIControl controlActor)
                uiControl = controlActor;
            return uiControl != null &&
                   uiControl.Control != null &&
                   uiControl.Control.VisibleInHierarchy &&
                   uiControl.Control.RootWindow != null;
        }

        private bool RayCastControl(ref Float2 location, out Control hit)
        {
#if false
            // Raycast only controls with content (eg. skips transparent panels)
            return RayCastChildren(ref location, out hit);
#else
            // Find any control under mouse (hierarchical)
            hit = GetChildAtRecursive(location);
            if (hit is View)
                hit = null;
            return hit != null;
#endif
        }

        private UIControlNode FindUIControlNode(Control control)
        {
            return FindUIControlNode(TransformGizmo.Owner.SceneGraphRoot, control);
        }

        private UIControlNode FindUIControlNode(SceneGraphNode node, Control control)
        {
            var result = node as UIControlNode;
            if (result != null && ((UIControl)result.Actor).Control == control)
                return result;
            foreach (var e in node.ChildNodes)
            {
                result = FindUIControlNode(e, control);
                if (result != null)
                    return result;
            }
            return null;
        }

        private void StartUndo()
        {
            var undo = TransformGizmo?.Owner?.Undo;
            if (undo == null || _undoBlock != null)
                return;
            _undoBlock = new UndoMultiBlock(undo, TransformGizmo.Selection.ConvertAll(x => x.EditableObject), "Edit control");
        }

        private void EndUndo()
        {
            if (_undoBlock == null)
                return;
            _undoBlock.Dispose();
            _undoBlock = null;
        }

        private void EndMovingControls()
        {
            if (!_mouseMovesControl)
                return;
            _mouseMovesControl = false;
            EndMouseCapture();
            Cursor = CursorType.Default;
            EndUndo();
        }

        private void EndMovingView()
        {
            if (!_mouseMovesView)
                return;
            _mouseMovesView = false;
            EndMouseCapture();
            Cursor = CursorType.Default;
        }
    }

    /// <summary>
    /// Control that can optionally disable inputs to the children.
    /// </summary>
    [HideInEditor]
    internal class InputsPassThrough : ContainerControl
    {
        private bool _isMouseOver;

        /// <summary>
        /// True if enable input events passing to the UI.
        /// </summary>
        public virtual bool EnableInputs => true;

        public override bool RayCast(ref Float2 location, out Control hit)
        {
            return RayCastChildren(ref location, out hit);
        }

        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise)
                return false;
            return base.ContainsPoint(ref location, precise);
        }

        public override bool OnCharInput(char c)
        {
            if (!EnableInputs)
                return false;
            return base.OnCharInput(c);
        }

        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            if (!EnableInputs)
                return DragDropEffect.None;
            return base.OnDragDrop(ref location, data);
        }

        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            if (!EnableInputs)
                return DragDropEffect.None;
            return base.OnDragEnter(ref location, data);
        }

        public override void OnDragLeave()
        {
            if (!EnableInputs)
                return;
            base.OnDragLeave();
        }

        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            if (!EnableInputs)
                return DragDropEffect.None;
            return base.OnDragMove(ref location, data);
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (!EnableInputs)
                return false;
            return base.OnKeyDown(key);
        }

        public override void OnKeyUp(KeyboardKeys key)
        {
            if (!EnableInputs)
                return;
            base.OnKeyUp(key);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseDoubleClick(location, button);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseDown(location, button);
        }

        public override bool IsMouseOver => _isMouseOver;

        public override void OnMouseEnter(Float2 location)
        {
            _isMouseOver = true;
            if (!EnableInputs)
                return;
            base.OnMouseEnter(location);
        }

        public override void OnMouseLeave()
        {
            _isMouseOver = false;
            if (!EnableInputs)
                return;
            base.OnMouseLeave();
        }

        public override void OnMouseMove(Float2 location)
        {
            if (!EnableInputs)
                return;
            base.OnMouseMove(location);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseUp(location, button);
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseWheel(location, delta);
        }
    }
}
