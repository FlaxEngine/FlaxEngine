// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// The blend space editor used by the animation multi blend nodes.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class BlendPointsEditor : ContainerControl
    {
        private readonly Animation.MultiBlend _node;
        private readonly bool _is2D;
        private Float2 _rangeX, _rangeY;
        private Float2 _debugPos = Float2.Minimum;
        private float _debugScale = 1.0f;
        private readonly List<BlendPoint> _blendPoints = new List<BlendPoint>();

        /// <summary>
        /// Represents single blend point.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Control" />
        internal class BlendPoint : Control
        {
            private readonly BlendPointsEditor _editor;
            private readonly int _index;
            private Float2 _mousePosOffset;
            private bool _isMouseDown, _mouseMoved;
            private object[] _mouseMoveStartValues;

            /// <summary>
            /// The default size for the blend points.
            /// </summary>
            public const float DefaultSize = 8.0f;

            /// <summary>
            /// Blend point index.
            /// </summary>
            public int Index => _index;

            /// <summary>
            /// Flag that indicates that user is moving this point with a mouse.
            /// </summary>
            public bool IsMouseDown => _isMouseDown;

            /// <summary>
            /// Initializes a new instance of the <see cref="BlendPoint"/> class.
            /// </summary>
            /// <param name="editor">The editor.</param>
            /// <param name="index">The blend point index.</param>
            public BlendPoint(BlendPointsEditor editor, int index)
            : base(0, 0, DefaultSize, DefaultSize)
            {
                _editor = editor;
                _index = index;
            }

            private void EndMove()
            {
                _isMouseDown = false;
                EndMouseCapture();
                if (_mouseMoveStartValues != null)
                {
                    // Add undo action
                    _editor._node.Surface.AddBatchedUndoAction(new EditNodeValuesAction(_editor._node, _mouseMoveStartValues, true));
                    _mouseMoveStartValues = null;
                }
                if (_mouseMoved)
                    _editor._node.Surface.MarkAsEdited();
            }

            /// <inheritdoc />
            public override void OnGotFocus()
            {
                base.OnGotFocus();

                _editor._node.SelectedAnimationIndex = _index;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                // Draw dot with outline
                var style = Style.Current;
                var icon = Editor.Instance.Icons.VisjectBoxClosed32;
                var size = Height;
                var rect = new Rectangle(new Float2(size * -0.5f) + Size * 0.5f, new Float2(size));
                var outline = Color.Black; // Shadow
                if (_isMouseDown)
                    outline = style.SelectionBorder;
                else if (IsMouseOver)
                    outline = style.BorderHighlighted;
                else if (_editor._node.SelectedAnimationIndex == _index)
                    outline = style.BackgroundSelected;
                Render2D.DrawSprite(icon, rect.MakeExpanded(4.0f), outline);
                Render2D.DrawSprite(icon, rect, style.Foreground);
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    Focus();
                    _isMouseDown = true;
                    _mouseMoved = false;
                    _mouseMoveStartValues = null;
                    _mousePosOffset = -location;
                    StartMouseCapture();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _isMouseDown)
                {
                    EndMove();
                    return true;
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                if (_isMouseDown && (_mouseMoved || Float2.DistanceSquared(location, _mousePosOffset) > 16.0f))
                {
                    if (!_mouseMoved)
                    {
                        // Capture initial state for undo
                        _mouseMoved = true;
                        _mouseMoveStartValues = _editor._node.Surface.Undo != null ? (object[])_editor._node.Values.Clone() : null;
                    }

                    var newLocation = Location + location + _mousePosOffset;
                    newLocation = _editor.BlendPointPosToBlendSpacePos(newLocation);
                    if (Root != null && Root.GetKey(KeyboardKeys.Control))
                    {
                        var data0 = (Float4)_editor._node.Values[0];
                        var rangeX = new Float2(data0.X, data0.Y);
                        var rangeY = _editor._is2D ? new Float2(data0.Z, data0.W) : Float2.One;
                        var grid = new Float2(Mathf.Abs(rangeX.Y - rangeX.X) * 0.01f, Mathf.Abs(rangeY.X - rangeY.Y) * 0.01f);
                        newLocation = Float2.SnapToGrid(newLocation, grid);
                    }
                    _editor.SetLocation(_index, newLocation);
                }

                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                if (_isMouseDown)
                {
                    EndMove();
                }

                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                if (_isMouseDown)
                {
                    EndMove();
                }

                base.OnLostFocus();
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                _isMouseDown = false;

                base.OnEndMouseCapture();
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                switch (key)
                {
                case KeyboardKeys.Delete:
                    _editor.SetAsset(_index, Guid.Empty);
                    return true;
                }
                return base.OnKeyDown(key);
            }
        }

        /// <summary>
        /// Gets a value indicating whether blend space is 2D, otherwise it is 1D.
        /// </summary>
        public bool Is2D => _is2D;

        /// <summary>
        /// Blend points count.
        /// </summary>
        public int PointsCount => (_node.Values.Length - 4) / 2; // 4 node values + 2 per blend point

        /// <summary>
        /// BLend points array.
        /// </summary>
        internal IReadOnlyList<BlendPoint> BlendPoints => _blendPoints;

        /// <summary>
        /// Initializes a new instance of the <see cref="BlendPointsEditor"/> class.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <param name="is2D">The value indicating whether blend space is 2D, otherwise it is 1D.</param>
        /// <param name="x">The X location.</param>
        /// <param name="y">The Y location.</param>
        /// <param name="width">The width.</param>
        /// <param name="height">The height.</param>
        public BlendPointsEditor(Animation.MultiBlend node, bool is2D, float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            _node = node;
            _is2D = is2D;
        }

        internal void AddPoint()
        {
            // Add random point within range
            var rand = new Float2(Mathf.Frac((float)Platform.TimeSeconds), (Platform.TimeCycles % 10000) / 10000.0f);
            AddPoint(Float2.Lerp(new Float2(_rangeX.X, _rangeY.X), new Float2(_rangeX.Y, _rangeY.Y), rand));
        }

        private void AddPoint(Float2 location)
        {
            // Reuse existing animation
            var count = PointsCount;
            Guid id = Guid.Empty;
            for (int i = 0; i < count; i++)
            {
                id = (Guid)_node.Values[5 + i * 2];
                if (id != Guid.Empty)
                    break;
            }
            if (id == Guid.Empty)
            {
                // Just use the first anim from project, user will change it
                var ids = FlaxEngine.Content.GetAllAssetsByType(typeof(FlaxEngine.Animation));
                if (ids.Length != 0)
                    id = ids[0];
                else
                    return;
            }

            AddPoint(id, location);
        }

        /// <summary>
        /// Sets the blend point asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <param name="location">The location.</param>
        public void AddPoint(Guid asset, Float2 location)
        {
            // Find the first free slot
            var count = PointsCount;
            if (count == Animation.MultiBlend.MaxAnimationsCount)
                return;
            var values = (object[])_node.Values.Clone();
            var index = 0;
            for (; index < count; index++)
            {
                var dataB = (Guid)_node.Values[5 + index * 2];
                if (dataB == Guid.Empty)
                    break;
            }
            if (index == count)
            {
                // Add another blend point
                Array.Resize(ref values, values.Length + 2);
            }

            values[4 + index * 2] = new Float4(location.X, _is2D ? location.Y : 0.0f, 0, 1.0f);
            values[5 + index * 2] = asset;
            _node.SetValues(values);

            // Auto-select
            _node.SelectedAnimationIndex = index;
        }

        /// <summary>
        /// Sets the blend point asset.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <param name="asset">The asset.</param>
        /// <param name="withUndo">True to use undo action.</param>
        public void SetAsset(int index, Guid asset, bool withUndo = true)
        {
            if (withUndo)
            {
                _node.SetValue(5 + index * 2, asset);
            }
            else
            {
                _node.Values[5 + index * 2] = asset;
                _node.Surface.MarkAsEdited();
            }

            _node.UpdateUI();
        }

        /// <summary>
        /// Sets the blend point location.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <param name="location">The location.</param>
        public void SetLocation(int index, Float2 location)
        {
            var dataA = (Float4)_node.Values[4 + index * 2];
            var ranges = (Float4)_node.Values[0];

            dataA.X = Mathf.Clamp(location.X, ranges.X, ranges.Y);
            if (_is2D)
                dataA.Y = Mathf.Clamp(location.Y, ranges.Z, ranges.W);

            _node.Values[4 + index * 2] = dataA;

            _node.UpdateUI();
        }

        /// <summary>
        /// Gets the blend points area.
        /// </summary>
        /// <param name="pointsArea">The control-space area.</param>
        public void GetPointsArea(out Rectangle pointsArea)
        {
            pointsArea = new Rectangle(Float2.Zero, Size);
            pointsArea.Expand(-10.0f);
        }

        /// <summary>
        /// Converts the blend point position into the blend space position (defined by min/max per axis).
        /// </summary>
        /// <param name="pos">The blend point control position.</param>
        /// <returns>The blend space position.</returns>
        public Float2 BlendPointPosToBlendSpacePos(Float2 pos)
        {
            GetPointsArea(out var pointsArea);
            pos += new Float2(BlendPoint.DefaultSize * 0.5f);
            if (_is2D)
            {
                pos = new Float2(
                                 Mathf.Remap(pos.X, pointsArea.Left, pointsArea.Right, _rangeX.X, _rangeX.Y),
                                 Mathf.Remap(pos.Y, pointsArea.Bottom, pointsArea.Top, _rangeY.X, _rangeY.Y)
                                );
            }
            else
            {
                pos = new Float2(
                                 Mathf.Remap(pos.X, pointsArea.Left, pointsArea.Right, _rangeX.X, _rangeX.Y),
                                 0
                                );
            }
            return pos;
        }

        /// <summary>
        /// Converts the blend space position into the blend point position.
        /// </summary>
        /// <param name="pos">The blend space position.</param>
        /// <returns>The blend point control position.</returns>
        public Float2 BlendSpacePosToBlendPointPos(Float2 pos)
        {
            if (_rangeX.IsZero)
            {
                var data0 = (Float4)_node.Values[0];
                _rangeX = new Float2(data0.X, data0.Y);
                _rangeY = _is2D ? new Float2(data0.Z, data0.W) : Float2.Zero;
            }
            GetPointsArea(out var pointsArea);
            if (_is2D)
            {
                pos = new Float2(
                                 Mathf.Remap(pos.X, _rangeX.X, _rangeX.Y, pointsArea.Left, pointsArea.Right),
                                 Mathf.Remap(pos.Y, _rangeY.X, _rangeY.Y, pointsArea.Bottom, pointsArea.Top)
                                );
            }
            else
            {
                pos = new Float2(
                                 Mathf.Remap(pos.X, _rangeX.X, _rangeX.Y, pointsArea.Left, pointsArea.Right),
                                 pointsArea.Center.Y
                                );
            }
            return pos;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Synchronize blend points collection
            var data0 = (Float4)_node.Values[0];
            _rangeX = new Float2(data0.X, data0.Y);
            _rangeY = _is2D ? new Float2(data0.Z, data0.W) : Float2.Zero;
            var count = PointsCount;
            while (_blendPoints.Count > count)
            {
                _blendPoints[count].Dispose();
                _blendPoints.RemoveAt(count);
            }
            while (_blendPoints.Count < count)
                _blendPoints.Add(null);
            for (int i = 0; i < count; i++)
            {
                var animId = (Guid)_node.Values[5 + i * 2];
                var dataA = (Float4)_node.Values[4 + i * 2];
                var location = new Float2(Mathf.Clamp(dataA.X, _rangeX.X, _rangeX.Y), _is2D ? Mathf.Clamp(dataA.Y, _rangeY.X, _rangeY.Y) : 0.0f);
                if (animId != Guid.Empty)
                {
                    if (_blendPoints[i] == null)
                    {
                        // Create missing blend point
                        _blendPoints[i] = new BlendPoint(this, i)
                        {
                            Parent = this,
                        };
                    }

                    // Update blend point
                    _blendPoints[i].Location = BlendSpacePosToBlendPointPos(location) - BlendPoint.DefaultSize * 0.5f;
                    var asset = Editor.Instance.ContentDatabase.FindAsset(animId);
                    var tooltip = asset?.ShortName ?? string.Empty;
                    tooltip += "\nX: " + location.X;
                    if (_is2D)
                        tooltip += "\nY: " + location.Y;
                    _blendPoints[i].TooltipText = tooltip;
                }
                else
                {
                    if (_blendPoints[i] != null)
                    {
                        // Removed unused blend point
                        _blendPoints[i].Dispose();
                        _blendPoints[i] = null;
                    }
                }
            }

            // Debug current playback position
            if (((AnimGraphSurface)_node.Surface).TryGetTraceEvent(_node, out var traceEvent))
            {
                var prev = _debugPos;
                if (_is2D)
                {
                    unsafe
                    {
                        // Unpack xy from 32-bits
                        Half2 packed = *(Half2*)&traceEvent.Value;
                        _debugPos = (Float2)packed;
                    }
                }
                else
                    _debugPos = new Float2(traceEvent.Value, 0.0f);

                // Scale debug pointer when it moves to make it more visible when investigating blending
                const float debugMaxSize = 2.0f;
                float debugScale = Mathf.Saturate(Float2.Distance(ref _debugPos, ref prev) / new Float2(_rangeX.Absolute.ValuesSum, _rangeY.Absolute.ValuesSum).Length * 100.0f) * debugMaxSize + 1.0f;
                float debugBlendSpeed = _debugScale <= debugScale ? 4.0f : 1.0f;
                _debugScale = Mathf.Lerp(_debugScale, debugScale, deltaTime * debugBlendSpeed);
            }
            else
            {
                _debugPos = Float2.Minimum;
                _debugScale = 1.0f;
            }

            base.Update(deltaTime);
        }

        /// <summary>
        /// Gets the blend area rect (in local control space for the range min-max).
        /// </summary>
        protected Rectangle BlendAreaRect
        {
            get
            {
                var upperLeft = BlendSpacePosToBlendPointPos(new Float2(_rangeX.X, _rangeY.X));
                var bottomRight = BlendSpacePosToBlendPointPos(new Float2(_rangeX.Y, _rangeY.Y));
                return new Rectangle(upperLeft, bottomRight - upperLeft);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            Focus();
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (button == MouseButton.Right)
            {
                // Show context menu
                var menu = new FlaxEditor.GUI.ContextMenu.ContextMenu();
                var b = menu.AddButton("Add point", OnAddPoint);
                b.Tag = location;
                b.Enabled = PointsCount < Animation.MultiBlend.MaxAnimationsCount;
                if (GetChildAt(location) is BlendPoint blendPoint)
                {
                    b = menu.AddButton("Remove point", OnRemovePoint);
                    b.Tag = blendPoint.Index;
                    b.TooltipText = blendPoint.TooltipText;
                }
                menu.Show(this, location);
            }

            return true;
        }

        private void OnAddPoint(FlaxEditor.GUI.ContextMenu.ContextMenuButton b)
        {
            AddPoint(BlendPointPosToBlendSpacePos((Float2)b.Tag));
        }

        private void OnRemovePoint(FlaxEditor.GUI.ContextMenu.ContextMenuButton b)
        {
            SetAsset((int)b.Tag, Guid.Empty);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var rect = new Rectangle(Float2.Zero, Size);
            var containsFocus = ContainsFocus;

            // Background
            _node.DrawEditorBackground(ref rect);

            // Grid
            _node.DrawEditorGrid(ref rect);

            base.Draw();

            // Draw debug position
            if (_debugPos.X > float.MinValue)
            {
                // Draw dot with outline
                var icon = Editor.Instance.Icons.VisjectBoxOpen32;
                var size = BlendPoint.DefaultSize * _debugScale;
                var debugPos = BlendSpacePosToBlendPointPos(_debugPos);
                var debugRect = new Rectangle(debugPos + new Float2(size * -0.5f) + size * 0.5f, new Float2(size));
                var outline = Color.Black; // Shadow
                Render2D.DrawSprite(icon, debugRect.MakeExpanded(2.0f), outline);
                Render2D.DrawSprite(icon, debugRect, style.ProgressNormal);
            }

            // Frame
            var frameColor = containsFocus ? style.BackgroundSelected : (IsMouseOver ? style.ForegroundGrey : style.ForegroundDisabled);
            Render2D.DrawRectangle(new Rectangle(1, 1, rect.Width - 2, rect.Height - 2), frameColor);
        }
    }

    public static partial class Animation
    {
        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the blending multiple animations.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public abstract class MultiBlend : SurfaceNode
        {
            private Button _addButton;
            private Button _removeButton;

            /// <summary>
            /// The blend space editor.
            /// </summary>
            protected BlendPointsEditor _editor;

            /// <summary>
            /// The selected animation label.
            /// </summary>
            protected readonly Label _selectedAnimationLabel;

            /// <summary>
            /// The selected animation combobox;
            /// </summary>
            protected readonly ComboBox _selectedAnimation;

            /// <summary>
            /// The animation picker.
            /// </summary>
            protected readonly AssetPicker _animationPicker;

            /// <summary>
            /// The animation speed label.
            /// </summary>
            protected readonly Label _animationSpeedLabel;

            /// <summary>
            /// The animation speed editor.
            /// </summary>
            protected readonly FloatValueBox _animationSpeed;

            /// <summary>
            /// Flag for editor UI updating. Used to skip value change events to prevent looping data flow.
            /// </summary>
            protected bool _isUpdatingUI;

            /// <summary>
            /// The maximum animations amount to blend per node.
            /// </summary>
            public const int MaxAnimationsCount = 255;

            /// <summary>
            /// Gets or sets the index of the selected animation.
            /// </summary>
            public int SelectedAnimationIndex
            {
                get => _selectedAnimation.SelectedIndex;
                set
                {
                    OnSelectedAnimationPopupShowing(_selectedAnimation);
                    _selectedAnimation.SelectedIndex = value;
                    UpdateUI();
                }
            }

            /// <inheritdoc />
            public MultiBlend(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                var layoutOffsetY = FlaxEditor.Surface.Constants.LayoutOffsetY;

                _selectedAnimationLabel = new Label(300, 3 * layoutOffsetY, 120.0f, layoutOffsetY)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "Selected Animation:",
                    Parent = this
                };
                _selectedAnimation = new ComboBox(_selectedAnimationLabel.X, 4 * layoutOffsetY, _selectedAnimationLabel.Width)
                {
                    TooltipText = "Select blend point to view and edit it",
                    Parent = this
                };
                _selectedAnimation.PopupShowing += OnSelectedAnimationPopupShowing;
                _selectedAnimation.SelectedIndexChanged += OnSelectedAnimationChanged;

                _animationPicker = new AssetPicker(new ScriptType(typeof(FlaxEngine.Animation)), new Float2(_selectedAnimation.Left, _selectedAnimation.Bottom + 4))
                {
                    Parent = this
                };
                _animationPicker.SelectedItemChanged += OnAnimationPickerItemChanged;

                _animationSpeedLabel = new Label(_animationPicker.Left, _animationPicker.Bottom + 4, 40, TextBox.DefaultHeight)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "Speed:",
                    Parent = this
                };
                _animationSpeed = new FloatValueBox(1.0f, _animationSpeedLabel.Right + 4, _animationSpeedLabel.Y, _selectedAnimation.Right - _animationSpeedLabel.Right - 4)
                {
                    SlideSpeed = 0.01f,
                    Parent = this
                };
                _animationSpeed.ValueChanged += OnAnimationSpeedValueChanged;

                var buttonsSize = 12;
                _addButton = new Button(_selectedAnimation.Right - buttonsSize, _selectedAnimation.Bottom + 4, buttonsSize, buttonsSize)
                {
                    Text = "+",
                    TooltipText = "Add a new blend point",
                    Parent = this
                };
                _addButton.Clicked += OnAddButtonClicked;
                _removeButton = new Button(_addButton.Left - buttonsSize - 4, _addButton.Y, buttonsSize, buttonsSize)
                {
                    Text = "-",
                    TooltipText = "Remove selected blend point",
                    Parent = this
                };
                _removeButton.Clicked += OnRemoveButtonClicked;
            }

            private void OnSelectedAnimationPopupShowing(ComboBox comboBox)
            {
                var items = comboBox.Items;
                items.Clear();
                var count = _editor.PointsCount;
                for (var i = 0; i < count; i++)
                {
                    var animId = (Guid)Values[5 + i * 2];
                    var path = string.Empty;
                    if (FlaxEngine.Content.GetAssetInfo(animId, out var assetInfo))
                        path = Path.GetFileNameWithoutExtension(assetInfo.Path);
                    items.Add(string.Format("[{0}] {1}", i, path));
                }
            }

            private void OnSelectedAnimationChanged(ComboBox comboBox)
            {
                UpdateUI();
            }

            private void OnAnimationPickerItemChanged()
            {
                if (_isUpdatingUI)
                    return;

                var selectedIndex = _selectedAnimation.SelectedIndex;
                if (selectedIndex != -1)
                {
                    var index = 5 + selectedIndex * 2;
                    SetValue(index, _animationPicker.Validator.SelectedID);
                }
            }

            private void OnAnimationSpeedValueChanged()
            {
                if (_isUpdatingUI)
                    return;

                var selectedIndex = _selectedAnimation.SelectedIndex;
                if (selectedIndex != -1)
                {
                    var index = 4 + selectedIndex * 2;
                    var data0 = (Float4)Values[index];
                    data0.W = _animationSpeed.Value;
                    SetValue(index, data0);
                }
            }

            private void OnAddButtonClicked()
            {
                _editor.AddPoint();
            }

            private void OnRemoveButtonClicked()
            {
                _editor.SetAsset(SelectedAnimationIndex, Guid.Empty);
            }

            private void DrawAxis(bool vertical, Float2 start, Float2 end, ref Color gridColor, ref Color labelColor, Font labelFont, float value, bool isLast)
            {
                // Draw line
                Render2D.DrawLine(start, end, gridColor);

                // Draw label
                var labelWidth = 50.0f;
                var labelHeight = 10.0f;
                var labelMargin = 2.0f;
                string label = Utils.RoundTo2DecimalPlaces(value).ToString(System.Globalization.CultureInfo.InvariantCulture);
                var hAlign = TextAlignment.Near;
                Rectangle labelRect;
                if (vertical)
                {
                    labelRect = new Rectangle(start.X + labelMargin * 2, start.Y, labelWidth, labelHeight);
                    if (isLast)
                        return; // Don't overlap with the first horizontal label
                }
                else
                {
                    labelRect = new Rectangle(start.X + labelMargin, start.Y - labelHeight - labelMargin, labelWidth, labelHeight);
                    if (isLast)
                    {
                        labelRect.X = start.X - labelMargin - labelRect.Width;
                        hAlign = TextAlignment.Far;
                    }
                }
                Render2D.DrawText(labelFont, label, labelRect, labelColor, hAlign, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, 0.7f);
            }

            /// <summary>
            /// Custom drawing logic for blend space background.
            /// </summary>
            public virtual void DrawEditorBackground(ref Rectangle rect)
            {
                var style = Style.Current;
                Render2D.FillRectangle(rect, style.Background.AlphaMultiplied(0.5f));
                Render2D.DrawRectangle(rect, IsMouseOver ? style.TextBoxBackgroundSelected : style.TextBoxBackground);
            }

            /// <summary>
            /// Custom drawing logic for blend space grid.
            /// </summary>
            public virtual void DrawEditorGrid(ref Rectangle rect)
            {
                var style = Style.Current;
                _editor.GetPointsArea(out var pointsArea);
                var data0 = (Float4)Values[0];
                var rangeX = new Float2(data0.X, data0.Y);
                int splits = 10;
                var gridColor = style.TextBoxBackgroundSelected * 1.1f;
                var labelColor = style.ForegroundDisabled;
                var labelFont = style.FontSmall;
                //var blendArea = BlendAreaRect;
                var blendArea = pointsArea;

                for (int i = 0; i <= splits; i++)
                {
                    float alpha = (float)i / splits;
                    float x = blendArea.Left + blendArea.Width * alpha;
                    float value = Mathf.Lerp(rangeX.X, rangeX.Y, alpha);
                    DrawAxis(false, new Float2(x, rect.Height - 2), new Float2(x, 1), ref gridColor, ref labelColor, labelFont, value, i == splits);
                }
                if (_editor.Is2D)
                {
                    var rangeY = new Float2(data0.Z, data0.W);
                    for (int i = 0; i <= splits; i++)
                    {
                        float alpha = (float)i / splits;
                        float y = blendArea.Top + blendArea.Height * alpha;
                        float value = Mathf.Lerp(rangeY.X, rangeY.Y, 1.0f - alpha);
                        DrawAxis(true, new Float2(1, y), new Float2(rect.Width - 2, y), ref gridColor, ref labelColor, labelFont, value, i == splits);
                    }
                }
                else
                {
                    float y = blendArea.Center.Y;
                    Render2D.DrawLine(new Float2(1, y), new Float2(rect.Width - 2, y), gridColor);
                }
            }

            /// <summary>
            /// Updates the editor UI.
            /// </summary>
            /// <param name="selectedIndex">Index of the selected blend point.</param>
            /// <param name="isValid">if set to <c>true</c> is selection valid.</param>
            /// <param name="data0">The packed data 0.</param>
            /// <param name="data1">The packed data 1.</param>
            public virtual void UpdateUI(int selectedIndex, bool isValid, ref Float4 data0, ref Guid data1)
            {
                if (isValid)
                {
                    _animationPicker.Validator.SelectedID = data1;
                    _animationSpeed.Value = data0.W;

                    var path = string.Empty;
                    if (FlaxEngine.Content.GetAssetInfo(data1, out var assetInfo))
                        path = Path.GetFileNameWithoutExtension(assetInfo.Path);
                    _selectedAnimation.Items[selectedIndex] = string.Format("[{0}] {1}", selectedIndex, path);
                }
                else
                {
                    _animationPicker.Validator.SelectedID = Guid.Empty;
                    _animationSpeed.Value = 1.0f;
                }
                _animationPicker.Enabled = isValid;
                _animationSpeedLabel.Enabled = isValid;
                _animationSpeed.Enabled = isValid;
                _addButton.Enabled = _editor.PointsCount < MaxAnimationsCount;
                _removeButton.Enabled = isValid && data1 != Guid.Empty;
            }

            /// <summary>
            /// Updates the editor UI.
            /// </summary>
            public void UpdateUI()
            {
                if (_isUpdatingUI)
                    return;
                _isUpdatingUI = true;

                var selectedIndex = _selectedAnimation.SelectedIndex;
                var isValid = selectedIndex >= 0 && selectedIndex < _editor.PointsCount;
                Float4 data0;
                Guid data1;
                if (isValid)
                {
                    data0 = (Float4)Values[4 + selectedIndex * 2];
                    data1 = (Guid)Values[5 + selectedIndex * 2];
                }
                else
                {
                    data0 = Float4.Zero;
                    data1 = Guid.Empty;
                }
                UpdateUI(selectedIndex, isValid, ref data0, ref data1);

                _isUpdatingUI = false;
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateUI();
            }

            /// <inheritdoc />
            public override void OnSpawned(SurfaceNodeActions action)
            {
                base.OnSpawned(action);

                // Select the first animation to make setup easier
                OnSelectedAnimationPopupShowing(_selectedAnimation);
                _selectedAnimation.SelectedIndex = 0;
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateUI();
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the blending multiple animations in 1D.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class MultiBlend1D : MultiBlend
        {
            private readonly Label _animationXLabel;
            private readonly FloatValueBox _animationX;

            /// <inheritdoc />
            public MultiBlend1D(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _animationXLabel = new Label(_animationSpeedLabel.Left, _animationSpeedLabel.Bottom + 4, 40, TextBoxBase.DefaultHeight)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "X:",
                    Parent = this
                };

                _animationX = new FloatValueBox(0.0f, _animationXLabel.Right + 4, _animationXLabel.Y, _selectedAnimation.Right - _animationXLabel.Right - 4)
                {
                    SlideSpeed = 0.01f,
                    Parent = this
                };
                _animationX.ValueChanged += OnAnimationXChanged;

                var size = Width - FlaxEditor.Surface.Constants.NodeMarginX * 2.0f;
                _editor = new BlendPointsEditor(this, false, FlaxEditor.Surface.Constants.NodeMarginX, _animationX.Bottom + 4.0f, size, 120.0f);
                _editor.Parent = this;
            }

            private void OnAnimationXChanged()
            {
                if (_isUpdatingUI)
                    return;

                var selectedIndex = _selectedAnimation.SelectedIndex;
                if (selectedIndex != -1)
                {
                    var index = 4 + selectedIndex * 2;
                    var data0 = (Float4)Values[index];
                    data0.X = _animationX.Value;
                    SetValue(index, data0);
                }
            }

            /// <inheritdoc />
            public override void UpdateUI(int selectedIndex, bool isValid, ref Float4 data0, ref Guid data1)
            {
                base.UpdateUI(selectedIndex, isValid, ref data0, ref data1);

                if (isValid)
                {
                    _animationX.Value = data0.X;
                }
                else
                {
                    _animationX.Value = 0.0f;
                }
                var ranges = (Float4)Values[0];
                _animationX.MinValue = ranges.X;
                _animationX.MaxValue = ranges.Y;
                _animationXLabel.Enabled = isValid;
                _animationX.Enabled = isValid;
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the blending multiple animations in 2D.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class MultiBlend2D : MultiBlend
        {
            private readonly Label _animationXLabel;
            private readonly FloatValueBox _animationX;
            private readonly Label _animationYLabel;
            private readonly FloatValueBox _animationY;
            private Float2[] _triangles;
            private Color[] _triangleColors;
            private Float2[] _selectedTriangles;
            private Color[] _selectedColors;

            /// <inheritdoc />
            public MultiBlend2D(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _animationXLabel = new Label(_animationSpeedLabel.Left, _animationSpeedLabel.Bottom + 4, 40, TextBox.DefaultHeight)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "X:",
                    Parent = this
                };

                _animationX = new FloatValueBox(0.0f, _animationXLabel.Right + 4, _animationXLabel.Y, _selectedAnimation.Right - _animationXLabel.Right - 4)
                {
                    SlideSpeed = 0.01f,
                    Parent = this
                };
                _animationX.ValueChanged += OnAnimationXChanged;

                _animationYLabel = new Label(_animationXLabel.Left, _animationXLabel.Bottom + 4, 40, TextBox.DefaultHeight)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Text = "Y:",
                    Parent = this
                };

                _animationY = new FloatValueBox(0.0f, _animationYLabel.Right + 4, _animationYLabel.Y, _selectedAnimation.Right - _animationYLabel.Right - 4)
                {
                    SlideSpeed = 0.01f,
                    Parent = this
                };
                _animationY.ValueChanged += OnAnimationYChanged;

                var size = Width - FlaxEditor.Surface.Constants.NodeMarginX * 2.0f;
                _editor = new BlendPointsEditor(this, true, FlaxEditor.Surface.Constants.NodeMarginX, _animationY.Bottom + 4.0f, size, size);
                _editor.Parent = this;
            }

            private void OnAnimationXChanged()
            {
                if (_isUpdatingUI)
                    return;

                var selectedIndex = _selectedAnimation.SelectedIndex;
                if (selectedIndex != -1)
                {
                    var index = 4 + selectedIndex * 2;
                    var data0 = (Float4)Values[index];
                    data0.X = _animationX.Value;
                    SetValue(index, data0);
                }
            }

            private void OnAnimationYChanged()
            {
                if (_isUpdatingUI)
                    return;

                var selectedIndex = _selectedAnimation.SelectedIndex;
                if (selectedIndex != -1)
                {
                    var index = 4 + selectedIndex * 2;
                    var data0 = (Float4)Values[index];
                    data0.Y = _animationY.Value;
                    SetValue(index, data0);
                }
            }

            private void ClearTriangles()
            {
                // Remove cache
                _triangles = null;
                _triangleColors = null;
                _selectedTriangles = null;
                _selectedColors = null;
            }

            private void CacheTriangles()
            {
                // Get locations of blend point vertices
                int pointsCount = _editor.PointsCount;
                int count = 0, j = 0;
                for (int i = 0; i < pointsCount; i++)
                {
                    var animId = (Guid)Values[5 + i * 2];
                    if (animId != Guid.Empty)
                        count++;
                }
                var vertices = new Float2[count];
                for (int i = 0; i < pointsCount; i++)
                {
                    var animId = (Guid)Values[5 + i * 2];
                    if (animId != Guid.Empty)
                    {
                        var dataA = (Float4)Values[4 + i * 2];
                        vertices[j++] = new Float2(dataA.X, dataA.Y);
                    }
                }

                // Triangulate
                _triangles = FlaxEngine.Utilities.Delaunay2D.Triangulate(vertices);
                _triangleColors = null;

                // Fix incorrect triangles (mirror logic in AnimGraphBase::onNodeLoaded)
                if (_triangles == null || _triangles.Length == 0)
                {
                    // Insert dummy triangles to have something working (eg. blend points are on the same axis)
                    var triangles = new List<Float2>();
                    int verticesLeft = vertices.Length;
                    while (verticesLeft >= 3)
                    {
                        verticesLeft -= 3;
                        triangles.Add(vertices[verticesLeft + 0]);
                        triangles.Add(vertices[verticesLeft + 1]);
                        triangles.Add(vertices[verticesLeft + 2]);
                    }
                    if (verticesLeft == 1)
                    {
                        triangles.Add(vertices[0]);
                        triangles.Add(vertices[0]);
                        triangles.Add(vertices[0]);
                    }
                    else if (verticesLeft == 2)
                    {
                        triangles.Add(vertices[0]);
                        triangles.Add(vertices[1]);
                        triangles.Add(vertices[0]);
                    }
                    _triangles = triangles.ToArray();
                }

                // Project to the blend space for drawing
                for (int i = 0; i < _triangles.Length; i++)
                    _triangles[i] = _editor.BlendSpacePosToBlendPointPos(_triangles[i]);

                // Check if anything is selected
                var selectedIndex = _selectedAnimation.SelectedIndex;
                if (selectedIndex != -1)
                {
                    // Find triangles that contain selected point
                    var dataA = (Float4)Values[4 + selectedIndex * 2];
                    var pos = _editor.BlendSpacePosToBlendPointPos(new Float2(dataA.X, dataA.Y));
                    var selectedTriangles = new List<Float2>();
                    var selectedColors = new List<Color>();
                    var style = Style.Current;
                    var triangleColor = style.TextBoxBackgroundSelected.AlphaMultiplied(0.6f);
                    var selectedTriangleColor = style.BackgroundSelected.AlphaMultiplied(0.6f);
                    _triangleColors = new Color[_triangles.Length];
                    for (int i = 0; i < _triangles.Length; i += 3)
                    {
                        var is0 = Float2.NearEqual(ref _triangles[i + 0], ref pos);
                        var is1 = Float2.NearEqual(ref _triangles[i + 1], ref pos);
                        var is2 = Float2.NearEqual(ref _triangles[i + 2], ref pos);
                        if (is0 || is1 || is2)
                        {
                            selectedTriangles.Add(_triangles[i + 0]);
                            selectedTriangles.Add(_triangles[i + 1]);
                            selectedTriangles.Add(_triangles[i + 2]);
                            selectedColors.Add(is0 ? Color.White : Color.Transparent);
                            selectedColors.Add(is1 ? Color.White : Color.Transparent);
                            selectedColors.Add(is2 ? Color.White : Color.Transparent);
                        }
                        _triangleColors[i + 0] = is0 ? selectedTriangleColor : triangleColor;
                        _triangleColors[i + 1] = is1 ? selectedTriangleColor : triangleColor;
                        _triangleColors[i + 2] = is2 ? selectedTriangleColor : triangleColor;
                    }
                    _selectedTriangles = selectedTriangles.ToArray();
                    _selectedColors = selectedColors.ToArray();
                }
            }

            /// <inheritdoc />
            public override void DrawEditorGrid(ref Rectangle rect)
            {
                base.DrawEditorGrid(ref rect);

                // Draw triangulated multi blend space
                var style = Style.Current;
                if (_triangles == null)
                    CacheTriangles();
                if (_triangleColors != null && (ContainsFocus || IsMouseOver))
                    Render2D.FillTriangles(_triangles, _triangleColors);
                else
                    Render2D.FillTriangles(_triangles, style.TextBoxBackgroundSelected.AlphaMultiplied(0.6f));
                Render2D.DrawTriangles(_triangles, style.Foreground);

                // Highlight selected blend point
                var selectedIndex = _selectedAnimation.SelectedIndex;
                if (selectedIndex != -1 && selectedIndex < _editor.BlendPoints.Count && (ContainsFocus || IsMouseOver))
                {
                    var point = _editor.BlendPoints[selectedIndex];
                    if (point != null)
                    {
                        var highlightColor = point.IsMouseDown ? style.SelectionBorder : style.BackgroundSelected;
                        Render2D.PushTint(ref highlightColor);
                        Render2D.DrawTriangles(_selectedTriangles, _selectedColors);
                        Render2D.PopTint();
                    }
                }
            }

            /// <inheritdoc />
            public override void UpdateUI(int selectedIndex, bool isValid, ref Float4 data0, ref Guid data1)
            {
                base.UpdateUI(selectedIndex, isValid, ref data0, ref data1);

                if (isValid)
                {
                    _animationX.Value = data0.X;
                    _animationY.Value = data0.Y;
                }
                else
                {
                    _animationX.Value = 0.0f;
                    _animationY.Value = 0.0f;
                }
                var ranges = (Float4)Values[0];
                _animationX.MinValue = ranges.X;
                _animationX.MaxValue = ranges.Y;
                _animationY.MinValue = ranges.Z;
                _animationY.MaxValue = ranges.W;
                _animationXLabel.Enabled = isValid;
                _animationX.Enabled = isValid;
                _animationYLabel.Enabled = isValid;
                _animationY.Enabled = isValid;
                ClearTriangles();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                ClearTriangles();
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                ClearTriangles();
            }
        }
    }
}
