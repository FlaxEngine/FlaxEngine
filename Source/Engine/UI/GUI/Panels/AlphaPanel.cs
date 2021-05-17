// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Changes alpha of all its children
    /// </summary>
    public class AlphaPanel : ContainerControl
    {
        /// <summary>
        /// The target alpha value.
        /// </summary>
        [EditorOrder(0), Limit(0, 1, 0.01f), Tooltip("The target alpha value.")]
        public float Alpha = 1.0f;

        /// <summary>
        /// Whether or not we should ignore previous alphas.
        /// </summary>
        [EditorOrder(10), Tooltip("Whether or not we should ignore previous alphas.")]
        public bool IgnoreStack;

        /// <inheritdoc/>
        public override void Draw()
        {
            Render2D.PeekTint(out Color oldColor);
            if (IgnoreStack)
            {
                Color newColor = new Color(oldColor.R, oldColor.G, oldColor.B, Alpha);
                Render2D.PushTint(ref newColor, false);
            }
            else
            {
                Color newColor = new Color(oldColor.R, oldColor.G, oldColor.B, oldColor.A * Alpha);
                Render2D.PushTint(ref newColor, false);
            }

            base.Draw();

            Render2D.PopTint();
        }
    }
}
