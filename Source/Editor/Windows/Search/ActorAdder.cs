using FlaxEditor.Modules;
using FlaxEditor.Scripting;
using FlaxEditor.Utilities;
using FlaxEngine;
using System;
using System.Collections.Generic;

namespace FlaxEditor.Windows.Search
{
    // TODO: Look at how `ContentFinder` does its wrapping of search results. There's always one result selected.
    internal class ActorAdder : SearchableEditorPopup
    {
        // TODO: Is there a besser way of passing in the base classes intizializer arguments while still being able to change `searchBarPrompt`
        public ActorAdder(float width = 440.0f, int visibleItemsCount = 12, string searchBarPrompt = "Type to search actor types...") : base(Editor.Instance.Icons.Add32, width, visibleItemsCount, searchBarPrompt)
        {
        }

        public override List<SearchResult> Search(string prompt)
        {
            List<SearchResult> results = new List<SearchResult>();

            foreach (ScriptType s in Editor.Instance.CodeEditing.Actors.Get())
            {
                ActorToolboxAttribute attribute = null;
                foreach (var e in s.GetAttributes(false))
                {
                    if (e is ActorToolboxAttribute actorToolboxAttribute)
                    {
                        attribute = actorToolboxAttribute;
                        break;
                    }
                }

                var text = (attribute == null) ? s.Name : string.IsNullOrEmpty(attribute.Name) ? s.Name : attribute.Name;

                // Display all actors on no search prompt
                if (!string.IsNullOrEmpty(prompt))
                {
                    if (!QueryFilterHelper.Match(prompt, text, out QueryFilterHelper.Range[] ranges))
                        continue;
                }

                results.Add(new SearchResult
                {
                    Name = s.Name,
                    Type = s.TypeName,
                    Item = null,
                });
            }

            return results;
        }

        public override ActorAdderPopupItem CreateSearchItemFromResult(SearchResult searchResult, float width, float height, int index)
        {
            ActorAdderPopupItem item;
            item = new ActorAdderPopupItem(searchResult.Name, searchResult.Item, Type.GetType(searchResult.Type), this, width, height);

            item.Y = index * height;

            return item;
        }

        public override void SelectItem()
        {
            if (SelectedItem as  ActorAdderPopupItem != null)
                (SelectedItem as ActorAdderPopupItem).Select();
        }
    }
}
