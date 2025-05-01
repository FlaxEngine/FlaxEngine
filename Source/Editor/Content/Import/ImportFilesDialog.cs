// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Dialogs;
using FlaxEditor.GUI.Tree;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Dialog used to edit import files settings.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Dialogs.Dialog" />
    public class ImportFilesDialog : Dialog
    {
        private TreeNode _rootNode;
        private CustomEditorPresenter _settingsEditor;

        /// <summary>
        /// Gets the entries count.
        /// </summary>
        public int EntriesCount
        {
            get
            {
                var result = 0;
                for (int i = 0; i < _rootNode.ChildrenCount; i++)
                {
                    if (_rootNode.Children[i].Tag is ImportFileEntry)
                        result++;
                }
                return result;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ImportFilesDialog"/> class.
        /// </summary>
        /// <param name="entries">The entries to edit settings.</param>
        public ImportFilesDialog(List<ImportFileEntry> entries)
        : base("Import files settings")
        {
            if (entries == null || entries.Count < 1)
                throw new ArgumentNullException();

            const float TotalWidth = 920;
            const float EditorHeight = 450;
            Width = TotalWidth;

            // Header and help description
            var headerLabel = new Label
            {
                Text = "Import settings",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, 40),
                Parent = this,
                Font = new FontReference(Style.Current.FontTitle)
            };
            var infoLabel = new Label
            {
                Text = "Specify options for importing files. Every file can have different settings. Select entries on the left panel to modify them.\nPro Tip: hold CTRL key and select entries to edit multiple at once or use CTRL+A to select them all.",
                HorizontalAlignment = TextAlignment.Near,
                Margin = new Margin(7),
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(10, -20, 45, 70),
                Parent = this
            };

            // Buttons
            const float ButtonsWidth = 60;
            const float ButtonsHeight = 24;
            const float ButtonsMargin = 8;
            var importButton = new Button
            {
                Text = "Import",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-ButtonsWidth - ButtonsMargin, ButtonsWidth, -ButtonsHeight - ButtonsMargin, ButtonsHeight),
                Parent = this
            };
            importButton.Clicked += OnSubmit;
            var cancelButton = new Button
            {
                Text = "Cancel",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-ButtonsWidth - ButtonsMargin - ButtonsWidth - ButtonsMargin, ButtonsWidth, -ButtonsHeight - ButtonsMargin, ButtonsHeight),
                Parent = this
            };
            cancelButton.Clicked += OnCancel;

            // Split panel for entries list and settings editor
            var splitPanel = new SplitPanel(Orientation.Horizontal, ScrollBars.Both, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(2, 2, infoLabel.Bottom + 2, ButtonsHeight + ButtonsMargin + ButtonsMargin),
                Parent = this
            };

            // Settings editor
            _settingsEditor = new CustomEditorPresenter(null);
            _settingsEditor.Panel.Parent = splitPanel.Panel2;

            // Setup tree
            var tree = new Tree(true)
            {
                Parent = splitPanel.Panel1
            };
            tree.RightClick += OnTreeRightClick;
            _rootNode = new TreeNode(false);
            for (int i = 0; i < entries.Count; i++)
            {
                var entry = entries[i];

                // TODO: add icons for textures/models/etc from FileEntry to tree node??
                var node = new ItemNode(entry)
                {
                    Parent = _rootNode
                };
            }
            _rootNode.Expand();
            _rootNode.ChildrenIndent = 0;
            _rootNode.Parent = tree;
            tree.Margin = new Margin(0.0f, 0.0f, -14.0f, 2.0f); // Hide root node
            tree.SelectedChanged += OnSelectedChanged;

            // Select the first item
            tree.Select(_rootNode.Children[0] as TreeNode);

            _dialogSize = new Float2(TotalWidth, EditorHeight + splitPanel.Offsets.Height);
        }

        private void OnTreeRightClick(TreeNode node, Float2 location)
        {
            var menu = new ContextMenu();
            menu.AddButton("Rename", OnRenameClicked);
            menu.AddButton("Don't import", OnDontImportClicked);
            menu.AddButton(Utilities.Constants.ShowInExplorer, OnShowInExplorerClicked);
            menu.Tag = node;
            menu.Show(node, location);
        }

        private void OnRenameClicked(ContextMenuButton button)
        {
            var node = (ItemNode)button.ParentContextMenu.Tag;
            node.StartRenaming();
        }

        private void OnDontImportClicked(ContextMenuButton button)
        {
            if (EntriesCount == 1)
            {
                OnCancel();
                return;
            }

            var node = (ItemNode)button.ParentContextMenu.Tag;
            if (_settingsEditor.Selection.Count == 1 && _settingsEditor.Selection[0] == node.Entry.Settings)
                _settingsEditor.Deselect();
            node.Dispose();
        }

        private void OnShowInExplorerClicked(ContextMenuButton button)
        {
            var node = (ItemNode)button.ParentContextMenu.Tag;
            FileSystem.ShowFileExplorer(Path.GetDirectoryName(node.Entry.SourceUrl));
        }

        private class ItemNode : TreeNode
        {
            public ImportFileEntry Entry => (ImportFileEntry)Tag;

            public ItemNode(ImportFileEntry entry)
            : base(false)
            {
                Text = Path.GetFileName(entry.SourceUrl);
                Tag = entry;
                LinkTooltip(entry.SourceUrl);
            }

            /// <inheritdoc />
            protected override bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
            {
                StartRenaming();
                return true;
            }

            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (base.OnKeyDown(key))
                    return true;
                switch (key)
                {
                case KeyboardKeys.F2:
                    StartRenaming();
                    return true;
                }
                return false;
            }

            /// <summary>
            /// Shows the rename popup for the item.
            /// </summary>
            public void StartRenaming()
            {
                // Start renaming the folder
                var entry = (ImportFileEntry)Tag;
                var shortName = Path.GetFileNameWithoutExtension(entry.ResultUrl);
                var dialog = RenamePopup.Show(this, HeaderRect, shortName, false);
                dialog.Tag = Tag;
                dialog.Renamed += OnRenamed;
            }

            private void OnRenamed(RenamePopup popup)
            {
                var entry = (ImportFileEntry)Tag;
                entry.ModifyResultFilename(popup.Text);
                Text = string.Format("{0} ({1})", Path.GetFileName(entry.SourceUrl), popup.Text);
            }
        }

        private void OnSelectedChanged(List<TreeNode> before, List<TreeNode> after)
        {
            var selection = new List<object>(after.Count);
            for (int i = 0; i < after.Count; i++)
            {
                if (after[i].Tag is ImportFileEntry fileEntry && fileEntry.HasSettings)
                    selection.Add(fileEntry.Settings);
            }

            _settingsEditor.Select(selection);
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            var entries = new List<ImportFileEntry>(_rootNode.ChildrenCount);
            for (int i = 0; i < _rootNode.ChildrenCount; i++)
            {
                if (_rootNode.Children[i].Tag is ImportFileEntry fileEntry)
                    entries.Add(fileEntry);
            }
            Editor.Instance.ContentImporting.LetThemBeImportedxD(entries);

            base.OnSubmit();
        }

        /// <inheritdoc />
        protected override void SetupWindowSettings(ref CreateWindowSettings settings)
        {
            base.SetupWindowSettings(ref settings);

            settings.MinimumSize = new Float2(300, 400);
            settings.HasSizingFrame = true;
        }
    }
}
