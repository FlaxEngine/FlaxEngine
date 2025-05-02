// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.VideoPlayer"/> frame displaying.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class VideoBrush : IBrush
    {
        /// <summary>
        /// The video player to display frame from it.
        /// </summary>
        public VideoPlayer Player;

        /// <summary>
        /// The texture sampling filter mode.
        /// </summary>
        [ExpandGroups]
        public BrushFilter Filter = BrushFilter.Linear;

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoBrush"/> class.
        /// </summary>
        public VideoBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoBrush"/> struct.
        /// </summary>
        /// <param name="player">The video player to preview.</param>
        public VideoBrush(VideoPlayer player)
        {
            Player = player;
        }

        /// <inheritdoc />
        public Float2 Size
        {
            get
            {
                if (Player && Player.Size.LengthSquared > 0)
                    return (Float2)Player.Size;
                return new Float2(1920, 1080);
            }
        }

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            var texture = Player?.Frame;
            if (texture == null || !texture.IsAllocated)
                texture = GPUDevice.Instance.DefaultBlackTexture;
            if (Filter == BrushFilter.Point)
                Render2D.DrawTexturePoint(texture, rect, color);
            else
                Render2D.DrawTexture(texture, rect, color);
        }
    }
}
