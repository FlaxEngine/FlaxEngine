// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
        public MaterialSurface(IVisjectSurfaceOwner owner, Action onSave, FlaxEditor.Undo undo)
        : base(owner, onSave, undo)
        {
        }

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
        public override bool CanUseNodeType(NodeArchetype nodeArchetype)
        {
            return (nodeArchetype.Flags & NodeFlags.MaterialGraph) != 0 && base.CanUseNodeType(nodeArchetype);
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
                    args.SurfaceLocation.X += node.Width + 10;
                }
            }

            base.HandleDragDropAssets(objects, args);
        }

        /// <inheritdoc/>
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left) return base.OnMouseDown(location, button);

            GetNode(_heldKey, out var archetype, out var node);

            if (archetype == 0 || node == 0) return base.OnMouseDown(location, button);

            var spawnedNode = Context.SpawnNode(archetype, node, _rootControl.PointFromParent(location));
            return base.OnMouseDown(location, button);
        }

        private void GetNode(KeyboardKeys key, out ushort archetype, out ushort node)
        {
            archetype = 0;
            node = 0;
            switch (key)
            {
                // Add node
                case KeyboardKeys.A:
                    archetype = 3;
                    node = 1;
                    break;
                // Subtract
                case KeyboardKeys.S:
                    archetype = 3;
                    node = 2;
                    break;
                // Multiply
                case KeyboardKeys.M:
                    archetype = 3;
                    node = 3;
                    break;
                // Divide
                case KeyboardKeys.D:
                    archetype = 3;
                    node = 5;
                    break;
                // Lerp
                case KeyboardKeys.L:
                    archetype = 3;
                    node = 25;
                    break;
                // Power
                case KeyboardKeys.E:
                    archetype = 3;
                    node = 23;
                    break;
                // Texture Sample
                case KeyboardKeys.T:
                    archetype = 5;
                    node = 1;
                    break;
                // Texture Coordinates
                case KeyboardKeys.U:
                    archetype = 5;
                    node = 2;
                    break;
                // Get Parameter
                case KeyboardKeys.G:
                    archetype = 6;
                    node = 1;
                    break;

                // Float
                case KeyboardKeys.Alpha1:
                    archetype = 2;
                    node = 3;
                    break;
                // Float2
                case KeyboardKeys.Alpha2:
                    archetype = 2;
                    node = 4;
                    break;
                // Float3
                case KeyboardKeys.Alpha3:
                    archetype = 2;
                    node = 5;
                    break;
                // Float4
                case KeyboardKeys.Alpha4:
                    archetype = 2;
                    node = 6;
                    break;
                // Color
                case KeyboardKeys.Alpha5:
                    archetype = 2;
                    node = 7;
                    break;


                default:
                    break;
            }
        }
    }
}
