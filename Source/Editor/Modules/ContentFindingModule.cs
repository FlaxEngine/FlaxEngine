// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using FlaxEditor.Content;
using FlaxEditor.SceneGraph;
using FlaxEditor.Surface.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Modules
{
    internal struct QuickAction
    {
        public string Name;
        public Action Action;
    }

    /// <summary>
    /// The search result.
    /// </summary>
    [HideInEditor]
    public struct SearchResult
    {
        /// <summary>
        /// The name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The type name.
        /// </summary>
        public string Type;

        /// <summary>
        /// The item.
        /// </summary>
        public object Item;
    }

    /// <summary>
    /// The content finding module.
    /// </summary>
    public class ContentFindingModule : EditorModule
    {
        private List<QuickAction> _quickActions = new List<QuickAction>();

        /// <summary>
        /// The content finding context menu.
        /// </summary>
        public ContentFinder Finder { get; private set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentFindingModule"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        internal ContentFindingModule(Editor editor)
        : base(editor)
        {
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            base.OnInit();

            Finder = new ContentFinder();
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            _quickActions.Clear();
            _quickActions = null;

            Finder?.Dispose();
            Finder = null;

            base.OnExit();
        }

        /// <summary>
        /// Shows the finder.
        /// </summary>
        /// <param name="control">The target control to show finder over it.</param>
        public void ShowFinder(Control control)
        {
            var position = (control.Size - new Vector2(Finder.Width, 300.0f)) * 0.5f;
            Finder.Show(control, position);
        }

        /// <summary>
        /// Adds <paramref name="action"/> to quick action list.
        /// </summary>
        /// <param name="name">The action's name.</param>
        /// <param name="action">The actual action callback.</param>
        public void AddQuickAction(string name, Action action)
        {
            _quickActions.Add(new QuickAction
            {
                Name = name,
                Action = action,
            });
        }

        /// <summary>
        /// Removes a quick action by name.
        /// </summary>
        /// <param name="name">The action's name.</param>
        /// <returns>True when it succeed, false if there is no Quick Action with this name.</returns>
        public bool RemoveQuickAction(string name)
        {
            foreach (var action in _quickActions)
            {
                if (action.Name.Equals(name))
                {
                    _quickActions.Remove(action);
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Searches any assets/scripts/quick actions that match the provided type and name.
        /// </summary>
        /// <param name="charsToFind">Two pattern can be used, the first one will just take a string without ':' and will only match names.
        /// The second looks like this "name:type", it will match name and type. Experimental : You can use regular expressions, might break if you are using ':' character.</param>
        /// <returns>The results list.</returns>
        public List<SearchResult> Search(string charsToFind)
        {
            // Special case if searching by object id
            if (charsToFind.Length == 32)
            {
                FlaxEngine.Json.JsonSerializer.ParseID(charsToFind, out var id);
                var item = Editor.Instance.ContentDatabase.Find(id);
                if (item is AssetItem assetItem)
                {
                    return new List<SearchResult>
                    {
                        new SearchResult { Name = item.ShortName, Type = assetItem.TypeName, Item = item }
                    };
                }
                var actor = FlaxEngine.Object.Find<Actor>(ref id);
                if (actor != null)
                {
                    return new List<SearchResult>
                    {
                        new SearchResult { Name = actor.Name, Type = actor.TypeName, Item = actor }
                    };
                }
            }

            string type = ".*";
            string name = charsToFind.Trim();

            if (charsToFind.Contains(':'))
            {
                var args = charsToFind.Split(':');
                type = ".*" + args[1].Trim() + ".*";
                name = ".*" + args[0].Trim() + ".*";
            }

            if (name.Equals(string.Empty))
                name = ".*";

            var typeRegex = new Regex(type, RegexOptions.IgnoreCase);
            var nameRegex = new Regex(name, RegexOptions.IgnoreCase);
            var matches = new List<SearchResult>();

            foreach (var project in Editor.Instance.ContentDatabase.Projects)
            {
                ProcessItems(nameRegex, typeRegex, project.Folder.Children, matches);
            }
            //ProcessSceneNodes(nameRegex, typeRegex, Editor.Instance.Scene.Root, matches);
            ProcessActors(nameRegex, typeRegex, Editor.Instance.Scene.Root, matches);

            _quickActions.ForEach(action =>
            {
                if (nameRegex.Match(action.Name).Success && typeRegex.Match("Quick Action").Success)
                    matches.Add(new SearchResult { Name = action.Name, Type = "Quick Action", Item = action });
            });

            return matches;
        }

        private void ProcessSceneNodes(Regex nameRegex, Regex typeRegex, SceneGraphNode root, List<SearchResult> matches)
        {
            root.ChildNodes.ForEach(node =>
            {
                var type = node.GetType();
                if (typeRegex.Match(type.Name).Success && nameRegex.Match(node.Name).Success)
                {
                    string finalName = type.Name;
                    if (Aliases.TryGetValue(finalName, out var alias))
                    {
                        finalName = alias;
                    }
                    matches.Add(new SearchResult { Name = node.Name, Type = finalName, Item = node });
                }
                if (node.ChildNodes.Count != 0)
                {
                    node.ChildNodes.ForEach(n => { ProcessSceneNodes(nameRegex, typeRegex, n, matches); });
                }
            });
        }

        private void ProcessActors(Regex nameRegex, Regex typeRegex, ActorNode node, List<SearchResult> matches)
        {
            if (node.Actor != null)
            {
                var type = node.Actor.GetType();
                if (typeRegex.Match(type.Name).Success && nameRegex.Match(node.Name).Success)
                {
                    string finalName = type.Name;
                    if (Aliases.TryGetValue(finalName, out var alias))
                    {
                        finalName = alias;
                    }
                    matches.Add(new SearchResult { Name = node.Name, Type = finalName, Item = node });
                }
            }

            for (var i = 0; i < node.ChildNodes.Count; i++)
            {
                if (node.ChildNodes[i] is ActorNode child)
                {
                    ProcessActors(nameRegex, typeRegex, child, matches);
                }
            }
        }

        private void ProcessItems(Regex nameRegex, Regex typeRegex, List<ContentItem> items, List<SearchResult> matches)
        {
            foreach (var contentItem in items)
            {
                if (contentItem.IsAsset)
                {
                    if (nameRegex.Match(contentItem.ShortName).Success)
                    {
                        var asset = contentItem as AssetItem;
                        if (asset == null || !typeRegex.Match(asset.TypeName).Success)
                            continue;

                        if (Aliases.TryGetValue(asset.TypeName, out var finalName))
                        {
                        }
                        else
                        {
                            var splits = asset.TypeName.Split('.');
                            finalName = splits[splits.Length - 1];
                        }
                        matches.Add(new SearchResult { Name = asset.ShortName, Type = finalName, Item = asset });
                    }
                }
                else if (contentItem.IsFolder)
                {
                    ProcessItems(nameRegex, typeRegex, ((ContentFolder)contentItem).Children, matches);
                }
                else if (contentItem.GetType().Name.Equals("FileItem"))
                {
                }
                else
                {
                    if (nameRegex.Match(contentItem.ShortName).Success && typeRegex.Match(contentItem.GetType().Name).Success)
                    {
                        string finalName = contentItem.GetType().Name.Replace("Item", "");

                        matches.Add(new SearchResult { Name = contentItem.ShortName, Type = finalName, Item = contentItem });
                    }
                }
            }
        }

        internal void Open(object o)
        {
            switch (o)
            {
            case ContentItem contentItem:
                Editor.Instance.ContentEditing.Open(contentItem);
                break;
            case QuickAction quickAction:
                quickAction.Action();
                break;
            case ActorNode actorNode:
                Editor.Instance.SceneEditing.Select(actorNode.Actor);
                Editor.Instance.Windows.EditWin.Viewport.FocusSelection();
                break;
            case Actor actor:
                Editor.Instance.SceneEditing.Select(actor);
                Editor.Instance.Windows.EditWin.Viewport.FocusSelection();
                break;
            }
        }

        /// <summary>
        /// The aliases to match the given type to its name.
        /// </summary>
        public static readonly Dictionary<string, string> Aliases = new Dictionary<string, string>
        {
            // Assets
            { "FlaxEditor.Content.Settings.AudioSettings", "Settings" },
            { "FlaxEditor.Content.Settings.BuildSettings", "Settings" },
            { "FlaxEditor.Content.Settings.GameSettings", "Settings" },
            { "FlaxEditor.Content.Settings.GraphicsSettings", "Settings" },
            { "FlaxEditor.Content.Settings.InputSettings", "Settings" },
            { "FlaxEditor.Content.Settings.LayersAndTagsSettings", "Settings" },
            { "FlaxEditor.Content.Settings.NavigationSettings", "Settings" },
            { "FlaxEditor.Content.Settings.LocalizationSettings", "Settings" },
            { "FlaxEditor.Content.Settings.PhysicsSettings", "Settings" },
            { "FlaxEditor.Content.Settings.TimeSettings", "Settings" },
            { "FlaxEditor.Content.Settings.UWPPlatformSettings", "Settings" },
            { "FlaxEditor.Content.Settings.WindowsPlatformSettings", "Settings" },
        };
    }
}
