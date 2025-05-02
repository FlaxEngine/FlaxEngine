// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Threading;
using FlaxEditor.Content;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The Visject Surface implementation for the materials editor.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.VisjectSurface" />
    [HideInEditor]
    public class MaterialSurface : VisjectSurface
    {
        /// <inheritdoc />
        public MaterialSurface(IVisjectSurfaceOwner owner, Action onSave = null, FlaxEditor.Undo undo = null)
        : base(owner, onSave, undo)
        {
        }

        /// <inheritdoc />
        public override bool CanLivePreviewValueChanges => false;

        /// <inheritdoc />
        public override bool UseContextMenuDescriptionPanel => true;

        /// <inheritdoc />
        public override string GetTypeName(ScriptType type)
        {
            if (type.Type == typeof(void))
                return "Material";
            if (type.Type == typeof(FlaxEngine.Object))
                return "Texture";
            return base.GetTypeName(type);
        }

        /// <inheritdoc />
        public override bool CanUseNodeType(GroupArchetype groupArchetype, NodeArchetype nodeArchetype)
        {
            return (nodeArchetype.Flags & NodeFlags.MaterialGraph) != 0 && base.CanUseNodeType(groupArchetype, nodeArchetype);
        }

        /// <inheritdoc />
        protected override bool ValidateDragItem(AssetItem assetItem)
        {
            if (assetItem.IsOfType<Texture>())
                return true;
            if (assetItem.IsOfType<CubeTexture>())
                return true;
            if (assetItem.IsOfType<MaterialBase>())
                return true;
            if (assetItem.IsOfType<MaterialFunction>())
                return true;
            if (assetItem.IsOfType<GameplayGlobals>())
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

                if (assetItem.IsOfType<Texture>())
                {
                    // Check if it's a normal map
                    bool isNormalMap = false;
                    var obj = FlaxEngine.Content.LoadAsync<Texture>(assetItem.ID);
                    if (obj)
                    {
                        Thread.Sleep(50);

                        if (!obj.WaitForLoaded())
                        {
                            isNormalMap = obj.IsNormalMap;
                        }
                    }

                    node = Context.SpawnNode(5, (ushort)(isNormalMap ? 4 : 1), args.SurfaceLocation, new object[] { assetItem.ID });
                }
                else if (assetItem.IsOfType<CubeTexture>())
                {
                    node = Context.SpawnNode(5, 3, args.SurfaceLocation, new object[] { assetItem.ID });
                }
                else if (assetItem.IsOfType<MaterialBase>())
                {
                    node = Context.SpawnNode(8, 1, args.SurfaceLocation, new object[] { assetItem.ID });
                }
                else if (assetItem.IsOfType<MaterialFunction>())
                {
                    node = Context.SpawnNode(1, 24, args.SurfaceLocation, new object[] { assetItem.ID });
                }
                else if (assetItem.IsOfType<GameplayGlobals>())
                {
                    node = Context.SpawnNode(7, 16, args.SurfaceLocation, new object[]
                    {
                        assetItem.ID,
                        string.Empty,
                    });
                }

                if (node != null)
                {
                    args.SurfaceLocation.Y += node.Height + 10;
                }
            }

            base.HandleDragDropAssets(objects, args);
        }
    }
}
