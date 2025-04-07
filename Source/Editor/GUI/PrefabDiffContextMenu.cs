// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The custom context menu that shows a tree of prefab diff items.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    public class PrefabDiffContextMenu : ContextMenuBase
    {
        /// <summary>
        /// The tree control where you should add your nodes.
        /// </summary>
        public readonly Tree.Tree Tree;

        /// <summary>
        /// The event called to revert all the changes applied.
        /// </summary>
        public event Action RevertAll;

        /// <summary>
        /// The event called to apply all the changes.
        /// </summary>
        public event Action ApplyAll;

        /// <summary>
        /// Initializes a new instance of the <see cref="PrefabDiffContextMenu"/> class.
        /// </summary>
        /// <param name="width">The control width.</param>
        /// <param name="height">The control height.</param>
        public PrefabDiffContextMenu(float width = 280, float height = 260)
        {
            // Context menu dimensions
            Size = new Float2(width, height);

            // Buttons
            float buttonsWidth = (width - 6.0f) * 0.5f;
            float buttonsHeight = 20.0f;

            var revertAll = new Button(2.0f, 2.0f, buttonsWidth, buttonsHeight)
            {
                Text = "Revert All",
                Parent = this
            };
            revertAll.Clicked += OnRevertAllClicked;

            var applyAll = new Button(revertAll.Right + 2.0f, 2.0f, buttonsWidth, buttonsHeight)
            {
                Text = "Apply All",
                Parent = this
            };
            applyAll.Clicked += OnApplyAllClicked;

            // Actual panel
            var panel1 = new Panel(ScrollBars.Vertical)
            {
                Bounds = new Rectangle(0, applyAll.Bottom + 2.0f, Width, Height - applyAll.Bottom - 2.0f),
                Parent = this
            };

            Tree = new Tree.Tree
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Parent = panel1
            };
        }

        private void OnRevertAllClicked()
        {
            Hide();
            RevertAll?.Invoke();
        }

        private void OnApplyAllClicked()
        {
            Hide();
            ApplyAll?.Invoke();
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Prepare
            Focus();

            base.OnShow();
        }

        /// <inheritdoc />
        public override void Hide()
        {
            if (!Visible)
                return;

            Focus(null);

            base.Hide();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Escape)
            {
                Hide();
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            RevertAll = null;
            ApplyAll = null;

            base.OnDestroy();
        }
    }
}
