// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Docking
{
    /// <summary>
    /// Dockable window mode.
    /// </summary>
    [HideInEditor]
    public enum DockState
    {
        /// <summary>
        /// The unknown state.
        /// </summary>
        Unknown = 0,

        /// <summary>
        /// The floating window.
        /// </summary>
        Float = 1,

        //DockTopAutoHide = 2,
        //DockLeftAutoHide = 3,
        //DockBottomAutoHide = 4,
        //DockRightAutoHide = 5,

        /// <summary>
        /// The dock fill as a tab.
        /// </summary>
        DockFill = 6,

        /// <summary>
        /// The dock top.
        /// </summary>
        DockTop = 7,

        /// <summary>
        /// The dock left.
        /// </summary>
        DockLeft = 8,

        /// <summary>
        /// The dock bottom.
        /// </summary>
        DockBottom = 9,

        /// <summary>
        /// The dock right.
        /// </summary>
        DockRight = 10,

        /// <summary>
        /// The hidden state.
        /// </summary>
        Hidden = 11
    }

    /// <summary>
    /// Dockable panel control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class DockPanel : ContainerControl
    {
        /// <summary>
        /// The default dock tabs header height.
        /// </summary>
        public const float DefaultHeaderHeight = 20;

        /// <summary>
        /// The default tabs header text left margin.
        /// </summary>
        public const float DefaultLeftTextMargin = 4;

        /// <summary>
        /// The default tabs header text right margin.
        /// </summary>
        public const float DefaultRightTextMargin = 8;

        /// <summary>
        /// The default tabs header buttons size.
        /// </summary>
        public const float DefaultButtonsSize = 15;

        /// <summary>
        /// The default tabs header buttons margin.
        /// </summary>
        public const float DefaultButtonsMargin = 2;

        /// <summary>
        /// The default splitters value.
        /// </summary>
        public const float DefaultSplitterValue = 0.25f;

        private readonly DockPanel _parentPanel;
        private readonly List<DockPanel> _childPanels = new List<DockPanel>();
        private readonly List<DockWindow> _tabs = new List<DockWindow>();
        private DockWindow _selectedTab;
        private DockPanelProxy _tabsProxy;

        /// <summary>
        /// Returns true if this panel is a master panel.
        /// </summary>
        public virtual bool IsMaster => false;

        /// <summary>
        /// Returns true if this panel is a floating window panel.
        /// </summary>
        public virtual bool IsFloating => false;

        /// <summary>
        /// Gets docking area bounds (tabs rectangle) in a screen space.
        /// </summary>
        public Rectangle DockAreaBounds
        {
            get
            {
                var parentWin = Root;
                if (parentWin == null)
                    throw new InvalidOperationException("Missing parent window.");
                var control = _tabsProxy != null ? (Control)_tabsProxy : this;
                var clientPos = control.PointToWindow(Float2.Zero);
                return new Rectangle(parentWin.PointToScreen(clientPos), control.Size * DpiScale);
            }
        }

        /// <summary>
        /// Gets the child panels.
        /// </summary>
        public List<DockPanel> ChildPanels => _childPanels;

        /// <summary>
        /// Gets the child panels count.
        /// </summary>
        public int ChildPanelsCount => _childPanels.Count;

        /// <summary>
        /// Gets the tabs.
        /// </summary>
        public List<DockWindow> Tabs => _tabs;

        /// <summary>
        /// Gets amount of the tabs in a dock panel.
        /// </summary>
        public int TabsCount => _tabs.Count;

        /// <summary>
        /// Gets or sets the index of the selected tab.
        /// </summary>
        public int SelectedTabIndex
        {
            get => _tabs.IndexOf(_selectedTab);
            set => SelectTab(value);
        }

        /// <summary>
        /// Gets the selected tab.
        /// </summary>
        public DockWindow SelectedTab => _selectedTab;

        /// <summary>
        /// Gets the first tab.
        /// </summary>
        public DockWindow FirstTab => _tabs.Count > 0 ? _tabs[0] : null;

        /// <summary>
        /// Gets the last tab.
        /// </summary>
        public DockWindow LastTab => _tabs.Count > 0 ? _tabs[_tabs.Count - 1] : null;

        /// <summary>
        /// Gets the parent panel.
        /// </summary>
        public DockPanel ParentDockPanel => _parentPanel;

        /// <summary>
        /// Gets the tabs header proxy.
        /// </summary>
        public DockPanelProxy TabsProxy => _tabsProxy;

        /// <summary>
        /// Initializes a new instance of the <see cref="DockPanel"/> class.
        /// </summary>
        /// <param name="parentPanel">The parent panel.</param>
        public DockPanel(DockPanel parentPanel)
        {
            AutoFocus = false;

            _parentPanel = parentPanel;
            _parentPanel?._childPanels.Add(this);

            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
        }

        /// <summary>
        /// Closes all the windows.
        /// </summary>
        /// <param name="reason">Window closing reason.</param>
        /// <returns>True if action has been cancelled (due to window internal logic).</returns>
        public bool CloseAll(ClosingReason reason = ClosingReason.CloseEvent)
        {
            while (_tabs.Count > 0)
            {
                if (_tabs[0].Close(reason))
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Gets tab at the given index.
        /// </summary>
        /// <param name="tabIndex">The index of the tab page.</param>
        /// <returns>The tab.</returns>
        public DockWindow GetTab(int tabIndex)
        {
            return _tabs[tabIndex];
        }

        /// <summary>
        /// Gets tab at the given index.
        /// </summary>
        /// <param name="tab">The tab page.</param>
        /// <returns>The index of the given tab.</returns>
        public int GetTabIndex(DockWindow tab)
        {
            return _tabs.IndexOf(tab);
        }

        /// <summary>
        /// Determines whether panel contains the specified tab.
        /// </summary>
        /// <param name="tab">The tab.</param>
        /// <returns><c>true</c> if panel contains the specified tab; otherwise, <c>false</c>.</returns>
        public bool ContainsTab(DockWindow tab)
        {
            return _tabs.Contains(tab);
        }

        /// <summary>
        /// Selects the tab page.
        /// </summary>
        /// <param name="tabIndex">The index of the tab page to select.</param>
        public void SelectTab(int tabIndex)
        {
            DockWindow tab = null;
            if (tabIndex >= 0 && _tabs.Count > tabIndex && _tabs.Count > 0)
                tab = _tabs[tabIndex];
            SelectTab(tab);
        }

        /// <summary>
        /// Selects the tab page.
        /// </summary>
        /// <param name="tab">The tab page to select.</param>
        /// <param name="autoFocus">True if focus tab after selection change.</param>
        public void SelectTab(DockWindow tab, bool autoFocus = true)
        {
            // Check if tab will change
            if (_selectedTab != tab)
            {
                // Change
                ContainerControl proxy;
                if (_selectedTab != null)
                {
                    proxy = _selectedTab.Parent;
                    _selectedTab.Parent = null;
                }
                else
                {
                    CreateTabsProxy();
                    proxy = _tabsProxy;
                }
                _selectedTab = tab;
                if (_selectedTab != null)
                {
                    _selectedTab.UnlockChildrenRecursive();
                    _selectedTab.Parent = proxy;
                    if (autoFocus)
                        _selectedTab.Focus();
                }
                OnSelectedTabChanged();
            }
            else if (autoFocus && _selectedTab != null && !_selectedTab.ContainsFocus)
            {
                _selectedTab.Focus();
            }
        }

        /// <summary>
        /// Called when selected tab gets changed.
        /// </summary>
        protected virtual void OnSelectedTabChanged()
        {
        }

        /// <summary>
        /// Performs hit test over dock panel
        /// </summary>
        /// <param name="position">Screen space position to test</param>
        /// <returns>Dock panel that has been hit or null if nothing found</returns>
        public DockPanel HitTest(ref Float2 position)
        {
            // Get parent window and transform point position into local coordinates system
            var parentWin = Root;
            if (parentWin == null)
                return null;
            var clientPos = parentWin.PointFromScreen(position);
            var localPos = PointFromWindow(clientPos);

            // Early out
            if (localPos.X < 0 || localPos.Y < 0)
                return null;
            if (localPos.X > Width || localPos.Y > Height)
                return null;

            // Test all docked controls (find the smallest one)
            float sizeLengthSquared = float.MaxValue;
            DockPanel result = null;
            for (int i = 0; i < _childPanels.Count; i++)
            {
                var panel = _childPanels[i].HitTest(ref position);
                if (panel != null)
                {
                    var sizeLen = panel.Size.LengthSquared;
                    if (sizeLen < sizeLengthSquared)
                    {
                        sizeLengthSquared = sizeLen;
                        result = panel;
                    }
                }
            }
            if (result != null)
                return result;

            // Itself
            return this;
        }

        /// <summary>
        /// Try get panel dock state
        /// </summary>
        /// <param name="splitterValue">Splitter value</param>
        /// <returns>Dock State</returns>
        public virtual DockState TryGetDockState(out float splitterValue)
        {
            DockState result = DockState.Unknown;
            splitterValue = DefaultSplitterValue;

            if (HasParent)
            {
                if (Parent.Parent is SplitPanel splitter)
                {
                    splitterValue = splitter.SplitterValue;
                    if (Parent == splitter.Panel1)
                    {
                        if (splitter.Orientation == Orientation.Horizontal)
                            result = DockState.DockLeft;
                        else
                            result = DockState.DockTop;
                    }
                    else
                    {
                        if (splitter.Orientation == Orientation.Horizontal)
                            result = DockState.DockRight;
                        else
                            result = DockState.DockBottom;
                        splitterValue = 1 - splitterValue;
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// Create child dock panel
        /// </summary>
        /// <param name="state">Dock panel state</param>
        /// <param name="splitterValue">Initial splitter value</param>
        /// <returns>Child panel</returns>
        public DockPanel CreateChildPanel(DockState state, float splitterValue)
        {
            CreateTabsProxy();

            // Create child dock panel
            var dockPanel = new DockPanel(this);

            // Switch dock mode
            Control c1;
            Control c2;
            Orientation o;
            switch (state)
            {
            case DockState.DockTop:
            {
                o = Orientation.Vertical;
                c1 = dockPanel;
                c2 = _tabsProxy;
                break;
            }
            case DockState.DockBottom:
            {
                splitterValue = 1 - splitterValue;
                o = Orientation.Vertical;
                c1 = _tabsProxy;
                c2 = dockPanel;
                break;
            }
            case DockState.DockLeft:
            {
                o = Orientation.Horizontal;
                c1 = dockPanel;
                c2 = _tabsProxy;
                break;
            }
            case DockState.DockRight:
            {
                splitterValue = 1 - splitterValue;
                o = Orientation.Horizontal;
                c1 = _tabsProxy;
                c2 = dockPanel;
                break;
            }
            default: throw new ArgumentOutOfRangeException();
            }

            // Create splitter and link controls
            var parent = _tabsProxy.Parent;
            var splitter = new SplitPanel(o, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                SplitterValue = splitterValue,
            };
            splitter.Panel1.AddChild(c1);
            splitter.Panel2.AddChild(c2);
            parent.AddChild(splitter);

            // Update
            splitter.UnlockChildrenRecursive();
            splitter.PerformLayout();

            return dockPanel;
        }

        internal void RemoveIt()
        {
            OnLastTabRemoved();
        }

        /// <summary>
        /// Called when last tab gets removed.
        /// </summary>
        protected virtual void OnLastTabRemoved()
        {
            // Check if dock panel is linked to the split panel
            if (HasParent)
            {
                if (Parent.Parent is SplitPanel splitter)
                {
                    // Check if there is another nested dock panel inside this dock panel and extract it here
                    var childPanels = _childPanels.ToArray();
                    if (childPanels.Length != 0)
                    {
                        // Move tabs from child panels into this one
                        DockWindow selectedTab = null;
                        foreach (var childPanel in childPanels)
                        {
                            var childPanelTabs = childPanel.Tabs.ToArray();
                            for (var i = 0; i < childPanelTabs.Length; i++)
                            {
                                var childPanelTab = childPanelTabs[i];
                                if (selectedTab == null && childPanelTab.IsSelected)
                                    selectedTab = childPanelTab;
                                childPanel.UndockWindow(childPanelTab);
                                AddTab(childPanelTab, false);
                            }
                        }
                        if (selectedTab != null)
                            SelectTab(selectedTab);
                    }
                    else
                    {
                        // Unlink splitter
                        var splitterParent = splitter.Parent;
                        Assert.IsNotNull(splitterParent);
                        splitter.Parent = null;

                        // Move controls from second split panel to the split panel parent
                        var scrPanel = Parent == splitter.Panel2 ? splitter.Panel1 : splitter.Panel2;
                        var srcPanelChildrenCount = scrPanel.ChildrenCount;
                        for (int i = srcPanelChildrenCount - 1; i >= 0 && scrPanel.ChildrenCount > 0; i--)
                        {
                            scrPanel.GetChild(i).Parent = splitterParent;
                        }
                        Assert.IsTrue(scrPanel.ChildrenCount == 0);
                        Assert.IsTrue(splitterParent.ChildrenCount == srcPanelChildrenCount);

                        // Delete
                        splitter.Dispose();
                    }
                }
                else if (!IsMaster)
                {
                    throw new InvalidOperationException();
                }
            }
            else if (!IsMaster)
            {
                throw new InvalidOperationException();
            }
        }

        internal virtual void DockWindowInternal(DockState state, DockWindow window, bool autoSelect = true, float? splitterValue = null)
        {
            DockWindow(state, window, autoSelect, splitterValue);
        }

        /// <summary>
        /// Docks the window.
        /// </summary>
        /// <param name="state">The state.</param>
        /// <param name="window">The window.</param>
        /// <param name="autoSelect">Whether or not to automatically select the window after docking it.</param>
        /// <param name="splitterValue">The splitter value to use when docking to window.</param>
        protected virtual void DockWindow(DockState state, DockWindow window, bool autoSelect = true, float? splitterValue = null)
        {
            CreateTabsProxy();

            // Check if dock like a tab or not
            if (state == DockState.DockFill)
            {
                // Add tab
                AddTab(window, autoSelect);
            }
            else
            {
                // Create child panel
                var dockPanel = CreateChildPanel(state, splitterValue ?? DefaultSplitterValue);

                // Dock window as a tab in a child panel
                dockPanel.DockWindow(DockState.DockFill, window);
            }
        }

        internal void UndockWindowInternal(DockWindow window)
        {
            UndockWindow(window);
        }

        /// <summary>
        /// Undocks the window.
        /// </summary>
        /// <param name="window">The window.</param>
        protected virtual void UndockWindow(DockWindow window)
        {
            var index = GetTabIndex(window);
            if (index == -1)
                throw new IndexOutOfRangeException();

            // Check if tab was selected
            if (window == SelectedTab)
            {
                // Change selection
                if (index == 0 && TabsCount > 1)
                    SelectTab(1);
                else
                    SelectTab(index - 1);
            }

            // Undock
            _tabs.RemoveAt(index);
            window.ParentDockPanel = null;

            // Check if has no more tabs
            if (_tabs.Count == 0)
            {
                OnLastTabRemoved();
            }
            else
            {
                // Update
                PerformLayout();
            }
        }

        /// <summary>
        /// Adds the tab.
        /// </summary>
        /// <param name="window">The window to insert as a tab.</param>
        /// <param name="autoSelect">True if auto-select newly added tab.</param>
        protected virtual void AddTab(DockWindow window, bool autoSelect = true)
        {
            _tabs.Add(window);
            window.ParentDockPanel = this;
            if (autoSelect)
                SelectTab(window);
        }

        private void CreateTabsProxy()
        {
            if (_tabsProxy == null)
            {
                // Create proxy and make set simple full dock
                _tabsProxy = new DockPanelProxy(this)
                {
                    Parent = this
                };
                _tabsProxy.UnlockChildrenRecursive();
            }
        }

        internal void MoveTabLeft(int index)
        {
            if (index > 0)
            {
                var tab = _tabs[index];
                _tabs.RemoveAt(index);
                _tabs.Insert(index - 1, tab);
            }
        }

        internal void MoveTabRight(int index)
        {
            if (index < _tabs.Count - 1)
            {
                var tab = _tabs[index];
                _tabs.RemoveAt(index);
                _tabs.Insert(index + 1, tab);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _parentPanel?._childPanels.Remove(this);

            base.OnDestroy();

            // Clear other tabs (not in the view)
            for (int i = 0; i < _tabs.Count; i++)
            {
                if (!_tabs[i].IsDisposing)
                {
                    _tabs[i].Dispose();
                }
            }
        }
    }
}
