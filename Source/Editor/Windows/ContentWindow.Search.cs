// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.Json;

namespace FlaxEditor.Windows
{
    public sealed partial class ContentWindow
    {
        private class ViewDropdown : ComboBox
        {
            public void OnClicked(int index)
            {
                OnItemClicked(index);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                // Cache data
                var clientRect = new Rectangle(Float2.Zero, Size);
                float margin = clientRect.Height * 0.2f;
                float boxSize = clientRect.Height - margin * 2;
                bool isOpened = IsPopupOpened;
                bool enabled = EnabledInHierarchy;
                Color backgroundColor = BackgroundColor;
                Color borderColor = BorderColor;
                Color arrowColor = ArrowColor;
                if (!enabled)
                {
                    backgroundColor *= 0.5f;
                    arrowColor *= 0.7f;
                }
                else if (isOpened || _mouseDown)
                {
                    backgroundColor = BackgroundColorSelected;
                    borderColor = BorderColorSelected;
                    arrowColor = ArrowColorSelected;
                }
                else if (IsMouseOver)
                {
                    backgroundColor = BackgroundColorHighlighted;
                    borderColor = BorderColorHighlighted;
                    arrowColor = ArrowColorHighlighted;
                }

                // Background
                Render2D.FillRectangle(clientRect, backgroundColor);
                Render2D.DrawRectangle(clientRect, borderColor);

                // Draw text
                float textScale = Height / DefaultHeight;
                var textRect = new Rectangle(margin, 0, clientRect.Width - boxSize - 2.0f * margin, clientRect.Height);
                Render2D.PushClip(textRect);
                var textColor = TextColor;
                Render2D.DrawText(Font.GetFont(), "View", textRect, enabled ? textColor : textColor * 0.5f, TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, textScale);
                Render2D.PopClip();

                // Arrow
                ArrowImage?.Draw(new Rectangle(clientRect.Width - margin - boxSize, margin, boxSize, boxSize), arrowColor);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                // Check flags
                if (_mouseDown && !_blockPopup)
                {
                    // Clear flag
                    _mouseDown = false;

                    // Ensure to have valid menu
                    if (_popupMenu == null)
                    {
                        _popupMenu = OnCreatePopup();
                        _popupMenu.MaximumItemsInViewCount = MaximumItemsInViewCount;
                        _popupMenu.VisibleChanged += cm =>
                        {
                            var win = Root;
                            _blockPopup = win != null && new Rectangle(Float2.Zero, Size).Contains(PointFromWindow(win.MousePosition));
                            if (!_blockPopup)
                                Focus();
                        };
                    }

                    // Check if menu hs been already shown
                    if (_popupMenu.Visible)
                    {
                        // Hide
                        _popupMenu.Hide();
                        return true;
                    }

                    // Show
                    _popupMenu.Show(this, new Float2(1, Height));
                }
                else
                {
                    _blockPopup = false;
                }

                return true;
            }
        }

        private void OnFoldersSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            var root = _root;
            root.LockChildrenRecursive();

            // Update tree
            var query = _foldersSearchBox.Text;
            root.UpdateFilter(query);

            root.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        /// <summary>
        /// Clears the items searching query text and filters.
        /// </summary>
        public void ClearItemsSearch()
        {
            // Skip if already cleared
            if (_itemsSearchBox.TextLength == 0)
                return;

            IsLayoutLocked = true;

            _itemsSearchBox.Clear();
            _viewDropdown.SelectedIndex = -1;

            IsLayoutLocked = false;

            UpdateItemsSearch();
        }

        private bool TryParseAssetId(string text, out AssetItem item)
        {
            item = null;
            if (text.Length != 32)
                return false;

            JsonSerializer.ParseID(text, out var id);
            item = Editor.ContentDatabase.FindAsset(id);
            return item != null;
        }

        private void UpdateItemsSearch()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            // Check if clear filters
            if (_itemsSearchBox.TextLength == 0 && !_viewDropdown.HasSelection)
            {
                _view.IsSearching = false;
                RefreshView();
                return;
            }

            // Apply filter
            var items = new List<ContentItem>(8);
            var query = _itemsSearchBox.Text;
            var filters = new bool[_viewDropdown.Items.Count];
            if (_viewDropdown.HasSelection)
            {
                // Update filters flags
                for (int i = 0; i < filters.Length; i++)
                {
                    filters[i] = _viewDropdown.Selection.Contains(i);
                }
            }
            else
            {
                // No filters
                for (int i = 0; i < filters.Length; i++)
                {
                    filters[i] = true;
                }
            }

            // Search by filter only
            bool showAllFiles = _showAllFiles;
            if (string.IsNullOrWhiteSpace(query))
            {
                if (SelectedNode == _root)
                {
                    // Special case for root folder
                    for (int i = 0; i < _root.ChildrenCount; i++)
                    {
                        if (_root.GetChild(i) is ContentTreeNode node)
                            UpdateItemsSearchFilter(node.Folder, items, filters, showAllFiles);
                    }
                }
                else
                {
                    UpdateItemsSearchFilter(CurrentViewFolder, items, filters, showAllFiles);
                }
            }
            // Search by asset ID
            else if (TryParseAssetId(query, out var assetItem))
            {
                items.Add(assetItem);
            }
            // Search by custom query text
            else
            {
                if (SelectedNode == _root)
                {
                    // Special case for root folder
                    for (int i = 0; i < _root.ChildrenCount; i++)
                    {
                        if (_root.GetChild(i) is ContentTreeNode node)
                            UpdateItemsSearchFilter(node.Folder, items, filters, showAllFiles, query);
                    }
                }
                else
                {
                    UpdateItemsSearchFilter(CurrentViewFolder, items, filters, showAllFiles, query);
                }
            }

            _view.IsSearching = true;
            _view.ShowItems(items, _sortType);
        }

        private void UpdateItemsSearchFilter(ContentFolder folder, List<ContentItem> items, bool[] filters, bool showAllFiles)
        {
            for (int i = 0; i < folder.Children.Count; i++)
            {
                var child = folder.Children[i];
                if (child is ContentFolder childFolder)
                {
                    UpdateItemsSearchFilter(childFolder, items, filters, showAllFiles);
                }
                else if (filters[(int)child.SearchFilter] && (showAllFiles || !(child is FileItem)))
                {
                    items.Add(child);
                }
            }
        }

        private void UpdateItemsSearchFilter(ContentFolder folder, List<ContentItem> items, bool[] filters, bool showAllFiles, string filterText)
        {
            for (int i = 0; i < folder.Children.Count; i++)
            {
                var child = folder.Children[i];
                if (child is ContentFolder childFolder)
                {
                    UpdateItemsSearchFilter(childFolder, items, filters, showAllFiles, filterText);
                }
                else if (filters[(int)child.SearchFilter] && (showAllFiles || !(child is FileItem)) && QueryFilterHelper.Match(filterText, child.ShortName))
                {
                    items.Add(child);
                }
            }
        }
    }
}
