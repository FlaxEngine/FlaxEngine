// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    /// <summary>
    /// Material parameters types.
    /// </summary>
    public enum MaterialParameterType : byte
    {
        /// <summary>
        /// The invalid type.
        /// </summary>
        Invalid = 0,

        /// <summary>
        /// The bool.
        /// </summary>
        Bool = 1,

        /// <summary>
        /// The integer.
        /// </summary>
        Integer = 2,

        /// <summary>
        /// The float.
        /// </summary>
        Float = 3,

        /// <summary>
        /// The vector2
        /// </summary>
        Vector2 = 4,

        /// <summary>
        /// The vector3.
        /// </summary>
        Vector3 = 5,

        /// <summary>
        /// The vector4.
        /// </summary>
        Vector4 = 6,

        /// <summary>
        /// The color.
        /// </summary>
        Color = 7,

        /// <summary>
        /// The texture.
        /// </summary>
        Texture = 8,

        /// <summary>
        /// The cube texture.
        /// </summary>
        CubeTexture = 9,

        /// <summary>
        /// The normal map texture.
        /// </summary>
        NormalMap = 10,

        /// <summary>
        /// The scene texture.
        /// </summary>
        SceneTexture = 11,

        /// <summary>
        /// The GPU texture (created from code).
        /// </summary>
        GPUTexture = 12,

        /// <summary>
        /// The matrix.
        /// </summary>
        Matrix = 13,

        /// <summary>
        /// The GPU texture array (created from code).
        /// </summary>
        GPUTextureArray = 14,

        /// <summary>
        /// The GPU volume texture (created from code).
        /// </summary>
        GPUTextureVolume = 15,

        /// <summary>
        /// The GPU cube texture (created from code).
        /// </summary>
        GPUTextureCube = 16,

        /// <summary>
        /// The RGBA channel selection mask.
        /// </summary>
        ChannelMask = 17,

        /// <summary>
        /// The gameplay global.
        /// </summary>
        GameplayGlobal = 18,
    }
}
