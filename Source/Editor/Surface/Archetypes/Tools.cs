// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Tools group.
    /// </summary>
    [HideInEditor]
    public static class Tools
    {
        private class ColorGradientNode : SurfaceNode
        {
            [HideInEditor]
            private class Gradient : ContainerControl
            {
                public ColorGradientNode Node;

                /// <inheritdoc />
                public override void Draw()
                {
                    base.Draw();

                    var style = Style.Current;
                    var bounds = new Rectangle(Float2.Zero, Size);
                    var count = (int)Node.Values[0];
                    if (count == 0)
                    {
                        Render2D.FillRectangle(bounds, Color.Black);
                    }
                    else if (count == 1)
                    {
                        Render2D.FillRectangle(bounds, (Color)Node.Values[2]);
                    }
                    else
                    {
                        var prevTime = (float)Node.Values[1];
                        var prevColor = (Color)Node.Values[2];
                        var width = Width;
                        var height = Height;

                        if (prevTime > 0.0f)
                        {
                            Render2D.FillRectangle(new Rectangle(Float2.Zero, prevTime * width, height), prevColor);
                        }

                        for (int i = 1; i < count; i++)
                        {
                            var curTime = (float)Node.Values[i * 2 + 1];
                            var curColor = (Color)Node.Values[i * 2 + 2];

                            Render2D.FillRectangle(new Rectangle(prevTime * width, 0, (curTime - prevTime) * width, height), prevColor, curColor, curColor, prevColor);

                            prevTime = curTime;
                            prevColor = curColor;
                        }

                        if (prevTime < 1.0f)
                        {
                            Render2D.FillRectangle(new Rectangle(prevTime * width, 0, (1.0f - prevTime) * width, height), prevColor);
                        }
                    }
                    Render2D.DrawRectangle(bounds, IsMouseOver ? style.BackgroundHighlighted : style.Background);
                }

                /// <inheritdoc />
                public override void OnDestroy()
                {
                    Node = null;

                    base.OnDestroy();
                }
            }

            [HideInEditor]
            private class GradientStop : Control
            {
                private bool _isMoving;
                private float _movedToTime = float.MaxValue;
                private Float2 _startMovePos;

                public ColorGradientNode Node;
                public Color Color;

                public void UpdateLocation(float time)
                {
                    Location = Node._gradient.BottomLeft + new Float2(time * Node._gradient.Width - Width * 0.5f, 0.0f);
                }

                /// <inheritdoc />
                public override void Draw()
                {
                    base.Draw();

                    var isSelected = Node._selected == this;
                    var arrowRect = new Rectangle(0, 0, 16.0f, 16.0f);
                    var arrowTransform = Matrix3x3.Translation2D(new Float2(-16.0f, -8.0f)) * Matrix3x3.RotationZ(-Mathf.PiOverTwo) * Matrix3x3.Translation2D(new Float2(8.0f, 0));
                    var color = Color;
                    if (IsMouseOver)
                        color *= 1.3f;
                    color.A = 1.0f;
                    var icons = Editor.Instance.Icons;
                    var icon = isSelected ? icons.VisjectArrowClosed32 : icons.VisjectArrowOpen32;

                    Render2D.PushTransform(ref arrowTransform);
                    Render2D.DrawSprite(icon, arrowRect, color);
                    Render2D.PopTransform();
                }

                /// <inheritdoc />
                public override bool OnMouseDown(Float2 location, MouseButton button)
                {
                    if (button == MouseButton.Left)
                    {
                        Node.Select(this);
                        _movedToTime = float.MaxValue;
                        _isMoving = true;
                        _startMovePos = location;
                        StartMouseCapture();
                        return true;
                    }

                    return base.OnMouseDown(location, button);
                }

                /// <inheritdoc />
                public override bool OnMouseUp(Float2 location, MouseButton button)
                {
                    if (button == MouseButton.Left && _isMoving)
                    {
                        _isMoving = false;
                        if (_movedToTime < float.MaxValue)
                        {
                            int index = Node._stops.IndexOf(this);
                            Node.SetStopTime(index, _movedToTime);
                            _movedToTime = float.MaxValue;
                        }
                        EndMouseCapture();
                    }

                    return base.OnMouseUp(location, button);
                }

                /// <inheritdoc />
                public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
                {
                    // Don't show tooltip is user is moving the stop
                    return base.OnShowTooltip(out text, out location, out area) && !_isMoving;
                }

                /// <inheritdoc />
                public override bool OnTestTooltipOverControl(ref Float2 location)
                {
                    // Don't show tooltip is user is moving the stop
                    return base.OnTestTooltipOverControl(ref location) && !_isMoving;
                }

                /// <inheritdoc />
                public override void OnMouseMove(Float2 location)
                {
                    if (_isMoving && Float2.DistanceSquared(ref location, ref _startMovePos) > 25.0f)
                    {
                        _startMovePos = Float2.Minimum;
                        int index = Node._stops.IndexOf(this);
                        _movedToTime = (PointToParent(location).X - Node._gradient.BottomLeft.X) / Node._gradient.Width;
                        _movedToTime = Node.ClampStopTime(index, _movedToTime);
                        UpdateLocation(_movedToTime);
                    }

                    base.OnMouseMove(location);
                }

                /// <inheritdoc />
                public override void OnEndMouseCapture()
                {
                    _isMoving = false;
                    _movedToTime = float.MaxValue;

                    base.OnEndMouseCapture();
                }
            }

            private Gradient _gradient;
            private readonly List<GradientStop> _stops = new List<GradientStop>();
            private GradientStop _selected;
            private Button _addButton;
            private Button _removeButton;
            private Label _labelValue;
            private FloatValueBox _timeValue;
            private ColorValueBox _colorValue;
            private const int MaxStops = 8;

            /// <inheritdoc />
            public ColorGradientNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                var upperLeft = GetBox(0).BottomLeft;
                var upperRight = GetBox(1).BottomRight;
                float gradientMargin = 20.0f;

                _gradient = new Gradient
                {
                    Node = this,
                    Bounds = new Rectangle(upperLeft + new Float2(gradientMargin, 10.0f), upperRight.X - upperLeft.X - gradientMargin * 2.0f, 40.0f),
                    Parent = this,
                };

                var controlsLevel = _gradient.Bottom + 4.0f + 20.0f + 40.0f;

                _labelValue = new Label(_gradient.Left, controlsLevel - 20.0f, 70.0f, 20.0f)
                {
                    Text = "Selected:",
                    VerticalAlignment = TextAlignment.Center,
                    HorizontalAlignment = TextAlignment.Near,
                    Parent = this
                };

                _timeValue = new FloatValueBox(0.0f, _gradient.Left, controlsLevel, 100.0f, 0.0f, 1.0f, 0.001f)
                {
                    Parent = this
                };
                _timeValue.ValueChanged += OnTimeValueChanged;
                _timeValue.SlidingEnd += OnTimeValueChanged;

                _colorValue = new ColorValueBox(Color.Black, _timeValue.Right + 4.0f, controlsLevel)
                {
                    Height = _timeValue.Height,
                    Parent = this
                };
                _colorValue.ValueChanged += OnColorValueChanged;

                _removeButton = new Button(_colorValue.Right + 4.0f, controlsLevel, 20, 20)
                {
                    Text = "-",
                    TooltipText = "Remove selected gradient stop",
                    Parent = this
                };
                _removeButton.Clicked += OnRemoveButtonClicked;

                _addButton = new Button(_gradient.Right - 20.0f, controlsLevel, 20, 20)
                {
                    Text = "+",
                    TooltipText = "Add gradient stop",
                    Parent = this
                };
                _addButton.Clicked += OnAddButtonClicked;

                UpdateStops();
            }

            private void OnAddButtonClicked()
            {
                var values = (object[])Values.Clone();
                var count = (int)values[0];

                var time = 0.0f;
                var color = Color.Black;
                var index = 0;
                if (count == 1)
                {
                    index = 1;
                    time = 1.0f;
                    color = Color.White;
                }
                else if (count > 1)
                {
                    index = 1;
                    var left = 1;
                    var right = 3;
                    time = ((float)values[left] + (float)values[right]) / 2;
                    color = Color.Lerp((Color)values[left + 1], (Color)values[right + 1], time);

                    // Shift higher stops to have empty place at stop 1
                    Array.Copy(values, 3, values, 5, count * 2 - 2);
                }

                // Insert
                values[1 + index * 2] = time;
                values[1 + index * 2 + 1] = color;

                values[0] = count + 1;

                SetValues(values);
            }

            private void OnRemoveButtonClicked()
            {
                var values = (object[])Values.Clone();
                var index = _stops.IndexOf(_selected);
                _selected = null;

                var count = (int)values[0];
                if (count > 0)
                    Array.Copy(values, 1 + index * 2 + 2, values, 1 + index * 2, (count - index - 1) * 2);
                values[0] = count - 1;

                SetValues(values);
            }

            private void OnTimeValueChanged()
            {
                var time = _timeValue.Value;
                var index = _stops.IndexOf(_selected);
                if (_timeValue.IsSliding)
                {
                    // Preview-only while sliding
                    time = ClampStopTime(index, time);
                    _selected.UpdateLocation(time);
                    return;
                }
                SetStopTime(index, time);
            }

            private void OnColorValueChanged()
            {
                var index = _stops.IndexOf(_selected);
                SetStopColor(index, _colorValue.Value);
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateStops();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _gradient = null;
                _stops.Clear();

                base.OnDestroy();
            }

            /// <inheritdoc />
            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _gradient.Enabled = canEdit;
                _addButton.Enabled = canEdit;
                _removeButton.Enabled = canEdit;
                _timeValue.Enabled = canEdit;
                _colorValue.Enabled = canEdit;
            }

            private void Select(GradientStop stop)
            {
                _selected = stop;
                UpdateStops();
            }

            private float ClampStopTime(int index, float time)
            {
                time = Mathf.Saturate(time);
                if (index != 0)
                {
                    time = Mathf.Max(time, (float)Values[1 + index * 2 - 2]);
                }
                if (index != _stops.Count - 1)
                {
                    time = Mathf.Min(time, (float)Values[1 + index * 2 + 2]);
                }
                return time;
            }

            private void SetStopTime(int index, float time)
            {
                time = ClampStopTime(index, time);
                SetValue(1 + index * 2, time);
            }

            private void SetStopColor(int index, Color color)
            {
                SetValue(1 + index * 2 + 1, color);
            }

            private void UpdateStops()
            {
                var count = (int)Values[0];

                // Remove unused stops
                while (_stops.Count > count)
                {
                    var last = _stops.Count - 1;
                    _stops[last].Dispose();
                    _stops.RemoveAt(last);
                }

                // Add missing stops
                while (_stops.Count < count)
                {
                    var stop = new GradientStop
                    {
                        AutoFocus = false,
                        Node = this,
                        Size = new Float2(16.0f, 16.0f),
                        Parent = this,
                    };
                    _stops.Add(stop);
                }

                // Update stops
                for (var i = 0; i < _stops.Count; i++)
                {
                    var stop = _stops[i];
                    var time = (float)Values[i * 2 + 1];
                    stop.UpdateLocation(time);
                    stop.Color = (Color)Values[i * 2 + 2];
                    stop.TooltipText = stop.Color + " at " + time;
                }

                // Update UI
                if (_selected != null)
                {
                    var index = _stops.IndexOf(_selected);
                    _timeValue.Value = (float)Values[index * 2 + 1];
                    _colorValue.Value = (Color)Values[index * 2 + 1 + 1];

                    _labelValue.Visible = true;
                    _timeValue.Visible = true;
                    _colorValue.Visible = true;
                    _removeButton.Visible = true;
                }
                else
                {
                    _labelValue.Visible = false;
                    _timeValue.Visible = false;
                    _colorValue.Visible = false;
                    _removeButton.Visible = false;
                }
                _addButton.Enabled = count < MaxStops;
            }
        }

        private class CurveNode<T> : SurfaceNode where T : struct
        {
            private BezierCurveEditor<T> _curve;
            private bool _isSavingCurve;

            public static NodeArchetype GetArchetype(ushort typeId, string title, Type valueType, T zero, T one)
            {
                return new NodeArchetype
                {
                    TypeID = typeId,
                    Title = title,
                    Create = (id, context, arch, groupArch) => new CurveNode<T>(id, context, arch, groupArch),
                    Description = "An animation spline represented by a set of keyframes, each representing an endpoint of a Bezier curve.",
                    Flags = NodeFlags.AllGraphs,
                    Size = new Float2(400, 180.0f),
                    DefaultValues = new object[]
                    {
                        // Keyframes count
                        2,

                        // Key 0
                        0.0f, // Time
                        zero, // Value
                        zero, // Tangent In
                        zero, // Tangent Out

                        // Key 1
                        1.0f, // Time
                        one, // Value
                        zero, // Tangent In
                        zero, // Tangent Out

                        // Empty keys zero-6
                        0.0f, zero, zero, zero,
                        0.0f, zero, zero, zero,
                        0.0f, zero, zero, zero,
                        0.0f, zero, zero, zero,
                        0.0f, zero, zero, zero,
                    },
                    Elements = new[]
                    {
                        NodeElementArchetype.Factory.Input(0, "Time", true, typeof(float), 0),
                        NodeElementArchetype.Factory.Output(0, "Value", valueType, 1),
                    }
                };
            }

            /// <inheritdoc />
            public CurveNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                var upperLeft = GetBox(0).BottomLeft;
                var upperRight = GetBox(1).BottomRight;
                float curveMargin = 20.0f;

                _curve = new BezierCurveEditor<T>
                {
                    MaxKeyframes = 7,
                    Bounds = new Rectangle(upperLeft + new Float2(curveMargin, 10.0f), upperRight.X - upperLeft.X - curveMargin * 2.0f, 140.0f),
                    Parent = this
                };
                _curve.Edited += OnCurveEdited;
                _curve.UnlockChildrenRecursive();
                _curve.PerformLayout();

                UpdateCurveKeyframes();
            }

            private void OnCurveEdited()
            {
                if (_isSavingCurve)
                    return;

                _isSavingCurve = true;

                var values = (object[])Values.Clone();
                var keyframes = _curve.Keyframes;
                var count = keyframes.Count;
                values[0] = count;
                for (int i = 0; i < count; i++)
                {
                    var k = keyframes[i];

                    values[i * 4 + 1] = k.Time;
                    values[i * 4 + 2] = k.Value;
                    values[i * 4 + 3] = k.TangentIn;
                    values[i * 4 + 4] = k.TangentOut;
                }
                SetValues(values);

                _isSavingCurve = false;
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                if (!_isSavingCurve)
                {
                    UpdateCurveKeyframes();
                }
            }

            private void UpdateCurveKeyframes()
            {
                var count = (int)Values[0];
                var keyframes = new BezierCurve<T>.Keyframe[count];
                for (int i = 0; i < count; i++)
                {
                    keyframes[i] = new BezierCurve<T>.Keyframe
                    {
                        Time = (float)Values[i * 4 + 1],
                        Value = (T)Values[i * 4 + 2],
                        TangentIn = (T)Values[i * 4 + 3],
                        TangentOut = (T)Values[i * 4 + 4],
                    };
                }
                _curve.SetKeyframes(keyframes);
            }

            /// <inheritdoc />
            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _curve.Enabled = canEdit;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _curve = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Surface node type for Gameplay Globals get.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        private class GetGameplayGlobalNode : SurfaceNode
        {
            private ComboBoxElement _combobox;
            private bool _isUpdating;

            /// <inheritdoc />
            public GetGameplayGlobalNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private void UpdateCombo()
            {
                if (_isUpdating)
                    return;
                _isUpdating = true;

                // Cache combo box
                if (_combobox == null)
                {
                    _combobox = (ComboBoxElement)_children[0];
                    _combobox.SelectedIndexChanged += OnSelectedChanged;
                }

                // Update items
                Type type = null;
                var toSelect = (string)Values[1];
                var asset = FlaxEngine.Content.Load<GameplayGlobals>((Guid)Values[0]);
                _combobox.ClearItems();
                if (asset)
                {
                    var values = asset.DefaultValues;
                    var tooltips = new string[values.Count];
                    var i = 0;
                    foreach (var e in values)
                    {
                        _combobox.AddItem(e.Key);
                        tooltips[i++] = "Type: " + CustomEditorsUtil.GetTypeNameUI(e.Value.GetType()) + ", default value: " + e.Value;
                        if (toSelect == e.Key)
                        {
                            type = e.Value.GetType();
                        }
                    }
                    _combobox.Tooltips = tooltips;
                }

                // Preserve selected item
                _combobox.SelectedItem = toSelect;

                // Update output value type
                var box = GetBox(0);
                if (type == null)
                {
                    box.Enabled = false;
                }
                else
                {
                    box.Enabled = true;
                    box.CurrentType = new ScriptType(type);
                }

                _isUpdating = false;
            }

            private void OnSelectedChanged(ComboBox cb)
            {
                if (_isUpdating)
                    return;

                var selected = _combobox.SelectedItem;
                if (selected != (string)Values[1])
                {
                    SetValue(1, selected);
                }
            }

            /// <inheritdoc />
            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _combobox.Enabled = canEdit;
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                UpdateCombo();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateCombo();
            }
        }

        private class ThisNode : SurfaceNode
        {
            /// <inheritdoc />
            public ThisNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                var type = ScriptType.Null;
                if (Context.Surface is VisualScriptSurface visjectSurface)
                    type = TypeUtils.GetType(visjectSurface.Script.ScriptTypeName);
                GetBox(0).CurrentType = type ? type : new ScriptType(typeof(VisualScript));
            }
        }

        private class AssetReferenceNode : SurfaceNode
        {
            /// <inheritdoc />
            public AssetReferenceNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateOutputBox();
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                UpdateOutputBox();
            }

            private void UpdateOutputBox()
            {
                var asset = FlaxEngine.Content.LoadAsync((Guid)Values[0]);
                var box = (OutputBox)GetBox(0);
                if (asset != null)
                {
                    box.CurrentType = new ScriptType(asset.GetType());
                    box.Text = System.IO.Path.GetFileNameWithoutExtension(asset.Path);
                    if (box.Text.Length > 16)
                        box.Text = string.Empty;
                }
                else
                {
                    box.CurrentType = new ScriptType(typeof(Asset));
                    box.Text = string.Empty;
                }
            }
        }

        private class ActorReferenceNode : SurfaceNode
        {
            /// <inheritdoc />
            public ActorReferenceNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                Level.SceneLoaded += OnSceneLoaded;
            }

            private void OnSceneLoaded(Scene scene, Guid sceneId)
            {
                var id = (Guid)Values[0];
                var picker = GetChild<ActorSelect>();
                if (id != Guid.Empty && picker.Value == null)
                    picker.ValueID = id;
                UpdateOutputBox();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateOutputBox();
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                UpdateOutputBox();
            }

            private void UpdateOutputBox()
            {
                var id = (Guid)Values[0];
                var actor = FlaxEngine.Object.Find<Actor>(ref id);
                var box = (OutputBox)GetBox(0);
                box.CurrentType = new ScriptType(actor != null ? actor.GetType() : typeof(Actor));
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                Level.SceneLoaded -= OnSceneLoaded;

                base.OnDestroy();
            }
        }

        internal class AsNode : SurfaceNode
        {
            private TypePickerControl _picker;

            /// <inheritdoc />
            public AsNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _picker = new TypePickerControl
                {
                    Type = ScriptType.FlaxObject,
                    Bounds = new Rectangle(FlaxEditor.Surface.Constants.NodeMarginX + 20, FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize, 160, 16),
                    Parent = this,
                };
                _picker.ValueChanged += () => SetValue(0, _picker.ValueTypeName);
            }

            /// <inheritdoc />
            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _picker.Enabled = canEdit;
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _picker.ValueTypeName = (string)Values[0];
                UpdateOutputBox();
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                if (Surface != null)
                    _picker.ValueTypeName = (string)Values[0];
                UpdateOutputBox();
            }

            private void UpdateOutputBox()
            {
                var type = TypeUtils.GetType((string)Values[0]);
                var box = (OutputBox)GetBox(0);
                box.CurrentType = type ? type : ScriptType.FlaxObject;
            }

            /// <summary>
            /// Sets the type of the picker and the type of the output box
            /// </summary>
            /// <param name="type">Target Type</param>
            public void SetPickerValue(ScriptType type)
            {
                _picker.Value = type;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _picker = null;

                base.OnDestroy();
            }
        }

        private class TypeReferenceNode : SurfaceNode
        {
            private TypePickerControl _picker;

            /// <inheritdoc />
            public TypeReferenceNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _picker = new TypePickerControl
                {
                    Type = ScriptType.Object,
                    Bounds = new Rectangle(FlaxEditor.Surface.Constants.NodeMarginX, FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize, 140, 16),
                    Parent = this,
                };
                _picker.ValueChanged += () => SetValue(0, _picker.ValueTypeName);
            }

            /// <inheritdoc />
            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _picker.Enabled = canEdit;
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _picker.ValueTypeName = (string)Values[0];
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                if (Surface != null)
                    _picker.ValueTypeName = (string)Values[0];
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _picker = null;

                base.OnDestroy();
            }
        }

        private class IsNode : SurfaceNode
        {
            private TypePickerControl _picker;

            /// <inheritdoc />
            public IsNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _picker = new TypePickerControl
                {
                    Type = ScriptType.FlaxObject,
                    Bounds = new Rectangle(FlaxEditor.Surface.Constants.NodeMarginX + 20, FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize, 160, 16),
                    Parent = this,
                };
                _picker.ValueChanged += () => SetValue(0, _picker.ValueTypeName);
            }

            /// <inheritdoc />
            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _picker.Enabled = canEdit;
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _picker.ValueTypeName = (string)Values[0];
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                if (Surface != null)
                    _picker.ValueTypeName = (string)Values[0];
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _picker = null;

                base.OnDestroy();
            }
        }

        private class CastNode : SurfaceNode
        {
            private TypePickerControl _picker;

            /// <inheritdoc />
            public CastNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch, ScriptType type)
            : base(id, context, nodeArch, groupArch)
            {
                _picker = new TypePickerControl
                {
                    Type = type,
                    Bounds = new Rectangle(FlaxEditor.Surface.Constants.NodeMarginX + 20, FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize, 160, 16),
                    Parent = this,
                };
                _picker.ValueChanged += () => SetValue(0, _picker.ValueTypeName);
            }

            /// <inheritdoc />
            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _picker.Enabled = canEdit;
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _picker.ValueTypeName = (string)Values[0];
                UpdateOutputBox();
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                if (Surface != null)
                    _picker.ValueTypeName = (string)Values[0];
                UpdateOutputBox();
            }

            private void UpdateOutputBox()
            {
                var type = TypeUtils.GetType((string)Values[0]);
                GetBox(4).CurrentType = type ? type : _picker.Type;
            }

            /// <summary>
            /// Sets the type of the picker and the type of the output box
            /// </summary>
            /// <param name="type">Target Type</param>
            public void SetPickerValue(ScriptType type)
            {
                _picker.Value = type;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _picker = null;

                base.OnDestroy();
            }
        }

        internal class RerouteNode : SurfaceNode, IConnectionInstigator
        {
            internal static readonly Float2 DefaultSize = new Float2(FlaxEditor.Surface.Constants.BoxSize);
            private Rectangle _localBounds;
            private InputBox _input;
            private OutputBox _output;
            private bool _isMouseDown, _isConnecting, _isMouseInConnectingBounds;
            private const float ConnectingBounds = -12.0f;

            /// <inheritdoc />
            public RerouteNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                Size = DefaultSize;
                _localBounds = new Rectangle(Float2.Zero, DefaultSize);
                Title = string.Empty;
                BackgroundColor = Color.Transparent;
            }

            /// <inheritdoc />
            protected override bool ShowTooltip => !string.IsNullOrEmpty(TooltipText) && _localBounds.Contains(ref _mousePosition) && !Surface.IsLeftMouseButtonDown && !Surface.IsRightMouseButtonDown && !Surface.IsPrimaryMenuOpened;

            /// <inheritdoc />
            public override bool OnTestTooltipOverControl(ref Float2 location)
            {
                return _localBounds.Contains(ref location) && ShowTooltip;
            }

            /// <inheritdoc />
            public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
            {
                var result = base.OnShowTooltip(out text, out _, out area);

                // Change the position
                location = new Float2(Width * 0.5f, Height);

                return result;
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                _input = (InputBox)GetBox(0);
                _output = (OutputBox)GetBox(1);

                _input.Location = Float2.Zero;
                _output.Location = Float2.Zero;
                _input.Visible = false;
                _output.Visible = false;

                _input.CurrentTypeChanged += OnInputBoxTypeChanged;

                UpdateBoxes();
            }

            private void OnInputBoxTypeChanged(Box inputBox)
            {
                UpdateBoxes();
            }

            /// <inheritdoc />
            public override void ConnectionTick(Box box)
            {
                base.ConnectionTick(box);

                UpdateBoxes();
            }

            private void UpdateBoxes()
            {
                if (Surface == null)
                    return;

                var type = _input.CurrentType;
                if (_input.TooltipText != null)
                {
                    TooltipText = _input.TooltipText;
                }
                else
                {
                    type = _input.HasAnyConnection ? _input.CurrentType : _output.CurrentType;
                    TooltipText = Surface.GetTypeName(type);
                }

                var isImpulse = type.IsVoid;
                _input.IsSingle = !isImpulse;
                _output.IsSingle = isImpulse;
            }

            /// <inheritdoc />
            public override bool CanSelect(ref Float2 location)
            {
                return new Rectangle(Location, DefaultSize).Contains(ref location);
            }

            /// <inheritdoc />
            protected override void UpdateRectangles()
            {
                _headerRect = Rectangle.Empty;
                _closeButtonRect = Rectangle.Empty;
                _footerRect = Rectangle.Empty;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Surface.Style;
                var connectionColor = style.Colors.Default;
                var type = ScriptType.Null;
                float barHorizontalOffset = -2;
                float barHeight = 3;

                if (_input.HasAnyConnection)
                {
                    type = _input.Connections[0].CurrentType;
                    var hints = _input.Connections[0].ParentNode.Archetype.ConnectionsHints;
                    Surface.Style.GetConnectionColor(type, hints, out connectionColor);
                }

                if (!_input.HasAnyConnection)
                    Render2D.FillRectangle(new Rectangle(-barHorizontalOffset - barHeight * 2, (DefaultSize.Y - barHeight) / 2, barHeight * 2, barHeight), connectionColor);
                if (!_output.HasAnyConnection)
                    Render2D.FillRectangle(new Rectangle(DefaultSize.X + barHorizontalOffset, (DefaultSize.Y - barHeight) / 2, barHeight * 2, barHeight), connectionColor);

                SpriteHandle icon;
                if (_input.HasAnyConnection && _output.HasAnyConnection)
                    icon = type.IsVoid ? style.Icons.ArrowClose : style.Icons.BoxClose;
                else
                    icon = type.IsVoid ? style.Icons.ArrowOpen : style.Icons.BoxOpen;
                Render2D.DrawSprite(icon, _localBounds, connectionColor);

                base.Draw();
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                    return true;

                if (button == MouseButton.Left)
                {
                    _isMouseDown = true;
                    _isConnecting = _isMouseInConnectingBounds;
                    if (_isConnecting)
                    {
                        Focus();
                        return true;
                    }
                }
                return false;
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                if (_isMouseDown)
                {
                    _isMouseDown = false;
                    if (Surface.CanEdit && _isConnecting)
                        Surface.ConnectingStart(this);
                }

                _isMouseInConnectingBounds = false;
                Cursor = CursorType.Default;

                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                _isMouseInConnectingBounds = IsMouseOver && _localBounds.MakeExpanded(ConnectingBounds).Contains(ref location); // Inner area for connecting, outer area for moving
                if (!_isMouseInConnectingBounds && !_isMouseDown)
                    Cursor = CursorType.SizeAll;
                else
                    Cursor = CursorType.Default;

                Surface.ConnectingOver(this);
                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (base.OnMouseUp(location, button))
                    return true;

                if (button == MouseButton.Left)
                {
                    _isMouseDown = false;
                    if (Surface.IsConnecting)
                        Surface.ConnectingEnd(this);
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public Float2 ConnectionOrigin
            {
                get
                {
                    var center = _localBounds.Center;
                    return PointToParent(ref center);
                }
            }

            /// <inheritdoc />
            public bool AreConnected(IConnectionInstigator other)
            {
                return _input.AreConnected(other) || _output.AreConnected(other);
            }

            /// <inheritdoc />
            public bool CanConnectWith(IConnectionInstigator other)
            {
                if (other is InputBox otherInput)
                {
                    return _output.CanConnectWith(otherInput);
                }
                if (other is OutputBox otherOutput)
                {
                    return _input.CanConnectWith(otherOutput);
                }
                if (other is RerouteNode otherReroute)
                {
                    if (_output.CurrentType.IsVoid)
                        return otherReroute._input.CanConnectWith(_output);
                    return otherReroute._output.CanConnectWith(_input);
                }
                return false;
            }

            /// <inheritdoc />
            public void DrawConnectingLine(ref Float2 startPos, ref Float2 endPos, ref Color color)
            {
                OutputBox.DrawConnection(Surface.Style, ref startPos, ref endPos, ref color, OutputBox.ConnectingConnectionThickness);
            }

            /// <inheritdoc />
            public void Connect(IConnectionInstigator other)
            {
                if (other is InputBox otherInput)
                {
                    _output.Connect(otherInput);
                }
                if (other is OutputBox otherOutput)
                {
                    _input.Connect(otherOutput);
                }
                if (other is RerouteNode otherReroute)
                {
                    if (_output.CurrentType.IsVoid)
                        otherReroute._input.Connect(_output);
                    else
                        otherReroute._output.Connect(_input);
                }
            }
        }

        private static NodeArchetype Noise(ushort id, string name, Type resultType, Type pointType, string desc)
        {
            return new NodeArchetype
            {
                TypeID = id,
                Title = name,
                Description = desc,
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Float2(150, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Point", true, pointType, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, resultType, 1),
                }
            };
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            new NodeArchetype
            {
                // [Deprecated]
                TypeID = 1,
                Title = "Fresnel",
                Description = "Calculates a falloff based on the dot product of the surface normal and the direction to the camera",
                Flags = NodeFlags.MaterialGraph | NodeFlags.NoSpawnViaGUI | NodeFlags.NoSpawnViaPaste,
                Size = new Float2(140, 60),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Exponent", true, typeof(float), 0),
                    NodeElementArchetype.Factory.Input(1, "Base Reflect Fraction", true, typeof(float), 1),
                    NodeElementArchetype.Factory.Input(2, "Normal", true, typeof(Float3), 2),
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Title = "Desaturation",
                Description = "Desaturates input, or converts the colors of its input into shades of gray, based a certain percentage",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(140, 130),
                DefaultValues = new object[]
                {
                    new Float3(0.3f, 0.59f, 0.11f)
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Input", true, typeof(Float3), 0),
                    NodeElementArchetype.Factory.Input(1, "Scale", true, typeof(float), 1),
                    NodeElementArchetype.Factory.Output(0, "Result", typeof(Float3), 2),
                    NodeElementArchetype.Factory.Text(0, Surface.Constants.LayoutOffsetY * 2 + 5, "Luminance Factors"),
                    NodeElementArchetype.Factory.Vector_X(0, Surface.Constants.LayoutOffsetY * 3 + 5, 0),
                    NodeElementArchetype.Factory.Vector_Y(0, Surface.Constants.LayoutOffsetY * 4 + 5, 0),
                    NodeElementArchetype.Factory.Vector_Z(0, Surface.Constants.LayoutOffsetY * 5 + 5, 0)
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Title = "Time",
                Description = "Game time constant",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(110, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Title = "Fresnel",
                Description = "Calculates a falloff based on the dot product of the surface normal and the direction to the camera",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(200, 60),
                DefaultValues = new object[]
                {
                    5.0f,
                    0.04f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Exponent", true, typeof(float), 0, 0),
                    NodeElementArchetype.Factory.Input(1, "Base Reflect Fraction", true, typeof(float), 1, 1),
                    NodeElementArchetype.Factory.Input(2, "Normal", true, typeof(Vector3), 2),
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Time",
                Description = "Game time constant",
                Flags = NodeFlags.AnimGraph,
                Size = new Float2(140, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Animation Time", typeof(float), 0),
                    NodeElementArchetype.Factory.Output(1, "Delta Seconds", typeof(float), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Panner",
                Description = "Animates UVs over time",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(170, 80),
                DefaultValues = new object[]
                {
                    false
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Time", true, typeof(float), 1),
                    NodeElementArchetype.Factory.Input(2, "Speed", true, typeof(Float2), 2),
                    NodeElementArchetype.Factory.Text(18, Surface.Constants.LayoutOffsetY * 3 + 5, "Fractional Part"),
                    NodeElementArchetype.Factory.Bool(0, Surface.Constants.LayoutOffsetY * 3 + 5, 0),
                    NodeElementArchetype.Factory.Output(0, "", typeof(Float2), 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Linearize Depth",
                Description = "Scene depth buffer texture lookup node",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Float2(240, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Hardware Depth", true, typeof(float), 0),
                    NodeElementArchetype.Factory.Output(0, "Linear Depth", typeof(float), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Time",
                Description = "Simulation time and update delta time access",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Float2(140, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Simulation Time", typeof(float), 0),
                    NodeElementArchetype.Factory.Output(1, "Delta Seconds", typeof(float), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 9,
                Title = "Transform Position To Screen UV",
                Description = "Transforms world-space position into screen space coordinates (normalized)",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Float2(300, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "World Space", true, typeof(Float3), 0),
                    NodeElementArchetype.Factory.Output(0, "Screen Space UV", typeof(Float2), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 10,
                Title = "Color Gradient",
                Create = (id, context, arch, groupArch) => new ColorGradientNode(id, context, arch, groupArch),
                Description = "Linear color gradient sampler",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(400, 150.0f),
                DefaultValues = new object[]
                {
                    // Stops count
                    2,

                    // Stop 0
                    0.1f,
                    Color.CornflowerBlue,

                    // Stop 1
                    0.9f,
                    Color.GreenYellow,

                    // Empty stops 2-7
                    0.0f, Color.Black,
                    0.0f, Color.Black,
                    0.0f, Color.Black,
                    0.0f, Color.Black,
                    0.0f, Color.Black,
                    0.0f, Color.Black,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Time", true, typeof(float), 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float4), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 11,
                Title = "Comment",
                AlternativeTitles = new[] { "//", "Group" },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (filterText.StartsWith("//"))
                    {
                        data = new object[]
                        {
                            filterText.Substring(2), // Title
                            new Color(1.0f, 1.0f, 1.0f, 0.2f), // Color
                            new Float2(400.0f, 400.0f), // Size
                            -1, // Order
                        };
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                },
                Create = (id, context, arch, groupArch) => new SurfaceComment(id, context, arch, groupArch),
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(400.0f, 400.0f),
                DefaultValues = new object[]
                {
                    "Comment", // Title
                    new Color(1.0f, 1.0f, 1.0f, 0.2f), // Color
                    new Float2(400.0f, 400.0f), // Size
                    -1, // Order
                },
            },
            CurveNode<float>.GetArchetype(12, "Curve", typeof(float), 0.0f, 1.0f),
            CurveNode<Float2>.GetArchetype(13, "Curve Float2", typeof(Float2), Float2.Zero, Float2.One),
            CurveNode<Float3>.GetArchetype(14, "Curve Float3", typeof(Float3), Float3.Zero, Float3.One),
            CurveNode<Float4>.GetArchetype(15, "Curve Float4", typeof(Float4), Float4.Zero, Float4.One),
            new NodeArchetype
            {
                TypeID = 16,
                Create = (id, context, arch, groupArch) => new GetGameplayGlobalNode(id, context, arch, groupArch),
                Title = "Get Gameplay Global",
                Description = "Gets the Gameplay Global variable value",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(220, 90),
                DefaultValues = new object[]
                {
                    Guid.Empty,
                    string.Empty
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, 70, 120),
                    NodeElementArchetype.Factory.Asset(0, 0, 0, typeof(GameplayGlobals)),
                    NodeElementArchetype.Factory.Output(0, "Value", null, 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 17,
                Title = "Platform Switch",
                Description = "Gets the input value based on the runtime-platform type",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(220, 240),
                ConnectionsHints = ConnectionsHint.Value,
                IndependentBoxes = new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
                DependentBoxes = new[] { 0 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 0),
                    NodeElementArchetype.Factory.Input(0, "Default", true, null, 1),
                    NodeElementArchetype.Factory.Input(1, "Windows", true, null, 2),
                    NodeElementArchetype.Factory.Input(2, "Xbox One", true, null, 3),
                    NodeElementArchetype.Factory.Input(3, "Windows Store", true, null, 4),
                    NodeElementArchetype.Factory.Input(4, "Linux", true, null, 5),
                    NodeElementArchetype.Factory.Input(5, "PlayStation 4", true, null, 6),
                    NodeElementArchetype.Factory.Input(6, "Xbox Scarlett", true, null, 7),
                    NodeElementArchetype.Factory.Input(7, "Android", true, null, 8),
                    NodeElementArchetype.Factory.Input(8, "Switch", true, null, 9),
                    NodeElementArchetype.Factory.Input(9, "PlayStation 5", true, null, 10),
                    NodeElementArchetype.Factory.Input(10, "Mac", true, null, 11),
                    NodeElementArchetype.Factory.Input(11, "iOS", true, null, 12),
                }
            },
            new NodeArchetype
            {
                TypeID = 18,
                Title = "Asset Reference",
                Create = (id, context, arch, groupArch) => new AssetReferenceNode(id, context, arch, groupArch),
                Description = "References an asset.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(200, 70),
                DefaultValues = new object[]
                {
                    Guid.Empty,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Asset), 0),
                    NodeElementArchetype.Factory.Asset(0, 0, 0, typeof(Asset)),
                }
            },
            new NodeArchetype
            {
                TypeID = 19,
                Title = "This Instance",
                Create = (id, context, arch, groupArch) => new ThisNode(id, context, arch, groupArch),
                Description = "Gets the reference to this script object instance (self).",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(140, 20),
                AlternativeTitles = new[]
                {
                    "self",
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Instance", typeof(VisualScript), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 20,
                Title = "To String",
                Description = "Converts the value into a human-readable string representation.",
                ConnectionsHints = ConnectionsHint.Anything,
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(140, 20),
                AlternativeTitles = new[]
                {
                    "tostring",
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(string), 0),
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, null, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 21,
                Title = "Actor Reference",
                Create = (id, context, arch, groupArch) => new ActorReferenceNode(id, context, arch, groupArch),
                Description = "References an actor.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(200, 20),
                DefaultValues = new object[]
                {
                    Guid.Empty,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Asset), 0),
                    NodeElementArchetype.Factory.Actor(0, 0, 0, typeof(Actor), 160),
                }
            },
            new NodeArchetype
            {
                TypeID = 22,
                Title = "As",
                AlternativeTitles = new[] { "Cast" },
                Create = (id, context, arch, groupArch) => new AsNode(id, context, arch, groupArch),
                Description = "Casts the object to a different type. Returns null if cast fails.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(200, 20),
                DefaultValues = new object[]
                {
                    string.Empty, // Typename
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(FlaxEngine.Object), 0),
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, typeof(FlaxEngine.Object), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 23,
                Title = "Type Reference",
                Create = (id, context, arch, groupArch) => new TypeReferenceNode(id, context, arch, groupArch),
                Description = "Scripting type picker.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(200, 40),
                DefaultValues = new object[]
                {
                    string.Empty, // Typename
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Type", typeof(Type), 0),
                    NodeElementArchetype.Factory.Output(1, "Type Name", typeof(string), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 24,
                Title = "Is",
                Create = (id, context, arch, groupArch) => new IsNode(id, context, arch, groupArch),
                Description = "Checks if the object is of the given type. Return true if so, false otherwise.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(200, 20),
                DefaultValues = new object[]
                {
                    string.Empty, // Typename
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 0),
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, typeof(FlaxEngine.Object), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 25,
                Title = "Cast",
                Create = (id, context, arch, groupArch) => new CastNode(id, context, arch, groupArch, new ScriptType(typeof(FlaxEngine.Object))),
                Description = "Tries to cast the object to a given type. Returns null if fails.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(200, 60),
                DefaultValues = new object[]
                {
                    string.Empty, // Typename
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, string.Empty, true, typeof(FlaxEngine.Object), 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(void), 2, true),
                    NodeElementArchetype.Factory.Output(1, "Failed", typeof(void), 3, true),
                    NodeElementArchetype.Factory.Output(2, string.Empty, typeof(FlaxEngine.Object), 4),
                }
            },
            new NodeArchetype
            {
                TypeID = 26,
                Title = "Cast Value",
                Create = (id, context, arch, groupArch) => new CastNode(id, context, arch, groupArch, ScriptType.Object),
                Description = "Tries to cast the object to a given type. Returns null if fails.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(200, 60),
                DefaultValues = new object[]
                {
                    string.Empty, // Typename
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, string.Empty, true, typeof(object), 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(void), 2, true),
                    NodeElementArchetype.Factory.Output(1, "Failed", typeof(void), 3, true),
                    NodeElementArchetype.Factory.Output(2, string.Empty, typeof(object), 4),
                }
            },
            new NodeArchetype
            {
                TypeID = 27,
                Title = "Is Null",
                Description = "Checks if the object is null. Return false if it's valid.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(150, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 0),
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, typeof(object), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 28,
                Title = "Is Valid",
                Description = "Checks if the object is valid. Return false if it's null.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(150, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 0),
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, typeof(object), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 29,
                Title = "Reroute",
                Create = (id, context, arch, groupArch) => new RerouteNode(id, context, arch, groupArch),
                Description = "Reroute a connection.",
                Flags = NodeFlags.NoCloseButton | NodeFlags.NoSpawnViaGUI | NodeFlags.AllGraphs,
                Size = RerouteNode.DefaultSize,
                ConnectionsHints = ConnectionsHint.All,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1, true),
                }
            },
            Noise(30, "Perlin Noise", typeof(float), typeof(Float2), "Classic Perlin noise (normalized to 0-1)."),
            Noise(31, "Simplex Noise", typeof(float), typeof(Float2), "Simplex noise (normalized to 0-1)."),
            Noise(32, "Worley Noise", typeof(Float2), typeof(Float2), "Worley noise (cellar noise with standard 3x3 search window for F1 and F2 values)."),
            Noise(33, "Voronoi Noise", typeof(Float3), typeof(Float2), "Voronoi noise (X=minDistToCell, Y=randomColor, Z=minEdgeDistance)."),
            Noise(34, "Custom Noise", typeof(float), typeof(Float3), "Custom noise function (3D -> 1D)."),
        };
    }
}
