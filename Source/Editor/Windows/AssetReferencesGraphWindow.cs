// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor tool window for <see cref="Asset"/> references debugging in a virtual dependencies graph.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    internal sealed class AssetReferencesGraphWindow : EditorWindow, IVisjectSurfaceOwner
    {
        private sealed class AssetNode : SurfaceNode
        {
            public readonly Guid AssetId;
            public float LayoutHeight;
            public int FirstChild, LastChild;
            private int _inputs, _outputs;

            public AssetNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch, Guid assetId)
            : base(id, context, nodeArch, groupArch)
            {
                AssetId = assetId;

                // Init node UI
                var picker = new AssetPicker
                {
                    Location = new Float2(40, 2 * Constants.LayoutOffsetY),
                    Width = 100.0f,
                    CanEdit = false,
                    Parent = this,
                };
                // TODO: display some asset info like disk size, memory usage, etc.
                var asset = FlaxEngine.Content.LoadAsync<Asset>(AssetId);
                if (asset != null)
                {
                    var path = asset.Path;
                    picker.Validator.SelectedAsset = asset;
                    Title = System.IO.Path.GetFileNameWithoutExtension(path);
                    TooltipText = asset.TypeName + '\n' + path;
                }
                else
                {
                    picker.Validator.SelectedID = AssetId;
                    var assetItem = picker.Validator.SelectedItem as AssetItem;
                    if (assetItem != null)
                    {
                        Title = assetItem.ShortName;
                        TooltipText = assetItem.TypeName + '\n' + assetItem.Path;
                    }
                    else
                    {
                        Title = AssetId.ToString();
                    }
                }
                ResizeAuto();
                LayoutHeight = Height;
            }

            public void ConnectTo(AssetNode target, bool reverse)
            {
                var outputNode = reverse ? target : this;
                var inputNode = reverse ? this : target;

                var output = new OutputBox(outputNode, NodeElementArchetype.Factory.Output(outputNode._outputs, string.Empty, ScriptType.Void, outputNode._outputs, true));
                outputNode.AddElement(output);
                outputNode._outputs++;

                var input = new InputBox(inputNode, NodeElementArchetype.Factory.Input(inputNode._inputs, string.Empty, true, ScriptType.Void, inputNode._outputs));
                inputNode.AddElement(input);
                inputNode._inputs++;

                output.Connect(input);
            }

            public override string ToString()
            {
                return Title;
            }
        }

        private static readonly NodeArchetype[] GraphNodes =
        {
            new NodeArchetype
            {
                TypeID = 1,
                Title = "Asset",
                Description = string.Empty,
                Flags = NodeFlags.AllGraphs | NodeFlags.NoRemove | NodeFlags.NoSpawnViaGUI | NodeFlags.NoCloseButton,
                Size = new Float2(150, 200),
            },
        };

        private static readonly List<GroupArchetype> GraphGroups = new List<GroupArchetype>
        {
            new GroupArchetype
            {
                GroupID = 1,
                Name = "Assets",
                Color = new Color(118, 82, 186),
                Archetypes = GraphNodes
            },
        };

        private sealed class Surface : VisjectSurface
        {
            public Surface(IVisjectSurfaceOwner owner)
            : base(owner)
            {
                CanEdit = false;
            }

            public void Init(List<SurfaceNode> nodes)
            {
                LockChildrenRecursive();
                Nodes.AddRange(nodes);
                foreach (var node in nodes)
                {
                    Context.OnControlLoaded(node, SurfaceNodeActions.Load);
                    node.OnSurfaceLoaded(SurfaceNodeActions.Load);
                    Context.OnControlSpawned(node, SurfaceNodeActions.Load);
                }
                ShowWholeGraph();
                UnlockChildrenRecursive();
                PerformLayout();
            }
        }

        private string _cacheFolder;
        private Guid _assetId;
        private Surface _surface;
        private Label _loadingLabel;
        private CancellationTokenSource _token;
        private Task _task;
        private const float MarginX = 200;
        private const float MarginY = 50;

        // Async task data
        private float _progress;
        private Dictionary<Guid, Guid[]> _refs;
        private List<SurfaceNode> _nodes;
        private HashSet<Guid> _nodesAssets;

        /// <summary>
        /// Initializes a new instance of the <see cref="PluginsWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <param name="assetItem">The asset.</param>
        public AssetReferencesGraphWindow(Editor editor, AssetItem assetItem)
        : base(editor, false, ScrollBars.None)
        {
            Title = assetItem.ShortName + " References";

            _cacheFolder = Path.Combine(Globals.ProjectCacheFolder, "References");
            if (!Directory.Exists(_cacheFolder))
                Directory.CreateDirectory(_cacheFolder);
            _assetId = assetItem.ID;
            _surface = new Surface(this)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this,
            };
            _loadingLabel = new Label
            {
                Text = "Loading...",
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this,
            };

            // Start async initialization
            _token = new CancellationTokenSource();
            _task = Task.Run(Load, _token.Token);
        }

        private AssetNode SpawnNode(Guid assetId)
        {
            _nodesAssets.Add(assetId);
            var node = new AssetNode((uint)_nodes.Count + 1, _surface.Context, GraphNodes[0], GraphGroups[0], assetId);
            _nodes.Add(node);
            return node;
        }

        private unsafe void SearchRefs(Guid assetId)
        {
            // Skip assets that never contain references to prevent loading them
            if (FlaxEngine.Content.GetAssetInfo(assetId, out var assetInfo) &&
                (assetInfo.TypeName == "FlaxEngine.Texture" ||
                 assetInfo.TypeName == "FlaxEngine.CubeTexture" ||
                 assetInfo.TypeName == "FlaxEngine.Shader"))
                return;

            // Skip if already in cache
            if (_refs.ContainsKey(assetId))
                return;

            // Try to load cached references form previous run
            var cachePath = Path.Combine(_cacheFolder, $"{FlaxEngine.Json.JsonSerializer.GetStringID(assetId)}.1.cache");
            var hasInfo = FlaxEngine.Content.GetAssetInfo(assetId, out var info);
            if (hasInfo && File.Exists(cachePath) && File.GetLastWriteTime(cachePath) > File.GetLastWriteTime(info.Path))
            {
                byte[] rawData = File.ReadAllBytes(cachePath);
                Guid[] loadedRefs = new Guid[rawData.Length / sizeof(Guid)];
                if (rawData.Length != 0)
                {
                    fixed (byte* rawDataPtr = rawData)
                    fixed (Guid* loadedRefsPtr = loadedRefs)
                        Unsafe.CopyBlock(loadedRefsPtr, rawDataPtr, (uint)rawData.Length);
                }
                _refs[assetId] = loadedRefs;
                return;
            }

            // Load asset (with cancel support)
            var obj = FlaxEngine.Object.TryFind<FlaxEngine.Object>(ref assetId);
            if (obj is Scene scene)
            {
                // Special case for scene assets that are also loaded
                _refs[assetId] = scene.GetAssetReferences();
                return;
            }
            var asset = obj as Asset;
            if (!asset)
                asset = FlaxEngine.Content.LoadAsync<Asset>(assetId);
            if (asset == null || asset.IsVirtual)
                return;
            while (asset && !asset.IsLoaded && !asset.LastLoadFailed)
            {
                if (_token.IsCancellationRequested)
                    return;
                Thread.Sleep(10);
            }
            if (!asset || !asset.IsLoaded)
                return;

            // Get direct references
            var references = asset.GetReferences();
            _refs[assetId] = references;

            // Save reference to the cache
            if (hasInfo)
            {
                byte[] rawData = new byte[references.Length * sizeof(Guid)];
                if (rawData.Length != 0)
                {
                    fixed (byte* rawDataPtr = rawData)
                    fixed (Guid* referencesPtr = references)
                        Unsafe.CopyBlock(rawDataPtr, referencesPtr, (uint)rawData.Length);
                }
                File.WriteAllBytes(cachePath, rawData);
            }
        }

        private void BuildGraph(AssetNode node, int level, bool reverse)
        {
            if (level == 0)
                return;
            level--;
            Guid[] assetRefs;
            if (reverse)
            {
                // Search for assets that reference this asset
                var list = new List<Guid>();
                foreach (var e in _refs)
                {
                    if (e.Value.Contains(node.AssetId))
                        list.Add(e.Key);
                }
                if (list.Count == 0)
                    return;
                assetRefs = list.ToArray();
            }
            else if (!_refs.TryGetValue(node.AssetId, out assetRefs))
                return;

            // Create child nodes
            node.FirstChild = _nodes.Count;
            for (int i = 0; i < assetRefs.Length; i++)
            {
                if (_token.IsCancellationRequested)
                    return;
                var assetRef = assetRefs[i];

                // Check if asset exists
                var obj = FlaxEngine.Object.TryFind<FlaxEngine.Object>(ref assetRef);
                if (!(obj is Asset) && !(obj is Scene))
                {
                    var asset = FlaxEngine.Content.LoadAsync<Asset>(assetRef);
                    if (asset == null || asset.IsVirtual)
                        continue;
                }

                // Skip nodes that were already added to the graph
                if (_nodesAssets.Contains(assetRef))
                    continue;

                // Build graph further
                var assetRefNode = SpawnNode(assetRef);
                node.ConnectTo(assetRefNode, reverse);
            }
            node.LastChild = _nodes.Count;

            // Build child nodes (recursive)
            var childrenCount = node.LastChild - node.FirstChild;
            if (childrenCount != 0)
                node.ResizeAuto();
            node.LayoutHeight = 0;
            for (int i = 0; i < childrenCount; i++)
            {
                if (_token.IsCancellationRequested)
                    return;
                var assetRefNode = (AssetNode)_nodes[node.FirstChild + i];
                BuildGraph(assetRefNode, level, reverse);
                node.LayoutHeight += assetRefNode.LayoutHeight + MarginY;
            }
            node.LayoutHeight = Mathf.Max(node.Height, node.LayoutHeight - MarginY);
        }

        private void ArrangeGraph(AssetNode node, bool reverse)
        {
            var childrenCount = node.LastChild - node.FirstChild;
            if (childrenCount == 0)
                return;

            // Place children relative to the node origin but account for the whole sub-tree layout
            var origin = new Float2(reverse ? node.Left : node.Right, node.Center.Y - node.LayoutHeight * 0.5f);
            var layoutProgress = 0.0f;
            var maxWidth = MarginX;
            if (reverse)
            {
                for (int i = 0; i < childrenCount; i++)
                {
                    var assetRefNode = (AssetNode)_nodes[node.FirstChild + i];
                    maxWidth = Mathf.Max(maxWidth, assetRefNode.Width + MarginX);
                }
            }
            for (int i = 0; i < childrenCount; i++)
            {
                var assetRefNode = (AssetNode)_nodes[node.FirstChild + i];
                if (reverse)
                    assetRefNode.Location = origin + new Float2(-maxWidth, layoutProgress + assetRefNode.LayoutHeight * 0.5f - assetRefNode.Height * 0.5f);
                else
                    assetRefNode.Location = origin + new Float2(MarginX, layoutProgress + assetRefNode.LayoutHeight * 0.5f - assetRefNode.Height * 0.5f);
                ArrangeGraph(assetRefNode, reverse);
                layoutProgress += assetRefNode.LayoutHeight + MarginY;
            }
        }

        private void LoadInner()
        {
            // Build asset references graph
            // TODO: add caching of asset refs in editor Cache (eg. asset refs + file write date)
            _refs = new Dictionary<Guid, Guid[]>();
            _progress = 0.0f;
            var assets = FlaxEngine.Content.GetAllAssets();
            _progress = 5.0f;
            for (var i = 0; i < assets.Length; i++)
            {
                var id = assets[i];
                if (_token.IsCancellationRequested)
                    return;
                SearchRefs(id);
                _progress = ((float)i / assets.Length) * 90.0f + 5.0f;
            }

            // Build surface graph
            _progress = 95.0f;
            _nodes = new List<SurfaceNode>();
            _nodesAssets = new HashSet<Guid>();
            var searchLevel = 4; // TODO: make it as an option (somewhere in window UI)
            // TODO: add option to filter assets by type (eg. show only textures as leaf nodes)
            var assetNode = SpawnNode(_assetId);
            // TODO: add some outline or tint color to the main node
            BuildGraph(assetNode, searchLevel, false);
            ArrangeGraph(assetNode, false);
            BuildGraph(assetNode, searchLevel, true);
            ArrangeGraph(assetNode, true);
            if (_token.IsCancellationRequested)
                return;
            _progress = 100.0f;

            // Update UI
            FlaxEngine.Scripting.InvokeOnUpdate(() =>
            {
                _surface.Init(_nodes);
                _loadingLabel.Visible = false;
            });
        }

        private void Load()
        {
            try
            {
                LoadInner();
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Close();
            }
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            base.OnUpdate();

            if (_loadingLabel.Visible)
            {
                _loadingLabel.Text = string.Format("Loading {0}%...", (int)_progress);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Wait for async end
            _token.Cancel();
            _task.Wait();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public Asset SurfaceAsset => null;

        /// <inheritdoc />
        public string SurfaceName => "References";

        /// <inheritdoc />
        public byte[] SurfaceData { get; set; }

        /// <inheritdoc />
        public VisjectSurfaceContext ParentContext => null;

        /// <inheritdoc />
        public void OnContextCreated(VisjectSurfaceContext context)
        {
        }

        /// <inheritdoc />
        public Undo Undo => null;

        /// <inheritdoc />
        public void OnSurfaceEditedChanged()
        {
        }

        /// <inheritdoc />
        public void OnSurfaceGraphEdited()
        {
        }

        /// <inheritdoc />
        public void OnSurfaceClose()
        {
        }
    }
}
