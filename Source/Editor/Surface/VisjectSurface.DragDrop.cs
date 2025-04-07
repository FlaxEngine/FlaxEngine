// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        private readonly DragAssets<DragDropEventArgs> _dragAssets;
        private readonly DragNames<DragDropEventArgs> _dragParameters;

        /// <summary>
        /// The custom drag drop event arguments.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.Drag.DragEventArgs" />
        public class DragDropEventArgs : DragEventArgs
        {
            /// <summary>
            /// The surface location.
            /// </summary>
            public Float2 SurfaceLocation;
        }

        /// <summary>
        /// Drag and drop handlers.
        /// </summary>
        public readonly DragHandlers DragHandlers = new DragHandlers();

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            return DragHandlers.OnDragEnter(data);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            return DragHandlers.Effect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            DragHandlers.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            var args = new DragDropEventArgs
            {
                SurfaceLocation = _rootControl.PointFromParent(ref location)
            };

            // Drag assets
            if (_dragAssets.HasValidDrag)
            {
                result = _dragAssets.Effect;

                // Process items
                HandleDragDropAssets(_dragAssets.Objects, args);
            }
            // Drag parameters
            else if (_dragParameters.HasValidDrag)
            {
                result = _dragParameters.Effect;

                // Process items
                HandleDragDropParameters(_dragParameters.Objects, args);
            }

            DragHandlers.OnDragDrop(args);

            return result;
        }

        /// <summary>
        /// Validates the asset items drag operation.
        /// </summary>
        /// <param name="assetItem">The asset item.</param>
        /// <returns>True if can drag that item, otherwise false.</returns>
        protected virtual bool ValidateDragItem(AssetItem assetItem)
        {
            return false;
        }

        /// <summary>
        /// Validates the parameter drag operation.
        /// </summary>
        /// <param name="parameterName">Name of the parameter.</param>
        /// <returns>True if can drag that parameter, otherwise false.</returns>
        protected virtual bool ValidateDragParameter(string parameterName)
        {
            return GetParameter(parameterName) != null;
        }

        /// <summary>
        /// Handles the drag drop assets action.
        /// </summary>
        /// <param name="objects">The objects.</param>
        /// <param name="args">The drag drop arguments data.</param>
        protected virtual void HandleDragDropAssets(List<AssetItem> objects, DragDropEventArgs args)
        {
        }

        /// <summary>
        /// Handles the drag drop surface parameters action.
        /// </summary>
        /// <param name="objects">The objects.</param>
        /// <param name="args">The drag drop arguments data.</param>
        protected virtual void HandleDragDropParameters(List<string> objects, DragDropEventArgs args)
        {
            // Try to get the setter node when holding the ALT key, otherwise get the getter node
            if (!Input.GetKey(KeyboardKeys.Alt) || !TryGetParameterSetterNodeArchetype(out var groupId, out var arch))
            {
                arch = GetParameterGetterNodeArchetype(out groupId);
            }

            for (int i = 0; i < objects.Count; i++)
            {
                var parameter = GetParameter(objects[i]);
                if (parameter == null)
                    throw new InvalidDataException();

                var values = (object[])arch.DefaultValues.Clone();
                values[0] = parameter.ID;

                var node = Context.SpawnNode(groupId, arch.TypeID, args.SurfaceLocation, values);

                if (node != null)
                    args.SurfaceLocation.X += node.Width + 10;
            }
        }

        /// <summary>
        /// Gets the parameter getter node archetype to use.
        /// </summary>
        /// <param name="groupId">The group ID.</param>
        /// <returns>The node archetype.</returns>
        protected internal virtual NodeArchetype GetParameterGetterNodeArchetype(out ushort groupId)
        {
            groupId = 6;
            return Archetypes.Parameters.Nodes[0];
        }

        /// <summary>
        /// Tries to get the parameter setter node archetype to use.
        /// </summary>
        /// <param name="groupId">The group ID.</param>
        /// <param name="archetype">The node archetype.</param>
        /// <returns>True if a setter node exists.</returns>
        protected virtual bool TryGetParameterSetterNodeArchetype(out ushort groupId, out NodeArchetype archetype)
        {
            groupId = 0;
            archetype = null;
            return false;
        }
    }
}
