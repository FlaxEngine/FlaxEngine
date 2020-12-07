// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The blur panel that applied the Gaussian-blur to all content beneath the control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class BlurPanel : ContainerControl
    {
        /// <summary>
        /// Gets or sets the blur strength. Defines how blurry the background is. Larger numbers increase blur, resulting in a larger runtime cost on the GPU.
        /// </summary>
        [EditorOrder(0), Limit(0, 100, 0.1f), Tooltip("Blur strength defines how blurry the background is. Larger numbers increase blur, resulting in a larger runtime cost on the GPU.")]
        public float BlurStrength { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="BlurPanel"/> class.
        /// </summary>
        public BlurPanel()
        {
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            float strength = BlurStrength;
            if (strength > Mathf.Epsilon)
            {
                Render2D.DrawBlur(new Rectangle(Vector2.Zero, Size), strength);
            }
        }
    }
}
