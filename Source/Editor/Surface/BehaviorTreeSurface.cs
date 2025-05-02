// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.ContextMenu;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The Visject Surface implementation for the Behavior Tree graphs.
    /// </summary>
    /// <seealso cref="VisjectSurface" />
    [HideInEditor]
    public class BehaviorTreeSurface : VisjectSurface
    {
        private static NodesCache _nodesCache = new NodesCache(IterateNodesCache);

        /// <inheritdoc />
        public BehaviorTreeSurface(IVisjectSurfaceOwner owner, Action onSave, FlaxEditor.Undo undo)
        : base(owner, onSave, undo, CreateStyle())
        {
            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
        }

        private static SurfaceStyle CreateStyle()
        {
            var editor = Editor.Instance;
            var style = SurfaceStyle.CreateStyleHandler(editor);
            style.DrawBox = DrawBox;
            style.DrawConnection = SurfaceStyle.DrawStraightConnection;
            return style;
        }

        private void OnScriptsReloadBegin()
        {
            _nodesCache.Clear();
        }

        private static void DrawBox(Box box)
        {
            var rect = new Rectangle(Float2.Zero, box.Size);
            const float minBoxSize = 5.0f;
            if (rect.Size.LengthSquared < minBoxSize * minBoxSize)
                return;

            var style = FlaxEngine.GUI.Style.Current;
            var color = style.LightBackground;
            if (box.IsMouseOver)
                color *= 1.2f;
            Render2D.FillRectangle(rect, color);
        }

        private void OnActiveContextMenuVisibleChanged(Control activeCM)
        {
            _nodesCache.Wait();
        }

        private static void IterateNodesCache(ScriptType scriptType, Dictionary<KeyValuePair<string, ushort>, GroupArchetype> cache, int version)
        {
            // Filter by BT node types only
            if (!new ScriptType(typeof(BehaviorTreeNode)).IsAssignableFrom(scriptType))
                return;

            // Skip in-built types
            if (scriptType == typeof(BehaviorTreeNode) ||
                scriptType == typeof(BehaviorTreeCompoundNode) ||
                scriptType == typeof(BehaviorTreeRootNode))
                return;

            // Nodes-only
            if (new ScriptType(typeof(BehaviorTreeDecorator)).IsAssignableFrom(scriptType))
                return;

            // Create group archetype
            var groupKey = new KeyValuePair<string, ushort>("Behavior Tree", 19);
            if (!cache.TryGetValue(groupKey, out var group))
            {
                group = new GroupArchetype
                {
                    GroupID = groupKey.Value,
                    Name = groupKey.Key,
                    Color = new Color(70, 220, 181),
                    Tag = version,
                    Archetypes = new List<NodeArchetype>(),
                };
                cache.Add(groupKey, group);
            }

            // Create node archetype
            var node = (NodeArchetype)Archetypes.BehaviorTree.Nodes[0].Clone();
            node.DefaultValues[0] = scriptType.TypeName;
            node.Flags &= ~NodeFlags.NoSpawnViaGUI;
            node.Title = Archetypes.BehaviorTree.Node.GetTitle(scriptType);
            node.Description = Editor.Instance.CodeDocs.GetTooltip(scriptType);
            ((IList<NodeArchetype>)group.Archetypes).Add(node);
        }

        /// <inheritdoc />
        protected override void OnShowPrimaryMenu(VisjectCM activeCM, Float2 location, Box startBox)
        {
            activeCM.ShowExpanded = true;
            _nodesCache.Get(activeCM);

            base.OnShowPrimaryMenu(activeCM, location, startBox);

            activeCM.VisibleChanged += OnActiveContextMenuVisibleChanged;
        }

        /// <inheritdoc />
        public override string GetTypeName(ScriptType type)
        {
            if (type == ScriptType.Void)
                return string.Empty; // Remove `Void` tooltip from connection areas
            return base.GetTypeName(type);
        }

        /// <inheritdoc />
        public override bool Load()
        {
            if (base.Load())
                return true;

            // Ensure to have Root node created (UI blocks spawning of it)
            if (RootContext.FindNode(19, 2) == null)
            {
                RootContext.SpawnNode(19, 2, Float2.Zero);
            }

            return false;
        }

        /// <inheritdoc />
        public override bool CanUseNodeType(GroupArchetype groupArchetype, NodeArchetype nodeArchetype)
        {
            // Comments
            if (groupArchetype.GroupID == 7 && nodeArchetype.TypeID == 11)
                return true;

            // Single root node
            if (groupArchetype.GroupID == 19 && nodeArchetype.TypeID == 2 && RootContext.FindNode(19, 2) != null)
                return false;

            // Behavior Tree nodes only
            return (nodeArchetype.Flags & NodeFlags.BehaviorTreeGraph) != 0 &&
                   groupArchetype.GroupID == 19 &&
                   base.CanUseNodeType(groupArchetype, nodeArchetype);
        }

        /// <inheritdoc />
        protected override bool ValidateDragItem(AssetItem assetItem)
        {
            if (assetItem.IsOfType<BehaviorTree>())
                return true;
            return base.ValidateDragItem(assetItem);
        }

        /// <inheritdoc />
        protected override void HandleDragDropAssets(List<AssetItem> objects, DragDropEventArgs args)
        {
            for (int i = 0; i < objects.Count; i++)
            {
                var assetItem = objects[i];
                SurfaceNode node = null;

                if (assetItem.IsOfType<BehaviorTree>())
                {
                    var instance = new BehaviorTreeSubTreeNode();
                    instance.Name = Utilities.Utils.GetPropertyNameUI(assetItem.ShortName);
                    instance.Tree = (BehaviorTree)assetItem.LoadAsync();
                    node = Context.SpawnNode(19, 1, args.SurfaceLocation, new object[]
                    {
                        typeof(BehaviorTreeSubTreeNode).FullName,
                        FlaxEngine.Json.JsonSerializer.SaveToBytes(instance),
                        null,
                    });
                    FlaxEngine.Object.Destroy(instance);
                }

                if (node != null)
                    args.SurfaceLocation.X += node.Width + 10;
            }
            base.HandleDragDropAssets(objects, args);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            _nodesCache.Wait();

            base.OnDestroy();
        }
    }
}
