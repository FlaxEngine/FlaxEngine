// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Tabs
{
    /// <summary>
    /// Represents control which contains collection of <see cref="Tab"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class Tabs : ContainerControl
    {
        /// <summary>
        /// Tab header control. Draw the tab title and handles selecting it.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Control" />
        public class TabHeader : Control
        {
            /// <summary>
            /// The tabs control.
            /// </summary>
            protected Tabs Tabs;

            /// <summary>
            /// The index of the tab.
            /// </summary>
            protected int Index;

            /// <summary>
            /// The tab.
            /// </summary>
            protected Tab Tab;

            /// <summary>
            /// Initializes a new instance of the <see cref="TabHeader"/> class.
            /// </summary>
            /// <param name="tabs">The tabs.</param>
            /// <param name="index">The tab index.</param>
            /// <param name="tab">The tab.</param>
            public TabHeader(Tabs tabs, int index, Tab tab)
            : base(Vector2.Zero, tabs._tabsSize)
            {
                Tabs = tabs;
                Index = index;
                Tab = tab;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Vector2 location, MouseButton button)
            {
                Tabs.SelectedTabIndex = Index;
                Tabs.Focus();
                return true;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Cache data
                var style = Style.Current;
                var tabRect = new Rectangle(Vector2.Zero, Size);
                bool isTabSelected = Tabs._selectedIndex == Index;
                bool isMouseOverTab = IsMouseOver;
                var textOffset = Tabs._orientation == Orientation.Horizontal ? 0 : 8;

                // Draw bar
                if (isTabSelected)
                {
                    if (Tabs._orientation == Orientation.Horizontal)
                    {
                        Render2D.FillRectangle(tabRect, style.BackgroundSelected);
                    }
                    else
                    {
                        const float lefEdgeWidth = 4;
                        var leftEdgeRect = tabRect;
                        leftEdgeRect.Size.X = lefEdgeWidth;
                        var fillRect = tabRect;
                        fillRect.Size.X -= lefEdgeWidth;
                        fillRect.Location.X += lefEdgeWidth;
                        Render2D.FillRectangle(fillRect, style.Background);
                        Render2D.FillRectangle(leftEdgeRect, style.BackgroundSelected);
                    }
                }
                else if (isMouseOverTab)
                {
                    Render2D.FillRectangle(tabRect, style.BackgroundHighlighted);
                }

                // Draw icon
                if (Tab.Icon.IsValid)
                {
                    Render2D.DrawSprite(Tab.Icon, tabRect.MakeExpanded(-8), style.Foreground);
                }

                // Draw text
                if (!string.IsNullOrEmpty(Tab.Text))
                {
                    Render2D.DrawText(style.FontMedium, Tab.Text, new Rectangle(tabRect.X + textOffset, tabRect.Y, tabRect.Width - textOffset, tabRect.Height), style.Foreground, Tabs.TabsTextHorizontalAlignment, Tabs.TabsTextVerticalAlignment);
                }
            }
        }

        /// <summary>
        /// The tabs headers container control. Arranges the tabs headers and support scrolling them.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Panel" />
        public class TabsHeader : Panel
        {
            /// <summary>
            /// The tabs control.
            /// </summary>
            protected Tabs Tabs;

            /// <summary>
            /// Initializes a new instance of the <see cref="TabsHeader"/> class.
            /// </summary>
            /// <param name="tabs">The tabs.</param>
            public TabsHeader(Tabs tabs)
            {
                Tabs = tabs;
            }

            /// <inheritdoc />
            protected override void PerformLayoutBeforeChildren()
            {
                base.PerformLayoutBeforeChildren();

                // Cache data
                var tabsSize = Tabs._tabsSize;
                var clientSize = GetClientArea();
                tabsSize = Vector2.Min(tabsSize, clientSize.Size);
                var tabRect = new Rectangle(Vector2.Zero, tabsSize);
                var tabStripOffset = Tabs._orientation == Orientation.Horizontal ? new Vector2(tabsSize.X, 0) : new Vector2(0, tabsSize.Y);

                // Arrange tab header controls
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is TabHeader tabHeader)
                    {
                        tabHeader.Bounds = tabRect;
                        tabRect.Offset(tabStripOffset);
                    }
                }
            }
        }

        /// <summary>
        /// The selected tab index.
        /// </summary>
        protected int _selectedIndex;

        /// <summary>
        /// The tabs size.
        /// </summary>
        protected Vector2 _tabsSize;

        /// <summary>
        /// The orientation.
        /// </summary>
        protected Orientation _orientation;

        /// <summary>
        /// Gets the size of the tabs.
        /// </summary>
        public Vector2 TabsSize
        {
            get => _tabsSize;
            set
            {
                _tabsSize = value;

                for (int i = 0; i < TabsPanel.ChildrenCount; i++)
                {
                    if (TabsPanel.Children[i] is TabHeader tabHeader)
                        tabHeader.Size = value;
                }

                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the orientation.
        /// </summary>
        public Orientation Orientation
        {
            get => _orientation;
            set
            {
                _orientation = value;

                if (UseScroll)
                    TabsPanel.ScrollBars = _orientation == Orientation.Horizontal ? ScrollBars.Horizontal : ScrollBars.Vertical;

                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the color of the tab strip background.
        /// </summary>
        public Color TabStripColor
        {
            get => TabsPanel.BackgroundColor;
            set => TabsPanel.BackgroundColor = value;
        }

        /// <summary>
        /// Occurs when selected tab gets changed.
        /// </summary>
        public event Action<Tabs> SelectedTabChanged;

        /// <summary>
        /// The tabs panel control.
        /// </summary>
        public readonly TabsHeader TabsPanel;

        /// <summary>
        /// Gets or sets the selected tab.
        /// </summary>
        public Tab SelectedTab
        {
            get => _selectedIndex == -1 ? null : Children[_selectedIndex + 1] as Tab;
            set => SelectedTabIndex = Children.IndexOf(value) - 1;
        }

        /// <summary>
        /// Gets or sets the selected tab index.
        /// </summary>
        public int SelectedTabIndex
        {
            get => _selectedIndex;
            set
            {
                var index = value;
                var tabsCount = Children.Count - 1;

                // Clamp index
                if (index < -1)
                    index = -1;
                else if (index >= tabsCount)
                    index = tabsCount - 1;

                // Check if index will change
                if (_selectedIndex != index)
                {
                    SelectedTab?.OnDeselected();
                    _selectedIndex = index;
                    PerformLayout();
                    OnSelectedTabChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether use scroll bars for the tabs.
        /// </summary>
        public bool UseScroll
        {
            get => TabsPanel.ScrollBars != ScrollBars.None;
            set
            {
                if (value)
                    TabsPanel.ScrollBars = _orientation == Orientation.Horizontal ? ScrollBars.Horizontal : ScrollBars.Vertical;
                else
                    TabsPanel.ScrollBars = ScrollBars.None;
            }
        }

        /// <summary>
        /// Gets or sets the horizontal tabs text alignment within the tab titles bounds.
        /// </summary>
        public TextAlignment TabsTextHorizontalAlignment { get; set; } = TextAlignment.Near;

        /// <summary>
        /// Gets or sets the vertical tabs text alignment within the tab titles bounds.
        /// </summary>
        public TextAlignment TabsTextVerticalAlignment { get; set; } = TextAlignment.Center;

        /// <summary>
        /// Initializes a new instance of the <see cref="Tabs"/> class.
        /// </summary>
        public Tabs()
        {
            AutoFocus = false;
            BackgroundColor = Style.Current.Background;

            _selectedIndex = -1;
            _tabsSize = new Vector2(70, 16);
            _orientation = Orientation.Horizontal;

            TabsPanel = new TabsHeader(this);

            TabStripColor = Style.Current.LightBackground;

            TabsPanel.Parent = this;
        }

        /// <summary>
        /// Adds the tab.
        /// </summary>
        /// <typeparam name="T">Tab control type.</typeparam>
        /// <param name="tab">The tab.</param>
        /// <returns>The tab.</returns>
        public T AddTab<T>(T tab) where T : Tab
        {
            if (tab == null)
                throw new ArgumentNullException();
            FlaxEngine.Assertions.Assert.IsFalse(Children.Contains(tab));

            // Add
            tab.Parent = this;

            // Check if has no selected tab
            if (_selectedIndex == -1)
                SelectedTab = tab;

            return tab;
        }

        /// <summary>
        /// Called when selected tab gets changed.
        /// </summary>
        protected virtual void OnSelectedTabChanged()
        {
            SelectedTabChanged?.Invoke(this);
            SelectedTab?.OnSelected();
        }

        /// <inheritdoc />
        public override void OnChildrenChanged()
        {
            bool wasLocked = TabsPanel.IsLayoutLocked;
            TabsPanel.IsLayoutLocked = true;

            // Update tabs headers
            TabsPanel.DisposeChildren();
            int index = 0;
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Tab tab)
                {
                    var tabHeader = new TabHeader(this, index++, tab);
                    TabsPanel.AddChild(tabHeader);
                }
            }

            TabsPanel.IsLayoutLocked = wasLocked;

            base.OnChildrenChanged();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Update all controls except not selected tabs
            var selectedTab = SelectedTab;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i].Enabled && (!(_children[i] is Tab) || _children[i] == selectedTab))
                {
                    _children[i].Update(deltaTime);
                }
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            // Fit the tabs panel
            TabsPanel.Size = _orientation == Orientation.Horizontal ? new Vector2(Width, _tabsSize.Y) : new Vector2(_tabsSize.X, Height);

            // Hide all pages except selected one
            var clientArea = _orientation == Orientation.Horizontal
                             ? new Rectangle(0, _tabsSize.Y, Width, Height - _tabsSize.Y)
                             : new Rectangle(_tabsSize.X, 0, Width - _tabsSize.X, Height);
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Tab tab)
                {
                    // Check if is selected or not
                    if (i - 1 == _selectedIndex)
                    {
                        // Show and fit size
                        tab.Visible = true;
                        tab.Bounds = clientArea;
                    }
                    else
                    {
                        // Hide
                        tab.Visible = false;
                    }
                }
            }
        }
    }
}
