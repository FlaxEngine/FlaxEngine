using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Experimental.UI;
using FlaxEditor.Content;
using FlaxEditor.Scripting;
using FlaxEngine.Utilities;
using FlaxEditor.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// </summary>
    [CustomEditor(typeof(FlaxEngine.Experimental.UI.UIBrush))]
    public class UIBrushEditor : GenericEditor
    {
        /// <summary>
        /// Gets the custom editor style.
        /// </summary>
        public override DisplayStyle Style => DisplayStyle.Inline;
        /// <summary>
        /// The picker
        /// </summary>
        //[]
        //public AssetPicker Picker;
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            //Picker = layout.Custom<AssetPicker>().CustomControl;
            //Picker.Size = Float2.One * 32;
            //Picker.Validator.FileExtension = ".UIBrush";
            
            //if (Picker.Validator.SelectedAsset == null)
            //{
            base.Initialize(layout);
            //}
            //if (Values.Count == 0)
            //    return;
            //if (Values[0] == null)
            //{
            //    assetRefEditor.Initialize(layout);
            //    return;
            //}
            //layout.Property(Values.Info.Name, Values);
            //assetRefEditor.Initialize(layout);
        }
        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();
        }
    }
}
