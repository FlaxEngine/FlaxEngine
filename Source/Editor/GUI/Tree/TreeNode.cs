// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Tree
{
    /// <summary>
    /// Tree node control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class TreeNode : ContainerControl
    {
        /// <summary>
        /// The default drag insert position margin.
        /// </summary>
        public const float DefaultDragInsertPositionMargin = 2.0f;

        /// <summary>
        /// The default node offset on Y axis.
        /// </summary>
        public const float DefaultNodeOffsetY = 0;

        private Tree _tree;

        private bool _opened, _canChangeOrder;
        private float _animationProgress, _cachedHeight;
        private bool _mouseOverArrow, _mouseOverHeader;
        private float _xOffset, _textWidth;
        private float _headerHeight = 16.0f;
        private Rectangle _headerRect;
        private SpriteHandle _iconCollaped, _iconOpened;
        private Margin _margin = new Margin(2.0f);
        private string _text;
        private bool _textChanged;
        private bool _isMouseDown;
        private float _mouseDownTime;
        private Vector2 _mouseDownPos;
        private Color _cachedTextColor;

        private DragItemPositioning _dragOverMode;
        private bool _isDragOverHeader;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        [EditorOrder(10), Tooltip("The node text.")]
        public string Text
        {
            get => _text;
            set
            {
                _text = value;
                _textChanged = true;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this node is expanded.
        /// </summary>
        [EditorOrder(20), Tooltip("If checked, node is expanded.")]
        public bool IsExpanded
        {
            get => _opened;
            set
            {
                if (value)
                    Expand(true);
                else
                    Collapse(true);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this node is collapsed.
        /// </summary>
        [HideInEditor, NoSerialize]
        public bool IsCollapsed
        {
            get => !_opened;
            set
            {
                if (value)
                    Collapse(true);
                else
                    Expand(true);
            }
        }

        /// <summary>
        /// Gets or sets the text margin.
        /// </summary>
        [EditorOrder(30), Tooltip("The margin of the text area.")]
        public Margin TextMargin
        {
            get => _margin;
            set
            {
                _margin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color TextColor { get; set; }

        /// <summary>
        /// Gets or sets the font used to render text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public FontReference TextFont { get; set; }

        /// <summary>
        /// Gets or sets the color of the background when tree node is selected.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BackgroundColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the color of the background when tree node is highlighted.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BackgroundColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the color of the background when tree node is selected but not focused.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BackgroundColorSelectedUnfocused { get; set; }

        /// <summary>
        /// Gets the parent tree control.
        /// </summary>
        public Tree ParentTree
        {
            get
            {
                if (_tree == null)
                {
                    if (Parent is TreeNode upNode)
                        _tree = upNode.ParentTree;
                    else if (Parent is Tree tree)
                        _tree = tree;
                }

                return _tree;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this node is root.
        /// </summary>
        public bool IsRoot => !(Parent is TreeNode);

        /// <summary>
        /// Gets the minimum width of the node sub-tree.
        /// </summary>
        public virtual float MinimumWidth
        {
            get
            {
                UpdateTextWidth();

                float minWidth = _xOffset + _textWidth + 6 + 16;
                if (_iconCollaped.IsValid)
                    minWidth += 16;

                if (_opened || _animationProgress < 1.0f)
                {
                    for (int i = 0; i < _children.Count; i++)
                    {
                        if (_children[i] is TreeNode node && node.Visible)
                        {
                            minWidth = Mathf.Max(minWidth, node.MinimumWidth);
                        }
                    }
                }

                return minWidth;
            }
        }

        /// <summary>
        /// The indent applied to the child nodes.
        /// </summary>
        [EditorOrder(30), Tooltip("The indentation applied to the child nodes.")]
        public float ChildrenIndent { get; set; } = 12.0f;

        /// <summary>
        /// The height of the tree node header area.
        /// </summary>
        [EditorOrder(40), Limit(1, 10000, 0.1f), Tooltip("The height of the tree node header area.")]
        public float HeaderHeight
        {
            get => _headerHeight;
            set
            {
                if (!Mathf.NearEqual(_headerHeight, value))
                {
                    _headerHeight = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the color of the icon.
        /// </summary>
        [EditorOrder(50), Tooltip("The color of the icon.")]
        public Color IconColor { get; set; } = Color.White;

        /// <summary>
        /// Gets the arrow rectangle.
        /// </summary>
        public Rectangle ArrowRect => new Rectangle(_xOffset + 2 + _margin.Left, 2, 12, 12);

        /// <summary>
        /// Gets the header rectangle.
        /// </summary>
        public Rectangle HeaderRect => _headerRect;

        /// <summary>
        /// Gets the header text rectangle.
        /// </summary>
        public Rectangle TextRect
        {
            get
            {
                var left = _xOffset + 16; // offset + arrow
                var textRect = new Rectangle(left, 0, Width - left, _headerHeight);

                // Margin
                _margin.ShrinkRectangle(ref textRect);

                // Icon
                if (_iconCollaped.IsValid)
                {
                    textRect.X += 18.0f;
                    textRect.Width -= 18.0f;
                }

                return textRect;
            }
        }

        /// <summary>
        /// Gets the drag over action type.
        /// </summary>
        public DragItemPositioning DragOverMode => _dragOverMode;

        /// <summary>
        /// Gets a value indicating whether this node has any visible child. Returns false if it has no children.
        /// </summary>
        public bool HasAnyVisibleChild
        {
            get
            {
                bool result = false;
                for (int i = 0; i < _children.Count; i++)
                {
                    if (_children[i] is TreeNode node && node.Visible)
                    {
                        result = true;
                        break;
                    }
                }
                return result;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TreeNode"/> class.
        /// </summary>
        public TreeNode()
        : this(false)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TreeNode"/> class.
        /// </summary>
        /// <param name="canChangeOrder">Enable/disable changing node order in parent tree node.</param>
        public TreeNode(bool canChangeOrder)
        : this(canChangeOrder, SpriteHandle.Invalid, SpriteHandle.Invalid)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TreeNode"/> class.
        /// </summary>
        /// <param name="canChangeOrder">Enable/disable changing node order in parent tree node.</param>
        /// <param name="iconCollapsed">The icon for node collapsed.</param>
        /// <param name="iconOpened">The icon for node opened.</param>
        public TreeNode(bool canChangeOrder, SpriteHandle iconCollapsed, SpriteHandle iconOpened)
        : base(0, 0, 64, 16)
        {
            _canChangeOrder = canChangeOrder;
            _animationProgress = 1.0f;
            _cachedHeight = _headerHeight;
            _iconCollaped = iconCollapsed;
            _iconOpened = iconOpened;
            _mouseDownTime = -1;

            var style = Style.Current;
            TextColor = style.Foreground;
            BackgroundColorSelected = style.BackgroundSelected;
            BackgroundColorHighlighted = style.BackgroundHighlighted;
            BackgroundColorSelectedUnfocused = style.LightBackground;
            TextFont = new FontReference(style.FontSmall);
        }

        /// <summary>
        /// Expand node.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void Expand(bool noAnimation = false)
        {
            // Parents first
            ExpandAllParents(noAnimation);

            // Change state
            bool prevState = _opened;
            _opened = true;
            if (prevState != _opened)
                _animationProgress = 1.0f - _animationProgress;

            if (noAnimation)
            {
                // Speed up an animation
                _animationProgress = 1.0f;
            }

            // Update
            OnExpandedChanged();
            if (HasParent)
                Parent.PerformLayout();
            else
                PerformLayout();
        }

        /// <summary>
        /// Collapse node.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void Collapse(bool noAnimation = false)
        {
            // Change state
            bool prevState = _opened;
            _opened = false;
            if (prevState != _opened)
                _animationProgress = 1.0f - _animationProgress;

            if (noAnimation)
            {
                // Speed up an animation
                _animationProgress = 1.0f;
            }

            // Update
            OnExpandedChanged();
            if (HasParent)
                Parent.PerformLayout();
            else
                PerformLayout();
        }

        /// <summary>
        /// Expand node and all the children.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void ExpandAll(bool noAnimation = false)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            Expand(noAnimation);

            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is TreeNode node)
                {
                    node.ExpandAll(noAnimation);
                }
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Collapse node and all the children.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void CollapseAll(bool noAnimation = false)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            Collapse(noAnimation);

            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is TreeNode node)
                {
                    node.CollapseAll(noAnimation);
                }
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Ensure that all node parents are expanded.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void ExpandAllParents(bool noAnimation = false)
        {
            (Parent as TreeNode)?.Expand(noAnimation);
        }

        /// <summary>
        /// Ends open/close animation by force.
        /// </summary>
        public void EndAnimation()
        {
            if (_animationProgress < 1.0f)
            {
                _animationProgress = 1.0f;
                PerformLayout();
            }
        }

        /// <summary>
        /// Select node in the tree.
        /// </summary>
        public void Select()
        {
            ParentTree.Select(this);
        }

        /// <summary>
        /// Called when drag and drop enters the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragEnterHeader(DragData data)
        {
            return DragDropEffect.None;
        }

        /// <summary>
        /// Called when drag and drop moves over the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragMoveHeader(DragData data)
        {
            return DragDropEffect.None;
        }

        /// <summary>
        /// Called when drag and drop performs over the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragDropHeader(DragData data)
        {
            return DragDropEffect.None;
        }

        /// <summary>
        /// Called when drag and drop leaves the node header area.
        /// </summary>
        protected virtual void OnDragLeaveHeader()
        {
        }

        /// <summary>
        /// Begins the drag drop operation.
        /// </summary>
        protected virtual void DoDragDrop()
        {
        }

        /// <summary>
        /// Called when mouse double clicks header.
        /// </summary>
        /// <param name="location">The mouse location.</param>
        /// <param name="button">The button.</param>
        /// <returns>True if event has been handled.</returns>
        protected virtual bool OnMouseDoubleClickHeader(ref Vector2 location, MouseButton button)
        {
            // Toggle open state
            if (_opened)
                Collapse();
            else
                Expand();

            // Handled
            return true;
        }

        /// <summary>
        /// Called when mouse is pressing node header for a long time.
        /// </summary>
        protected virtual void OnLongPress()
        {
        }

        /// <summary>
        /// Called when expanded/collapsed state changes.
        /// </summary>
        protected virtual void OnExpandedChanged()
        {
        }

        /// <summary>
        /// Tests the header hit.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>True if hits it.</returns>
        protected virtual bool TestHeaderHit(ref Vector2 location)
        {
            return _headerRect.Contains(ref location);
        }

        /// <summary>
        /// Updates the drag over mode based on the given mouse location.
        /// </summary>
        /// <param name="location">The location.</param>
        private void UpdateDrawPositioning(ref Vector2 location)
        {
            if (new Rectangle(_headerRect.X, _headerRect.Y - DefaultDragInsertPositionMargin - DefaultNodeOffsetY, _headerRect.Width, DefaultDragInsertPositionMargin * 2.0f).Contains(location))
                _dragOverMode = DragItemPositioning.Above;
            else if ((IsCollapsed || !HasAnyVisibleChild) && new Rectangle(_headerRect.X, _headerRect.Bottom - DefaultDragInsertPositionMargin, _headerRect.Width, DefaultDragInsertPositionMargin * 2.0f).Contains(location))
                _dragOverMode = DragItemPositioning.Below;
            else
                _dragOverMode = DragItemPositioning.At;
        }

        /// <summary>
        /// Caches the color of the text for this node. Called during update before children nodes but after parent node so it can reuse parent tree node data.
        /// </summary>
        /// <returns>Text color.</returns>
        protected virtual Color CacheTextColor()
        {
            return Enabled ? TextColor : TextColor * 0.6f;
        }

        /// <summary>
        /// Updates the cached width of the text.
        /// </summary>
        protected void UpdateTextWidth()
        {
            if (_textChanged)
            {
                var font = TextFont.GetFont();
                if (font)
                {
                    _textWidth = font.MeasureText(_text).X;
                    _textChanged = false;
                }
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Cache text color
            _cachedTextColor = CacheTextColor();

            // Drop/down animation
            if (_animationProgress < 1.0f)
            {
                bool isDeltaSlow = deltaTime > (1 / 20.0f);

                // Update progress
                if (isDeltaSlow)
                {
                    _animationProgress = 1.0f;
                }
                else
                {
                    const float openCloseAnimationTime = 0.1f;
                    _animationProgress += deltaTime / openCloseAnimationTime;
                    if (_animationProgress > 1.0f)
                        _animationProgress = 1.0f;
                }

                // Arrange controls
                PerformLayout();
            }

            // Check for long press
            const float longPressTimeSeconds = 0.6f;
            if (_isMouseDown && Time.UnscaledGameTime - _mouseDownTime > longPressTimeSeconds)
            {
                OnLongPress();
            }

            // Don't update collapsed children
            if (_opened)
            {
                base.Update(deltaTime);
            }
            else
            {
                // Manually update tooltip
                if (TooltipText != null && IsMouseOver)
                    Tooltip.OnMouseOverControl(this, deltaTime);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            var tree = ParentTree;
            bool isSelected = tree.Selection.Contains(this);
            bool isFocused = tree.ContainsFocus;
            var left = _xOffset + 16; // offset + arrow
            var textRect = new Rectangle(left, 0, Width - left, _headerHeight);
            _margin.ShrinkRectangle(ref textRect);

            // Draw background
            if (isSelected || _mouseOverHeader)
            {
                Render2D.FillRectangle(_headerRect, (isSelected && isFocused) ? BackgroundColorSelected : (_mouseOverHeader ? BackgroundColorHighlighted : BackgroundColorSelectedUnfocused));
            }

            // Draw arrow
            if (HasAnyVisibleChild)
            {
                Render2D.DrawSprite(_opened ? style.ArrowDown : style.ArrowRight, ArrowRect, _mouseOverHeader ? style.Foreground : style.ForegroundGrey);
            }

            // Draw icon
            if (_iconCollaped.IsValid)
            {
                Render2D.DrawSprite(_opened ? _iconOpened : _iconCollaped, new Rectangle(textRect.Left, 0, 16, 16), IconColor);
                textRect.X += 18.0f;
                textRect.Width -= 18.0f;
            }

            // Draw text
            Render2D.DrawText(TextFont.GetFont(), _text, textRect, _cachedTextColor, TextAlignment.Near, TextAlignment.Center);

            // Draw drag and drop effect
            if (IsDragOver)
            {
                Color dragOverColor = style.BackgroundSelected * 0.6f;
                Rectangle rect;
                switch (_dragOverMode)
                {
                case DragItemPositioning.At:
                    rect = textRect;
                    break;
                case DragItemPositioning.Above:
                    rect = new Rectangle(textRect.X, textRect.Y - DefaultDragInsertPositionMargin - DefaultNodeOffsetY, textRect.Width, DefaultDragInsertPositionMargin * 2.0f);
                    break;
                case DragItemPositioning.Below:
                    rect = new Rectangle(textRect.X, textRect.Bottom - DefaultDragInsertPositionMargin, textRect.Width, DefaultDragInsertPositionMargin * 2.0f);
                    break;
                default:
                    rect = Rectangle.Empty;
                    break;
                }
                Render2D.FillRectangle(rect, dragOverColor);
            }

            // Base
            if (_opened)
            {
                if (ClipChildren)
                {
                    Render2D.PushClip(new Rectangle(0, _headerHeight, Width, Height - _headerHeight));
                    base.Draw();
                    Render2D.PopClip();
                }
                else
                {
                    base.Draw();
                }
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            // Check if mouse hits bar and node isn't a root
            if (_mouseOverHeader)
            {
                // Check if left button goes down
                if (button == MouseButton.Left)
                {
                    _isMouseDown = true;
                    _mouseDownPos = location;
                    _mouseDownTime = Time.UnscaledGameTime;
                }

                // Handled
                Focus();
                return true;
            }

            // Base
            if (_opened)
                return base.OnMouseDown(location, button);

            // Handled
            Focus();
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            // Check if mouse hits bar
            if (button == MouseButton.Right && TestHeaderHit(ref location))
            {
                ParentTree.OnRightClickInternal(this, ref location);
            }

            // Clear flag for left button
            if (button == MouseButton.Left)
            {
                // Clear flag
                _isMouseDown = false;
                _mouseDownTime = -1;
            }

            // Check if mouse hits bar and node isn't a root
            if (_mouseOverHeader)
            {
                // Prevent from selecting node when user is just clicking at an arrow
                if (!_mouseOverArrow)
                {
                    // Check if user is pressing control key
                    var tree = ParentTree;
                    var window = tree.Root;
                    if (window.GetKey(KeyboardKeys.Shift))
                    {
                        // Select range
                        tree.SelectRange(this);
                    }
                    else if (window.GetKey(KeyboardKeys.Control))
                    {
                        // Add/Remove
                        tree.AddOrRemoveSelection(this);
                    }
                    else
                    {
                        // Select
                        tree.Select(this);
                    }
                }

                // Check if mouse hits arrow
                if (HasAnyVisibleChild && _mouseOverArrow)
                {
                    // Toggle open state
                    if (_opened)
                        Collapse();
                    else
                        Expand();
                }

                // Handled
                Focus();
                return true;
            }

            // Base
            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            // Check if mouse hits bar
            if (TestHeaderHit(ref location))
            {
                return OnMouseDoubleClickHeader(ref location, button);
            }

            // Check if animation has been finished
            if (_animationProgress >= 1.0f)
            {
                // Base
                return base.OnMouseDoubleClick(location, button);
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            // Cache flags
            _mouseOverArrow = HasAnyVisibleChild && ArrowRect.Contains(location);
            _mouseOverHeader = new Rectangle(0, 0, Width, _headerHeight - 1).Contains(location);
            if (_mouseOverHeader)
            {
                // Allow non-scrollable controls to stay on top of the header and override the mouse behaviour
                for (int i = 0; i < Children.Count; i++)
                {
                    if (!Children[i].IsScrollable && IntersectsChildContent(Children[i], location, out _))
                    {
                        _mouseOverHeader = false;
                        break;
                    }
                }
            }

            // Check if start drag and drop
            if (_isMouseDown && Vector2.Distance(_mouseDownPos, location) > 10.0f)
            {
                // Clear flag
                _isMouseDown = false;
                _mouseDownTime = -1;

                // Start
                DoDragDrop();
                return;
            }

            // Check if animation has been finished
            if (_animationProgress >= 1.0f)
            {
                // Base
                if (_opened)
                    base.OnMouseMove(location);
            }
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flags
            _mouseOverArrow = false;
            _mouseOverHeader = false;

            // Check if start drag and drop
            if (_isMouseDown)
            {
                // Clear flag
                _isMouseDown = false;
                _mouseDownTime = -1;

                // Start
                DoDragDrop();
            }

            // Base
            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Base
            if (_opened)
                return base.OnKeyDown(key);
            return false;
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            // Base
            if (_opened)
                base.OnKeyUp(key);
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            PerformLayout();
            ParentTree.UpdateSize();
            base.OnChildResized(control);
        }

        /// <inheritdoc />
        public override void OnParentResized()
        {
            base.OnParentResized();

            Width = Parent.Width;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);

            // Check if no children handled that event
            _dragOverMode = DragItemPositioning.None;
            if (result == DragDropEffect.None)
            {
                UpdateDrawPositioning(ref location);

                // Check if mouse is over header
                _isDragOverHeader = TestHeaderHit(ref location);
                if (_isDragOverHeader)
                {
                    // Check if mouse is over arrow
                    if (ArrowRect.Contains(location) && HasAnyVisibleChild)
                    {
                        // Expand node (no animation)
                        Expand(true);
                    }

                    result = OnDragEnterHeader(data);
                }

                if (result == DragDropEffect.None)
                    _dragOverMode = DragItemPositioning.None;
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);

            // Check if no children handled that event
            _dragOverMode = DragItemPositioning.None;
            if (result == DragDropEffect.None)
            {
                UpdateDrawPositioning(ref location);

                // Check if mouse is over header
                bool isDragOverHeader = TestHeaderHit(ref location);
                if (isDragOverHeader)
                {
                    if (ArrowRect.Contains(location) && HasAnyVisibleChild)
                    {
                        // Expand node (no animation)
                        Expand(true);
                    }

                    if (!_isDragOverHeader)
                        result = OnDragEnterHeader(data);
                    else
                        result = OnDragMoveHeader(data);
                }
                _isDragOverHeader = isDragOverHeader;

                if (result == DragDropEffect.None)
                    _dragOverMode = DragItemPositioning.None;
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);

            // Check if no children handled that event
            if (result == DragDropEffect.None)
            {
                UpdateDrawPositioning(ref location);

                // Check if mouse is over header
                if (TestHeaderHit(ref location))
                {
                    result = OnDragDropHeader(data);
                }
            }

            // Clear cache
            _isDragOverHeader = false;
            _dragOverMode = DragItemPositioning.None;

            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            // Clear cache
            if (_isDragOverHeader)
            {
                _isDragOverHeader = false;
                OnDragLeaveHeader();
            }
            _dragOverMode = DragItemPositioning.None;

            base.OnDragLeave();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            _headerRect = new Rectangle(0, 0, Width, _headerHeight);
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            // Check if update is locked
            if (IsLayoutLocked && !force)
                return;

            // Update the nodes nesting level before the actual positioning
            float xOffset = _xOffset + ChildrenIndent;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is TreeNode node && node.Visible)
                    node._xOffset = xOffset;
            }

            base.PerformLayout(force);
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            _cachedTextColor = CacheTextColor();

            // Prepare
            float y = _headerHeight;
            float height = _headerHeight;
            float xOffset = _xOffset + ChildrenIndent;
            y -= _cachedHeight * (_opened ? 1.0f - _animationProgress : _animationProgress);

            // Arrange children
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is TreeNode node && node.Visible)
                {
                    node._xOffset = xOffset;
                    node.Location = new Vector2(0, y);
                    float nodeHeight = node.Height + DefaultNodeOffsetY;
                    y += nodeHeight;
                    height += nodeHeight;
                }
            }

            // Cache calculated height
            _cachedHeight = height;

            // Force to be closed
            if (_animationProgress >= 1.0f && !_opened)
            {
                y = _headerHeight;
            }

            // Set height
            Height = Mathf.Max(_headerHeight, y);
        }

        /// <inheritdoc />
        protected override void OnParentChangedInternal()
        {
            // Clear cached tree
            _tree = null;
            if (Parent != null)
                Width = Parent.Width;

            base.OnParentChangedInternal();
        }

        /// <inheritdoc />
        public override int Compare(Control other)
        {
            if (other is TreeNode node)
            {
                return string.Compare(Text, node.Text, StringComparison.InvariantCulture);
            }
            return base.Compare(other);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            ParentTree?.Selection.Remove(this);

            base.OnDestroy();
        }
    }
}
