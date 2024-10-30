// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEngine.GUI;
using System;
using System.Linq;

// TODO: Sort of each one of these classes into their own file or rename the file to something more generic

namespace FlaxEditor.Windows.Search
{
    /// <summary>
    /// The base class for all items displayed in classes inheriting <see cref="SearchableEditorPopup"/>.
    /// </summary>
    [HideInEditor]
    internal class PopupItemBase : ContainerControl
    {
        protected SearchableEditorPopup _finder;
        protected Image _icon;
        protected Label _nameLabel;

        /// <summary>
        /// The item name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The item object reference.
        /// </summary>
        public object Item;

        /// <summary>
        /// The context menu.
        /// </summary>
        public ContextMenu Cm;

        /// <summary>
        /// Initializes a new instance of the <see cref="PopupItemBase"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="item">The item.</param>
        /// <param name="finder">The finder.</param>
        /// <param name="width">The item width.</param>
        /// <param name="height">The item height.</param> 
        public PopupItemBase(string name, object item, SearchableEditorPopup finder, float width, float height)
        {
            Size = new Float2(width, height);
            Name = name;
            Item = item;
            //Parent = finder;
            _finder = finder;

            var logoSize = 15.0f;
            var icon = new Image
            {
                Size = new Float2(logoSize),
                Location = new Float2(5, (height - logoSize) / 2)
            };
            _icon = icon;

            _nameLabel = AddChild<Label>();
            _nameLabel.Height = 25;
            _nameLabel.Location = new Float2(icon.X + icon.Width + 5, (height - _nameLabel.Height) / 2);
            _nameLabel.Text = Name;
            _nameLabel.HorizontalAlignment = TextAlignment.Near;

            // TODO: Either remove this or figure out what it does (preferably both)

            //var typeLabel = AddChild<Label>();
            //typeLabel.Height = 25;
            //typeLabel.Location = new Float2((height - _nameLabel.Height) / 2, X + width - typeLabel.Width - 17);
            //typeLabel.HorizontalAlignment = TextAlignment.Far;
            ////typeLabel.Text = Type;
            //typeLabel.Text = "ASDASDASDASD";
            //typeLabel.TextColor = Style.Current.ForegroundGrey;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _finder.Hide();
                Select();
            }

            return base.OnMouseUp(location, button);
            //return true;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            var root = RootWindow;
            if (root != null)
            {
                root.Cursor = CursorType.Hand;
            }

            _finder.SelectedItem = this;
            _finder.Hand = true;
        }

        /// <summary>
        /// Select the current SearchItem.
        /// </summary>
        public virtual void Select()
        {
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Item = null;
            _finder = null;
            _icon = null;

            base.OnDestroy();
        }
    }
