// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEditor.Actions;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.GUI.Tabs;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="Spline"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(Spline)), DefaultEditor]
    public class SplineEditor : ActorEditor
    {
        /// <summary>
        /// Storage undo spline data
        /// </summary>
        private struct UndoData
        {
            public Spline Spline;
            public BezierCurve<Transform>.Keyframe[] BeforeKeyframes;
        }

        /// <summary>
        /// Basis for creating tangent manipulation types for bezier curves.
        /// </summary>
        private class EditTangentOptionBase
        {
            /// <summary>
            /// Spline editor reference.
            /// </summary>
            public SplineEditor Editor;

            /// <summary>
            /// Called when user set selected tangent mode.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public virtual void OnSetMode(Spline spline, int index)
            {
            }

            /// <summary>
            /// Called when user select a keyframe (spline point) of current selected spline on editor viewport.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public virtual void OnSelectKeyframe(Spline spline, int index)
            {
            }

            /// <summary>
            /// Called when user select a tangent of current keyframe selected from spline.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public virtual void OnSelectTangent(Spline spline, int index)
            {
            }

            /// <summary>
            /// Called when the tangent in from current keyframe selected from spline is moved on editor viewport.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public virtual void OnMoveTangentIn(Spline spline, int index)
            {
            }

            /// <summary>
            /// Called when the tangent out from current keyframe selected from spline is moved on editor viewport.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Current spline selected on editor viewport.</param>
            public virtual void OnMoveTangentOut(Spline spline, int index)
            {
            }
        }

        /// <summary>
        /// Edit curve options manipulate the curve as free mode
        /// </summary>
        private sealed class FreeTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                if (IsLinearTangentMode(spline, index) || IsSmoothInTangentMode(spline, index) || IsSmoothOutTangentMode(spline, index))
                {
                    SetPointSmooth(Editor, spline, index);
                }
            }
        }

        /// <summary>
        /// Edit curve options to set tangents to linear
        /// </summary>
        private sealed class LinearTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                SetKeyframeLinear(spline, index);

                // change the selection to tangent parent (a spline point / keyframe)
                Editor.SetSelectSplinePointNode(spline, index);
            }
        }

        /// <summary>
        /// Edit curve options to align tangents of selected spline
        /// </summary>
        private sealed class AlignedTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                SmoothIfNotAligned(spline, index);
            }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index)
            {
                SmoothIfNotAligned(spline, index);
            }

            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index)
            {
                SetPointAligned(spline, index, true);
            }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index)
            {
                SetPointAligned(spline, index, false);
            }

            private void SmoothIfNotAligned(Spline spline, int index)
            {
                if (!IsAlignedTangentMode(spline, index))
                {
                    SetPointSmooth(Editor, spline, index);
                }
            }

            private void SetPointAligned(Spline spline, int index, bool alignWithIn)
            {
                var keyframe = spline.GetSplineKeyframe(index);
                var referenceTangent = alignWithIn ? keyframe.TangentIn : keyframe.TangentOut;
                var otherTangent = !alignWithIn ? keyframe.TangentIn : keyframe.TangentOut;

                // inverse of reference tangent
                otherTangent.Translation = -referenceTangent.Translation.Normalized * otherTangent.Translation.Length;

                if (alignWithIn)
                    keyframe.TangentOut = otherTangent;
                if (!alignWithIn)
                    keyframe.TangentIn = otherTangent;

                spline.SetSplineKeyframe(index, keyframe);
            }
        }

        /// <summary>
        /// Edit curve options manipulate the curve setting selected point 
        /// tangent in as smoothed but tangent out as linear
        /// </summary>
        private sealed class SmoothInTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                SetTangentSmoothIn(Editor, spline, index);
                Editor.SetSelectTangentIn(spline, index);
            }
        }

        /// <summary>
        /// Edit curve options manipulate the curve setting selected point 
        /// tangent in as linear but tangent out as smoothed
        /// </summary>
        private sealed class SmoothOutTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                SetTangentSmoothOut(Editor, spline, index);
                Editor.SetSelectTangentOut(spline, index);
            }
        }

        private sealed class IconTab : Tab
        {
            private sealed class IconTabHeader : Tabs.TabHeader
            {
                public IconTabHeader(Tabs tabs, Tab tab)
                : base(tabs, tab)
                {
                }

                public override bool OnMouseUp(Float2 location, MouseButton button)
                {
                    if (EnabledInHierarchy && Tab.Enabled)
                        ((IconTab)Tab)._action();
                    return true;
                }

                public override void Draw()
                {
                    base.Draw();

                    var tab = (IconTab)Tab;
                    var enabled = EnabledInHierarchy && tab.EnabledInHierarchy;
                    var style = FlaxEngine.GUI.Style.Current;
                    var size = Size;
                    var textHeight = 16.0f;
                    var iconSize = size.Y - textHeight;
                    var iconRect = new Rectangle((Width - iconSize) / 2, 0, iconSize, iconSize);
                    if (tab._mirrorIcon)
                    {
                        iconRect.Location.X += iconRect.Size.X;
                        iconRect.Size.X *= -1;
                    }
                    var color = style.Foreground;
                    if (!enabled)
                        color *= 0.6f;
                    Render2D.DrawSprite(tab._customIcon, iconRect, color);
                    Render2D.DrawText(style.FontMedium, tab._customText, new Rectangle(0, iconSize, size.X, textHeight), color, TextAlignment.Center, TextAlignment.Center);
                }
            }

            private readonly Action _action;
            private readonly string _customText;
            private readonly SpriteHandle _customIcon;
            private readonly bool _mirrorIcon;

            public IconTab(Action action, string text, SpriteHandle icon, bool mirrorIcon = false)
            : base(string.Empty, SpriteHandle.Invalid)
            {
                _action = action;
                _customText = text;
                _customIcon = icon;
                _mirrorIcon = mirrorIcon;
            }

            public override Tabs.TabHeader CreateHeader()
            {
                return new IconTabHeader((Tabs)Parent, this);
            }
        }

        private EditTangentOptionBase _currentTangentMode;

        private Tabs _selectedPointsTabs, _allPointsTabs;
        private Tab _freeTangentTab;
        private Tab _linearTangentTab;
        private Tab _alignedTangentTab;
        private Tab _smoothInTangentTab;
        private Tab _smoothOutTangentTab;
        private Tab _setLinearAllTangentsTab;
        private Tab _setSmoothAllTangentsTab;

        private bool _tanInChanged;
        private bool _tanOutChanged;
        private Vector3 _lastTanInPos;
        private Vector3 _lastTanOutPos;
        private Spline _selectedSpline;
        private SplineNode.SplinePointNode _selectedPoint;
        private SplineNode.SplinePointNode _lastPointSelected;
        private SplineNode.SplinePointTangentNode _selectedTangentIn;
        private SplineNode.SplinePointTangentNode _selectedTangentOut;

        private UndoData[] _selectedSplinesUndoData;

        private bool HasPointSelected => _selectedPoint != null;
        private bool HasTangentsSelected => _selectedTangentIn != null || _selectedTangentOut != null;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            _currentTangentMode = new FreeTangentMode { Editor = this };
            if (Values.HasDifferentTypes || !(Values[0] is Spline spline))
                return;
            _selectedSpline = spline;

            layout.Space(10);
            var tabSize = 46;
            var icons = Editor.Instance.Icons;

            layout.Header("Selected spline point");
            _selectedPointsTabs = new Tabs
            {
                Height = tabSize,
                TabsSize = new Float2(tabSize),
                AutoTabsSize = true,
                Parent = layout.ContainerControl,
            };
            _linearTangentTab = _selectedPointsTabs.AddTab(new IconTab(OnSetSelectedLinear, "Linear", icons.SplineLinear64));
            _freeTangentTab = _selectedPointsTabs.AddTab(new IconTab(OnSetSelectedFree, "Free", icons.SplineFree64));
            _alignedTangentTab = _selectedPointsTabs.AddTab(new IconTab(OnSetSelectedAligned, "Aligned", icons.SplineAligned64));
            _smoothInTangentTab = _selectedPointsTabs.AddTab(new IconTab(OnSetSelectedSmoothIn, "Smooth In", icons.SplineSmoothIn64));
            _smoothOutTangentTab = _selectedPointsTabs.AddTab(new IconTab(OnSetSelectedSmoothOut, "Smooth Out", icons.SplineSmoothIn64, true));
            _selectedPointsTabs.SelectedTabIndex = -1;

            layout.Header("All spline points");
            _allPointsTabs = new Tabs
            {
                Height = tabSize,
                TabsSize = new Float2(tabSize),
                AutoTabsSize = true,
                Parent = layout.ContainerControl,
            };
            _setLinearAllTangentsTab = _allPointsTabs.AddTab(new IconTab(OnSetTangentsLinear, "Set Linear Tangents", icons.SplineLinear64));
            _setSmoothAllTangentsTab = _allPointsTabs.AddTab(new IconTab(OnSetTangentsSmooth, "Set Smooth Tangents", icons.SplineAligned64));
            _allPointsTabs.SelectedTabIndex = -1;

            if (_selectedSpline)
                _selectedSpline.SplineUpdated += OnSplineEdited;

            SetSelectedTangentTypeAsCurrent();
            UpdateEditTabsSelection();
            UpdateButtonsEnabled();
        }

        /// <inheritdoc/>
        protected override void Deinitialize()
        {
            if (_selectedSpline)
                _selectedSpline.SplineUpdated -= OnSplineEdited;
        }

        private void OnSplineEdited()
        {
            UpdateEditTabsSelection();
            UpdateButtonsEnabled();
        }

        /// <inheritdoc/>
        public override void Refresh()
        {
            base.Refresh();

            UpdateSelectedPoint();
            UpdateSelectedTangent();

            if (!CanEditTangent())
                return;

            var index = _lastPointSelected.Index;
            var currentTangentInPosition = _selectedSpline.GetSplineLocalTangent(index, true).Translation;
            var currentTangentOutPosition = _selectedSpline.GetSplineLocalTangent(index, false).Translation;

            if (_selectedTangentIn != null)
            {
                _tanInChanged = _lastTanInPos != currentTangentInPosition;
                _lastTanInPos = currentTangentInPosition;
            }

            if (_selectedTangentOut != null)
            {
                _tanOutChanged = _lastTanOutPos != currentTangentOutPosition;
                _lastTanOutPos = currentTangentOutPosition;
            }

            if (_tanInChanged)
                _currentTangentMode.OnMoveTangentIn(_selectedSpline, index);
            if (_tanOutChanged)
                _currentTangentMode.OnMoveTangentOut(_selectedSpline, index);

            currentTangentInPosition = _selectedSpline.GetSplineLocalTangent(index, true).Translation;
            currentTangentOutPosition = _selectedSpline.GetSplineLocalTangent(index, false).Translation;

            // Update last tangents position after changes
            if (_selectedSpline)
                _lastTanInPos = currentTangentInPosition;
            if (_selectedSpline)
                _lastTanOutPos = currentTangentOutPosition;
            _tanInChanged = false;
            _tanOutChanged = false;
        }

        private void SetSelectedTangentTypeAsCurrent()
        {
            if (_lastPointSelected == null || _selectedPoint == null)
                return;
            if (IsLinearTangentMode(_selectedSpline, _lastPointSelected.Index))
                SetModeLinear();
            else if (IsAlignedTangentMode(_selectedSpline, _lastPointSelected.Index))
                SetModeAligned();
            else if (IsSmoothInTangentMode(_selectedSpline, _lastPointSelected.Index))
                SetModeSmoothIn();
            else if (IsSmoothOutTangentMode(_selectedSpline, _lastPointSelected.Index))
                SetModeSmoothOut();
            else if (IsFreeTangentMode(_selectedSpline, _lastPointSelected.Index))
                SetModeFree();
        }

        private void UpdateEditTabsSelection()
        {
            _selectedPointsTabs.Enabled = CanEditTangent();
            if (!_selectedPointsTabs.Enabled)
            {
                _selectedPointsTabs.SelectedTabIndex = -1;
                return;
            }

            var isFree = _currentTangentMode is FreeTangentMode;
            var isLinear = _currentTangentMode is LinearTangentMode;
            var isAligned = _currentTangentMode is AlignedTangentMode;
            var isSmoothIn = _currentTangentMode is SmoothInTangentMode;
            var isSmoothOut = _currentTangentMode is SmoothOutTangentMode;

            if (isFree)
                _selectedPointsTabs.SelectedTab = _freeTangentTab;
            else if (isLinear)
                _selectedPointsTabs.SelectedTab = _linearTangentTab;
            else if (isAligned)
                _selectedPointsTabs.SelectedTab = _alignedTangentTab;
            else if (isSmoothIn)
                _selectedPointsTabs.SelectedTab = _smoothInTangentTab;
            else if (isSmoothOut)
                _selectedPointsTabs.SelectedTab = _smoothOutTangentTab;
            else
                _selectedPointsTabs.SelectedTabIndex = -1;
        }

        private void UpdateButtonsEnabled()
        {
            _linearTangentTab.Enabled = CanEditTangent();
            _freeTangentTab.Enabled = CanEditTangent();
            _alignedTangentTab.Enabled = CanEditTangent();
            _smoothInTangentTab.Enabled = CanSetTangentSmoothIn();
            _smoothOutTangentTab.Enabled = CanSetTangentSmoothOut();
            _setLinearAllTangentsTab.Enabled = CanSetAllTangentsLinear();
            _setSmoothAllTangentsTab.Enabled = CanSetAllTangentsSmooth();
        }

        private bool CanEditTangent()
        {
            return !HasDifferentTypes && !HasDifferentValues && (HasPointSelected || HasTangentsSelected);
        }

        private bool CanSetTangentSmoothIn()
        {
            if (!CanEditTangent())
                return false;
            return _lastPointSelected.Index != 0;
        }

        private bool CanSetTangentSmoothOut()
        {
            if (!CanEditTangent())
                return false;
            return _lastPointSelected.Index < _selectedSpline.SplinePointsCount - 1;
        }

        private bool CanSetAllTangentsSmooth()
        {
            return _selectedSpline != null;
        }

        private bool CanSetAllTangentsLinear()
        {
            return _selectedSpline != null;
        }

        private void SetModeLinear()
        {
            if (_currentTangentMode is LinearTangentMode)
                return;
            _currentTangentMode = new LinearTangentMode { Editor = this };
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeFree()
        {
            if (_currentTangentMode is FreeTangentMode)
                return;
            _currentTangentMode = new FreeTangentMode { Editor = this };
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeAligned()
        {
            if (_currentTangentMode is AlignedTangentMode)
                return;
            _currentTangentMode = new AlignedTangentMode { Editor = this };
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeSmoothIn()
        {
            if (_currentTangentMode is SmoothInTangentMode)
                return;
            _currentTangentMode = new SmoothInTangentMode { Editor = this };
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeSmoothOut()
        {
            if (_currentTangentMode is SmoothOutTangentMode)
                return;
            _currentTangentMode = new SmoothOutTangentMode { Editor = this };
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private List<SceneGraphNode> GetSelection()
        {
            if (Presenter.Owner is Windows.Assets.PrefabWindow prefabWindow)
                return prefabWindow.Selection;
            return Editor.Instance.SceneEditing.Selection;
        }

        private void Select(SceneGraphNode node)
        {
            if (Presenter.Owner is Windows.Assets.PrefabWindow prefabWindow)
            {
                prefabWindow.Select(node);
                return;
            }
            Editor.Instance.SceneEditing.Select(node);
        }

        private void UpdateSelectedPoint()
        {
            // works only if select one spline
            if (_selectedSpline)
            {
                var selection = GetSelection();
                var currentSelected = selection.Count != 0 ? selection[0] : null;
                if (currentSelected == _selectedPoint)
                    return;
                if (currentSelected is SplineNode.SplinePointNode selectedPoint)
                {
                    _selectedPoint = selectedPoint;
                    _lastPointSelected = _selectedPoint;
                    _currentTangentMode.OnSelectKeyframe(_selectedSpline, _lastPointSelected.Index);
                }
                else
                {
                    _selectedPoint = null;
                }
            }
            else
            {
                _selectedPoint = null;
            }

            SetSelectedTangentTypeAsCurrent();
            UpdateEditTabsSelection();
            UpdateButtonsEnabled();
        }

        private void UpdateSelectedTangent()
        {
            // works only if select one spline
            var selection = GetSelection();
            if (_lastPointSelected == null || selection.Count != 1)
            {
                _selectedTangentIn = null;
                _selectedTangentOut = null;
                return;
            }
            var currentSelected = selection[0];
            if (currentSelected is not SplineNode.SplinePointTangentNode selectedPoint)
            {
                _selectedTangentIn = null;
                _selectedTangentOut = null;
                return;
            }

            if (currentSelected == _selectedTangentIn)
                return;
            if (currentSelected == _selectedTangentOut)
                return;

            var index = _lastPointSelected.Index;

            if (currentSelected.Transform == _selectedSpline.GetSplineTangent(index, true))
            {
                _selectedTangentIn = selectedPoint;
                _selectedTangentOut = null;
                _currentTangentMode.OnSelectTangent(_selectedSpline, index);
                return;
            }
            if (currentSelected.Transform == _selectedSpline.GetSplineTangent(index, false))
            {
                _selectedTangentOut = selectedPoint;
                _selectedTangentIn = null;
                _currentTangentMode.OnSelectTangent(_selectedSpline, index);
                return;
            }

            _selectedTangentIn = null;
            _selectedTangentOut = null;
        }

        private void StartEditSpline()
        {
            if (Presenter.Undo != null && Presenter.Undo.Enabled)
            {
                // Capture 'before' state for undo
                var splines = new List<UndoData>();
                for (int i = 0; i < Values.Count; i++)
                {
                    if (Values[i] is Spline spline)
                    {
                        splines.Add(new UndoData
                        {
                            Spline = spline,
                            BeforeKeyframes = spline.SplineKeyframes.Clone() as BezierCurve<Transform>.Keyframe[]
                        });
                    }
                }
                _selectedSplinesUndoData = splines.ToArray();
            }
        }

        private void EndEditSpline()
        {
            // Update buttons state
            UpdateEditTabsSelection();

            if (Presenter.Undo != null && Presenter.Undo.Enabled)
            {
                // Add undo
                foreach (var splineUndoData in _selectedSplinesUndoData)
                {
                    Presenter.Undo.AddAction(new EditSplineAction(_selectedSpline, splineUndoData.BeforeKeyframes));
                    SplineNode.OnSplineEdited(splineUndoData.Spline);
                    Editor.Instance.Scene.MarkSceneEdited(splineUndoData.Spline.Scene);
                }
            }
        }

        private void OnSetSelectedLinear()
        {
            StartEditSpline();
            SetModeLinear();
            EndEditSpline();
        }

        private void OnSetSelectedFree()
        {
            StartEditSpline();
            SetModeFree();
            EndEditSpline();
        }

        private void OnSetSelectedAligned()
        {
            StartEditSpline();
            SetModeAligned();
            EndEditSpline();
        }

        private void OnSetSelectedSmoothIn()
        {
            StartEditSpline();
            SetModeSmoothIn();
            EndEditSpline();
        }

        private void OnSetSelectedSmoothOut()
        {
            StartEditSpline();
            SetModeSmoothOut();
            EndEditSpline();
        }

        private void OnSetTangentsLinear()
        {
            StartEditSpline();
            _selectedSpline.SetTangentsLinear();
            _selectedSpline.UpdateSpline();
            EndEditSpline();
        }

        private void OnSetTangentsSmooth()
        {
            StartEditSpline();
            _selectedSpline.SetTangentsSmooth();
            _selectedSpline.UpdateSpline();
            EndEditSpline();
        }

        private static bool IsFreeTangentMode(Spline spline, int index)
        {
            if (IsLinearTangentMode(spline, index) ||
                IsAlignedTangentMode(spline, index) ||
                IsSmoothInTangentMode(spline, index) ||
                IsSmoothOutTangentMode(spline, index))
            {
                return false;
            }
            return true;
        }

        private static bool IsLinearTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            return keyframe.TangentIn.Translation.Length == 0 && keyframe.TangentOut.Translation.Length == 0;
        }

        private static bool IsAlignedTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            var tangentIn = keyframe.TangentIn.Translation;
            var tangentOut = keyframe.TangentOut.Translation;
            if (tangentIn.Length == 0 || tangentOut.Length == 0)
                return false;

            var angleBetweenTwoTangents = Vector3.Dot(tangentIn.Normalized, tangentOut.Normalized);
            if (angleBetweenTwoTangents < -0.99f)
                return true;

            return false;
        }

        private static bool IsSmoothInTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            return keyframe.TangentIn.Translation.Length > 0 && keyframe.TangentOut.Translation.Length == 0;
        }

        private static bool IsSmoothOutTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            return keyframe.TangentOut.Translation.Length > 0 && keyframe.TangentIn.Translation.Length == 0;
        }

        private static void SetKeyframeLinear(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            keyframe.TangentIn.Translation = Vector3.Zero;
            keyframe.TangentOut.Translation = Vector3.Zero;

            var lastSplineIndex = spline.SplinePointsCount - 1;
            if (index == lastSplineIndex && spline.IsLoop)
            {
                var lastPoint = spline.GetSplineKeyframe(lastSplineIndex);
                lastPoint.TangentIn.Translation = Vector3.Zero;
                lastPoint.TangentOut.Translation = Vector3.Zero;
                spline.SetSplineKeyframe(lastSplineIndex, lastPoint);
            }

            spline.SetSplineKeyframe(index, keyframe);
            spline.UpdateSpline();
        }

        private static void SetTangentSmoothIn(SplineEditor editor, Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);

            // Auto smooth tangent if's linear
            if (keyframe.TangentIn.Translation.Length == 0)
            {
                var cameraTransform = editor.Presenter.Owner.PresenterViewport.ViewTransform.Translation;
                var smoothRange = SplineNode.NodeSizeByDistance(spline.GetSplineTangent(index, false).Translation, 10f, cameraTransform);
                var previousKeyframe = spline.GetSplineKeyframe(index - 1);
                var tangentDirection = keyframe.Value.WorldToLocalVector(previousKeyframe.Value.Translation - keyframe.Value.Translation);
                tangentDirection = tangentDirection.Normalized * smoothRange;
                keyframe.TangentIn.Translation = tangentDirection;
            }

            keyframe.TangentOut.Translation = Vector3.Zero;
            spline.SetSplineKeyframe(index, keyframe);
            spline.UpdateSpline();
        }

        private static void SetTangentSmoothOut(SplineEditor editor, Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);

            // Auto smooth tangent if's linear
            if (keyframe.TangentOut.Translation.Length == 0)
            {
                var cameraTransform = editor.Presenter.Owner.PresenterViewport.ViewTransform.Translation;
                var smoothRange = SplineNode.NodeSizeByDistance(spline.GetSplineTangent(index, false).Translation, 10f, cameraTransform);
                var nextKeyframe = spline.GetSplineKeyframe(index + 1);
                var tangentDirection = keyframe.Value.WorldToLocalVector(nextKeyframe.Value.Translation - keyframe.Value.Translation);
                tangentDirection = tangentDirection.Normalized * smoothRange;
                keyframe.TangentOut.Translation = tangentDirection;
            }

            keyframe.TangentIn.Translation = Vector3.Zero;

            spline.SetSplineKeyframe(index, keyframe);
            spline.UpdateSpline();
        }

        private static void SetPointSmooth(SplineEditor editor, Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            var tangentInSize = keyframe.TangentIn.Translation.Length;
            var tangentOutSize = keyframe.TangentOut.Translation.Length;

            var isLastKeyframe = index >= spline.SplinePointsCount - 1;
            var isFirstKeyframe = index <= 0;
            var cameraTransform = editor.Presenter.Owner.PresenterViewport.ViewTransform.Translation;
            var smoothRange = SplineNode.NodeSizeByDistance(spline.GetSplinePoint(index), 10f, cameraTransform);

            // Force smooth it's linear point
            if (tangentInSize == 0f)
                tangentInSize = smoothRange;
            if (tangentOutSize == 0f)
                tangentOutSize = smoothRange;

            // Try get next / last keyframe
            var nextKeyframe = !isLastKeyframe ? spline.GetSplineKeyframe(index + 1) : keyframe;
            var previousKeyframe = !isFirstKeyframe ? spline.GetSplineKeyframe(index - 1) : keyframe;

            // calc form from Spline.cpp -> SetTangentsSmooth
            // get tangent direction
            var tangentDirection = (keyframe.Value.Translation - previousKeyframe.Value.Translation + nextKeyframe.Value.Translation - keyframe.Value.Translation).Normalized;

            keyframe.TangentIn.Translation = -tangentDirection;
            keyframe.TangentOut.Translation = tangentDirection;

            keyframe.TangentIn.Translation *= tangentInSize;
            keyframe.TangentOut.Translation *= tangentOutSize;

            spline.SetSplineKeyframe(index, keyframe);
            spline.UpdateSpline();
        }

        private static SplineNode.SplinePointNode GetSplinePointNode(Spline spline, int index)
        {
            return (SplineNode.SplinePointNode)SceneGraphFactory.FindNode(spline.ID).ChildNodes[index];
        }

        private static SplineNode.SplinePointTangentNode GetSplineTangentInNode(Spline spline, int index)
        {
            var point = GetSplinePointNode(spline, index);
            var tangentIn = spline.GetSplineTangent(index, true);
            var tangentNodes = point.ChildNodes;

            // find tangent in node comparing all child nodes position
            for (int i = 0; i < tangentNodes.Count; i++)
            {
                if (tangentNodes[i].Transform.Translation == tangentIn.Translation)
                {
                    return (SplineNode.SplinePointTangentNode)tangentNodes[i];
                }
            }

            return null;
        }

        private SplineNode.SplinePointTangentNode GetSplineTangentOutNode(Spline spline, int index)
        {
            var point = GetSplinePointNode(spline, index);
            var tangentOut = spline.GetSplineTangent(index, false);
            var tangentNodes = point.ChildNodes;

            // find tangent out node comparing all child nodes position
            for (int i = 0; i < tangentNodes.Count; i++)
            {
                if (tangentNodes[i].Transform.Translation == tangentOut.Translation)
                {
                    return (SplineNode.SplinePointTangentNode)tangentNodes[i];
                }
            }

            return null;
        }

        private void SetSelectSplinePointNode(Spline spline, int index)
        {
            Select(GetSplinePointNode(spline, index));
        }

        private void SetSelectTangentIn(Spline spline, int index)
        {
            Select(GetSplineTangentInNode(spline, index));
        }

        private void SetSelectTangentOut(Spline spline, int index)
        {
            Select(GetSplineTangentOutNode(spline, index));
        }
    }
}
