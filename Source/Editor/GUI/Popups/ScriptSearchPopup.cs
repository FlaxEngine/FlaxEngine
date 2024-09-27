// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Popup that shows the list of scripts to pick. Supports searching and basic type filtering.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu" />
    public class ScriptSearchPopup : ItemsListContextMenu
    {
        /// <summary>
        /// The script item.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu.Item" />
        public class ScriptItemView : Item
        {
            private Script _script;

            /// <summary>
            /// Gets the script.
            /// </summary>
            public Script Script => _script;

            /// <summary>
            /// Initializes a new instance of the <see cref="ScriptItemView"/> class.
            /// </summary>
            /// <param name="script">The script.</param>
            public ScriptItemView(Script script)
            {
                _script = script;

                var type = TypeUtils.GetObjectType(script);
                Name = script.Actor ? $"{type.Name} ({script.Actor.Name})" : type.Name;

                var str = type.Name;
                var o = script.Parent;
                while (o)
                {
                    str = o.Name + " -> " + str;
                    o = o.Parent;
                }
                TooltipText = str;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _script = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Validates if the given script item can be used to pick it.
        /// </summary>
        /// <param name="script">The script.</param>
        /// <returns>True if is valid.</returns>
        public delegate bool IsValidDelegate(Script script);

        private IsValidDelegate _isValid;
        private Action<Script> _selected;

        private ScriptSearchPopup(IsValidDelegate isValid, Action<Script> selected, CustomEditors.IPresenterOwner context)
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
            _selected(((ScriptItemView)item).Script);
        }

        private void Find(Actor actor)
        {
            if (!actor)
                return;

            for (int i = 0; i < actor.ScriptsCount; i++)
            {
                var script = actor.GetScript(i);
                if (_isValid(script))
                {
                    AddItem(new ScriptItemView(script));
                }
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
        /// <param name="isValid">Event called to check if a given script item is valid to be used.</param>
        /// <param name="selected">Event called on script item pick.</param>
        /// <param name="context">The presenter owner context (i.e. PrefabWindow, PropertiesWindow).</param>
        /// <returns>The dialog.</returns>
        public static ScriptSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid, Action<Script> selected, CustomEditors.IPresenterOwner context)
        {
            var popup = new ScriptSearchPopup(isValid, selected, context);
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
