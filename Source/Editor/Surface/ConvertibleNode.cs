using System;
using System.Linq;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// A special type of node that adds the functionality to convert nodes to parameters
    /// </summary>
    internal class ConvertibleNode : SurfaceNode
    {
        private readonly ScriptType _type;
        private readonly Func<object[], object> _convertFunction;

        /// <inheritdoc />
        public ConvertibleNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch, ScriptType type, Func<object[], object> convertFunction = null)
        : base(id, context, nodeArch, groupArch)
        {
            _type = type;
            _convertFunction = convertFunction;
        }

        /// <inheritdoc />
        public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Float2 location)
        {
            base.OnShowSecondaryContextMenu(menu, location);

            menu.AddSeparator();
            menu.AddButton("Convert to Parameter", OnConvertToParameter);
        }

        private void OnConvertToParameter()
        {
            if (Surface.Owner is not IVisjectSurfaceWindow window)
                throw new Exception("Surface owner is not a Visject Surface Window");

            Asset asset = Surface.Owner.SurfaceAsset;
            if (asset == null || !asset.IsLoaded)
            {
                Editor.LogError("Asset is null or not loaded");
                return;
            }

            // Add parameter to editor
            var paramIndex = Surface.Parameters.Count;
            object initValue = _convertFunction == null ? Values[0] : _convertFunction.Invoke(Values);
            var paramAction = new AddRemoveParamAction
            {
                Window = window,
                IsAdd = true,
                Name = Utilities.Utils.IncrementNameNumber("New parameter", x => OnParameterRenameValidate(x)),
                Type = _type,
                Index = paramIndex,
                InitValue = initValue,
            };
            paramAction.Do();

            // Spawn Get Parameter Node based on the added parameter
            Guid parameterGuid = Surface.Parameters[paramIndex].ID;
            bool undoEnabled = Surface.Undo.Enabled;
            Surface.Undo.Enabled = false;
            NodeArchetype arch = Surface.GetParameterGetterNodeArchetype(out var groupId);
            SurfaceNode node = Surface.Context.SpawnNode(groupId, arch.TypeID, this.Location, new object[] { parameterGuid });
            Surface.Undo.Enabled = undoEnabled;

            if (node is not Archetypes.Parameters.SurfaceNodeParamsGet getNode)
                throw new Exception("Node is not a ParamsGet node!");

            // Recreate connections of constant node
            // Constant nodes and parameter nodes should have the same ports, so we can just iterate through the connections
            var boxes = GetBoxes();
            for (int i = 0; i < boxes.Count; i++)
            {
                Box box = boxes[i];
                if (!box.HasAnyConnection)
                    continue;

                if (!getNode.TryGetBox(box.ID, out Box paramBox))
                    continue;

                // Iterating backwards, because the CreateConnection method deletes entries from the box connections when target box IsSingle is set to true
                for (int k = box.Connections.Count - 1; k >= 0; k--)
                {
                    Box connectedBox = box.Connections[k];
                    paramBox.CreateConnection(connectedBox);
                }
            }

            // Add undo actions and remove constant node
            var spawnNodeAction = new AddRemoveNodeAction(getNode, true);
            var removeConstantAction = new AddRemoveNodeAction(this, false);

            Surface.AddBatchedUndoAction(new MultiUndoAction(paramAction, spawnNodeAction, removeConstantAction));
            removeConstantAction.Do();
        }

        private bool OnParameterRenameValidate(string value)
        {
            if (Surface.Owner is not IVisjectSurfaceWindow window)
                throw new Exception("Surface owner is not a Visject Surface Window");
            return !string.IsNullOrWhiteSpace(value) && window.VisjectSurface.Parameters.All(x => x.Name != value);
        }
    }
}
