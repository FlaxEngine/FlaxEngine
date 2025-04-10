// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Base class for all GUI controls
    /// </summary>
    public partial class Control : IComparable, IDrawable
    {
        private struct AnchorPresetData
        {
            public AnchorPresets Preset;
            public Float2 Min;
            public Float2 Max;

            public AnchorPresetData(AnchorPresets preset, Float2 min, Float2 max)
            {
                Preset = preset;
                Min = min;
                Max = max;
            }
        }

        private static readonly AnchorPresetData[] AnchorPresetsData =
        {
            new AnchorPresetData(AnchorPresets.TopLeft, new Float2(0, 0), new Float2(0, 0)),
            new AnchorPresetData(AnchorPresets.TopCenter, new Float2(0.5f, 0), new Float2(0.5f, 0)),
            new AnchorPresetData(AnchorPresets.TopRight, new Float2(1, 0), new Float2(1, 0)),

            new AnchorPresetData(AnchorPresets.MiddleLeft, new Float2(0, 0.5f), new Float2(0, 0.5f)),
            new AnchorPresetData(AnchorPresets.MiddleCenter, new Float2(0.5f, 0.5f), new Float2(0.5f, 0.5f)),
            new AnchorPresetData(AnchorPresets.MiddleRight, new Float2(1, 0.5f), new Float2(1, 0.5f)),

            new AnchorPresetData(AnchorPresets.BottomLeft, new Float2(0, 1), new Float2(0, 1)),
            new AnchorPresetData(AnchorPresets.BottomCenter, new Float2(0.5f, 1), new Float2(0.5f, 1)),
            new AnchorPresetData(AnchorPresets.BottomRight, new Float2(1, 1), new Float2(1, 1)),

            new AnchorPresetData(AnchorPresets.HorizontalStretchTop, new Float2(0, 0), new Float2(1, 0)),
            new AnchorPresetData(AnchorPresets.HorizontalStretchMiddle, new Float2(0, 0.5f), new Float2(1, 0.5f)),
            new AnchorPresetData(AnchorPresets.HorizontalStretchBottom, new Float2(0, 1), new Float2(1, 1)),

            new AnchorPresetData(AnchorPresets.VerticalStretchLeft, new Float2(0, 0), new Float2(0, 1)),
            new AnchorPresetData(AnchorPresets.VerticalStretchCenter, new Float2(0.5f, 0), new Float2(0.5f, 1)),
            new AnchorPresetData(AnchorPresets.VerticalStretchRight, new Float2(1, 0), new Float2(1, 1)),

            new AnchorPresetData(AnchorPresets.StretchAll, new Float2(0, 0), new Float2(1, 1)),
        };

        private ContainerControl _parent;
        private RootControl _root;
        private bool _isDisposing, _isFocused, _isNavFocused;

        // State

        // TODO: convert to flags

        private bool _isMouseOver, _isDragOver;
        private bool _isVisible = true;
        private bool _isEnabled = true;
        private bool _autoFocus = true;
        private bool _pivotRelativeSizing = false;
        private List<int> _touchOvers;
        private RootControl.UpdateDelegate _tooltipUpdate;

        // Transform

        private Rectangle _bounds;
        private Margin _offsets = new Margin(0.0f, 100.0f, 0.0f, 30.0f);
        private Float2 _anchorMin;
        private Float2 _anchorMax;
        private Float2 _scale = new Float2(1.0f);
        private Float2 _pivot = new Float2(0.5f);
        private Float2 _shear;
        private float _rotation;
        internal Matrix3x3 _cachedTransform;
        internal Matrix3x3 _cachedTransformInv;

        // Style

        private Color _backgroundColor = Color.Transparent;
        private IBrush _backgroundBrush = null;

        // Tooltip

        private string _tooltipText;
        private Tooltip _tooltip;

        /// <summary>
        /// Action is invoked, when location is changed
        /// </summary>
        public event Action<Control> LocationChanged;

        /// <summary>
        /// Action is invoked, when size is changed
        /// </summary>
        public event Action<Control> SizeChanged;

        /// <summary>
        /// Action is invoked, when parent is changed
        /// </summary>
        public event Action<Control> ParentChanged;

        /// <summary>
        /// Action is invoked, when visibility is changed
        /// </summary>
        public event Action<Control> VisibleChanged;

        #region Public Properties

        /// <summary>
        /// Parent control (the one above in the tree hierarchy)
        /// </summary>
        [HideInEditor, NoSerialize]
        public ContainerControl Parent
        {
            get => _parent;
            set
            {
                if (_parent == value)
                    return;

                Float2 oldParentSize;
                if (_parent != null)
                {
                    oldParentSize = _parent.Size;
                    _parent.RemoveChildInternal(this);
                }
                else
                {
                    oldParentSize = Float2.Zero;
                    ClearState();
                }

                _parent = value;
                _parent?.AddChildInternal(this);

                CacheRootHandle();
                OnParentChangedInternal();

                // Check if parent size has been changed
                if (_parent != null && !_parent.Size.Equals(ref oldParentSize))
                {
                    OnParentResized();
                }
            }
        }

        /// <summary>
        /// Checks if control has parent container control
        /// </summary>
        public bool HasParent => _parent != null;

        /// <summary>
        /// Gets or sets zero-based index of the control inside the parent container list.
        /// </summary>
        [HideInEditor, NoSerialize]
        public int IndexInParent
        {
            get => _parent?.GetChildIndex(this) ?? -1;
            set => _parent?.ChangeChildIndex(this, value);
        }

        /// <summary>
        /// Gets or sets control background color (transparent color (alpha=0) means no background rendering)
        /// </summary>
        [ExpandGroups, EditorDisplay("Background Style"), EditorOrder(2000), Tooltip("The control background color. Use transparent color (alpha=0) to hide background.")]
        public Color BackgroundColor
        {
            get => _backgroundColor;
            set => _backgroundColor = value;
        }

        /// <summary>
        /// Gets or sets control background brush used to fill the contents. Uses Background Color property as tint color.
        /// </summary>
        [EditorDisplay("Background Style"), EditorOrder(2001)]
        public IBrush BackgroundBrush
        {
            get => _backgroundBrush;
            set
            {
                _backgroundBrush = value;

#if FLAX_EDITOR
                // Auto-reset background color so brush is visible as it uses it for tint
                if (value != null && _backgroundColor == Color.Transparent && FlaxEditor.CustomEditors.CustomEditor.IsSettingValue)
                    _backgroundColor = Color.White;
#endif
            }
        }

        /// <summary>
        /// Gets or sets the anchor preset used by the control anchors (based on <see cref="AnchorMin"/> and <see cref="AnchorMax"/>).
        /// </summary>
        /// <remarks>To change anchor preset with current control bounds preservation use <see cref="SetAnchorPreset"/>.</remarks>
        [NoSerialize, EditorDisplay("Transform"), HideInEditor, EditorOrder(980), Tooltip("The anchor preset used by the control anchors.")]
        public AnchorPresets AnchorPreset
        {
            get
            {
                var result = AnchorPresets.Custom;
                for (int i = 0; i < AnchorPresetsData.Length; i++)
                {
                    if (Float2.NearEqual(ref _anchorMin, ref AnchorPresetsData[i].Min) &&
                        Float2.NearEqual(ref _anchorMax, ref AnchorPresetsData[i].Max))
                    {
                        result = AnchorPresetsData[i].Preset;
                        break;
                    }
                }
                return result;
            }
            set => SetAnchorPreset(value, false);
        }

        /// <summary>
        /// Gets or sets a value indicating whether this control is scrollable (scroll bars affect it).
        /// </summary>
        [HideInEditor, NoSerialize]
        public bool IsScrollable { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether the control can respond to user interaction
        /// </summary>
        [EditorOrder(520), Tooltip("If checked, control will receive input events of the user interaction.")]
        public bool Enabled
        {
            get => _isEnabled;
            set
            {
                if (_isEnabled != value)
                {
                    _isEnabled = value;
                    if (!_isEnabled)
                        ClearState();
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether the control is enabled in the hierarchy (it's enabled and all it's parents are enabled as well).
        /// </summary>
        public bool EnabledInHierarchy
        {
            get
            {
                if (!_isEnabled)
                    return false;
                if (_parent != null)
                    return _parent.EnabledInHierarchy;
                return true;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the control is visible
        /// </summary>
        [EditorOrder(510), Tooltip("If checked, control will be visible.")]
        public bool Visible
        {
            get => _isVisible;
            set
            {
                if (_isVisible != value)
                {
                    _isVisible = value;
                    if (!_isVisible)
                        ClearState();

                    OnVisibleChanged();
                    _parent?.PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether the control is visible in the hierarchy (it's visible and all it's parents are visible as well).
        /// </summary>
        public bool VisibleInHierarchy
        {
            get
            {
                if (!_isVisible)
                    return false;
                if (_parent != null)
                    return _parent.VisibleInHierarchy;
                return true;
            }
        }

        /// <summary>
        /// Returns true if control is during disposing state (on destroy)
        /// </summary>
        public bool IsDisposing => _isDisposing;

        /// <summary>
        /// Gets the GUI tree root control which contains that control (or null if not linked to any)
        /// </summary>
        public virtual RootControl Root => _root;

        /// <summary>
        /// Gets the GUI window root control which contains that control (or null if not linked to any).
        /// </summary>
        public virtual WindowRootControl RootWindow => _root?.RootWindow;

        /// <summary>
        /// Gets the control DPI scale factor (1 is default). Includes custom DPI scale.
        /// </summary>
        public float DpiScale => RootWindow?.Window?.DpiScale ?? Platform.DpiScale;

        /// <summary>
        /// Gets screen position of the control (upper left corner).
        /// </summary>
        public Float2 ScreenPos
        {
            get
            {
                var parentWin = Root;
                if (parentWin == null)
                    throw new InvalidOperationException("Missing parent window.");
                var clientPos = PointToWindow(Float2.Zero);
                return parentWin.PointToScreen(clientPos);
            }
        }

        #endregion

        /// <summary>
        /// Gets or sets the cursor (per window). Control should restore cursor to the default value eg. when mouse leaves it's area.
        /// </summary>
        [HideInEditor, NoSerialize]
        public virtual CursorType Cursor
        {
            get => _parent?.Cursor ?? CursorType.Default;

            set
            {
                if (_parent != null)
                    _parent.Cursor = value;
            }
        }

        /// <summary>
        /// The custom tag object value linked to the control.
        /// </summary>
        [HideInEditor, NoSerialize]
        public object Tag;

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        public Control()
        {
            _bounds = new Rectangle(_offsets.Left, _offsets.Top, _offsets.Right, _offsets.Bottom);
            UpdateTransform();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        /// <param name="x">X coordinate</param>
        /// <param name="y">Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        public Control(float x, float y, float width, float height)
        : this(new Rectangle(x, y, width, height))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        /// <param name="location">Upper left corner location.</param>
        /// <param name="size">Bounds size.</param>
        public Control(Float2 location, Float2 size)
        : this(new Rectangle(location, size))
        {
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="bounds">Window bounds</param>
        public Control(Rectangle bounds)
        {
            _bounds = bounds;
            _offsets = new Margin(bounds.X, bounds.Width, bounds.Y, bounds.Height);
            UpdateTransform();
        }

        /// <summary>
        /// Performs control logic update.
        /// </summary>
        /// <param name="deltaTime">The delta time in seconds (since last update).</param>
        public delegate void UpdateDelegate(float deltaTime);

        /// <summary>
        /// Delete control (will unlink from the parent and start to dispose)
        /// </summary>
        public void Dispose()
        {
            if (_isDisposing)
                return;

            // Call event
            OnDestroy();

            // Unlink
            Parent = null;
        }

        /// <summary>
        /// Perform control update and all its children
        /// </summary>
        /// <param name="deltaTime">Delta time in seconds</param>
        [NoAnimate]
        public virtual void Update(float deltaTime)
        {
            // TODO: move all controls to use UpdateDelegate and remove this generic Update
        }

        private void OnUpdateTooltip(float deltaTime)
        {
            Tooltip?.OnMouseOverControl(this, deltaTime);
        }

        /// <summary>
        /// Draw control
        /// </summary>
        [NoAnimate]
        public virtual void Draw()
        {
            // Paint Background
            var rect = new Rectangle(Float2.Zero, _bounds.Size);
            if (BackgroundBrush != null)
            {
                BackgroundBrush.Draw(rect, _backgroundColor);
            }
            else if (_backgroundColor.A > 0.0f)
            {
                Render2D.FillRectangle(rect, _backgroundColor);
            }
        }

        /// <summary>
        /// Update control layout
        /// </summary>
        /// <param name="force">True if perform layout by force even if cached state wants to skip it due to optimization.</param>
        [NoAnimate]
        public virtual void PerformLayout(bool force = false)
        {
        }

        /// <summary>
        /// Called to clear UI state. For example, removes mouse over state or drag and drop when control gets disabled or hidden (including hierarchy).
        /// </summary>
        public virtual void ClearState()
        {
            Defocus();
            if (_isMouseOver)
                OnMouseLeave();
            if (_isDragOver)
                OnDragLeave();
            while (_touchOvers != null && _touchOvers.Count != 0)
                OnTouchLeave(_touchOvers[0]);
        }

        #region Focus

        /// <summary>
        /// If checked, the control can receive automatic focus (eg. on user click or UI navigation).
        /// </summary>
        [EditorOrder(512), Tooltip("If checked, the control can receive automatic focus (eg. on user click or UI navigation).")]
        public bool AutoFocus
        {
            get => _autoFocus;
            set => _autoFocus = value;
        }

        /// <summary>
        /// Gets a value indicating whether the control, currently has the input focus
        /// </summary>
        public virtual bool ContainsFocus => _isFocused;

        /// <summary>
        /// Gets a value indicating whether the control has input focus
        /// </summary>
        public bool IsFocused => _isFocused;

        /// <summary>
        /// Gets a value indicating whether the control has UI navigation focus.
        /// </summary>
        public bool IsNavFocused => _isNavFocused;

        /// <summary>
        /// Sets input focus to the control
        /// </summary>
        public virtual void Focus()
        {
            if (!IsFocused)
            {
                Focus(this);
            }
        }

        /// <summary>
        /// Removes input focus from the control
        /// </summary>
        public virtual void Defocus()
        {
            if (ContainsFocus)
            {
                Focus(null);
            }
        }

        /// <summary>
        /// When control gets input focus
        /// </summary>
        [NoAnimate]
        public virtual void OnGotFocus()
        {
            // Cache flag
            _isFocused = true;
            _isNavFocused = false;
        }

        /// <summary>
        /// When control losts input focus
        /// </summary>
        [NoAnimate]
        public virtual void OnLostFocus()
        {
            // Clear flag
            _isFocused = false;
            _isNavFocused = false;
        }

        /// <summary>
        /// Action fired when control gets 'Contains Focus' state
        /// </summary>
        [NoAnimate]
        public virtual void OnStartContainsFocus()
        {
        }

        /// <summary>
        /// Action fired when control lost 'Contains Focus' state
        /// </summary>
        [NoAnimate]
        public virtual void OnEndContainsFocus()
        {
        }

        /// <summary>
        /// Focus that control
        /// </summary>
        /// <param name="c">Control to focus</param>
        /// <returns>True if control got a focus</returns>
        protected virtual bool Focus(Control c)
        {
            return _parent != null && _parent.Focus(c);
        }

        /// <summary>
        /// Starts the mouse tracking. Used by the scrollbars, splitters, etc.
        /// </summary>
        /// <param name="useMouseScreenOffset">If set to <c>true</c> will use mouse screen offset.</param>
        [NoAnimate]
        public void StartMouseCapture(bool useMouseScreenOffset = false)
        {
            var parent = Root;
            parent?.StartTrackingMouse(this, useMouseScreenOffset);
        }

        /// <summary>
        /// Ends the mouse tracking.
        /// </summary>
        [NoAnimate]
        public void EndMouseCapture()
        {
            var parent = Root;
            parent?.EndTrackingMouse();
        }

        /// <summary>
        /// When mouse goes up/down not over the control but it has user focus so remove that focus from it (used by scroll
        /// bars, sliders etc.)
        /// </summary>
        [NoAnimate]
        public virtual void OnEndMouseCapture()
        {
        }

        #endregion

        #region Navigation

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Up"/> direction.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Control NavTargetUp;

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Down"/> direction.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Control NavTargetDown;

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Left"/> direction.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Control NavTargetLeft;

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Right"/> direction.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Control NavTargetRight;

        /// <summary>
        /// Gets the next navigation control to focus for the given direction. Returns null for automated direction resolving.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <returns>The target navigation control or null to use automatic navigation.</returns>
        [NoAnimate]
        public virtual Control GetNavTarget(NavDirection direction)
        {
            switch (direction)
            {
            case NavDirection.Up: return NavTargetUp;
            case NavDirection.Down: return NavTargetDown;
            case NavDirection.Left: return NavTargetLeft;
            case NavDirection.Right: return NavTargetRight;
            case NavDirection.Next:
                if (NavTargetRight != null)
                    return NavTargetRight;
                if (NavTargetDown != null)
                    return NavTargetDown;
                return null;
            case NavDirection.Previous:
                if (NavTargetLeft != null)
                    return NavTargetLeft;
                if (NavTargetUp != null)
                    return NavTargetUp;
                return null;
            default: return null;
            }
        }

        /// <summary>
        /// Gets the navigation origin location for this control. It's the starting anchor point for searching navigable controls in the nearby area. By default the origin points are located on the control bounds edges.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <returns>The navigation origin for the automatic navigation.</returns>
        [NoAnimate]
        public virtual Float2 GetNavOrigin(NavDirection direction)
        {
            var size = Size;
            switch (direction)
            {
            case NavDirection.Up: return new Float2(size.X * 0.5f, 0);
            case NavDirection.Down: return new Float2(size.X * 0.5f, size.Y);
            case NavDirection.Left: return new Float2(0, size.Y * 0.5f);
            case NavDirection.Right: return new Float2(size.X, size.Y * 0.5f);
            case NavDirection.Next: return Float2.Zero;
            case NavDirection.Previous: return size;
            default: return size * 0.5f;
            }
        }

        /// <summary>
        /// Performs the UI navigation for this control.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <param name="location">The navigation start location (in the control-space).</param>
        /// <param name="caller">The control that calls the event.</param>
        /// <param name="visited">The list with visited controls. Used to skip recursive navigation calls when doing traversal across the UI hierarchy.</param>
        /// <returns>The target navigation control or null if didn't performed any navigation.</returns>
        public virtual Control OnNavigate(NavDirection direction, Float2 location, Control caller, List<Control> visited)
        {
            if (caller == _parent && AutoFocus && Visible)
                return this;
            return _parent?.OnNavigate(direction, PointToParent(GetNavOrigin(direction)), caller, visited);
        }

        /// <summary>
        /// Focuses the control by the UI navigation system. Called during navigating around UI with gamepad/keyboard navigation. Focuses the control and sets the <see cref="IsNavFocused"/> flag.
        /// </summary>
        public virtual void NavigationFocus()
        {
            Focus();
            if (IsFocused)
            {
                _isNavFocused = true;

                // Ensure to be in a view
                var parent = Parent;
                while (parent != null)
                {
                    if (parent is Panel panel && ((panel.VScrollBar != null && panel.VScrollBar.Enabled) || (panel.HScrollBar != null && panel.HScrollBar.Enabled)))
                    {
                        panel.ScrollViewTo(this);
                        break;
                    }
                    parent = parent.Parent;
                }
            }
        }

        /// <summary>
        /// Generic user interaction event for a control used by UI navigation (eg. user submits on the currently focused control).
        /// </summary>
        public virtual void OnSubmit()
        {
        }

        #endregion

        #region Mouse

        /// <summary>
        /// Check if mouse is over that item or its child items
        /// </summary>
        public virtual bool IsMouseOver => _isMouseOver;

        /// <summary>
        /// When mouse enters control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        [NoAnimate]
        public virtual void OnMouseEnter(Float2 location)
        {
            // Set flag
            _isMouseOver = true;

            // Update tooltip
            if (ShowTooltip && OnTestTooltipOverControl(ref location))
            {
                Tooltip?.OnMouseEnterControl(this);
                SetUpdate(ref _tooltipUpdate, OnUpdateTooltip);
            }
        }

        /// <summary>
        /// When mouse moves over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        [NoAnimate]
        public virtual void OnMouseMove(Float2 location)
        {
            // Update tooltip
            if (ShowTooltip && OnTestTooltipOverControl(ref location))
            {
                if (_tooltipUpdate == null)
                {
                    Tooltip?.OnMouseEnterControl(this);
                    SetUpdate(ref _tooltipUpdate, OnUpdateTooltip);
                }
            }
            else if (_tooltipUpdate != null)
            {
                SetUpdate(ref _tooltipUpdate, null);
                Tooltip?.OnMouseLeaveControl(this);
            }
        }

        /// <summary>
        /// When mouse leaves control's area
        /// </summary>
        [NoAnimate]
        public virtual void OnMouseLeave()
        {
            // Clear flag
            _isMouseOver = false;

            // Update tooltip
            if (_tooltipUpdate != null)
            {
                SetUpdate(ref _tooltipUpdate, null);
                Tooltip?.OnMouseLeaveControl(this);
            }
        }

        /// <summary>
        /// When mouse wheel moves
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="delta">Mouse wheel move delta. A positive value indicates that the wheel was rotated forward, away from the user; a negative value indicates that the wheel was rotated backward, toward the user. Normalized to [-1;1] range.</param>
        /// <returns>True if event has been handled</returns>
        [NoAnimate]
        public virtual bool OnMouseWheel(Float2 location, float delta)
        {
            return false;
        }

        /// <summary>
        /// When mouse goes down over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="button">Mouse buttons state (flags)</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        [NoAnimate]
        public virtual bool OnMouseDown(Float2 location, MouseButton button)
        {
            return false;
        }

        /// <summary>
        /// When mouse goes up over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="button">Mouse buttons state (flags)</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        [NoAnimate]
        public virtual bool OnMouseUp(Float2 location, MouseButton button)
        {
            return false;
        }

        /// <summary>
        /// When mouse double clicks over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="button">Mouse buttons state (flags)</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        [NoAnimate]
        public virtual bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            return false;
        }

        #endregion

        #region Keyboard

        /// <summary>
        /// On input character
        /// </summary>
        /// <param name="c">Input character</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        [NoAnimate]
        public virtual bool OnCharInput(char c)
        {
            return false;
        }

        /// <summary>
        /// When key goes down
        /// </summary>
        /// <param name="key">Key value</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        [NoAnimate]
        public virtual bool OnKeyDown(KeyboardKeys key)
        {
            return false;
        }

        /// <summary>
        /// When key goes up
        /// </summary>
        /// <param name="key">Key value</param>
        [NoAnimate]
        public virtual void OnKeyUp(KeyboardKeys key)
        {
        }

        #endregion

        #region Touch

        /// <summary>
        /// Check if touch is over that item or its child items
        /// </summary>
        public virtual bool IsTouchOver => _touchOvers != null && _touchOvers.Count != 0;

        /// <summary>
        /// Determines whether the given touch pointer is over the control.
        /// </summary>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <returns>True if given touch pointer is over the control, otherwise false.</returns>
        public virtual bool IsTouchPointerOver(int pointerId)
        {
            return _touchOvers != null && _touchOvers.Contains(pointerId);
        }

        /// <summary>
        /// When touch enters control's area
        /// </summary>
        /// <param name="location">Touch location in Control Space</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        [NoAnimate]
        public virtual void OnTouchEnter(Float2 location, int pointerId)
        {
            if (_touchOvers == null)
                _touchOvers = new List<int>();
            _touchOvers.Add(pointerId);
        }

        /// <summary>
        /// When touch enters control's area.
        /// </summary>
        /// <param name="location">Touch location in Control Space.</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        [NoAnimate]
        public virtual bool OnTouchDown(Float2 location, int pointerId)
        {
            return false;
        }

        /// <summary>
        /// When touch moves over control's area.
        /// </summary>
        /// <param name="location">Touch location in Control Space.</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        [NoAnimate]
        public virtual void OnTouchMove(Float2 location, int pointerId)
        {
        }

        /// <summary>
        /// When touch goes up over control's area.
        /// </summary>
        /// <param name="location">Touch location in Control Space</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        [NoAnimate]
        public virtual bool OnTouchUp(Float2 location, int pointerId)
        {
            return false;
        }

        /// <summary>
        /// When touch leaves control's area
        /// </summary>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        [NoAnimate]
        public virtual void OnTouchLeave(int pointerId)
        {
            _touchOvers.Remove(pointerId);
            if (_touchOvers.Count == 0)
                OnTouchLeave();
        }

        /// <summary>
        /// When all touch leaves control's area
        /// </summary>
        [NoAnimate]
        public virtual void OnTouchLeave()
        {
        }

        #endregion

        #region Drag&Drop

        /// <summary>
        /// Check if mouse dragging is over that item or its child items.
        /// </summary>
        public virtual bool IsDragOver => _isDragOver;

        /// <summary>
        /// When mouse dragging enters control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="data">The data. See <see cref="DragDataText"/> and <see cref="DragDataFiles"/>.</param>
        /// <returns>The drag event result effect.</returns>
        [NoAnimate]
        public virtual DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            // Set flag
            _isDragOver = true;
            return DragDropEffect.None;
        }

        /// <summary>
        /// When mouse dragging moves over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="data">The data. See <see cref="DragDataText"/> and <see cref="DragDataFiles"/>.</param>
        /// <returns>The drag event result effect.</returns>
        [NoAnimate]
        public virtual DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            return DragDropEffect.None;
        }

        /// <summary>
        /// When mouse dragging drops on control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="data">The data. See <see cref="DragDataText"/> and <see cref="DragDataFiles"/>.</param>
        /// <returns>The drag event result effect.</returns>
        [NoAnimate]
        public virtual DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            // Clear flag
            _isDragOver = false;
            return DragDropEffect.None;
        }

        /// <summary>
        /// When mouse dragging leaves control's area
        /// </summary>
        [NoAnimate]
        public virtual void OnDragLeave()
        {
            // Clear flag
            _isDragOver = false;
        }

        /// <summary>
        /// Starts the drag and drop operation.
        /// </summary>
        /// <param name="data">The data.</param>
        [NoAnimate]
        public virtual void DoDragDrop(DragData data)
        {
            // Hide tooltip
            Tooltip?.Hide();
            Root?.DoDragDrop(data);
        }

        #endregion

        #region Tooltip

        /// <summary>
        /// Gets or sets the tooltip text.
        /// </summary>
        [HideInEditor, NoSerialize]
        public string TooltipText
        {
            get => _tooltipText;
            set => _tooltipText = value;
        }

        /// <summary>
        /// Gets or sets the custom tooltip control linked. Use null to show default shared tooltip from the current <see cref="Style"/>.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Tooltip CustomTooltip
        {
            get => _tooltip;
            set => _tooltip = value;
        }

        /// <summary>
        /// Gets the tooltip used by this control (custom or shared one).
        /// </summary>
        public Tooltip Tooltip => _tooltip ?? Style.Current.SharedTooltip;

        /// <summary>
        /// Gets a value indicating whether show control tooltip (control is in a proper state, tooltip text is valid, etc.). Can be used to implement custom conditions for showing tooltips (eg. based on current mouse location within the control bounds).
        /// </summary>
        /// <remarks>Tooltip can be only visible if mouse is over the control area (see <see cref="IsMouseOver"/>).</remarks>
        protected virtual bool ShowTooltip => !string.IsNullOrEmpty(_tooltipText);

        /// <summary>
        /// Links the tooltip.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="customTooltip">The custom tooltip.</param>
        /// <returns>This control pointer. Useful for creating controls in code.</returns>
        [NoAnimate]
        public Control LinkTooltip(string text, Tooltip customTooltip = null)
        {
            _tooltipText = text;
            _tooltip = customTooltip;
            return this;
        }

        /// <summary>
        /// Unlinks the tooltip.
        /// </summary>
        [NoAnimate]
        public void UnlinkTooltip()
        {
            if (_tooltipUpdate != null)
            {
                SetUpdate(ref _tooltipUpdate, null);
                Tooltip?.OnMouseLeaveControl(this);
            }
            _tooltipText = null;
            _tooltip = null;
        }

        /// <summary>
        /// Called when tooltip wants to be shown. Allows modifying its appearance.
        /// </summary>
        /// <param name="text">The tooltip text to show.</param>
        /// <param name="location">The popup start location (in this control local space).</param>
        /// <param name="area">The allowed area of mouse movement to show tooltip (in this control local space).</param>
        /// <returns>True if can show tooltip, otherwise false to skip.</returns>
        public virtual bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            text = _tooltipText;
            location = Size * new Float2(0.5f, 1.0f);
            area = new Rectangle(Float2.Zero, Size);
            return ShowTooltip;
        }

        /// <summary>
        /// Called when tooltip gets created and shown for this control. Can be used to customize tooltip UI.
        /// </summary>
        /// <param name="tooltip">The tooltip.</param>
        public virtual void OnTooltipShown(Tooltip tooltip)
        {
        }

        /// <summary>
        /// Called when tooltip is visible and tests if the given mouse location (in control space) is valid (is over the content).
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>True if tooltip can be still visible, otherwise false.</returns>
        public virtual bool OnTestTooltipOverControl(ref Float2 location)
        {
            return ContainsPoint(ref location) && ShowTooltip;
        }

        #endregion

        #region Helper Functions

        /// <summary>
        /// Performs a raycast against UI controls hierarchy to find any intersecting control content. Uses <see cref="ContainsPoint"/> with precise check (skips transparent surfaces and empty panels).
        /// </summary>
        /// <param name="location">The position to intersect UI with.</param>
        /// <param name="hit">The result control that intersects with the raycast.</param>
        /// <returns>True if ray hits any matching control, otherwise false.</returns>
        public virtual bool RayCast(ref Float2 location, out Control hit)
        {
            if (ContainsPoint(ref location, true))
            {
                hit = this;
                return true;
            }
            hit = null;
            return false;
        }

        /// <summary>
        /// Checks if given location point in Parent Space intersects with the control content and calculates local position.
        /// </summary>
        /// <param name="locationParent">The location in Parent Space.</param>
        /// <param name="location">The location of intersection in Control Space.</param>
        /// <returns>True if given point in Parent Space intersects with this control content, otherwise false.</returns>
        public virtual bool IntersectsContent(ref Float2 locationParent, out Float2 location)
        {
            location = PointFromParent(ref locationParent);
            return ContainsPoint(ref location);
        }

        /// <summary>
        /// Checks if this control contains given point in local Control Space.
        /// </summary>
        /// <param name="location">Point location in Control Space to check</param>
        /// <param name="precise">True if perform precise intersection test against the control content (eg. with hit mask or transparency threshold). Otherwise, only simple bounds-check will be performed.</param>
        /// <returns>True if point is inside control's area, otherwise false.</returns>
        public virtual bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            return location.X >= 0 &&
                   location.Y >= 0 &&
                   location.X <= _bounds.Size.X &&
                   location.Y <= _bounds.Size.Y;
        }

        /// <summary>
        /// Converts point in local control's space into one of the parent control coordinates
        /// </summary>
        /// <param name="parent">This control parent of any other parent.</param>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in parent control coordinates</returns>
        public Float2 PointToParent(ContainerControl parent, Float2 location)
        {
            if (parent == null)
                throw new ArgumentNullException();
            Control c = this;
            while (c != null)
            {
                location = c.PointToParent(ref location);
                c = c.Parent;
                if (c == parent)
                    break;
            }
            return location;
        }

        /// <summary>
        /// Converts point in local control's space into parent control coordinates.
        /// </summary>
        /// <param name="location">The input location of the point to convert.</param>
        /// <returns>The converted point location in parent control coordinates.</returns>
        public Float2 PointToParent(Float2 location)
        {
            return PointToParent(ref location);
        }

        /// <summary>
        /// Converts point in local control's space into parent control coordinates.
        /// </summary>
        /// <param name="location">The input location of the point to convert.</param>
        /// <returns>The converted point location in parent control coordinates.</returns>
        public virtual Float2 PointToParent(ref Float2 location)
        {
            Matrix3x3.Transform2D(ref location, ref _cachedTransform, out var result);
            return result;
        }

        /// <summary>
        /// Converts point in parent control coordinates into local control's space.
        /// </summary>
        /// <param name="locationParent">The input location of the point to convert.</param>
        /// <returns>The converted point location in control's space.</returns>
        public Float2 PointFromParent(Float2 locationParent)
        {
            return PointFromParent(ref locationParent);
        }

        /// <summary>
        /// Converts point in parent control coordinates into local control's space.
        /// </summary>
        /// <param name="locationParent">The input location of the point to convert.</param>
        /// <returns>The converted point location in control's space.</returns>
        public virtual Float2 PointFromParent(ref Float2 locationParent)
        {
            Matrix3x3.Transform2D(ref locationParent, ref _cachedTransformInv, out var result);
            return result;
        }

        /// <summary>
        /// Converts point in one of the parent control coordinates into local control's space.
        /// </summary>
        /// <param name="parent">This control parent of any other parent.</param>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>The converted point location in control's space.</returns>
        public Float2 PointFromParent(ContainerControl parent, Float2 location)
        {
            if (parent == null)
                throw new ArgumentNullException();
            var path = new List<Control>();

            Control c = this;
            while (c != null && c != parent)
            {
                path.Add(c);
                c = c.Parent;
            }
            for (int i = path.Count - 1; i >= 0; i--)
            {
                location = path[i].PointFromParent(ref location);
            }
            return location;
        }

        /// <summary>
        /// Converts point in local control's space into window coordinates
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in window coordinates</returns>
        public Float2 PointToWindow(Float2 location)
        {
            location = PointToParent(ref location);
            if (_parent != null)
            {
                location = _parent.PointToWindow(location);
            }
            return location;
        }

        /// <summary>
        /// Converts point in the window coordinates into control's space
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in control's space</returns>
        public Float2 PointFromWindow(Float2 location)
        {
            if (_parent != null)
            {
                location = _parent.PointFromWindow(location);
            }
            return PointFromParent(ref location);
        }

        /// <summary>
        /// Converts point in the local control's space into screen coordinates
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in screen coordinates</returns>
        public virtual Float2 PointToScreen(Float2 location)
        {
            location = PointToParent(ref location);
            if (_parent != null)
            {
                location = _parent.PointToScreen(location);
            }
            return location;
        }

        /// <summary>
        /// Converts point in screen coordinates into the local control's space
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in local control's space</returns>
        public virtual Float2 PointFromScreen(Float2 location)
        {
            if (_parent != null)
            {
                location = _parent.PointFromScreen(location);
            }
            return PointFromParent(ref location);
        }

#if FLAX_EDITOR
        /// <summary>
        /// Bounds rectangle for editor UI.
        /// </summary>
        public virtual Rectangle EditorBounds => new Rectangle(Float2.Zero, _bounds.Size);
#endif

        #endregion

        #region Control Action

        /// <summary>
        /// Called when control location gets changed.
        /// </summary>
        protected virtual void OnLocationChanged()
        {
            LocationChanged?.Invoke(this);
        }

        /// <summary>
        /// Called when control size gets changed.
        /// </summary>
        protected virtual void OnSizeChanged()
        {
            SizeChanged?.Invoke(this);
            _parent?.OnChildResized(this);
        }

        /// <summary>
        /// Sets the scale and updates the transform.
        /// </summary>
        /// <param name="scale">The scale.</param>
        protected virtual void SetScaleInternal(ref Float2 scale)
        {
            _scale = scale;
            UpdateTransform();
            _parent?.OnChildResized(this);
        }

        /// <summary>
        /// Sets the pivot and updates the transform.
        /// </summary>
        /// <param name="pivot">The pivot.</param>
        protected virtual void SetPivotInternal(ref Float2 pivot)
        {
            _pivot = pivot;
            UpdateTransform();
            _parent?.OnChildResized(this);
        }

        /// <summary>
        /// Sets the shear and updates the transform.
        /// </summary>
        /// <param name="shear">The shear.</param>
        protected virtual void SetShearInternal(ref Float2 shear)
        {
            _shear = shear;
            UpdateTransform();
            _parent?.OnChildResized(this);
        }

        /// <summary>
        /// Sets the rotation angle and updates the transform.
        /// </summary>
        /// <param name="rotation">The rotation (in degrees).</param>
        protected virtual void SetRotationInternal(float rotation)
        {
            _rotation = rotation;
            UpdateTransform();
            _parent?.OnChildResized(this);
        }

        /// <summary>
        /// Called when visible state gets changed.
        /// </summary>
        protected virtual void OnVisibleChanged()
        {
            // Clear state when control gets hidden
            if (!_isVisible && _isMouseOver)
            {
                OnMouseLeave();
            }

            VisibleChanged?.Invoke(this);
        }

        /// <summary>
        /// Action fired when parent control gets changed.
        /// </summary>
        protected virtual void OnParentChangedInternal()
        {
            ParentChanged?.Invoke(this);
        }

        /// <summary>
        /// Caches the root control handle.
        /// </summary>
        internal virtual void CacheRootHandle()
        {
            if (_root != null)
                RemoveUpdateCallbacks(_root);

            _root = _parent?.Root;
            if (_root != null)
                AddUpdateCallbacks(_root);
        }

        /// <summary>
        /// Adds the custom control logic update callbacks to the root.
        /// </summary>
        /// <param name="root">The root.</param>
        protected virtual void AddUpdateCallbacks(RootControl root)
        {
            if (_tooltipUpdate != null)
                root.UpdateCallbacksToAdd.Add(_tooltipUpdate);
        }

        /// <summary>
        /// Removes the custom control logic update callbacks from the root.
        /// </summary>
        /// <param name="root">The root.</param>
        protected virtual void RemoveUpdateCallbacks(RootControl root)
        {
            if (_tooltipUpdate != null)
                root.UpdateCallbacksToRemove.Add(_tooltipUpdate);
        }

        /// <summary>
        /// Helper utility function to sets the update callback to the root. Does nothing if value has not been modified. Handles if control has no root or parent.
        /// </summary>
        /// <param name="onUpdate">The cached update callback delegate (field in the custom control implementation).</param>
        /// <param name="value">The value to assign.</param>
        protected void SetUpdate(ref UpdateDelegate onUpdate, UpdateDelegate value)
        {
            if (onUpdate == value)
                return;
            if (_root != null && onUpdate != null)
                _root.UpdateCallbacksToRemove.Add(onUpdate);
            onUpdate = value;
            if (_root != null && onUpdate != null)
                _root.UpdateCallbacksToAdd.Add(onUpdate);
        }

        /// <summary>
        /// Action fired when parent control gets resized (also when control gets non-null parent).
        /// </summary>
        public virtual void OnParentResized()
        {
            if (!_anchorMin.IsZero || !_anchorMax.IsZero)
            {
                UpdateBounds();
            }
        }

        /// <summary>
        /// Method called when managed instance should be destroyed
        /// </summary>
        [NoAnimate]
        public virtual void OnDestroy()
        {
            // Set disposing flag
            _isDisposing = true;
            Defocus();
            UnlinkTooltip();
            Tag = null;
        }

        #endregion

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is Control c)
                return Compare(c);
            return 0;
        }

        /// <summary>
        /// Compares this control with the other control.
        /// </summary>
        /// <param name="other">The other.</param>
        /// <returns>Comparision result.</returns>
        public virtual int Compare(Control other)
        {
            return (int)(Y - other.Y);
        }
    }
}
