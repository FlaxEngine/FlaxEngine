// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Undo
{
    /// <summary>
    /// The terrain holes mask editing action that records before and after states to swap between unmodified and modified terrain data.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    /// <seealso cref="EditTerrainMapAction" />
    [Serializable]
    unsafe class EditTerrainHolesMapAction : EditTerrainMapAction
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="EditTerrainHolesMapAction"/> class.
        /// </summary>
        /// <param name="terrain">The terrain.</param>
        public EditTerrainHolesMapAction(FlaxEngine.Terrain terrain)
        : base(terrain, sizeof(byte))
        {
        }

        /// <inheritdoc />
        public override string ActionString => "Edit terrain holes";

        /// <inheritdoc />
        protected override IntPtr GetData(ref Int2 patchCoord, object tag)
        {
            return new IntPtr(TerrainTools.GetHolesMaskData(Terrain, ref patchCoord));
        }

        /// <inheritdoc />
        protected override void SetData(ref Int2 patchCoord, IntPtr data, object tag)
        {
            var offset = Int2.Zero;
            var size = new Int2((int)Mathf.Sqrt(_heightmapLength));
            if (TerrainTools.ModifyHolesMask(Terrain, ref patchCoord, (byte*)data, ref offset, ref size))
                throw new Exception("Failed to modify the terrain holes.");
        }
    }
}
