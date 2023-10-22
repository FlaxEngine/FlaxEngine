// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// GUI control that contains two child panels and the splitter between them.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.SplitContainer{Panel,Panel}" />
    [HideInEditor]
    public class SplitPanel : SplitContainer<Panel,Panel>
    {
        /// <summary>
        /// The first panel (left or upper based on Orientation).
        /// </summary>
        public Panel Panel1 => Control1;

        /// <summary>
        /// The second panel.
        /// </summary>
        public Panel Panel2 => Control2;

        /// <summary>
        /// Initializes a new instance of the <see cref="SplitPanel"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        /// <param name="panel1Scroll">The panel1 scroll bars.</param>
        /// <param name="panel2Scroll">The panel2 scroll bars.</param>
        public SplitPanel(Orientation orientation = Orientation.Horizontal, ScrollBars panel1Scroll = ScrollBars.Both, ScrollBars panel2Scroll = ScrollBars.Both) : 
            base(new Panel(panel1Scroll), new Panel(panel2Scroll),orientation)
        {       
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SplitPanel"/> class.
        /// </summary>
        /// <param name="panel1">The first panel.</param>
        /// <param name="panel2">The second panel.</param>
        /// <param name="orientation">The orientation.</param>     
        public SplitPanel(Panel panel1, Panel panel2, Orientation orientation = Orientation.Horizontal) :
            base(panel1, panel2, orientation)
        {
        }
    }

    /// <summary>
    /// GUI control that contains two child controls and the splitter between them.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.SplitContainer{Control,Control}" />
    [HideInEditor]
    public class SplitContainer : SplitContainer<Control,Control>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SplitContainer"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>      
        public SplitContainer(Orientation orientation = Orientation.Horizontal) :
            base(orientation)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SplitPanel"/> class.
        /// </summary>
        /// <param name="control1">The first control.</param>
        /// <param name="control2">The second control.</param>
        /// <param name="orientation">The orientation.</param>     
        public SplitContainer(Control control1, Control control2, Orientation orientation = Orientation.Horizontal) :
            base(control1, control2, orientation)
        {
        }

    }


    /// <summary>
    /// GUI control that contains two child controls and the splitter between them.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class SplitContainer<TControl1, TControl2> : ContainerControl
        where TControl1 : Control
        where TControl2 : Control
    {
        /// <summary>
        /// The splitter size (in pixels).
        /// </summary>
        public const int SplitterSize = 4;

        /// <summary>
        /// The splitter half size (in pixels).
        /// </summary>
        private const int SplitterSizeHalf = SplitterSize / 2;

        private Orientation _orientation;
        private float _splitterValue;
        private Rectangle _splitterRect;
        private bool _splitterClicked, _mouseOverSplitter;
        private bool _cursorChanged;

        /// <summary>
        /// The first control (left or upper based on Orientation).
        /// </summary>
        public readonly TControl1 Control1;

        /// <summary>
        /// The second control.
        /// </summary>
        public readonly TControl2 Control2;

        /// <summary>
        /// Gets or sets the panel orientation.
        /// </summary>
        /// <value>
        /// The orientation.
        /// </value>
        public Orientation Orientation
        {
            get => _orientation;
            set
            {
                if (_orientation != value)
                {
                    _orientation = value;
                    UpdateSplitRect();
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the splitter value (always in range [0; 1]).
        /// </summary>
        /// <value>
        /// The splitter value (always in range [0; 1]).
        /// </value>
        public float SplitterValue
        {
            get => _splitterValue;
            set
            {
                value = Mathf.Saturate(value);
                if (!Mathf.NearEqual(_splitterValue, value))
                {
                    // Set new value
                    _splitterValue = value;

                    // Calculate rectangle and update panels
                    UpdateSplitRect();
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SplitContainer{TControl1,TControl2}"/> class.
        /// </summary>
        /// <param name="control1">The first conrol.</param>
        /// <param name="control2">The second conrol.</param>
        /// <param name="orientation">The orientation.</param>
        public SplitContainer(TControl1 control1, TControl2 control2, Orientation orientation = Orientation.Horizontal)
        {
            AutoFocus = false;

            _orientation = orientation;
            _splitterValue = 0.5f;

            Control1 = control1;
            Control2 = control2;

            Control1.Parent = this;
            Control2.Parent = this;

            UpdateSplitRect();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SplitPanel"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>       
        public SplitContainer(Orientation orientation = Orientation.Horizontal)
        {
            AutoFocus = false;

            _orientation = orientation;
            _splitterValue = 0.5f;

            Control1 = System.Activator.CreateInstance(typeof(TControl1), true) as TControl1;
            Control2 = System.Activator.CreateInstance(typeof(TControl2), true) as TControl2;

            Control1.Parent = this;
            Control2.Parent = this;

            UpdateSplitRect();
        }

        private void UpdateSplitRect()
        {
            if (_orientation == Orientation.Horizontal)
            {
                var split = Mathf.RoundToInt(_splitterValue * Width);
                _splitterRect = new Rectangle(Mathf.Clamp(split - SplitterSizeHalf, 0.0f, Width), 0, SplitterSize, Height);
            }
            else
            {
                var split = Mathf.RoundToInt(_splitterValue * Height);
                _splitterRect = new Rectangle(0, Mathf.Clamp(split - SplitterSizeHalf, 0.0f, Height), Width, SplitterSize);
            }
        }

        private void StartTracking()
        {
            // Start move
            _splitterClicked = true;

            // Start capturing mouse
            StartMouseCapture();
        }

        private void EndTracking()
        {
            if (_splitterClicked)
            {
                // Clear flag
                _splitterClicked = false;

                // End capturing mouse
                EndMouseCapture();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw splitter
            var style = Style.Current;
            Render2D.FillRectangle(_splitterRect, _splitterClicked ? style.BackgroundSelected : _mouseOverSplitter ? style.BackgroundHighlighted : style.LightBackground);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            EndTracking();

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mouseOverSplitter = _splitterRect.Contains(location);

            if (_splitterClicked)
            {
                SplitterValue = _orientation == Orientation.Horizontal ? location.X / Width : location.Y / Height;
                Cursor = _orientation == Orientation.Horizontal ? CursorType.SizeWE : CursorType.SizeNS;
                _cursorChanged = true;
            }
            else if (_mouseOverSplitter)
            {
                Cursor = _orientation == Orientation.Horizontal ? CursorType.SizeWE : CursorType.SizeNS;
                _cursorChanged = true;
            }
            else if (_cursorChanged)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _splitterRect.Contains(location))
            {
                // Start moving splitter
                StartTracking();
                Focus();
                return false;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_splitterClicked)
            {
                EndTracking();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flag
            _mouseOverSplitter = false;
            if (_cursorChanged)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            EndTracking();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            UpdateSplitRect();
            PerformLayout();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            if (_orientation == Orientation.Horizontal)
            {
                var split = Mathf.RoundToInt(_splitterValue * Width);
                Control1.Bounds = new Rectangle(0, 0, split - SplitterSizeHalf, Height);
                Control2.Bounds = new Rectangle(split + SplitterSizeHalf, 0, Width - split - SplitterSizeHalf, Height);
            }
            else
            {
                var split = Mathf.RoundToInt(_splitterValue * Height);
                Control1.Bounds = new Rectangle(0, 0, Width, split - SplitterSizeHalf);
                Control2.Bounds = new Rectangle(0, split + SplitterSizeHalf, Width, Height - split - SplitterSizeHalf);
            }
        }
    }
}
