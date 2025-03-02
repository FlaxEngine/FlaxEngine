// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.Windows
{
    public partial class ContentWindow
    {
        /// <summary>
        /// Occurs when content window wants to show the context menu for the given content item. Allows to add custom options.
        /// </summary>
        public event Action<ContextMenu, ContentItem> ContextMenuShow;

        private void ShowContextMenuForItem(ContentItem item, ref Float2 location, bool isTreeNode)
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
            ContextMenu cm = new ContextMenu
            {
                Tag = item
            };

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
                cm.AddButton(Utilities.Constants.ShowInExplorer, () => FileSystem.ShowFileExplorer(CurrentViewFolder.Path));
            }
            else if (isValidElement)
            {
                b = cm.AddButton("Open", () => Open(item));
                b.Enabled = proxy != null || isFolder;

                if (_view.SelectedCount > 1)
                    b = cm.AddButton("Open (all selected)", () =>
                    {
                        foreach (var e in _view.Selection)
                            Open(e);
                    });

                cm.AddButton(Utilities.Constants.ShowInExplorer, () => FileSystem.ShowFileExplorer(System.IO.Path.GetDirectoryName(item.Path)));
                
                if (!String.IsNullOrEmpty(Editor.Instance.Windows.ContentWin._itemsSearchBox.Text))
                {
                    cm.AddButton("Show in Content Panel", () =>
                    {
                        Editor.Instance.Windows.ContentWin.ClearItemsSearch();
                        Editor.Instance.Windows.ContentWin.Select(item);
                    });
                }

                if (item.HasDefaultThumbnail == false)
                {
                    if (_view.SelectedCount > 1)
                        cm.AddButton("Refresh thumbnails", () =>
                        {
                            foreach (var e in _view.Selection)
                                e.RefreshThumbnail();
                        });
                    else
                        cm.AddButton("Refresh thumbnail", item.RefreshThumbnail);
                }

                if (!isFolder)
                {
                    b = cm.AddButton("Reimport", ReimportSelection);
                    b.Enabled = proxy != null && proxy.CanReimport(item);

                    if (item is BinaryAssetItem binaryAsset)
                    {
                        if (!binaryAsset.GetImportPath(out string importPath))
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
                        if (assetItem.IsLoaded)
                            cm.AddButton("Reload", assetItem.Reload);
                        cm.AddButton("Copy asset ID", () => Clipboard.Text = JsonSerializer.GetStringID(assetItem.ID));
                        cm.AddButton("Select actors using this asset", () => Editor.SceneEditing.SelectActorsUsingAsset(assetItem.ID));
                        cm.AddButton("Show asset references graph", () => Editor.Windows.Open(new AssetReferencesGraphWindow(Editor, assetItem)));
                    }

                    if (Editor.CanExport(item.Path))
                    {
                        b = cm.AddButton("Export", ExportSelection);
                    }
                }

                if (isFolder && folder.Node is MainContentTreeNode)
                {
                    cm.AddSeparator();
                }
                else
                {
                    cm.AddButton("Delete", () => Delete(item));
                    cm.AddSeparator();
                    cm.AddButton("Duplicate", _view.Duplicate);
                    cm.AddButton("Cut", _view.Cut);
                    cm.AddButton("Copy", _view.Copy);
                }

                b = cm.AddButton("Paste", _view.Paste);
                b.Enabled = _view.CanPaste();

                if (isFolder && folder.Node is MainContentTreeNode)
                {
                    // Do nothing
                }
                else
                {
                    cm.AddButton("Rename", () => Rename(item));
                }

                // Custom options
                ContextMenuShow?.Invoke(cm, item);
                proxy?.OnContentWindowContextMenu(cm, item);
                item.OnContextMenu(cm);

                cm.AddButton("Copy name to Clipboard", () => Clipboard.Text = item.NamePath);

                cm.AddButton("Copy path to Clipboard", () => Clipboard.Text = item.Path);
            }
            else
            {
                cm.AddButton(Utilities.Constants.ShowInExplorer, () => FileSystem.ShowFileExplorer(CurrentViewFolder.Path));

                b = cm.AddButton("Paste", _view.Paste);
                b.Enabled = _view.CanPaste();

                cm.AddButton("Refresh", () => Editor.ContentDatabase.RefreshFolder(CurrentViewFolder, true));

                cm.AddButton("Refresh all thumbnails", RefreshViewItemsThumbnails);
            }

            cm.AddSeparator();

            // Check if is source folder to add new module
            if (folder?.ParentFolder?.Node is ProjectTreeNode parentFolderNode && folder.Node == parentFolderNode.Source)
            {
                var button = cm.AddButton("New module");
                button.CloseMenuOnClick = false;
                button.Clicked += () => NewModule(button, parentFolderNode.Source.Path);
            }

            if (!isRootFolder && !(item is ContentFolder projectFolder && projectFolder.Node is ProjectTreeNode))
            {
                cm.AddButton("New folder", NewFolder);
            }

            // Loop through each proxy and user defined json type and add them to the context menu
            var actorType = new ScriptType(typeof(Actor));
            var scriptType = new ScriptType(typeof(Script));
            foreach (var type in Editor.CodeEditing.All.Get())
            {
                if (type.IsAbstract || type.Type == null)
                    continue;
                if (actorType.IsAssignableFrom(type) || scriptType.IsAssignableFrom(type))
                    continue;

                // Get attribute
                ContentContextMenuAttribute attribute = null;
                foreach (var typeAttribute in type.GetAttributes(false))
                {
                    if (typeAttribute is ContentContextMenuAttribute contentContextMenuAttribute)
                    {
                        attribute = contentContextMenuAttribute;
                        break;
                    }
                }
                if (attribute == null)
                    continue;

                // Get context proxy
                ContentProxy p = null;
                if (type.Type.IsSubclassOf(typeof(ContentProxy)))
                {
                    p = Editor.ContentDatabase.Proxy.Find(x => x.GetType() == type.Type);
                }
                else if (type.CanCreateInstance)
                {
                    // User can use attribute to put their own assets into the content context menu
                    var generic = typeof(SpawnableJsonAssetProxy<>).MakeGenericType(type.Type);
                    var instance = Activator.CreateInstance(generic);
                    p = instance as AssetProxy;
                }
                if (p == null)
                    continue;

                if (p.CanCreate(folder))
                {
                    var parts = attribute.Path.Split('/');
                    ContextMenuChildMenu childCM = null;
                    bool mainCM = true;
                    for (int i = 0; i < parts?.Length; i++)
                    {
                        var part = parts[i].Trim();
                        if (i == parts.Length - 1)
                        {
                            if (mainCM)
                            {
                                cm.AddButton(part, () => NewItem(p));
                                mainCM = false;
                            }
                            else if (childCM != null)
                            {
                                childCM.ContextMenu.AddButton(part, () => NewItem(p));
                                childCM.ContextMenu.AutoSort = true;
                            }
                        }
                        else
                        {
                            if (mainCM)
                            {
                                childCM = cm.GetOrAddChildMenu(part);
                                childCM.ContextMenu.AutoSort = true;
                                mainCM = false;
                            }
                            else if (childCM != null)
                            {
                                childCM = childCM.ContextMenu.GetOrAddChildMenu(part);
                                childCM.ContextMenu.AutoSort = true;
                            }
                        }
                    }
                }
            }

            if (folder.CanHaveAssets)
            {
                cm.AddButton("Import file", () =>
                {
                    _view.ClearSelection();
                    Editor.ContentImporting.ShowImportFileDialog(CurrentViewFolder);
                });
            }

            // Remove any leftover separator
            if (cm.ItemsContainer.Children.LastOrDefault() is ContextMenuSeparator)
                cm.ItemsContainer.Children.Last().Dispose();

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
                else if (selection[i] is PrefabItem prefabItem)
                {
                    var prefab = FlaxEngine.Content.Load<Prefab>(prefabItem.ID);
                    var modelPrefab = prefab.GetDefaultInstance().GetScript<ModelPrefab>();
                    if (!modelPrefab)
                        continue;
                    var importPath = modelPrefab.ImportPath;
                    var editor = Editor.Instance;
                    if (editor.ContentImporting.GetReimportPath("Model Prefab", ref importPath))
                        continue;
                    var folder = editor.ContentDatabase.Find(Path.GetDirectoryName(prefab.Path)) as ContentFolder;
                    if (folder == null)
                        continue;
                    var importOptions = modelPrefab.ImportOptions;
                    importOptions.Type = FlaxEngine.Tools.ModelTool.ModelType.Prefab;
                    editor.ContentImporting.Import(importPath, folder, true, importOptions);
                }
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

        private void NewModule(ContextMenuButton button, string path)
        {
            var popup = new ContextMenuBase
            {
                Size = new Float2(230, 125),
                ClipChildren = false,
                CullChildren = false,
            };
            popup.Show(button, new Float2(button.Width, 0));

            var nameLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Name",
                HorizontalAlignment = TextAlignment.Near,
            };
            nameLabel.LocalX += 10;
            nameLabel.LocalY += 10;

            var nameTextBox = new TextBox
            {
                Parent = popup,
                WatermarkText = "Module Name",
                AnchorPreset = AnchorPresets.TopLeft,
                IsMultiline = false,
            };
            nameTextBox.LocalX += 100;
            nameTextBox.LocalY += 10;
            var defaultTextBoxBorderColor = nameTextBox.BorderColor;
            var defaultTextBoxBorderSelectedColor = nameTextBox.BorderSelectedColor;
            nameTextBox.TextChanged += () =>
            {
                if (string.IsNullOrEmpty(nameTextBox.Text))
                {
                    nameTextBox.BorderColor = defaultTextBoxBorderColor;
                    nameTextBox.BorderSelectedColor = defaultTextBoxBorderSelectedColor;
                    return;
                }

                var pluginPath = Path.Combine(Globals.ProjectFolder, "Source", nameTextBox.Text);
                if (!IsValidModuleName(nameTextBox.Text) || Directory.Exists(pluginPath))
                {
                    nameTextBox.BorderColor = Color.Red;
                    nameTextBox.BorderSelectedColor = Color.Red;
                }
                else
                {
                    nameTextBox.BorderColor = defaultTextBoxBorderColor;
                    nameTextBox.BorderSelectedColor = defaultTextBoxBorderSelectedColor;
                }
            };

            var editorLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Editor",
                HorizontalAlignment = TextAlignment.Near,
            };
            editorLabel.LocalX += 10;
            editorLabel.LocalY += 35;

            var editorCheckBox = new CheckBox
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
            };
            editorCheckBox.LocalY += 35;
            editorCheckBox.LocalX += 100;

            var cppLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "C++",
                HorizontalAlignment = TextAlignment.Near,
            };
            cppLabel.LocalX += 10;
            cppLabel.LocalY += 60;

            var cppCheckBox = new CheckBox
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
            };
            cppCheckBox.LocalY += 60;
            cppCheckBox.LocalX += 100;

            var submitButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Create",
                Width = 70,
            };
            submitButton.LocalX += 40;
            submitButton.LocalY += 90;
            submitButton.Clicked += () =>
            {
                // TODO: Check all modules in project including plugins
                if (!IsValidModuleName(nameTextBox.Text))
                {
                    Editor.LogWarning("Invalid module name. Module names cannot contain spaces, start with a number or contain non-alphanumeric characters.");
                    return;
                }
                
                if (Directory.Exists(Path.Combine(Globals.ProjectFolder, "Source", nameTextBox.Text)))
                {
                    Editor.LogWarning("Cannot create module due to name conflict.");
                    return;
                }
                Editor.CodeEditing.CreateModule(path, nameTextBox.Text, editorCheckBox.Checked, cppCheckBox.Checked);
                nameTextBox.Clear();
                editorCheckBox.Checked = false;
                cppCheckBox.Checked = false;
                popup.Hide();
                button.ParentContextMenu.Hide();
            };

            var cancelButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Cancel",
                Width = 70,
            };
            cancelButton.LocalX += 120;
            cancelButton.LocalY += 90;
            cancelButton.Clicked += () =>
            {
                nameTextBox.Clear();
                editorCheckBox.Checked = false;
                cppCheckBox.Checked = false;
                popup.Hide();
                button.ParentContextMenu.Hide();
            };
        }

        private static bool IsValidModuleName(string text)
        {
            if (text.Contains(' '))
                return false;
            if (char.IsDigit(text[0]))
                return false;
            if (text.Any(c => !char.IsLetterOrDigit(c) && c != '_'))
                return false;
            return true;
        }
    }
}
