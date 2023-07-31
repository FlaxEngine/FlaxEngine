// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.ContextMenu
{
    /// <summary>
    /// Context menu single select group.
    /// </summary>
    [HideInEditor]
    class ContextMenuSingleSelectGroup<T>
    {
        public struct SingleSelectGroupItem<U>
        {
            public string text;
            public U value;
            public Action onSelected;
            public List<ContextMenuButton> buttons;
        }

        private List<ContextMenu> _menus = new List<ContextMenu>();
        private List<SingleSelectGroupItem<T>> _items = new List<SingleSelectGroupItem<T>>();
        public Action<T> OnSelectionChanged;

        public SingleSelectGroupItem<T> activeItem;

        public ContextMenuSingleSelectGroup<T> AddItem(string text, T value, Action onSelected = null)
        {
            var item = new SingleSelectGroupItem<T>
            {
                text = text,
                value = value,
                onSelected = onSelected,
                buttons = new List<ContextMenuButton>()
            };
            _items.Add(item);

            foreach (var contextMenu in _menus)
                AddItemToContextMenu(contextMenu, item);

            return this;
        }

        public ContextMenuSingleSelectGroup<T> AddItemsToContextMenu(ContextMenu contextMenu)
        {
            _menus.Add(contextMenu);

            for (int i = 0; i < _items.Count; i++)
            {
                AddItemToContextMenu(contextMenu, _items[i]);
            }

            return this;
        }

        private void AddItemToContextMenu(ContextMenu contextMenu, SingleSelectGroupItem<T> item)
        {
            var btn = contextMenu.AddButton(item.text, () =>
            {
                SetItemAsActive(item);
            });

            item.buttons.Add(btn);

            if (item.Equals(activeItem))
            {
                btn.Checked = true;
            }
        }

        private void DeselectAll()
        {
            foreach (var item in _items)
            {
                foreach (var btn in item.buttons) btn.Checked = false;
            }
        }

        public void SetItemAsActive(T value)
        {
            var index = _items.FindIndex(x => x.value.Equals(value));

            if (index == -1) return;

            SetItemAsActive(_items[index]);
        }

        private void SetItemAsActive(SingleSelectGroupItem<T> item)
        {
            DeselectAll();
            activeItem = item;

            var index = _items.IndexOf(item);
            OnSelectionChanged?.Invoke(item.value);
            item.onSelected?.Invoke();

            foreach (var btn in item.buttons)
            {
                btn.Checked = true;
            }
        }
    }
}
