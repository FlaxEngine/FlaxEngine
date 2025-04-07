// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using FlaxEditor;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Docking;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Options;
using FlaxEditor.Surface;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEngine.Windows.Search
{
    /// <summary>
    /// Content searching asset types.
    /// </summary>
    [Flags]
    public enum SearchAssetTypes
    {
        /// <summary>
        /// The none.
        /// </summary>
        None = 0,

        /// <summary>
        /// The visual script.
        /// </summary>
        VisualScript = 1 << 0,

        /// <summary>
        /// The material.
        /// </summary>
        Material = 1 << 1,

        /// <summary>
        /// The animation graph.
        /// </summary>
        AnimGraph = 1 << 2,

        /// <summary>
        /// The particle emitter.
        /// </summary>
        ParticleEmitter = 1 << 3,

        /// <summary>
        /// All types.
        /// </summary>
        All = VisualScript | Material | AnimGraph | ParticleEmitter,
    }

    /// <summary>
    /// Interface for Editor windows to customize search.
    /// </summary>
    public interface ISearchWindow
    {
        /// <summary>
        /// Gets the type of the asset for the search.
        /// </summary>
        SearchAssetTypes AssetType { get; }
    }

    /// <summary>
    /// The content searching window. Allows to search inside Visual Scripts, Materials, Particles and other assets.
    /// </summary>
    [HideInEditor]
    internal class ContentSearchWindow : EditorWindow
    {
        /// <summary>
        /// Content searching location types.
        /// </summary>
        public enum SearchLocations
        {
            /// <summary>
            /// Searches in the currently opened asset (last focused).
            /// </summary>
            CurrentAsset,

            /// <summary>
            /// Searches in all opened asset windows.
            /// </summary>
            AllOpenedAssets,

            /// <summary>
            /// Searches in all asset in the project.
            /// </summary>
            AllAssets,
        }

        private sealed class SearchSurfaceContext : ISurfaceContext
        {
            /// <inheritdoc />
            public Asset SurfaceAsset { get; set; }

            /// <inheritdoc />
            public string SurfaceName => string.Empty;

            /// <inheritdoc />
            public byte[] SurfaceData { get; set; }

            /// <inheritdoc />
            public VisjectSurfaceContext ParentContext => null;

            /// <inheritdoc />
            public void OnContextCreated(VisjectSurfaceContext context)
            {
            }
        }

        [HideInEditor]
        private sealed class SearchResultTreeNode : TreeNode
        {
            public Action<SearchResultTreeNode> Navigate;

            public SearchResultTreeNode()
            : base(false, SpriteHandle.Invalid, SpriteHandle.Invalid)
            {
            }

            public SearchResultTreeNode(SpriteHandle icon)
            : base(false, icon, icon)
            {
            }

            /// <inheritdoc />
            public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
            {
                var result = base.OnShowTooltip(out text, out location, out area);
                location = new Float2(ChildrenIndent, HeaderHeight);
                return result;
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                InputOptions options = FlaxEditor.Editor.Instance.Options.Options.Input;
                if (IsFocused)
                {
                    if (key == KeyboardKeys.Return && Navigate != null)
                    {
                        Navigate.Invoke(this);
                        return true;
                    }
                    if (options.Copy.Process(this))
                    {
                        Clipboard.Text = Text;
                        return true;
                    }
                }
                return base.OnKeyDown(key);
            }

            /// <inheritdoc />
            protected override bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
            {
                if (Navigate != null)
                {
                    Navigate.Invoke(this);
                    return true;
                }
                return base.OnMouseDoubleClickHeader(ref location, button);
            }
        }

        private TextBox _searchBox;
        private Tree _resultsTree;
        private TreeNode _resultsTreeRoot;
        private Label _loadingLabel;
        private float _progress;
        private CancellationTokenSource _token;
        private Task _task;
        private bool _searchTextExact;
        private string _searchText;
        private List<TreeNode> _pendingResults = new List<TreeNode>();
        private SearchSurfaceContext _searchSurfaceContext = new SearchSurfaceContext();
        private VisjectSurfaceContext _visjectSurfaceContext;
        private SurfaceStyle _visjectSurfaceStyle;

        /// <summary>
        /// The current search location type.
        /// </summary>
        public SearchLocations SearchLocation = SearchLocations.AllAssets;

        /// <summary>
        /// The current search asset types (flags).
        /// </summary>
        public SearchAssetTypes SearchAssets = SearchAssetTypes.VisualScript;

        /// <summary>
        /// Gets or sets the current search query text.
        /// </summary>
        public string Query
        {
            get => _searchBox.Text;
            set => _searchBox.Text = value;
        }

        /// <summary>
        /// The target window which called the search tool. Used as a context for <see cref="SearchLocations.CurrentAsset"/> search.
        /// </summary>
        public DockWindow TargetWindow;

        /// <inheritdoc />
        public ContentSearchWindow(Editor editor)
        : base(editor, true, ScrollBars.Vertical)
        {
            Title = "Content Search";
            Icon = Editor.Icons.Search64;

            // Setup UI
            var topPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, 22.0f),
                BackgroundColor = Style.Current.Background,
                Parent = this,
            };
            var optionsButton = new Button(2, 2, 60.0f, 18.0f)
            {
                Text = "Options",
                TooltipText = "Change search options",
                Parent = topPanel,
            };
            optionsButton.ButtonClicked += OnOptionsDropdownClicked;
            _searchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = topPanel,
                Bounds = new Rectangle(optionsButton.Right + 2.0f, 2, topPanel.Width - 4.0f - optionsButton.Width, 18.0f),
            };
            _searchBox.TextBoxEditEnd += OnSearchBoxEditEnd;
            _resultsTree = new Tree(false)
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, topPanel.Bottom + 4 - 16.0f, 0),
                IsScrollable = true,
                Parent = this,
                IndexInParent = 0,
            };
            _resultsTreeRoot = new TreeNode
            {
                ChildrenIndent = 0,
                BackgroundColorSelected = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BackgroundColorSelectedUnfocused = Color.Transparent,
                TextColor = Color.Transparent,
            };
            _resultsTreeRoot.Expand();
            _resultsTree.AddChild(_resultsTreeRoot);
            _loadingLabel = new Label
            {
                AnchorPreset = AnchorPresets.HorizontalStretchBottom,
                Offsets = new Margin(0, 0, -20.0f, 0.0f),
                Parent = this,
                Visible = false,
            };

            _visjectSurfaceContext = new VisjectSurfaceContext(null, null, _searchSurfaceContext);
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            base.OnUpdate();

            if (_loadingLabel.Visible)
                _loadingLabel.Text = string.Format("Searching {0}%...", (int)_progress);
            _resultsTreeRoot.Expand(true);

            lock (_pendingResults)
            {
                // Add in batches or after search end
                if (_pendingResults.Count > 10 || _token == null)
                {
                    LockChildrenRecursive();
                    for (int i = 0; i < _pendingResults.Count; i++)
                        _resultsTreeRoot.AddChild(_pendingResults[i]);
                    _pendingResults.Clear();
                    UnlockChildrenRecursive();
                    PerformLayout();
                }
            }
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            if (key == KeyboardKeys.ArrowDown && _resultsTree.Selection.Count == 0 && _resultsTreeRoot.HasAnyVisibleChild && (_searchBox.IsFocused || IsFocused))
            {
                // Focus the first item from results
                _resultsTree.Select(_resultsTreeRoot.Children[0] as TreeNode);
                return;
            }

            base.OnKeyUp(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Cancel();

            base.OnDestroy();
        }

        private void OnOptionsDropdownClicked(Button optionsButton)
        {
            var menu = new ContextMenu();

            var m = menu.AddChildMenu("Search Location").ContextMenu;
            var entries = new List<EnumComboBox.Entry>();
            EnumComboBox.BuildEntriesDefault(typeof(SearchLocations), entries);
            foreach (var e in entries)
            {
                var value = (SearchLocations)e.Value;
                var b = m.AddButton(e.Name, () =>
                {
                    SearchLocation = value;
                    Search();
                });
                b.TooltipText = e.Tooltip;
                b.Checked = SearchLocation == value;
            }

            m = menu.AddChildMenu("Search Assets").ContextMenu;
            entries.Clear();
            EnumComboBox.BuildEntriesDefault(typeof(SearchAssetTypes), entries);
            foreach (var e in entries)
            {
                var value = (SearchAssetTypes)e.Value;
                var b = m.AddButton(e.Name, () =>
                {
                    if (value == SearchAssetTypes.None || value == SearchAssetTypes.All)
                        SearchAssets = value;
                    else
                        SearchAssets ^= value;
                    Search();
                });
                b.TooltipText = e.Tooltip;
                if (value == SearchAssetTypes.None || value == SearchAssetTypes.All)
                    b.Checked = SearchAssets == value;
                else
                    b.Checked = SearchAssets.HasFlag(value);
            }

            menu.Show(Parent, Location + optionsButton.BottomLeft);
        }

        private void OnSearchBoxEditEnd(TextBoxBase searchBox)
        {
            Search();
            Focus();
        }

        private void Cancel()
        {
            if (_token != null)
            {
                try
                {
                    // Wait for async end
                    _token.Cancel();
                    _task.Wait();
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                }
                finally
                {
                    _token = null;
                }
            }
            lock (_pendingResults)
            {
                _pendingResults.Clear();
            }
        }

        /// <summary>
        /// Initializes the searching (stops the current one if any active).
        /// </summary>
        public void Search()
        {
            // Stop
            Cancel();

            // Clear results
            _resultsTreeRoot.DisposeChildren();
            _resultsTreeRoot.Expand();

            // Start async searching
            _searchText = _searchBox.Text;
            _searchText = _searchText.Trim();
            if (_searchText.Length == 0)
                return;
            _searchTextExact = _searchText.Length > 2 && _searchText[0] == '\"' && _searchText[_searchText.Length - 1] == '\"';
            if (_searchTextExact)
                _searchText = _searchText.Substring(1, _searchText.Length - 2);
            _token = new CancellationTokenSource();
            _task = Task.Run(SearchAsync, _token.Token);
            _progress = 0.0f;
            _loadingLabel.Visible = true;
        }

        private void SearchAsync()
        {
            try
            {
                SearchAsyncInner();

                lock (_pendingResults)
                {
                    if (!_token.IsCancellationRequested && _resultsTreeRoot.ChildrenCount == 0 && _pendingResults.Count == 0)
                    {
                        _pendingResults.Add(new SearchResultTreeNode
                        {
                            Text = "No results",
                        });
                    }
                }
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                lock (_pendingResults)
                    _pendingResults.Clear();
            }
            finally
            {
                _visjectSurfaceContext.Clear();
                _progress = 100.0f;
                _loadingLabel.Visible = false;
                _token = null;
            }
        }

        private void SearchAsyncInner()
        {
            // Get the assets for search
            Guid[] assets = null;
            switch (SearchLocation)
            {
            case SearchLocations.CurrentAsset:
                if (TargetWindow is AssetEditorWindow assetEditorWindow)
                    assets = new[] { assetEditorWindow.Item.ID };
                break;
            case SearchLocations.AllOpenedAssets:
                assets = new Guid[Editor.Windows.Windows.Count];
                for (var i = 0; i < Editor.Windows.Windows.Count; i++)
                {
                    if (Editor.Windows.Windows[i] is AssetEditorWindow window)
                        assets[i] = window.Item.ID;
                }
                break;
            case SearchLocations.AllAssets:
                assets = Content.GetAllAssets();
                break;
            }
            if (assets == null)
                return;

            // Build valid asset typenames list
            var searchAssets = SearchAssets;
            var validTypeNames = new HashSet<string>();
            if (searchAssets.HasFlag(SearchAssetTypes.VisualScript))
            {
                validTypeNames.Add(typeof(VisualScript).FullName);
            }
            if (searchAssets.HasFlag(SearchAssetTypes.Material))
            {
                validTypeNames.Add(typeof(Material).FullName);
                validTypeNames.Add(typeof(MaterialFunction).FullName);
            }
            if (searchAssets.HasFlag(SearchAssetTypes.AnimGraph))
            {
                validTypeNames.Add(typeof(AnimationGraph).FullName);
                validTypeNames.Add(typeof(AnimationGraphFunction).FullName);
            }
            if (searchAssets.HasFlag(SearchAssetTypes.ParticleEmitter))
            {
                validTypeNames.Add(typeof(ParticleEmitter).FullName);
                validTypeNames.Add(typeof(ParticleEmitterFunction).FullName);
            }

            // Iterate over all assets
            var tempFolder = StringUtils.NormalizePath(Path.GetDirectoryName(Globals.TemporaryFolder));
            var nodePath = new List<uint>();
            for (var i = 0; i < assets.Length && !_token.IsCancellationRequested; i++)
            {
                var id = assets[i];
                _progress = ((float)i / assets.Length) * 100.0f;
                if (!Content.GetAssetInfo(id, out var assetInfo))
                    continue;
                if (!validTypeNames.Contains(assetInfo.TypeName))
                    continue;
                if (assetInfo.Path.StartsWith(tempFolder))
                    continue;
                // TODO: implement assets indexing or other caching for faster searching
                var asset = Content.LoadAsync(id);
                if (asset == null)
                    continue;

                // Search asset contents
                nodePath.Clear();
                if (asset is VisualScript visualScript)
                    SearchAsyncInnerVisject(asset, visualScript.LoadSurface(), nodePath);
                else if (asset is Material material)
                    SearchAsyncInnerVisject(asset, material.LoadSurface(false), nodePath);
                else if (asset is MaterialFunction materialFunction)
                    SearchAsyncInnerVisject(asset, materialFunction.LoadSurface(), nodePath);
                else if (asset is AnimationGraph animationGraph)
                    SearchAsyncInnerVisject(asset, animationGraph.LoadSurface(), nodePath);
                else if (asset is AnimationGraphFunction animationGraphFunction)
                    SearchAsyncInnerVisject(asset, animationGraphFunction.LoadSurface(), nodePath);
                else if (asset is ParticleEmitter particleEmitter)
                    SearchAsyncInnerVisject(asset, particleEmitter.LoadSurface(false), nodePath);
                else if (asset is ParticleEmitterFunction particleEmitterFunction)
                    SearchAsyncInnerVisject(asset, particleEmitterFunction.LoadSurface(), nodePath);

                // Don't eat whole performance
                Thread.Sleep(15);
            }
        }

        private bool IsSearchMatch(ref string text)
        {
            if (_searchTextExact)
                return text.Equals(_searchText, StringComparison.Ordinal);
            return text.IndexOf(_searchText, StringComparison.OrdinalIgnoreCase) != -1;
        }

        private void AddAssetSearchResult(ref SearchResultTreeNode assetTreeNode, Asset asset)
        {
            if (assetTreeNode != null)
                return;
            assetTreeNode = new SearchResultTreeNode
            {
                Text = FlaxEditor.Utilities.Utils.GetAssetNamePath(asset.Path),
                Tag = asset.ID,
                Navigate = OnNavigateAsset,
            };
        }

        private void SearchAsyncInnerVisject(Asset asset, byte[] surfaceData, List<uint> nodePath)
        {
            // Load Visject surface from data
            if (surfaceData == null || surfaceData.Length == 0)
                return;
            _searchSurfaceContext.SurfaceAsset = asset;
            _searchSurfaceContext.SurfaceData = surfaceData;
            if (_visjectSurfaceContext.Load())
            {
                Editor.LogError("Failed to load Visject Surface for " + asset.Path);
                return;
            }
            if (_visjectSurfaceStyle == null)
                _visjectSurfaceStyle = SurfaceStyle.CreateDefault(Editor);
            SearchResultTreeNode assetTreeNode = null;

            // Search parameters
            foreach (var parameter in _visjectSurfaceContext.Parameters)
            {
                SearchResultTreeNode parameterTreeNode = null;
                SearchVisjectMatch(parameter.Value, (matchedValue, matchedText) =>
                {
                    AddAssetSearchResult(ref assetTreeNode, asset);
                    if (parameterTreeNode == null)
                        parameterTreeNode = new SearchResultTreeNode
                        {
                            Text = parameter.Name,
                            Tag = assetTreeNode.Tag,
                            Navigate = assetTreeNode.Navigate,
                            Parent = assetTreeNode,
                        };
                    var valueTreeNode = AddVisjectSearchResult(matchedValue, matchedText);
                    valueTreeNode.Tag = assetTreeNode.Tag;
                    valueTreeNode.Navigate = assetTreeNode.Navigate;
                    valueTreeNode.Parent = parameterTreeNode;
                }, "Default Value: ");
            }

            // Search nodes
            var newTreeNodes = new List<SearchResultTreeNode>();
            var nodes = _visjectSurfaceContext.Nodes.ToArray();
            foreach (var node in nodes)
            {
                newTreeNodes.Clear();
                if (node.Values != null)
                {
                    foreach (var value in node.Values)
                    {
                        SearchVisjectMatch(value, (matchedValue, matchedText) =>
                        {
                            var valueTreeNode = AddVisjectSearchResult(matchedValue, matchedText, node.Archetype.ConnectionsHints);
                            valueTreeNode.Tag = new VisjectNodeTag { NodeId = node.ID, NodePath = nodePath.ToArray() };
                            valueTreeNode.Navigate = OnNavigateVisjectNode;
                            newTreeNodes.Add(valueTreeNode);
                        });
                    }
                }
                if (node is ISurfaceContext context)
                {
                    nodePath.Add(node.ID);
                    SearchAsyncInnerVisject(asset, context.SurfaceData, nodePath);
                    nodePath.RemoveAt(nodePath.Count - 1);
                }
                var nodeSearchText = node.ContentSearchText;

                if (newTreeNodes.Count != 0 || (nodeSearchText != null && IsSearchMatch(ref nodeSearchText)) || node.Search(_searchText))
                {
                    AddAssetSearchResult(ref assetTreeNode, asset);
                    var nodeTreeNode = new SearchResultTreeNode
                    {
                        Text = node.Title,
                        TooltipText = node.TooltipText,
                        Tag = new VisjectNodeTag { NodeId = node.ID, NodePath = nodePath.ToArray() },
                        Navigate = OnNavigateVisjectNode,
                        Parent = assetTreeNode,
                    };
                    foreach (var newTreeNode in newTreeNodes)
                    {
                        newTreeNode.Parent = nodeTreeNode;
                    }
                }
            }

            if (assetTreeNode != null)
            {
                assetTreeNode.ExpandAll();
                lock (_pendingResults)
                    _pendingResults.Add(assetTreeNode);
            }
        }

        private void SearchVisjectMatch(object value, Action<object, string> matched, string prefix = null)
        {
            var text = value as string;
            if (value == null || value is byte[])
                return;
            if (text == null)
            {
                if (value is IList asList)
                {
                    // Check element of the list in separate
                    for (int i = 0; i < asList.Count; i++)
                        SearchVisjectMatch(asList[i], matched, $"[{i}] ");
                    return;
                }
                if (value is IDictionary asDictionary)
                {
                    // Check each pair of the dictionary in separate
                    foreach (var key in asDictionary.Keys)
                    {
                        var isMatch = false;
                        var keyValue = asDictionary[key];
                        object keyMatchedValue = key, valueMatchedValue = keyValue;
                        string keyMatchedText = null, valueMatchedText = null;
                        SearchVisjectMatch(key, (matchedValue, matchedText) =>
                        {
                            isMatch = true;
                            keyMatchedValue = matchedValue;
                            keyMatchedText = matchedText;
                        });
                        SearchVisjectMatch(keyValue, (matchedValue, matchedText) =>
                        {
                            isMatch = true;
                            valueMatchedValue = matchedValue;
                            valueMatchedText = matchedText;
                        });
                        if (isMatch)
                        {
                            if (keyMatchedText == null)
                                keyMatchedText = keyMatchedValue?.ToString() ?? string.Empty;
                            if (valueMatchedText == null)
                                valueMatchedText = valueMatchedValue?.ToString() ?? string.Empty;
                            matched(valueMatchedValue, $"[{keyMatchedText}]: {valueMatchedText}");
                        }
                    }
                    return;
                }

                text = value.ToString();

                // Special case for assets (and Guid converted into asset) to search in asset name path
                if (value is Guid asGuid)
                {
                    if (Content.GetAssetInfo(asGuid, out _))
                        value = Content.LoadAsync<Asset>(asGuid);
                    else
                        text = Json.JsonSerializer.GetStringID(asGuid);
                }
                if (value is Asset asAsset)
                    text = FlaxEditor.Utilities.Utils.GetAssetNamePath(asAsset.Path);
            }
            if (IsSearchMatch(ref text))
            {
                if (prefix != null)
                    text = prefix + text;
                matched(value, text);
            }
        }

        private SearchResultTreeNode AddVisjectSearchResult(object value, string text, ConnectionsHint connectionsHint = ConnectionsHint.None)
        {
            var valueType = TypeUtils.GetObjectType(value);
            _visjectSurfaceStyle.GetConnectionColor(valueType, connectionsHint, out var valueColor);
            return new SearchResultTreeNode(_visjectSurfaceStyle.Icons.BoxClose)
            {
                Text = text,
                TooltipText = valueType.ToString(),
                IconColor = valueColor,
            };
        }

        private void OnNavigateAsset(SearchResultTreeNode treeNode)
        {
            var assetId = (Guid)treeNode.Tag;
            var contentItem = Editor.ContentDatabase.FindAsset(assetId);
            Editor.ContentEditing.Open(contentItem);
        }

        private struct VisjectNodeTag
        {
            public uint NodeId;
            public uint[] NodePath;
        }

        private void OnNavigateVisjectNode(SearchResultTreeNode treeNode)
        {
            var tag = (VisjectNodeTag)treeNode.Tag;
            var assetId = Guid.Empty;
            var assetTreeNode = treeNode.Parent;
            while (!(assetTreeNode.Tag is Guid))
                assetTreeNode = assetTreeNode.Parent;
            assetId = (Guid)assetTreeNode.Tag;
            var contentItem = Editor.ContentDatabase.FindAsset(assetId);
            if (Editor.ContentEditing.Open(contentItem) is IVisjectSurfaceWindow window)
            {
                var context = window.VisjectSurface.OpenContext(tag.NodePath) ?? window.VisjectSurface.Context;
                var node = context.FindNode(tag.NodeId);
                if (node != null)
                {
                    // Focus this node
                    window.VisjectSurface.FocusNode(node);
                }
                else
                {
                    // Retry once the surface gets loaded
                    window.SurfaceLoaded += () => OnNavigateVisjectNode(treeNode);
                }
            }
        }
    }
}
