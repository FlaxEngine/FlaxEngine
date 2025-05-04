// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.Dialogs;
using FlaxEditor.GUI.Tree;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Dialog used to edit new file settings.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Dialogs.Dialog" />
    public class CreateFilesDialog : Dialog
    {
        private CreateFileEntry _entry;
        private CustomEditorPresenter _settingsEditor;

        /// <summary>
        /// Initializes a new instance of the <see cref="CreateFilesDialog"/> class.
        /// </summary>
        /// <param name="entry">The entry to edit it's settings.</param>
        public CreateFilesDialog(CreateFileEntry entry)
        : base("Create file settings")
        {
            _entry = entry ?? throw new ArgumentNullException();

            const float TotalWidth = 520;
            const float EditorHeight = 250;
            Width = TotalWidth;

            // Header and help description
            var headerLabel = new Label
            {
                Text = "Asset Options",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, 40),
                Parent = this,
                Font = new FontReference(Style.Current.FontTitle)
            };
            var infoLabel = new Label
            {
                Text = "Specify options for creating new asset",
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
            var createButton = new Button
            {
                Text = "Create",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-ButtonsWidth - ButtonsMargin, ButtonsWidth, -ButtonsHeight - ButtonsMargin, ButtonsHeight),
                Parent = this,
                Enabled = entry.CanBeCreated,
            };
            createButton.Clicked += OnSubmit;
            var cancelButton = new Button
            {
                Text = "Cancel",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-ButtonsWidth - ButtonsMargin - ButtonsWidth - ButtonsMargin, ButtonsWidth, -ButtonsHeight - ButtonsMargin, ButtonsHeight),
                Parent = this,
            };
            cancelButton.Clicked += OnCancel;

            // Panel for settings editor
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(2, 2, infoLabel.Bottom + 2, EditorHeight),
                Parent = this,
            };

            // Settings editor
            _settingsEditor = new CustomEditorPresenter(null);
            _settingsEditor.Panel.Parent = panel;

            _dialogSize = new Float2(TotalWidth, panel.Bottom);

            _settingsEditor.Select(_entry.Settings);
            _settingsEditor.Modified += () => createButton.Enabled = _entry.CanBeCreated;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            Editor.Instance.ContentImporting.LetThemBeCreatedxD(_entry);

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
