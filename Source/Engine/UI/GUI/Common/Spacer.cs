// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Helper control used to insert blank space into the layout.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [ActorToolbox("GUI")]
    public sealed class Spacer : ContainerControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Spacer"/> class.
        /// </summary>
        public Spacer()
        : this(100, 100)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Spacer"/> class.
        /// </summary>
        /// <param name="width">The width.</param>
        /// <param name="height">The height.</param>
        public Spacer(float width, float height)
        : base(0, 0, width, height)
        {
            AutoFocus = false;
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
