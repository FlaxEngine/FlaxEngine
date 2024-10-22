// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.ComponentModel;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Helper control class for other panels.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public abstract class PanelWithMargins : ContainerControl
    {
        /// <summary>
        /// The panel area margins.
        /// </summary>
        protected Margin _margin = new Margin(2.0f);

        /// <summary>
        /// The space between the items.
        /// </summary>
        protected float _spacing = 2;

        /// <summary>
        /// The auto size flag.
        /// </summary>
        protected bool _autoSize = true;

        /// <summary>
        /// The control offset.
        /// </summary>
        protected Float2 _offset;

        /// <summary>
        /// The child controls alignment within layout area.
        /// </summary>
        protected TextAlignment _alignment = TextAlignment.Near;

        /// <summary>
        /// Gets or sets the left margin.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float LeftMargin
        {
            get => _margin.Left;
            set
            {
                if (!Mathf.NearEqual(_margin.Left, value))
                {
                    _margin.Left = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the right margin.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float RightMargin
        {
            get => _margin.Right;
            set
            {
                if (!Mathf.NearEqual(_margin.Right, value))
                {
                    _margin.Right = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the top margin.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float TopMargin
        {
            get => _margin.Top;
            set
            {
                if (!Mathf.NearEqual(_margin.Top, value))
                {
                    _margin.Top = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the bottom margin.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float BottomMargin
        {
            get => _margin.Bottom;
            set
            {
                if (!Mathf.NearEqual(_margin.Bottom, value))
                {
                    _margin.Bottom = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the child controls spacing.
        /// </summary>
        [EditorOrder(10), Tooltip("The child controls spacing (the space between controls).")]
        public float Spacing
        {
            get => _spacing;
            set
            {
                if (!Mathf.NearEqual(_spacing, value))
                {
                    _spacing = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the child controls offset (additive).
        /// </summary>
        [EditorOrder(20), Tooltip("The child controls offset (additive).")]
        public Float2 Offset
        {
            get => _offset;
            set
            {
                if (!Float2.NearEqual(ref _offset, ref value))
                {
                    _offset = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the value indicating whenever the panel size will be based on a children dimensions.
        /// </summary>
        [EditorOrder(30), DefaultValue(true), Tooltip("If checked, the panel size will be based on a children dimensions.")]
        public bool AutoSize
        {
            get => _autoSize;
            set
            {
                if (_autoSize != value)
                {
                    _autoSize = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the value indicating whenever the panel can resize children controls (eg. auto-fit width/height).
        /// </summary>
        [EditorOrder(35), DefaultValue(true), Tooltip("If checked, the panel can resize children controls (eg. auto-fit width/height).")]
        public bool ControlChildSize { get; set; } = true;

        /// <summary>
        /// Gets or sets the panel area margin.
        /// </summary>
        [EditorOrder(40), Tooltip("The panel area margin.")]
        public Margin Margin
        {
            get => _margin;
            set
            {
                if (_margin != value)
                {
                    _margin = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the child controls alignment within layout area.
        /// </summary>
        [EditorOrder(50), VisibleIf(nameof(AutoSize), true)]
        public TextAlignment Alignment
        {
            get => _alignment;
            set
            {
                if (_alignment != value)
                {
                    _alignment = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PanelWithMargins"/> class.
        /// </summary>
        protected PanelWithMargins()
        : base(0, 0, 64, 64)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise && BackgroundColor.A <= 0.0f) // Go through transparency
                return false;
            return base.ContainsPoint(ref location, precise);
        }
    }
}
