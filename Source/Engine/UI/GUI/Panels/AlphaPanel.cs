using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Changes alpha of all its children
    /// </summary>
    public class AlphaPanel : ContainerControl
    {
        /// <summary>
        /// The target alpha value
        /// </summary>
        public float Alpha;
        /// <summary>
        /// Whether or not we should ignore previous alphas
        /// </summary>
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
