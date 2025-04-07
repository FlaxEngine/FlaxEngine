// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.GUI.Docking
{
    /// <summary>
    /// Master Dock Panel control used as a root control for dockable windows workspace.
    /// </summary>
    /// <seealso cref="DockPanel" />
    public class MasterDockPanel : DockPanel
    {
        /// <summary>
        /// Array with all created dock windows for that master panel.
        /// </summary>
        public readonly List<DockWindow> Windows = new List<DockWindow>(32);

        /// <summary>
        /// Array with all floating windows for that master panel.
        /// </summary>
        public readonly List<FloatWindowDockPanel> FloatingPanels = new List<FloatWindowDockPanel>(4);

        /// <summary>
        /// Gets the visible windows count.
        /// </summary>
        /// <value>
        /// The visible windows count.
        /// </value>
        public int VisibleWindowsCount
        {
            get
            {
                int result = 0;
                for (int i = 0; i < Windows.Count; i++)
                {
                    if (Windows[i].Visible)
                        result++;
                }
                return result;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MasterDockPanel"/> class.
        /// </summary>
        public MasterDockPanel()
        : base(null)
        {
        }

        /// <summary>
        /// Resets windows layout.
        /// </summary>
        public void ResetLayout()
        {
            // Close all windows
            var windows = Windows.ToArray();
            for (int i = 0; i < windows.Length; i++)
                windows[i].Close();

            // Ensure that has no docked windows
            var childPanels = ChildPanels.ToArray();
            for (int i = 0; i < childPanels.Length; i++)
                childPanels[i].Dispose();

            // Delete remaining controls (except tabs proxy)
            if (TabsProxy != null)
                TabsProxy.Parent = null;
            DisposeChildren();
            if (TabsProxy != null)
                TabsProxy.Parent = this;
        }

        /// <summary>
        /// Performs hit test over dock panel.
        /// </summary>
        /// <param name="position">Screen space position to test.</param>
        /// <param name="excluded">Floating window to omit during searching (and all docked to that one).</param>
        /// <returns>Dock panel that has been hit or null if nothing found.</returns>
        public DockPanel HitTest(ref Float2 position, FloatWindowDockPanel excluded)
        {
            // Check all floating windows
            // TODO: gather windows order and take it into account when performing test
            for (int i = 0; i < FloatingPanels.Count; i++)
            {
                var win = FloatingPanels[i];
                if (win.Visible && win != excluded)
                {
                    var result = win.HitTest(ref position);
                    if (result != null)
                        return result;
                }
            }

            // Base
            return base.HitTest(ref position);
        }

        internal void LinkWindow(DockWindow window)
        {
            // Add to the windows list
            Windows.Add(window);
        }

        internal void UnlinkWindow(DockWindow window)
        {
            // Call event to the window
            window.OnUnlinkInternal();

            // Prevent this action during disposing (we don't want to modify Windows list)
            if (IsDisposing)
                return;

            // Remove from the windows list
            Windows.Remove(window);
        }

        /// <inheritdoc />
        public override bool IsMaster => true;

        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();

            Windows.Clear();
            FloatingPanels.Clear();
        }

        /// <inheritdoc />
        public override DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = 0.5f;
            return DockState.DockFill;
        }
    }
}
