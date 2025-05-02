// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using FlaxEditor.Content;
using FlaxEditor.GUI.Docking;
using FlaxEditor.SceneGraph;
using FlaxEditor.Windows.Search;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Windows.Search;

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
        private List<QuickAction> _quickActions;
        private ContentFinder _finder;
        private ContentSearchWindow _searchWin;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentFindingModule"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        internal ContentFindingModule(Editor editor)
        : base(editor)
        {
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            if (_quickActions != null)
            {
                _quickActions.Clear();
                _quickActions = null;
            }

            if (_finder != null)
            {
                _finder.Dispose();
                _finder = null;
            }

            if (_searchWin != null)
            {
                _searchWin.Dispose();
                _searchWin = null;
            }

            base.OnExit();
        }

        /// <summary>
        /// Shows the content search window.
        /// </summary>
        public void ShowSearch()
        {
            // Try to find the currently focused editor window that might call this
            DockWindow window = null;
            foreach (var editorWindow in Editor.Windows.Windows)
            {
                if (editorWindow.Visible && editorWindow.ContainsFocus && editorWindow.Parent != null)
                {
                    window = editorWindow;
                    break;
                }
            }

            ShowSearch(window);
        }

        /// <summary>
        /// Shows the content search window.
        /// </summary>
        /// <param name="control">The target control to show search for it.</param>
        /// <param name="query">The initial query for the search.</param>
        public void ShowSearch(Control control, string query = null)
        {
            // Try to find the owning window
            DockWindow window = null;
            while (control != null && window == null)
            {
                window = control as DockWindow;
                control = control.Parent;
            }

            ShowSearch(window, query);
        }

        /// <summary>
        /// Shows the content search window.
        /// </summary>
        /// <param name="window">The target window to show search next to it.</param>
        /// <param name="query">The initial query for the search.</param>
        public void ShowSearch(DockWindow window, string query = null)
        {
            if (_searchWin == null)
                _searchWin = new ContentSearchWindow(Editor);
            _searchWin.TargetWindow = window;
            if (!_searchWin.IsHidden)
            {
                // Focus
                _searchWin.SelectTab();
                _searchWin.Focus();
            }
            else if (window != null)
            {
                // Show docked to the target window
                _searchWin.Show(DockState.DockBottom, window);
                _searchWin.SearchLocation = ContentSearchWindow.SearchLocations.CurrentAsset;
            }
            else
            {
                // Show floating
                _searchWin.ShowFloating();
                _searchWin.SearchLocation = ContentSearchWindow.SearchLocations.AllAssets;
            }
            if (window is ISearchWindow searchWindow)
            {
                _searchWin.SearchAssets = searchWindow.AssetType;
            }
            if (query != null)
            {
                _searchWin.Query = query;
                _searchWin.Search();
            }
        }

        /// <summary>
        /// Shows the content finder popup.
        /// </summary>
        /// <param name="control">The target control to show finder over it.</param>
        public void ShowFinder(Control control)
        {
            var finder = _finder ?? (_finder = new ContentFinder());
            if (control == null)
                control = Editor.Instance.Windows.MainWindow.GUI;
            var position = (control.Size - new Float2(finder.Width, 300.0f)) * 0.5f;
            finder.Show(control, position);
        }

        /// <summary>
        /// Adds <paramref name="action"/> to quick action list.
        /// </summary>
        /// <param name="name">The action's name.</param>
        /// <param name="action">The actual action callback.</param>
        public void AddQuickAction(string name, Action action)
        {
            if (_quickActions == null)
                _quickActions = new List<QuickAction>();
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
        /// <returns>True when it succeeds, false if there is no Quick Action with this name.</returns>
        public bool RemoveQuickAction(string name)
        {
            if (_quickActions == null)
                return false;
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
                var actor = FlaxEngine.Object.Find<Actor>(ref id, true);
                if (actor != null)
                {
                    return new List<SearchResult>
                    {
                        new SearchResult { Name = actor.Name, Type = actor.TypeName, Item = actor }
                    };
                }
                var script = FlaxEngine.Object.Find<Script>(ref id, true);
                if (script != null && script.Actor != null)
                {
                    string actorPathStart = $"{script.Actor.Name}/";

                    return new List<SearchResult>
                    {
                        new SearchResult { Name = $"{actorPathStart}{script.TypeName}", Type = script.TypeName, Item = script }
                    };
                }
            }
            Profiler.BeginEvent("ContentFinding.Search");

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
                Profiler.BeginEvent(project.Project.Name);
                ProcessItems(nameRegex, typeRegex, project.Folder.Children, matches);
                Profiler.EndEvent();
            }
            //ProcessSceneNodes(nameRegex, typeRegex, Editor.Instance.Scene.Root, matches);
            {
                Profiler.BeginEvent("Actors");
                ProcessActors(nameRegex, typeRegex, Editor.Instance.Scene.Root, matches);
                Profiler.EndEvent();
            }

            if (_quickActions != null)
            {
                Profiler.BeginEvent("QuickActions");
                foreach (var action in _quickActions)
                {
                    if (nameRegex.Match(action.Name).Success && typeRegex.Match("Quick Action").Success)
                        matches.Add(new SearchResult { Name = action.Name, Type = "Quick Action", Item = action });
                }
                Profiler.EndEvent();
            }

            // Editor window
            foreach (var window in Editor.Windows.Windows)
            {
                if (window is Windows.Assets.AssetEditorWindow)
                    continue;
                var windowName = window.Title + " (window)";
                if (nameRegex.Match(windowName).Success)
                    matches.Add(new SearchResult { Name = windowName, Type = "Window", Item = window });
            }

            Profiler.EndEvent();
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
                var name = contentItem.ShortName;
                if (contentItem.IsAsset)
                {
                    if (nameRegex.Match(name).Success)
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
                        matches.Add(new SearchResult { Name = name, Type = finalName, Item = asset });
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
                    if (nameRegex.Match(name).Success && typeRegex.Match(contentItem.GetType().Name).Success)
                    {
                        string finalName = contentItem.GetType().Name.Replace("Item", "");
                        if (contentItem is ScriptItem)
                            name = contentItem.FileName; // Show extension for scripts (esp. for .h and .cpp files of the same name)
                        matches.Add(new SearchResult { Name = name, Type = finalName, Item = contentItem });
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
            case Script script:
                if (script.Actor != null)
                {
                    Editor.Instance.SceneEditing.Select(script.Actor);
                    Editor.Instance.Windows.EditWin.Viewport.FocusSelection();
                }
                break;
            case Windows.EditorWindow window:
                window.FocusOrShow();
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
            { "FlaxEditor.Content.Settings.NetworkSettings", "Settings" },
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
