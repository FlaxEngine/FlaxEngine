// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Changes alpha of all its children
    /// </summary>
    [ActorToolbox("GUI")]
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

        /// <summary>
        /// Initializes a new instance of the <see cref="AlphaPanel"/> class.
        /// </summary>
        public AlphaPanel()
        {
            AutoFocus = false;
        }

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

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise) // Ignore as visual-only element
                return false;
            return base.ContainsPoint(ref location, precise);
        }
    }
}
