// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Radial menu control that arranges child controls (of type Image) in a circle.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class RadialMenu : ContainerControl
    {
        private bool _materialIsDirty = true;
        private float _angle;
        private float _selectedSegment;
        private int _highlightSegment = -1;
        private MaterialBase _material;
        private MaterialInstance _materialInstance;
        private int _segmentCount;
        private Color _highlightColor;
        private Color _foregroundColor;
        private Color _selectionColor;
        private float _edgeOffset;
        private float _thickness = 0.2f;

        private float USize => Size.X < Size.Y ? Size.X : Size.Y;
        private bool ShowMatProp => _material != null;

        /// <summary>
        /// The material to use for menu background drawing.
        /// </summary>
        [EditorOrder(1)]
        public MaterialBase Material
        {
            get => _material;
            set
            {
                if (_material == value)
                    return;
                _material = value;
                Object.Destroy(ref _materialInstance);
                _materialIsDirty = true;
            }
        }

        /// <summary>
        /// Gets or sets the edge offset.
        /// </summary>
        [EditorOrder(2), Range(0, 1)]
        public float EdgeOffset
        {
            get => _edgeOffset;
            set
            {
                _edgeOffset = Math.Clamp(value, 0, 1);
                _materialIsDirty = true;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the thickness.
        /// </summary>
        [EditorOrder(3), Range(0, 1), VisibleIf(nameof(ShowMatProp))]
        public float Thickness
        {
            get => _thickness;
            set
            {
                _thickness = Math.Clamp(value, 0, 1);
                _materialIsDirty = true;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets control background color (transparent color (alpha=0) means no background rendering).
        /// </summary>
        [VisibleIf(nameof(ShowMatProp))]
        public new Color BackgroundColor
        {
            get => base.BackgroundColor;
            set
            {
                _materialIsDirty = true;
                base.BackgroundColor = value;
            }
        }

        /// <summary>
        /// Gets or sets the color of the highlight.
        /// </summary>
        [VisibleIf(nameof(ShowMatProp))]
        public Color HighlightColor
        {
            get => _highlightColor;
            set
            {
                _materialIsDirty = true;
                _highlightColor = value;
            }
        }

        /// <summary>
        /// Gets or sets the color of the foreground.
        /// </summary>
        [VisibleIf(nameof(ShowMatProp))]
        public Color ForegroundColor
        {
            get => _foregroundColor;
            set
            {
                _materialIsDirty = true;
                _foregroundColor = value;
            }
        }

        /// <summary>
        /// Gets or sets the color of the selection.
        /// </summary>
        [VisibleIf(nameof(ShowMatProp))]
        public Color SelectionColor
        {
            get => _selectionColor;
            set
            {
                _materialIsDirty = true;
                _selectionColor = value;
            }
        }

        /// <summary>
        /// The selected callback
        /// </summary>
        [HideInEditor]
        public Action<int> Selected;

        /// <summary>
        /// The allow change selection when inside
        /// </summary>
        [VisibleIf(nameof(ShowMatProp))]
        public bool AllowChangeSelectionWhenInside;

        /// <summary>
        /// The center as button
        /// </summary>
        [VisibleIf(nameof(ShowMatProp))]
        public bool CenterAsButton;

        /// <summary>
        /// Initializes a new instance of the <see cref="RadialMenu"/> class.
        /// </summary>
        public RadialMenu()
        : this(0, 0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RadialMenu"/> class.
        /// </summary>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        public RadialMenu(float x, float y, float width = 100, float height = 100)
        : base(x, y, width, height)
        {
            var style = Style.Current;
            if (style != null)
            {
                BackgroundColor = style.BackgroundNormal;
                HighlightColor = style.BackgroundSelected;
                ForegroundColor = style.BackgroundHighlighted;
                SelectionColor = style.BackgroundSelected;
            }
            _material = Content.LoadAsyncInternal<MaterialBase>("Engine/DefaultRadialMenu");
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RadialMenu"/> class.
        /// </summary>
        /// <param name="location">Position</param>
        /// <param name="size">Size</param>
        public RadialMenu(Float2 location, Float2 size)
        : this(location.X, location.Y, size.X, size.Y)
        {
        }

        /// <inheritdoc/>
        public override void DrawSelf()
        {
            if (_materialInstance == null && Material != null)
            {
                _materialInstance = Material.CreateVirtualInstance();
                _materialIsDirty = true;
            }
            if (_materialInstance != null)
            {
                if (_materialIsDirty)
                {
                    _materialInstance.SetParameterValue("RadialMenu_EdgeOffset", Math.Clamp(1 - _edgeOffset, 0, 1));
                    _materialInstance.SetParameterValue("RadialMenu_Thickness", Math.Clamp(_thickness, 0, 1));
                    _materialInstance.SetParameterValue("RadialMenu_Angle", (1.0f / _segmentCount) * Mathf.Pi);
                    _materialInstance.SetParameterValue("RadialMenu_SegmentCount", _segmentCount);
                    _materialInstance.SetParameterValue("RadialMenu_HighlightColor", _highlightColor);
                    _materialInstance.SetParameterValue("RadialMenu_ForegroundColor", _foregroundColor);
                    _materialInstance.SetParameterValue("RadialMenu_BackgroundColor", BackgroundColor);
                    _materialInstance.SetParameterValue("RadialMenu_Rotation", -_angle + Mathf.Pi);
                    UpdateSelectionColor();
                    _materialIsDirty = false;
                }
                Render2D.DrawMaterial(_materialInstance, new Rectangle(Float2.Zero, new Float2(Size.X < Size.Y ? Size.X : Size.Y)));
            }
        }

        /// <inheritdoc/>
        public override void OnMouseMove(Float2 location)
        {
            if (_materialInstance != null)
            {
                if (_highlightSegment == -1)
                {
                    var min = ((1 - _edgeOffset) - _thickness) * USize * 0.5f;
                    var max = (1 - _edgeOffset) * USize * 0.5f;
                    var val = ((USize * 0.5f) - location).Length;
                    if (Mathf.IsInRange(val, min, max) || val < min && AllowChangeSelectionWhenInside)
                    {
                        UpdateAngle(ref location);
                    }
                }
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc/>
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (_materialInstance == null)
                return base.OnMouseDown(location, button);

            var min = ((1 - _edgeOffset) - _thickness) * USize * 0.5f;
            var max = (1 - _edgeOffset) * USize * 0.5f;
            var val = ((USize * 0.5f) - location).Length;
            var c = val < min && CenterAsButton;
            var selected = (int)_selectedSegment;
            selected++;
            if (Mathf.IsInRange(val, min, max) || c)
            {
                _highlightSegment = c ? 0 : selected;
                UpdateSelectionColor();
                return true;
            }
            else
            {
                _highlightSegment = -1;
                UpdateSelectionColor();
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc/>
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_materialInstance == null)
                return base.OnMouseDown(location, button);

            if (_highlightSegment >= 0)
            {
                Selected?.Invoke(_highlightSegment);
                _highlightSegment = -1;
                UpdateSelectionColor();
                var min = ((1 - _edgeOffset) - _thickness) * USize * 0.5f;
                var max = (1 - _edgeOffset) * USize * 0.5f;
                var val = ((USize * 0.5f) - location).Length;
                if (Mathf.IsInRange(val, min, max) || val < min && AllowChangeSelectionWhenInside)
                {
                    UpdateAngle(ref location);
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc/>
        public override void OnMouseLeave()
        {
            if (_materialInstance == null)
                return;

            _selectedSegment = 0;
            _highlightSegment = -1;
            Selected?.Invoke(_highlightSegment);
            UpdateSelectionColor();
        }

        /// <inheritdoc/>
        public override void OnChildrenChanged()
        {
            _segmentCount = 0;
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Image)
                {
                    _segmentCount++;
                }
            }
            _materialIsDirty = true;

            base.OnChildrenChanged();
        }

        /// <inheritdoc/>
        public override void PerformLayout(bool force = false)
        {
            var sa = -1.0f / _segmentCount * Mathf.TwoPi;
            var midp = USize * 0.5f;
            var mp = ((1 - _edgeOffset) - (_thickness * 0.5f)) * midp;
            float f = 0;
            if (_segmentCount % 2 != 0)
            {
                f += sa * 0.5f;
            }
            if (_materialInstance == null)
            {
                for (int i = 0; i < Children.Count; i++)
                {
                    Children[i].Center = Rotate2D(new Float2(0, mp), f) + midp;
                    f += sa;
                }
            }
            else
            {
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is Image)
                    {
                        Children[i].Center = Rotate2D(new Float2(0, mp), f) + midp;
                        f += sa;
                    }
                }
            }

            base.PerformLayout(force);
        }

        private void UpdateSelectionColor()
        {
            Color color;
            if (_highlightSegment == -1)
            {
                color = _foregroundColor;
            }
            else
            {
                if (CenterAsButton)
                {
                    color = _highlightSegment > 0 ? SelectionColor : _foregroundColor;
                }
                else
                {
                    color = SelectionColor;
                }
            }
            _materialInstance.SetParameterValue("RadialMenu_SelectionColor", color);
        }

        private void UpdateAngle(ref Float2 location)
        {
            var size = new Float2(USize);
            var p = (size * 0.5f) - location;
            var sa = (1.0f / _segmentCount) * Mathf.TwoPi;
            _angle = Mathf.Atan2(p.X, p.Y);
            _angle = Mathf.Ceil((_angle - (sa * 0.5f)) / sa) * sa;
            _selectedSegment = _angle;
            _selectedSegment = Mathf.RoundToInt((_selectedSegment < 0 ? Mathf.TwoPi + _selectedSegment : _selectedSegment) / sa);
            if (float.IsNaN(_angle) || float.IsInfinity(_angle))
                _angle = 0;
            _materialInstance.SetParameterValue("RadialMenu_Rotation", -_angle + Mathf.Pi);
        }

        private static Float2 Rotate2D(Float2 point, float angle)
        {
            return new Float2(Mathf.Cos(angle) * point.X + Mathf.Sin(angle) * point.Y,
                              Mathf.Cos(angle) * point.Y - Mathf.Sin(angle) * point.X);
        }
    }
}
