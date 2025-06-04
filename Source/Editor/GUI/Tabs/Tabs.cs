// Copyright (c) Wojciech Figat. All rights reserved.

using System;
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
            /// The tab.
            /// </summary>
            protected Tab Tab;

            /// <summary>
            /// Initializes a new instance of the <see cref="TabHeader"/> class.
            /// </summary>
            /// <param name="tabs">The tabs.</param>
            /// <param name="tab">The tab.</param>
            public TabHeader(Tabs tabs, Tab tab)
            : base(Float2.Zero, tabs._tabsSize)
            {
                Tabs = tabs;
                Tab = tab;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (EnabledInHierarchy && Tab.Enabled)
                {
                    Tabs.SelectedTab = Tab;
                    Tab.PerformLayout(true);
                    Tabs.Focus();
                }
                return true;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var enabled = EnabledInHierarchy && Tab.EnabledInHierarchy;
                var tabRect = new Rectangle(Float2.Zero, Size);
                var textOffset = Tabs._orientation == Orientation.Horizontal ? 0 : 8;

                // Draw bar
                if (Tabs.SelectedTab == Tab)
                {
                    var color = style.BackgroundSelected;
                    if (!enabled)
                        color *= 0.6f;
                    if (Tabs._orientation == Orientation.Horizontal)
                    {
                        Render2D.FillRectangle(tabRect, color);
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
                        Render2D.FillRectangle(leftEdgeRect, color);
                    }
                }
                else if (IsMouseOver && enabled)
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

                // Arrange tab header controls
                var pos = Float2.Zero;
                var sizeMask = Tabs._orientation == Orientation.Horizontal ? Float2.UnitX : Float2.UnitY;
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is TabHeader tabHeader)
                    {
                        tabHeader.Location = pos;
                        pos += tabHeader.Size * sizeMask;
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
        protected Float2 _tabsSize;

        /// <summary>
        /// Automatic tab size based on the fill axis.
        /// </summary>
        protected bool _autoTabsSizeAuto;

        /// <summary>
        /// The orientation.
        /// </summary>
        protected Orientation _orientation;

        /// <summary>
        /// Gets the size of the tabs.
        /// </summary>
        public Float2 TabsSize
        {
            get => _tabsSize;
            set
            {
                _tabsSize = value;
                if (!_autoTabsSizeAuto)
                {
                    for (int i = 0; i < TabsPanel.ChildrenCount; i++)
                    {
                        if (TabsPanel.Children[i] is TabHeader tabHeader)
                            tabHeader.Size = value;
                    }
                }
                PerformLayout();
            }
        }

        /// <summary>
        /// Enables automatic tabs size to fill the space.
        /// </summary>
        public bool AutoTabsSize
        {
            get => _autoTabsSizeAuto;
            set
            {
                if (_autoTabsSizeAuto == value)
                    return;
                _autoTabsSizeAuto = value;
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
                if (_orientation == value)
                    return;
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
            get => _selectedIndex < 0 || Children.Count <= (_selectedIndex+1) ? null : Children[_selectedIndex + 1] as Tab;
            set => SelectedTabIndex = value != null ? Children.IndexOf(value) - 1 : -1;
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
                    var prev = SelectedTab;
                    if (prev != null)
                    {
                        prev._selectedInTabs = null;
                        prev.OnDeselected();
                    }
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
            _tabsSize = new Float2(70, 16);
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
            var selectedTab = SelectedTab;
            SelectedTabChanged?.Invoke(this);
            if (selectedTab != null)
            {
                selectedTab._selectedInTabs = this;
                selectedTab.OnSelected();
            }
        }

        /// <inheritdoc />
        public override void OnChildrenChanged()
        {
            bool wasLocked = TabsPanel.IsLayoutLocked;
            TabsPanel.IsLayoutLocked = true;

            // Update tabs headers
            TabsPanel.DisposeChildren();
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Tab tab)
                    TabsPanel.AddChild(tab.CreateHeader());
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
            var tabsSize = _tabsSize;
            if (_autoTabsSizeAuto)
            {
                // Horizontal is default for Toolbox so tabs go to the right
                int tabsCount = 0;
                for (int i = 0; i < TabsPanel.ChildrenCount; i++)
                {
                    if (TabsPanel.Children[i] is TabHeader)
                        tabsCount++;
                }
                if (tabsCount == 0)
                    tabsCount = 1;
                if (_orientation == Orientation.Horizontal)
                    tabsSize.X = Width / tabsCount;
                else
                    tabsSize.Y = Height / tabsCount;
                for (int i = 0; i < TabsPanel.ChildrenCount; i++)
                {
                    if (TabsPanel.Children[i] is TabHeader tabHeader)
                        tabHeader.Size = tabsSize;
                }
            }
            else if (UseScroll)
            {
                // If scroll bar is visible it covers part of the tab header so include this in tab size to improve usability
                if (_orientation == Orientation.Horizontal && TabsPanel.HScrollBar.Visible)
                {
                    tabsSize.Y += TabsPanel.HScrollBar.Height;
                    var style = Style.Current;
                    TabsPanel.HScrollBar.TrackColor = style.Background;
                    TabsPanel.HScrollBar.ThumbColor = style.ForegroundGrey;
                }
                else if (_orientation == Orientation.Vertical && TabsPanel.VScrollBar.Visible)
                {
                    tabsSize.X += TabsPanel.VScrollBar.Width;
                    var style = Style.Current;
                    TabsPanel.VScrollBar.TrackColor = style.Background;
                    TabsPanel.VScrollBar.ThumbColor = style.ForegroundGrey;
                }
            }

            // Fit the tabs panel
            TabsPanel.Size = _orientation == Orientation.Horizontal
                             ? new Float2(Width, tabsSize.Y)
                             : new Float2(tabsSize.X, Height);

            // Hide all pages except selected one
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Tab tab)
                {
                    if (i - 1 == _selectedIndex)
                    {
                        // Show and fit size
                        tab.Visible = true;
                        tab.Bounds = _orientation == Orientation.Horizontal
                                     ? new Rectangle(0, tabsSize.Y, Width, Height - tabsSize.Y)
                                     : new Rectangle(tabsSize.X, 0, Width - tabsSize.X, Height);
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
