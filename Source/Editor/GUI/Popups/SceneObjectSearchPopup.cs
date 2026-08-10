// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Popup that shows the list of scene objects to pick. Supports searching and basic type filtering.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu" />
    public class SceneObjectSearchPopup : ItemsListContextMenu
    {
        /// <summary>
        /// The scene object item.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu.Item" />
        public class SceneObjectItemView : Item
        {
            private SceneObject _object;

            /// <summary>
            /// Gets the scene object.
            /// </summary>
            public SceneObject Object => _object;

            /// <summary>
            /// Initializes a new instance of the <see cref="SceneObjectItemView"/> class.
            /// </summary>
            /// <param name="obj">The object.</param>
            public SceneObjectItemView(SceneObject obj)
            {
                _object = obj;
                Category = obj is Actor ? "Actors" : "Scripts";
                if (obj is Script script)
                {
                    var type = TypeUtils.GetObjectType(script);
                    Name = script.Actor ? $"{type.Name} ({script.Actor.Name})" : type.Name;
                }
                else if (obj is Actor actor)
                {
                    Name = actor.Name;
                }
                else
                {
                    Name = obj.ToString();
                }
                TooltipText = Utilities.Utils.GetTooltip(obj);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _object = null;
                base.OnDestroy();
            }
        }

        /// <summary>
        /// Validates if the given scene object item can be used to pick it.
        /// </summary>
        /// <param name="obj">The scene object.</param>
        /// <returns>True if is valid.</returns>
        public delegate bool IsValidDelegate(SceneObject obj);

        private IsValidDelegate _isValid;
        private Action<SceneObject> _selected;

        private SceneObjectSearchPopup(IsValidDelegate isValid, Action<SceneObject> selected, CustomEditors.IPresenterOwner context)
        {
            _isValid = isValid;
            _selected = selected;

            ItemClicked += OnItemClicked;

            if (context is PropertiesWindow || context == null)
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
            _selected(((SceneObjectItemView)item).Object);
        }

        private void Find(Actor actor)
        {
            if (!actor)
                return;

            if (_isValid(actor))
                AddItem(new SceneObjectItemView(actor));

            for (int i = 0; i < actor.ScriptsCount; i++)
            {
                var script = actor.GetScript(i);
                if (_isValid(script))
                    AddItem(new SceneObjectItemView(script));
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
        /// <param name="isValid">Event called to check if a given scene object item is valid to be used.</param>
        /// <param name="selected">Event called on scene object item pick.</param>
        /// <param name="context">The presenter owner context (i.e. PrefabWindow, PropertiesWindow).</param>
        /// <returns>The dialog.</returns>
        public static SceneObjectSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid, Action<SceneObject> selected, CustomEditors.IPresenterOwner context)
        {
            var popup = new SceneObjectSearchPopup(isValid, selected, context);
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
