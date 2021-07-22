using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Possible navigation directiosn
    /// </summary>
    public enum NavDir
    {
        /// <summary>
        /// Left
        /// </summary>
        Left,
        /// <summary>
        /// Right
        /// </summary>
        Right,
        /// <summary>
        /// Up
        /// </summary>
        Up,
        /// <summary>
        /// Down
        /// </summary>
        Down,

        /// <summary>
        /// Selects the next, in auto nav Left and then Down
        /// </summary>
        Next,
        /// <summary>
        /// Selects the previous, in auto nav Right and then Up
        /// </summary>
        Previous
    }
}
