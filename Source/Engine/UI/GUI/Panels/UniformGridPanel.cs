// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// A panel that evenly divides up available space between all of its children.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class UniformGridPanel : ContainerControl
    {
        private Margin _slotPadding;
        private int _slotsV, _slotsH;

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
        /// Initializes a new instance of the <see cref="UniformGridPanel"/> class.
        /// </summary>
        public UniformGridPanel()
        : this(2)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UniformGridPanel"/> class.
        /// </summary>
        /// <param name="slotPadding">The slot padding.</param>
        public UniformGridPanel(float slotPadding = 2)
        {
            AutoFocus = false;
            SlotPadding = new Margin(slotPadding);
            _slotsH = _slotsV = 5;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            int slotsV = _slotsV;
            int slotsH = _slotsH;
            Float2 slotSize;
            if (_slotsV + _slotsH == 0)
            {
                slotSize = HasChildren ? Children[0].Size : new Float2(32);
                slotsH = Mathf.CeilToInt(Width / slotSize.X);
                slotsV = Mathf.CeilToInt(Height / slotSize.Y);
            }
            else if (slotsH == 0)
            {
                float size = Height / slotsV;
                slotSize = new Float2(size);
            }
            else if (slotsV == 0)
            {
                float size = Width / slotsH;
                slotSize = new Float2(size);
            }
            else
            {
                slotSize = new Float2(Width / slotsH, Height / slotsV);
            }

            int i = 0;
            for (int y = 0; y < slotsV; y++)
            {
                int end = Mathf.Min(i + slotsH, _children.Count) - i;
                if (end <= 0)
                    break;

                for (int x = 0; x < end; x++)
                {
                    var slotBounds = new Rectangle(slotSize.X * x, slotSize.Y * y, slotSize.X, slotSize.Y);
                    _slotPadding.ShrinkRectangle(ref slotBounds);

                    var c = _children[i++];
                    c.Bounds = slotBounds;
                }
            }
        }
    }
}
