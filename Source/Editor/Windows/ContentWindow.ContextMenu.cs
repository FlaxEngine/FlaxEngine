// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.Json;

namespace FlaxEditor.Windows
{
    public partial class ContentWindow
    {
        /// <summary>
        /// Occurs when content window wants to show the context menu for the given content item. Allows to add custom options.
        /// </summary>
        public event Action<ContextMenu, ContentItem> ContextMenuShow;

        private void ShowContextMenuForItem(ContentItem item, ref Vector2 location, bool isTreeNode)
        {
            Assert.IsNull(_newElement);

            // Cache data
            bool isValidElement = item != null;
            var proxy = Editor.ContentDatabase.GetProxy(item);
            ContentFolder folder = null;
            bool isFolder = false;
            if (isValidElement)
            {
                isFolder = item.IsFolder;
                folder = isFolder ? (ContentFolder)item : item.ParentFolder;
            }
            else
            {
                folder = CurrentViewFolder;
            }
            Assert.IsNotNull(folder);
            bool isRootFolder = CurrentViewFolder == _root.Folder;

            // Create context menu
            ContextMenuButton b;
            ContextMenuChildMenu c;
            ContextMenu cm = new ContextMenu();
            cm.Tag = item;
            if (isTreeNode)
            {
                b = cm.AddButton("Expand All", OnExpandAllClicked);
                b.Enabled = CurrentViewFolder.Node.ChildrenCount != 0;

                b = cm.AddButton("Collapse All", OnCollapseAllClicked);
                b.Enabled = CurrentViewFolder.Node.ChildrenCount != 0;

                cm.AddSeparator();
            }
            if (item is ContentFolder contentFolder && contentFolder.Node is ProjectTreeNode)
            {
                cm.AddButton("Show in explorer", () => FileSystem.ShowFileExplorer(CurrentViewFolder.Path));
            }
            else if (isValidElement)
            {
                b = cm.AddButton("Open", () => Open(item));
                b.Enabled = proxy != null || isFolder;

                cm.AddButton("Show in explorer", () => FileSystem.ShowFileExplorer(System.IO.Path.GetDirectoryName(item.Path)));

                if (item.HasDefaultThumbnail == false)
                {
                    cm.AddButton("Refresh thumbnail", item.RefreshThumbnail);
                }

                if (!isFolder)
                {
                    b = cm.AddButton("Reimport", ReimportSelection);
                    b.Enabled = proxy != null && proxy.CanReimport(item);

                    if (item is BinaryAssetItem binaryAsset)
                    {
                        string importPath;
                        if (!binaryAsset.GetImportPath(out importPath))
                        {
                            string importLocation = System.IO.Path.GetDirectoryName(importPath);
                            if (!string.IsNullOrEmpty(importLocation) && System.IO.Directory.Exists(importLocation))
                            {
                                cm.AddButton("Show import location", () => FileSystem.ShowFileExplorer(importLocation));
                            }
                        }
                    }

                    if (item is AssetItem assetItem)
                    {
                        cm.AddButton("Copy asset ID", () => Clipboard.Text = JsonSerializer.GetStringID(assetItem.ID));
                    }

                    if (Editor.CanExport(item.Path))
                    {
                        b = cm.AddButton("Export", ExportSelection);
                    }
                }

                cm.AddButton("Delete", () => Delete(item));

                cm.AddSeparator();

                cm.AddButton("Duplicate", _view.Duplicate);

                cm.AddButton("Copy", _view.Copy);

                b = cm.AddButton("Paste", _view.Paste);
                b.Enabled = _view.CanPaste();

                cm.AddButton("Rename", () => Rename(item));

                // Custom options
                ContextMenuShow?.Invoke(cm, item);
                proxy?.OnContentWindowContextMenu(cm, item);

                cm.AddButton("Copy name to Clipboard", () => Clipboard.Text = item.NamePath);

                cm.AddButton("Copy path to Clipboard", () => Clipboard.Text = item.Path);
            }
            else
            {
                cm.AddButton("Show in explorer", () => FileSystem.ShowFileExplorer(CurrentViewFolder.Path));

                b = cm.AddButton("Paste", _view.Paste);
                b.Enabled = _view.CanPaste();

                cm.AddButton("Refresh", () => Editor.ContentDatabase.RefreshFolder(CurrentViewFolder, true));

                cm.AddButton("Refresh all thumbnails", RefreshViewItemsThumbnails);
            }

            cm.AddSeparator();

            if (!isRootFolder)
            {
                cm.AddButton("New folder", NewFolder);
            }

            c = cm.AddChildMenu("New");
            c.ContextMenu.Tag = item;
            c.ContextMenu.AutoSort = true;
            int newItems = 0;
            for (int i = 0; i < Editor.ContentDatabase.Proxy.Count; i++)
            {
                var p = Editor.ContentDatabase.Proxy[i];
                if (p.CanCreate(folder))
                {
                    c.ContextMenu.AddButton(p.Name, () => NewItem(p));
                    newItems++;
                }
            }
            c.Enabled = newItems > 0;

            if (folder.CanHaveAssets)
            {
                cm.AddButton("Import file", () =>
                {
                    _view.ClearSelection();
                    Editor.ContentImporting.ShowImportFileDialog(CurrentViewFolder);
                });
            }

            // Show it
            cm.Show(this, location);
        }

        private void OnExpandAllClicked(ContextMenuButton button)
        {
            CurrentViewFolder.Node.ExpandAll();
        }

        private void OnCollapseAllClicked(ContextMenuButton button)
        {
            CurrentViewFolder.Node.CollapseAll();
        }

        /// <summary>
        /// Refreshes thumbnails for all the items in the view.
        /// </summary>
        private void RefreshViewItemsThumbnails()
        {
            var items = _view.Items;
            for (int i = 0; i < items.Count; i++)
            {
                items[i].RefreshThumbnail();
            }
        }

        /// <summary>
        /// Reimports the selected assets.
        /// </summary>
        private void ReimportSelection()
        {
            var selection = _view.Selection;
            for (int i = 0; i < selection.Count; i++)
            {
                if (selection[i] is BinaryAssetItem binaryAssetItem)
                    Editor.ContentImporting.Reimport(binaryAssetItem);
            }
        }

        private bool Export(ContentItem item, string outputFolder)
        {
            if (item is ContentFolder folder)
            {
                for (int i = 0; i < folder.Children.Count; i++)
                {
                    if (Export(folder.Children[i], outputFolder))
                        return true;
                }
            }
            else if (item is AssetItem asset && Editor.CanExport(asset.Path))
            {
                if (Editor.Export(asset.Path, outputFolder))
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Exports the selected items.
        /// </summary>
        private void ExportSelection()
        {
            if (FileSystem.ShowBrowseFolderDialog(Editor.Windows.MainWindow, null, "Select the output folder", out var outputFolder))
                return;

            var selection = _view.Selection;
            for (int i = 0; i < selection.Count; i++)
            {
                if (Export(selection[i], outputFolder))
                    return;
            }
        }
    }
}
