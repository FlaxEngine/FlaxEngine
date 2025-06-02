// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// A panel that evenly divides up available space between all of its children.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [ActorToolbox("GUI")]
    public class UniformGridPanel : ContainerControl
    {
        private Margin _slotPadding;
        private int _slotsV, _slotsH;
        private Float2 _slotSpacing;

        /// <summary>
        /// Gets or sets the padding given to each slot.
        /// </summary>
        [EditorOrder(0), Tooltip("The padding margin applied to each item slot.")]
        public Margin SlotPadding
        {
            get => _slotPadding;
            set
            {
                _slotPadding = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the amount of slots horizontally. Use 0 to don't limit it.
        /// </summary>
        [EditorOrder(10), Limit(0, 100000, 0.1f), Tooltip("The amount of slots horizontally. Use 0 to don't limit it.")]
        public int SlotsHorizontally
        {
            get => _slotsH;
            set
            {
                value = Mathf.Clamp(value, 0, 1000000);
                if (value != _slotsH)
                {
                    _slotsH = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the amount of slots vertically. Use 0 to don't limit it.
        /// </summary>
        [EditorOrder(20), Limit(0, 100000, 0.1f), Tooltip("The amount of slots vertically. Use 0 to don't limit it.")]
        public int SlotsVertically
        {
            get => _slotsV;
            set
            {
                value = Mathf.Clamp(value, 0, 1000000);
                if (value != _slotsV)
                {
                    _slotsV = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets grid slot spacing.
        /// </summary>
        [EditorOrder(30), Limit(0), Tooltip("The Grid slot spacing.")]
        public Float2 SlotSpacing
        {
            get => _slotSpacing;
            set
            {
                if (!Float2.NearEqual(ref _slotSpacing, ref value))
                {
                    _slotSpacing = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UniformGridPanel"/> class.
        /// </summary>
        public UniformGridPanel()
        : this(0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UniformGridPanel"/> class.
        /// </summary>
        /// <param name="slotPadding">The slot padding.</param>
        public UniformGridPanel(float slotPadding)
        {
            AutoFocus = false;
            _slotPadding = new Margin(slotPadding);
            _slotSpacing = new Float2(2);
            _slotsH = _slotsV = 5;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            int slotsV = _slotsV;
            int slotsH = _slotsH;
            Float2 slotSize;
            Float2 size = Size;
            bool applySpacing = true;
            APPLY_SPACING:
            if (_slotsV + _slotsH == 0)
            {
                slotSize = HasChildren ? Children[0].Size : new Float2(32);
                slotsH = Mathf.CeilToInt(size.X / slotSize.X);
                slotsV = Mathf.CeilToInt(size.Y / slotSize.Y);
            }
            else if (slotsH == 0)
            {
                slotSize = new Float2(size.Y / slotsV);
            }
            else if (slotsV == 0)
            {
                slotSize = new Float2(size.X / slotsH);
            }
            else
            {
                slotSize = new Float2(size.X / slotsH, size.Y / slotsV);
            }
            if (applySpacing && _slotSpacing != Float2.Zero)
            {
                applySpacing = false;
                size -= _slotSpacing * new Float2(slotsH > 1 ? slotsH - 1 : 0, slotsV > 1 ? slotsV - 1 : 0);
                goto APPLY_SPACING;
            }

            int i = 0;
            for (int y = 0; y < slotsV; y++)
            {
                int end = Mathf.Min(i + slotsH, _children.Count) - i;
                if (end <= 0)
                    break;

                for (int x = 0; x < end; x++)
                {
                    var slotBounds = new Rectangle((slotSize + _slotSpacing) * new Float2(x, y), slotSize);
                    _slotPadding.ShrinkRectangle(ref slotBounds);

                    var c = _children[i++];
                    c.Bounds = slotBounds;
                }
            }
        }
    }
}
