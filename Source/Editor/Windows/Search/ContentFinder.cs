// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.Modules;
using FlaxEngine;

namespace FlaxEditor.Windows.Search
{
    /// <summary>
    /// The content finder popup. Allows to search for project items and quickly navigate to.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ContextMenu.ContextMenuBase" />
    [HideInEditor]
    internal class ContentFinder : SearchableEditorPopup
    {
        public ContentFinder(float width = 440.0f, int visibleItemsCount = 12, string searchBarPrompt = "Type to search content...") : base(Editor.Instance.Icons.Folder32, width, visibleItemsCount, searchBarPrompt) 
        {
        }

        public override PopupItemBase CreateSearchItemFromResult(SearchResult searchResult, float width, float height, int index)
        {
            PopupItemBase searchItem;
            if (searchResult.Item is ContentItem contentItem)
                searchItem = new ContentFinderPopupItem(searchResult.Name, searchResult.Type, contentItem, this, width, height);
            else
                searchItem = new PopupItemBase(searchResult.Name, searchResult.Item, this, width, height);
            searchItem.Y = index * height;

            return searchItem;
        }

        public override List<SearchResult> Search(string prompt)
        {
            var results = Editor.Instance.ContentFinding.Search(prompt);

            return results;
        }

        public override void SelectItem()
        {
            if (SelectedItem as ContentFinderPopupItem != null)
                (SelectedItem as ContentFinderPopupItem).Select();
        }
    }
}
