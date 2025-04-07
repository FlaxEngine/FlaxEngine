// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial class MaterialBase
    {
        /// <summary>
        /// Gets a value indicating whether this material is a surface shader (can be used with a normal meshes).
        /// </summary>
        public bool IsSurface => Info.Domain == MaterialDomain.Surface;

        /// <summary>
        /// Gets a value indicating whether this material is post fx (cannot be used with a normal meshes).
        /// </summary>
        public bool IsPostFx => Info.Domain == MaterialDomain.PostProcess;

        /// <summary>
        /// Gets a value indicating whether this material is decal (cannot be used with a normal meshes).
        /// </summary>
        public bool IsDecal => Info.Domain == MaterialDomain.Decal;

        /// <summary>
        /// Gets a value indicating whether this material is a GUI shader (cannot be used with a normal meshes).
        /// </summary>
        public bool IsGUI => Info.Domain == MaterialDomain.GUI;

        /// <summary>
        /// Gets a value indicating whether this material is a terrain shader (cannot be used with a normal meshes).
        /// </summary>
        public bool IsTerrain => Info.Domain == MaterialDomain.Terrain;

        /// <summary>
        /// Gets a value indicating whether this material is a particle shader (cannot be used with a normal meshes).
        /// </summary>
        public bool IsParticle => Info.Domain == MaterialDomain.Particle;
    }
}
