// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Popup that shows the list of actors to pick. Supports searching and basic type filtering.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu" />
    public class ActorSearchPopup : ItemsListContextMenu
    {
        /// <summary>
        /// The actor item.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu.Item" />
        public class ActorItemView : Item
        {
            private Actor _actor;

            /// <summary>
            /// Gets the actor.
            /// </summary>
            public Actor Actor => _actor;

            /// <summary>
            /// Initializes a new instance of the <see cref="ActorItemView"/> class.
            /// </summary>
            /// <param name="actor">The actor.</param>
            public ActorItemView(Actor actor)
            {
                _actor = actor;
                Name = actor.Name;
                TooltipText = Utilities.Utils.GetTooltip(actor);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _actor = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Validates if the given actor item can be used to pick it.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>True if is valid.</returns>
        public delegate bool IsValidDelegate(Actor actor);

        private IsValidDelegate _isValid;
        private Action<Actor> _selected;

        private ActorSearchPopup(IsValidDelegate isValid, Action<Actor> selected, CustomEditors.IPresenterOwner context)
        {
            _isValid = isValid;
            _selected = selected;

            ItemClicked += OnItemClicked;

            if (context is PropertiesWindow propertiesWindow || context == null)
            {
                // TODO: use async thread to search scenes
                for (int i = 0; i < Level.ScenesCount; i++)
                {
                    Find(Level.GetScene(i));
                }
            }
            else if (context is PrefabWindow prefabWindow)
            {
                Find(prefabWindow.Graph.MainActor);
            }
            
            SortItems();
        }

        private void OnItemClicked(Item item)
        {
            _selected(((ActorItemView)item).Actor);
        }

        private void Find(Actor actor)
        {
            if (!actor)
                return;

            if (_isValid(actor))
            {
                AddItem(new ActorItemView(actor));
            }

            for (int i = 0; i < actor.ChildrenCount; i++)
            {
                Find(actor.GetChild(i));
            }
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        /// <param name="showTarget">The show target.</param>
        /// <param name="showTargetLocation">The show target location.</param>
        /// <param name="isValid">Event called to check if a given actor item is valid to be used.</param>
        /// <param name="selected">Event called on actor item pick.</param>
        /// <param name="context">The presenter owner context (i.e. PrefabWindow, PropertiesWindow).</param>
        /// <returns>The dialog.</returns>
        public static ActorSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid, Action<Actor> selected, CustomEditors.IPresenterOwner context)
        {
            var popup = new ActorSearchPopup(isValid, selected, context);
            popup.Show(showTarget, showTargetLocation);
            return popup;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _isValid = null;
            _selected = null;

            base.OnDestroy();
        }
    }
}
