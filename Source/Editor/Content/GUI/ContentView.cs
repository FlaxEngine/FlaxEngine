// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Options;
using FlaxEditor.SceneGraph;
using FlaxEditor.Windows;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content.GUI
{
    /// <summary>
    /// The content items view modes.
    /// </summary>
    [HideInEditor]
    public enum ContentViewType
    {
        /// <summary>
        /// The uniform tiles.
        /// </summary>
        Tiles,

        /// <summary>
        /// The vertical list.
        /// </summary>
        List,
    }

    /// <summary>
    /// The method sort for items.
    /// </summary>
    public enum SortType
    {
        /// <summary>
        /// The classic alphabetic sort method (A-Z).
        /// </summary>
        AlphabeticOrder,

        /// <summary>
        /// The reverse alphabetic sort method (Z-A).
        /// </summary>
        AlphabeticReverse
    }

    /// <summary>
    /// Main control for <see cref="ContentWindow"/> used to present collection of <see cref="ContentItem"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    /// <seealso cref="FlaxEditor.Content.IContentItemOwner" />
    [HideInEditor]
    public partial class ContentView : ContainerControl, IContentItemOwner
    {
        private readonly List<ContentItem> _items = new List<ContentItem>(256);
        private readonly List<ContentItem> _selection = new List<ContentItem>();

        private float _viewScale = 1.0f;
        private ContentViewType _viewType = ContentViewType.Tiles;
        private bool _isRubberBandSpanning;
        private Float2 _mousePressLocation;
        private Rectangle _rubberBandRectangle;
        private bool _isCutting;
        private List<ContentItem> _cutItems = new List<ContentItem>();

        private bool _validDragOver;
        private DragActors _dragActors;

        #region External Events

        /// <summary>
        /// Called when user wants to open the item.
        /// </summary>
        public event Action<ContentItem> OnOpen;

        /// <summary>
        /// Called when user wants to rename the item.
        /// </summary>
        public event Action<ContentItem> OnRename;

        /// <summary>
        /// Called when user wants to delete the item.
        /// </summary>
        public event Action<List<ContentItem>> OnDelete;

        /// <summary>
        /// Called when user wants to paste the files/folders. Bool is for cutting.
        /// </summary>
        public event Action<string[], bool> OnPaste;

        /// <summary>
        /// Called when user wants to duplicate the item(s).
        /// </summary>
        public event Action<List<ContentItem>> OnDuplicate;

        /// <summary>
        /// Called when user wants to navigate backward.
        /// </summary>
        public event Action OnNavigateBack;

        /// <summary>
        /// Occurs when view scale gets changed.
        /// </summary>
        public event Action ViewScaleChanged;

        /// <summary>
        /// Occurs when view type gets changed.
        /// </summary>
        public event Action ViewTypeChanged;

        #endregion

        /// <summary>
        /// Gets the items.
        /// </summary>
        public List<ContentItem> Items => _items;

        /// <summary>
        /// Gets the items count.
        /// </summary>
        public int ItemsCount => _items.Count;

        /// <summary>
        /// Gets the selected items.
        /// </summary>
        public List<ContentItem> Selection => _selection;

        /// <summary>
        /// Gets the selected count.
        /// </summary>
        public int SelectedCount => _selection.Count;

        /// <summary>
        /// Gets a value indicating whether any item is selected.
        /// </summary>
        public bool HasSelection => _selection.Count > 0;

        /// <summary>
        /// Gets or sets the view scale.
        /// </summary>
        public float ViewScale
        {
            get => _viewScale;
            set
            {
                value = Mathf.Clamp(value, 0.3f, 3.0f);
                if (value != _viewScale)
                {
                    _viewScale = value;
                    ViewScaleChanged?.Invoke();
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the type of the view.
        /// </summary>
        public ContentViewType ViewType
        {
            get => _viewType;
            set
            {
                if (_viewType != value)
                {
                    _viewType = value;
                    ViewTypeChanged?.Invoke();
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Flag is used to indicate if user is searching for items. Used to show a proper message to the user.
        /// </summary>
        public bool IsSearching;

        /// <summary>
        /// Flag used to indicate whenever show full file names including extensions.
        /// </summary>
        public bool ShowFileExtensions;

        /// <summary>
        /// The input actions collection to processed during user input.
        /// </summary>
        public readonly InputActionsContainer InputActions;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentView"/> class.
        /// </summary>
        public ContentView()
        {
            // Setup input actions
            InputActions = new InputActionsContainer(new[]
            {
                new InputActionsContainer.Binding(options => options.Delete, () =>
                {
                    if (HasSelection)
                        OnDelete?.Invoke(_selection);
                }),
                new InputActionsContainer.Binding(options => options.SelectAll, SelectAll),
                new InputActionsContainer.Binding(options => options.DeselectAll, DeselectAll),
                new InputActionsContainer.Binding(options => options.Rename, () =>
                {
                    if (HasSelection && _selection[0].CanRename)
                    {
                        if (_selection.Count > 1)
                            Select(_selection[0]);
                        OnRename?.Invoke(_selection[0]);
                    }
                }),
                new InputActionsContainer.Binding(options => options.Copy, Copy),
                new InputActionsContainer.Binding(options => options.Paste, Paste),
                new InputActionsContainer.Binding(options => options.Cut, Cut),
                new InputActionsContainer.Binding(options => options.Undo, () =>
                {
                    if (_isCutting)
                        UpdateContentItemCut(false);
                }),
                new InputActionsContainer.Binding(options => options.Duplicate, Duplicate),
            });
        }

        /// <summary>
        /// Clears the items in the view.
        /// </summary>
        public void ClearItems()
        {
            // Lock layout
            var wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Deselect items first
            ClearSelection();

            // Remove references and unlink items
            for (int i = 0; i < _items.Count; i++)
            {
                var item = _items[i];
                item.Parent = null;
                item.RemoveReference(this);
            }
            _items.Clear();

            // Unload and perform UI layout
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Shows the items collection in the view.
        /// </summary>
        /// <param name="items">The items to show.</param>
        /// <param name="sortType">The sort method for items.</param>
        /// <param name="additive">If set to <c>true</c> items will be added to the current list. Otherwise items list will be cleared before.</param>
        /// <param name="keepSelection">If set to <c>true</c> selected items list will be preserved. Otherwise selection will be cleared before.</param>
        public void ShowItems(List<ContentItem> items, SortType sortType, bool additive = false, bool keepSelection = false)
        {
            if (items == null)
                throw new ArgumentNullException();

            // Check if show nothing or not change view
            if (items.Count == 0)
            {
                // Deselect items if need to
                if (!additive)
                    ClearItems();
                return;
            }

            // Lock layout
            var wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            var selection = !additive && keepSelection ? _selection.ToArray() : null;

            // Deselect items if need to
            if (!additive)
                ClearItems();

            // Add references and link items
            for (int i = 0; i < items.Count; i++)
            {
                var item = items[i];
                if (item.Visible && !_items.Contains(item))
                {
                    item.Parent = this;
                    item.AddReference(this);
                    _items.Add(item);
                }
            }
            if (selection != null)
            {
                _selection.Clear();
                _selection.AddRange(selection);
            }

            // Sort items depending on sortMethod parameter
            _children.Sort(((control, control1) =>
                               {
                                   if (control == null || control1 == null)
                                       return 0;
                                   if (sortType == SortType.AlphabeticReverse)
                                   {
                                       if (control.CompareTo(control1) > 0)
                                           return -1;
                                       if (control.CompareTo(control1) == 0)
                                           return 0;
                                       return 1;
                                   }
                                   return control.CompareTo(control1);
                               }));

            // Unload and perform UI layout
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Determines whether the specified item is selected.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns><c>true</c> if the specified item is selected; otherwise, <c>false</c>.</returns>
        public bool IsSelected(ContentItem item)
        {
            return _selection.Contains(item);
        }

        /// <summary>
        /// Clears the selected items collection.
        /// </summary>
        public void ClearSelection()
        {
            if (_selection.Count == 0)
                return;

            _selection.Clear();
        }

        /// <summary>
        /// Selects the specified items.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <param name="additive">If set to <c>true</c> items will be added to the current selection. Otherwise selection will be cleared before.</param>
        public void Select(List<ContentItem> items, bool additive = false)
        {
            if (items == null)
                throw new ArgumentNullException();

            // Check if nothing to select
            if (items.Count == 0)
            {
                // Deselect items if need to
                if (!additive)
                    ClearSelection();
                return;
            }

            // Lock layout
            var wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Select items
            if (additive)
            {
                for (int i = 0; i < items.Count; i++)
                {
                    if (!_selection.Contains(items[i]))
                        _selection.Add(items[i]);
                }
            }
            else
            {
                _selection.Clear();
                _selection.AddRange(items);
            }

            // Unload and perform UI layout
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Selects the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="additive">If set to <c>true</c> item will be added to the current selection. Otherwise selection will be cleared before.</param>
        public void Select(ContentItem item, bool additive = false)
        {
            if (item == null)
                throw new ArgumentNullException();

            // Lock layout
            var wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Select item
            if (additive)
            {
                if (!_selection.Contains(item))
                    _selection.Add(item);
            }
            else
            {
                _selection.Clear();
                _selection.Add(item);
            }

            // Unload and perform UI layout
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        private void BulkSelectUpdate(bool select = true)
        {
            // Lock layout
            var wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Select items
            _selection.Clear();
            if (select)
                _selection.AddRange(_items);

            // Unload and perform UI layout
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Selects all the items.
        /// </summary>
        public void SelectAll()
        {
            BulkSelectUpdate(true);
        }

        /// <summary>
        /// Deselects all the items.
        /// </summary>
        public void DeselectAll()
        {
            BulkSelectUpdate(false);
        }

        /// <summary>
        /// Deselects the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void Deselect(ContentItem item)
        {
            if (item == null)
                throw new ArgumentNullException();

            // Lock layout
            var wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Deselect item
            if (_selection.Contains(item))
                _selection.Remove(item);

            // Unload and perform UI layout
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Duplicates the selected items.
        /// </summary>
        public void Duplicate()
        {
            UpdateContentItemCut(false);
            OnDuplicate?.Invoke(_selection);
        }

        /// <summary>
        /// Copies the selected items (to the system clipboard).
        /// </summary>
        public void Copy()
        {
            if (_selection.Count == 0)
                return;

            var files = _selection.ConvertAll(x => x.Path).ToArray();
            Clipboard.Files = files;
            UpdateContentItemCut(false);
        }

        /// <summary>
        /// Returns true if user can paste data to the view (copied any files before).
        /// </summary>
        /// <returns>True if can paste files.</returns>
        public bool CanPaste()
        {
            var files = Clipboard.Files;
            return files != null && files.Length > 0;
        }

        /// <summary>
        /// Pastes the copied items (from the system clipboard).
        /// </summary>
        public void Paste()
        {
            var files = Clipboard.Files;
            if (files == null || files.Length == 0)
                return;

            OnPaste?.Invoke(files, _isCutting);
            UpdateContentItemCut(false);
        }

        /// <summary>
        /// Cuts the items.
        /// </summary>
        public void Cut()
        {
            Copy();
            UpdateContentItemCut(true);
        }

        private void UpdateContentItemCut(bool cut)
        {
            _isCutting = cut;
  
            // Add selection to cut list
            if (cut)
                _cutItems.AddRange(_selection);
            
            // Update item with if it is being cut.
            foreach (var item in _cutItems)
            {
                item.IsBeingCut = cut;
            }
            
            // Clean up cut items
            if (!cut)
                _cutItems.Clear();
        }

        /// <summary>
        /// Gives focus and selects the first item in the view.
        /// </summary>
        public void SelectFirstItem()
        {
            if (_items.Count > 0)
            {
                _items[0].Focus();
                Select(_items[0]);
            }
            else
            {
                Focus();
            }
        }

        /// <summary>
        /// Refreshes thumbnails of all items in the <see cref="ContentView"/>.
        /// </summary>
        public void RefreshThumbnails()
        {
            for (int i = 0; i < _items.Count; i++)
                _items[i].RefreshThumbnail();
        }

        #region Internal events

        /// <summary>
        /// Called when user clicks on an item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void OnItemClick(ContentItem item)
        {
            bool isSelected = _selection.Contains(item);

            // Add/remove from selection
            if (Root.GetKey(KeyboardKeys.Control))
            {
                if (isSelected)
                    Deselect(item);
                else
                    Select(item, true);
            }
            // Range select
            else if (_selection.Count != 0 && Root.GetKey(KeyboardKeys.Shift))
            {
                int min = _selection.Min(x => x.IndexInParent);
                int max = _selection.Max(x => x.IndexInParent);
                min = Mathf.Max(Mathf.Min(min, item.IndexInParent), 0);
                max = Mathf.Min(Mathf.Max(max, item.IndexInParent), _children.Count - 1);
                var selection = new List<ContentItem>(_selection);
                for (int i = min; i <= max; i++)
                {
                    if (_children[i] is ContentItem cc && !selection.Contains(cc))
                    {
                        selection.Add(cc);
                    }
                }
                Select(selection);
            }
            // Select
            else
            {
                Select(item);
            }
        }

        /// <summary>
        /// Called when user wants to open item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void OnItemDoubleClick(ContentItem item)
        {
            OnOpen?.Invoke(item);
        }

        #endregion

        #region IContentItemOwner

        /// <inheritdoc />
        void IContentItemOwner.OnItemDeleted(ContentItem item)
        {
            _selection.Remove(item);
            _items.Remove(item);
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemDispose(ContentItem item)
        {
            _selection.Remove(item);
            _items.Remove(item);
        }

        #endregion

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;

            // Check if drag is over
            if (IsDragOver && _validDragOver)
            {
                var bounds = new Rectangle(Float2.One, Size - Float2.One * 2);
                Render2D.FillRectangle(bounds, style.Selection);
                Render2D.DrawRectangle(bounds, style.SelectionBorder);
            }

            // Check if it's an empty thing
            if (_items.Count == 0)
            {
                Render2D.DrawText(style.FontSmall, IsSearching ? "No results" : "Empty", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
            }

            // Selection
            if (_isRubberBandSpanning)
            {
                Render2D.FillRectangle(_rubberBandRectangle, style.Selection);
                Render2D.DrawRectangle(_rubberBandRectangle, style.SelectionBorder);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                _mousePressLocation = location;
                _rubberBandRectangle = new Rectangle(_mousePressLocation, 0, 0);
                _isRubberBandSpanning = true;
                StartMouseCapture();
                return true;
            }

            return AutoFocus && Focus(this);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isRubberBandSpanning)
            {
                _rubberBandRectangle.Width = location.X - _mousePressLocation.X;
                _rubberBandRectangle.Height = location.Y - _mousePressLocation.Y;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_isRubberBandSpanning)
            {
                _isRubberBandSpanning = false;
                EndMouseCapture();
                if (_rubberBandRectangle.Width < 0 || _rubberBandRectangle.Height < 0)
                {
                    // make sure we have a well-formed rectangle i.e. size is positive and X/Y is upper left corner
                    var size = _rubberBandRectangle.Size;
                    _rubberBandRectangle.X = Mathf.Min(_rubberBandRectangle.X, _rubberBandRectangle.X + _rubberBandRectangle.Width);
                    _rubberBandRectangle.Y = Mathf.Min(_rubberBandRectangle.Y, _rubberBandRectangle.Y + _rubberBandRectangle.Height);
                    size.X = Mathf.Abs(size.X);
                    size.Y = Mathf.Abs(size.Y);
                    _rubberBandRectangle.Size = size;
                }
                var itemsInRectangle = _items.Where(t => _rubberBandRectangle.Intersects(t.Bounds)).ToList();
                Select(itemsInRectangle, Input.GetKey(KeyboardKeys.Shift) || Input.GetKey(KeyboardKeys.Control));
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            // Check if pressing control key
            if (Root.GetKey(KeyboardKeys.Control))
            {
                // Zoom
                ViewScale += delta * 0.05f;

                // Handled
                return true;
            }

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Navigate backward
            if (key == KeyboardKeys.Backspace)
            {
                OnNavigateBack?.Invoke();
                return true;
            }

            if (InputActions.Process(Editor.Instance, this, key))
                return true;

            // Check if sth is selected
            if (HasSelection)
            {
                // Open
                if (key == KeyboardKeys.Return && _selection.Count != 0)
                {
                    foreach (var e in _selection.ToArray())
                        OnOpen?.Invoke(e);
                    return true;
                }

                // Movement with arrows
                {
                    var root = _selection[0];
                    var size = root.Size;
                    var offset = Float2.Minimum;
                    ContentItem item = null;
                    if (key == KeyboardKeys.ArrowUp)
                    {
                        offset = new Float2(0, -size.Y);
                    }
                    else if (key == KeyboardKeys.ArrowDown)
                    {
                        offset = new Float2(0, size.Y);
                    }
                    else if (key == KeyboardKeys.ArrowRight)
                    {
                        offset = new Float2(size.X, 0);
                    }
                    else if (key == KeyboardKeys.ArrowLeft)
                    {
                        offset = new Float2(-size.X, 0);
                    }
                    if (offset != Float2.Minimum)
                    {
                        item = GetChildAt(root.Location + size / 2 + offset) as ContentItem;
                    }
                    if (item != null)
                    {
                        OnItemClick(item);
                        return true;
                    }
                }
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (base.OnCharInput(c))
                return true;

            if (char.IsLetterOrDigit(c) && _items.Count != 0)
            {
                // Jump to the item starting with this character
                c = char.ToLowerInvariant(c);
                for (int i = 0; i < _items.Count; i++)
                {
                    var item = _items[i];
                    var name = item.ShortName;
                    if (!string.IsNullOrEmpty(name) && char.ToLowerInvariant(name[0]) == c)
                    {
                        Select(item);
                        if (Parent is Panel panel)
                            panel.ScrollViewTo(item, true);
                        break;
                    }
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            // Check if drop file(s)
            if (data is DragDataFiles)
            {
                _validDragOver = true;
                return DragDropEffect.Copy;
            }

            // Check if drop actor(s)
            if (_dragActors == null)
                _dragActors = new DragActors(ValidateDragActors);
            if (_dragActors.OnDragEnter(data))
            {
                _validDragOver = true;
                return DragDropEffect.Move;
            }

            return DragDropEffect.None;
        }

        private bool ValidateDragActors(ActorNode actor)
        {
            return actor.CanCreatePrefab && Editor.Instance.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;
        }

        private void ImportActors(DragActors actors, ContentFolder location)
        {
            foreach (var actorNode in actors.Objects)
            {
                var actor = actorNode.Actor;
                if (actors.Objects.Contains(actorNode.ParentNode as ActorNode))
                    continue;

                Editor.Instance.Prefabs.CreatePrefab(actor, false);
            }
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            _validDragOver = false;
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (data is DragDataFiles)
            {
                _validDragOver = true;
                result = DragDropEffect.Copy;
            }
            else if (_dragActors != null && _dragActors.HasValidDrag)
            {
                _validDragOver = true;
                result = DragDropEffect.Move;
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            // Check if drop file(s)
            if (data is DragDataFiles files)
            {
                // Import files
                var currentFolder = Editor.Instance.Windows.ContentWin.CurrentViewFolder;
                if (currentFolder != null)
                    Editor.Instance.ContentImporting.Import(files.Files, currentFolder);
                result = DragDropEffect.Copy;
            }
            // Check if drop actor(s)
            else if (_dragActors != null && _dragActors.HasValidDrag)
            {
                // Import actors
                var currentFolder = Editor.Instance.Windows.ContentWin.CurrentViewFolder;
                if (currentFolder != null)
                    ImportActors(_dragActors, currentFolder);

                _dragActors.OnDragDrop();
                result = DragDropEffect.Move;
            }

            // Clear cache
            _validDragOver = false;

            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _validDragOver = false;
            _dragActors?.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            float width = GetClientArea().Width;
            float x = 0, y = 1;
            float viewScale = _viewScale * 0.97f;

            switch (ViewType)
            {
            case ContentViewType.Tiles:
            {
                float defaultItemsWidth = ContentItem.DefaultWidth * viewScale;
                int itemsToFit = Mathf.FloorToInt(width / defaultItemsWidth) - 1;
                if (itemsToFit < 1)
                    itemsToFit = 1;
                int xSpace = 4;
                float itemsWidth = width / Mathf.Max(itemsToFit, 1) - xSpace;
                float itemsHeight = itemsWidth / defaultItemsWidth * (ContentItem.DefaultHeight * viewScale);
                var flooredItemsWidth = Mathf.Floor(itemsWidth);
                var flooredItemsHeight = Mathf.Floor(itemsHeight);
                x = itemsToFit == 1 ? 1 : itemsWidth / itemsToFit + xSpace;
                for (int i = 0; i < _children.Count; i++)
                {
                    var c = _children[i];
                    c.Bounds = new Rectangle(Mathf.Floor(x), Mathf.Floor(y), flooredItemsWidth, flooredItemsHeight);

                    x += (itemsWidth + xSpace) + (itemsWidth + xSpace) / itemsToFit;
                    if (x + itemsWidth > width)
                    {
                        x = itemsToFit == 1 ? 1 : itemsWidth / itemsToFit + xSpace;
                        y += itemsHeight + 7;
                    }
                }
                if (x > 0)
                    y += itemsHeight;

                break;
            }
            case ContentViewType.List:
            {
                float itemsHeight = 50.0f * viewScale;
                for (int i = 0; i < _children.Count; i++)
                {
                    var c = _children[i];
                    c.Bounds = new Rectangle(x, y, width, itemsHeight);
                    y += itemsHeight + 1;
                }
                y += 40.0f;

                break;
            }
            default: throw new ArgumentOutOfRangeException();
            }

            // Set maximum size and fit the parent container
            if (HasParent)
                y = Mathf.Max(y, Parent.Height);
            Height = y;

            base.PerformLayoutBeforeChildren();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;

            // Ensure to unlink all items
            ClearItems();

            base.OnDestroy();
        }
    }
}
