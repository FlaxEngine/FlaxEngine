using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine.Experimental.UI
{
    /// <summary>
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Class,AllowMultiple= true,Inherited=true)]
    public class UIDesignerAttribute : Attribute
    {
        /// <summary>
        /// The display label
        /// </summary>
        public string DisplayLabel = "DisplayLabel";

        /// <summary>
        /// The category name
        /// </summary>
        public string CategoryName = "CategoryName";

        /// <summary>
        /// The editor component
        /// </summary>
        public bool EditorComponent = false;

        /// <summary>
        /// The hidden in designer
        /// </summary>
        public bool HiddenInDesigner = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="UIDesignerAttribute"/> class.
        /// </summary>
        /// <param name="DisplayLabel">The display label.</param>
        /// <param name="CategoryName">Name of the category.</param>
        /// <param name="EditorComponent">if set to <c>true</c> [editor component].</param>
        /// <param name="HiddenInDesigner">if set to <c>true</c> [hidden in designer].</param>
        public UIDesignerAttribute(string DisplayLabel= "DisplayLabel", string CategoryName = "CategoryName", bool EditorComponent = false, bool HiddenInDesigner = false) 
        {
            this.DisplayLabel        = DisplayLabel        ;
            this.CategoryName        = CategoryName        ;
            this.EditorComponent     = EditorComponent     ;
            this.HiddenInDesigner    = HiddenInDesigner    ;
        }
    }
}
