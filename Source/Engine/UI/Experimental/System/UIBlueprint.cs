using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine.Experimental.UI
{
    partial class UIBlueprint
    {
        /// <summary>
        /// Gets the variable.
        /// <para></para>
        /// <note type="note">
        /// Remember to cache the return value the call is relatively expensive
        /// </note>
        /// </summary>
        /// <typeparam name="T">
        ///   <span class="keyword">typeof</span>(<see cref="UIComponent" />)</typeparam>
        /// <param name="VarName">Name of the variable.</param>
        /// <returns>A <span class="typeparameter">T</span> where <span class="typeparameter">T</span> is <span class="keyword">typeof</span>(<see cref="UIComponent" />) it can return null</returns>
        public T GetVariable<T>(string VarName) where T : UIComponent
        {
            return (T)GetVariable_Internal(VarName);
        }
    }
}
