// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Undo
{
    /// <summary>
    /// The terrain splatmap editing action that records before and after states to swap between unmodified and modified terrain data.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    /// <seealso cref="EditTerrainMapAction" />
    [Serializable]
    unsafe class EditTerrainSplatMapAction : EditTerrainMapAction
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="EditTerrainSplatMapAction"/> class.
        /// </summary>
        /// <param name="terrain">The terrain.</param>
        public EditTerrainSplatMapAction(FlaxEngine.Terrain terrain)
        : base(terrain, Color32.SizeInBytes)
        {
        }

        /// <inheritdoc />
        public override string ActionString => "Edit terrain splatmap";

        /// <inheritdoc />
        protected override IntPtr GetData(ref Int2 patchCoord, object tag)
        {
            return new IntPtr(TerrainTools.GetSplatMapData(Terrain, ref patchCoord, (int)tag));
        }

        /// <inheritdoc />
        protected override void SetData(ref Int2 patchCoord, IntPtr data, object tag)
        {
            var offset = Int2.Zero;
            var size = new Int2((int)Mathf.Sqrt(_heightmapLength));
            if (TerrainTools.ModifySplatMap(Terrain, ref patchCoord, (int)tag, (Color32*)data, ref offset, ref size))
                throw new Exception("Failed to modify the splatmap.");
        }
    }
}
