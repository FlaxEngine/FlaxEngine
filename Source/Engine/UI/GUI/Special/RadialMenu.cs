using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// 
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class RadialMenu : ContainerControl
    {
        [NoSerialize] private bool IsDirty = true;
        [NoSerialize] private float m_Angle;
        [NoSerialize] private float m_SelectedSegment;
        [NoSerialize] private int m_F_SelectedSegment = -1;

        private MaterialInstance MaterialInstance;
        private sbyte m_SegmentCount;
        private Color highlightColor;
        private Color forgraundColor;
        private Color selectionColor;
        private float m_EdgeOffset;
        private float m_Thickness = 0.0f;
        private float USize => Size.X < Size.Y ? Size.X : Size.Y;
        private bool ShowMatProp => MaterialInstance != null;
        private MaterialBase material;

        /// <summary>
        /// The material
        /// </summary>
        [EditorOrder(1)]
        public MaterialBase Material
        {
            get => material;
            set
            {
                material = value;
                if (material == null)
                {
                    FlaxEngine.Object.DestroyNow(MaterialInstance);
                    MaterialInstance = null;
                }
                else
                {
                    IsDirty = true;
                }
            }
        }

        /// <summary>
        /// Gets or sets the edge offset.
        /// </summary>
        /// <value>
        /// The edge offset.
        /// </value>
        [EditorOrder(2), Range(0, 1)]
        public float EdgeOffset
        {
            get
            {
                return m_EdgeOffset;
            }
            set
            {
                m_EdgeOffset = Math.Clamp(value, 0, 1);
                IsDirty = true;
                this.PerformLayout();
            }
        }
        /// <summary>
        /// Gets or sets the thickness.
        /// </summary>
        /// <value>
        /// The thickness.
        /// </value>
        [EditorOrder(3), Range(0, 1), VisibleIf("ShowMatProp")]
        public float Thickness
        {
            get
            {
                return m_Thickness;
            }
            set
            {
                m_Thickness = Math.Clamp(value, 0, 1);
                IsDirty = true;
                this.PerformLayout();
            }
        }
        /// <summary>
        /// Gets or sets control background color (transparent color (alpha=0) means no background rendering)
        /// </summary>
        [VisibleIf("ShowMatProp")]
        public new Color BackgroundColor //force overload
        {
            get => base.BackgroundColor;
            set
            {
                IsDirty = true;
                base.BackgroundColor = value;
            }
        }
        /// <summary>
        /// Gets or sets the color of the highlight.
        /// </summary>
        /// <value>
        /// The color of the highlight.
        /// </value>
        [VisibleIf("ShowMatProp")]
        public Color HighlightColor { get => highlightColor; set { IsDirty = true; highlightColor = value; } }
        /// <summary>
        /// Gets or sets the color of the forgraund.
        /// </summary>
        /// <value>
        /// The color of the forgraund.
        /// </value>
        [VisibleIf("ShowMatProp")]
        public Color ForgraundColor { get => forgraundColor; set { IsDirty = true; forgraundColor = value; } }
        /// <summary>
        /// Gets or sets the color of the selection.
        /// </summary>
        /// <value>
        /// The color of the selection.
        /// </value>
        [VisibleIf("ShowMatProp")]
        public Color SelectionColor { get => selectionColor; set { IsDirty = true; selectionColor = value; } }

        /// <summary>
        /// The selected callback
        /// </summary>
        [HideInEditor]
        public Action<int> Selected;

        /// <summary>
        /// The allow change selection when inside
        /// </summary>
        [VisibleIf("ShowMatProp")]
        public bool AllowChangeSelectionWhenInside;
        /// <summary>
        /// The center as button
        /// </summary>
        [VisibleIf("ShowMatProp")]
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
                ForgraundColor = style.BackgroundHighlighted;
                SelectionColor = style.BackgroundSelected;
            }
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

        /// <summary>
        /// Draws the control.
        /// </summary>
        public override void DrawSelf()
        {
            if (MaterialInstance != null)
            {
                if (IsDirty)
                {
                    MaterialInstance.SetParameterValue("RadialPieMenu_EdgeOffset", Math.Clamp(1 - m_EdgeOffset, 0, 1));
                    MaterialInstance.SetParameterValue("RadialPieMenu_Thickness", Math.Clamp(m_Thickness, 0, 1));
                    MaterialInstance.SetParameterValue("RadialPieMenu_Angle", ((float)1 / m_SegmentCount) * Mathf.Pi);
                    MaterialInstance.SetParameterValue("RadialPieMenu_SCount", m_SegmentCount);

                    MaterialInstance.SetParameterValue("RadialPieMenu_HighlightColor", HighlightColor);
                    MaterialInstance.SetParameterValue("RadialPieMenu_ForgraundColor", ForgraundColor);
                    MaterialInstance.SetParameterValue("RadialPieMenu_BackgroundColor", BackgroundColor);
                    MaterialInstance.SetParameterValue("RadialPieMenu_Rotation", -m_Angle + Mathf.Pi);
                    UpdateFSS();
                    IsDirty = false;
                }
                Render2D.DrawMaterial(MaterialInstance, new Rectangle(Float2.Zero, new Float2(Size.X < Size.Y ? Size.X : Size.Y)));
            }
            else
            {
                if (Material != null)
                {
                    MaterialInstance = Material.CreateVirtualInstance();
                }
            }
        }
        /// <inheritdoc/>
        public override void OnMouseMove(Float2 location)
        {
            if (MaterialInstance != null)
            {
                if (m_F_SelectedSegment == -1)
                {
                    var min = ((1 - m_EdgeOffset) - m_Thickness) * USize * 0.5f;
                    var max = (1 - m_EdgeOffset) * USize * 0.5f;
                    var val = ((USize * 0.5f) - location).Length;
                    if (Mathf.IsInRange(val, min, max) || val < min && AllowChangeSelectionWhenInside)
                    {
                        var size = new Float2(USize);
                        var p = (size * 0.5f) - location;
                        var Sa = ((float)1 / m_SegmentCount) * Mathf.TwoPi;
                        m_Angle = Mathf.Atan2(p.X, p.Y);
                        m_Angle = Mathf.Ceil((m_Angle - (Sa * 0.5f)) / Sa) * Sa;
                        m_SelectedSegment = m_Angle;
                        m_SelectedSegment = Mathf.RoundToInt((m_SelectedSegment < 0 ? Mathf.TwoPi + m_SelectedSegment : m_SelectedSegment) / Sa);
                        if (float.IsNaN(m_Angle) || float.IsInfinity(m_Angle))
                            m_Angle = 0;
                        MaterialInstance.SetParameterValue("RadialPieMenu_Rotation", -m_Angle + Mathf.Pi);
                    }
                }
            }
            base.OnMouseMove(location);
        }
        /// <inheritdoc/>
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (MaterialInstance == null)
                return base.OnMouseDown(location, button);

            var min = ((1 - m_EdgeOffset) - m_Thickness) * USize * 0.5f;
            var max = (1 - m_EdgeOffset) * USize * 0.5f;
            var val = ((USize * 0.5f) - location).Length;
            var c = val < min && CenterAsButton;
            var selected = (int)m_SelectedSegment;
            selected++;
            if (Mathf.IsInRange(val, min, max) || c)
            {
                if (c)
                {
                    m_F_SelectedSegment = 0;
                }
                else
                {
                    m_F_SelectedSegment = selected;
                }
                UpdateFSS();
                return true;
            }
            else
            {
                m_F_SelectedSegment = -1;
                UpdateFSS();
            }
            return base.OnMouseDown(location, button);
        }
        /// <inheritdoc/>
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (MaterialInstance == null)
                return base.OnMouseDown(location, button);

            if (m_F_SelectedSegment >= 0)
            {
                if (Selected != null)
                {
                    Selected(m_F_SelectedSegment);
                }
                m_F_SelectedSegment = -1;
                UpdateFSS();
                var min = ((1 - m_EdgeOffset) - m_Thickness) * USize * 0.5f;
                var max = (1 - m_EdgeOffset) * USize * 0.5f;
                var val = ((USize * 0.5f) - location).Length;
                if (Mathf.IsInRange(val, min, max) || val < min && AllowChangeSelectionWhenInside)
                {
                    var size = new Float2(USize);
                    var p = (size * 0.5f) - location;
                    var Sa = ((float)1 / m_SegmentCount) * Mathf.TwoPi;
                    m_Angle = Mathf.Atan2(p.X, p.Y);
                    m_Angle = Mathf.Ceil((m_Angle - (Sa * 0.5f)) / Sa) * Sa;
                    m_SelectedSegment = m_Angle;
                    m_SelectedSegment = Mathf.RoundToInt((m_SelectedSegment < 0 ? Mathf.TwoPi + m_SelectedSegment : m_SelectedSegment) / Sa);
                    if (float.IsNaN(m_Angle) || float.IsInfinity(m_Angle))
                        m_Angle = 0;
                    MaterialInstance.SetParameterValue("RadialPieMenu_Rotation", -m_Angle + Mathf.Pi);
                }
            }
            return base.OnMouseUp(location, button);
        }
        private void UpdateFSS()
        {
            if (m_F_SelectedSegment == -1)
            {
                MaterialInstance.SetParameterValue("RadialPieMenu_SelectionColor", ForgraundColor);
            }
            else
            {
                if (CenterAsButton)
                {
                    if (m_F_SelectedSegment > 0)
                    {
                        MaterialInstance.SetParameterValue("RadialPieMenu_SelectionColor", SelectionColor);
                    }
                    else
                    {
                        MaterialInstance.SetParameterValue("RadialPieMenu_SelectionColor", ForgraundColor);
                    }
                }
                else
                {
                    MaterialInstance.SetParameterValue("RadialPieMenu_SelectionColor", SelectionColor);
                }
            }
        }
        /// <inheritdoc/>
        public override void OnMouseLeave()
        {
            if (MaterialInstance == null)
                return;

            m_SelectedSegment = 0;
            m_F_SelectedSegment = -1;
            if (Selected != null)
            {
                Selected(m_F_SelectedSegment);
            }
            UpdateFSS();
        }
        /// <inheritdoc/>
        public override void OnChildrenChanged()
        {
            m_SegmentCount = 0;
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Image)
                {
                    m_SegmentCount++;
                }
            }
            IsDirty = true;
            base.OnChildrenChanged();
        }
        /// <inheritdoc/>
        public override void PerformLayout(bool force = false)
        {
            var Sa = -1.0f / m_SegmentCount * Mathf.TwoPi;
            var midp = USize * 0.5f;
            var mp = ((1 - m_EdgeOffset) - (m_Thickness * 0.5f)) * midp;
            float f = 0;
            if (m_SegmentCount % 2 != 0)
            {
                f += Sa * 0.5f;
            }
            if (MaterialInstance == null)
            {
                for (int i = 0; i < Children.Count; i++)
                {
                    Children[i].Center = Rotate2D(new Float2(0, mp), f) + midp;
                    f += Sa;
                }
            }
            else
            {
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is Image)
                    {
                        Children[i].Center = Rotate2D(new Float2(0, mp), f) + midp;
                        f += Sa;
                    }
                }
            }

            base.PerformLayout(force);
        }
        private Float2 Rotate2D(Float2 point, float angle)
        {
            return new Float2(Mathf.Cos(angle) * point.X + Mathf.Sin(angle) * point.Y,
            Mathf.Cos(angle) * point.Y - Mathf.Sin(angle) * point.X);
        }
    }
}
