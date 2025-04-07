// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.GUI.ContextMenu
{
    /// <summary>
    /// Context menu for a single selectable option from range of values (eg. enum).
    /// </summary>
    [HideInEditor]
    class ContextMenuSingleSelectGroup<T>
    {
        private struct SingleSelectGroupItem
        {
            public string Text;
            public string Tooltip;
            public T Value;
            public Action Selected;
            public List<ContextMenuButton> Buttons;
        }

        private List<ContextMenu> _menus = new List<ContextMenu>();
        private List<SingleSelectGroupItem> _items = new List<SingleSelectGroupItem>();
        private bool _hasSelected = false;
        private SingleSelectGroupItem _selectedItem;

        public T Selected
        {
            get => _selectedItem.Value;
            set
            {
                var index = _items.FindIndex(x => x.Value.Equals(value));
                if (index != -1 && (!_hasSelected || !_selectedItem.Value.Equals(value)))
                {
                    SetSelected(_items[index]);
                }
            }
        }

        public Action<T> SelectedChanged;

        public ContextMenuSingleSelectGroup<T> AddItem(string text, T value, Action selected = null, string tooltip = null)
        {
            var item = new SingleSelectGroupItem
            {
                Text = text,
                Tooltip = tooltip,
                Value = value,
                Selected = selected,
                Buttons = new List<ContextMenuButton>()
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
                AddItemToContextMenu(contextMenu, _items[i]);
            return this;
        }

        private void AddItemToContextMenu(ContextMenu contextMenu, SingleSelectGroupItem item)
        {
            var btn = contextMenu.AddButton(item.Text, () => { SetSelected(item); });
            if (item.Tooltip != null)
                btn.TooltipText = item.Tooltip;
            item.Buttons.Add(btn);
            if (_hasSelected && item.Equals(_selectedItem))
                btn.Checked = true;
        }

        private void SetSelected(SingleSelectGroupItem item)
        {
            foreach (var e in _items)
            {
                foreach (var btn in e.Buttons)
                    btn.Checked = false;
            }
            _selectedItem = item;
            _hasSelected = true;

            SelectedChanged?.Invoke(item.Value);
            item.Selected?.Invoke();

            foreach (var btn in item.Buttons)
                btn.Checked = true;
        }
    }
}
