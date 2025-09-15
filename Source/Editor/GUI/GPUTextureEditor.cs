#if FLAX_EDITOR
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Tools.Foliage;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// basic custom property editor for GPUTexture
    /// </summary>
    [CustomEditor(typeof(GPUTexture))]
    public class GPUTexturePropertyEditor : GenericEditor
    {
        public override DisplayStyle Style => DisplayStyle.Inline;

        ImageElement imageElement;
        GroupElement group;
        /// <summary>
        /// </summary>
        /// <param name="layout"></param>
        public override void Initialize(LayoutElementsContainer layout)
        {
            imageElement = (group = layout.Group("")).Image(SpriteHandle.Default);
            
            //todo fix the AddSettingsButton func
            //shit is buged
            //the code below (until the Paint) is untested the Clear might not work
            group.AddSettingsButton();
            group.SetupContextMenu += (ContextMenu.ContextMenu cxm, DropPanel dp) =>
            {
                cxm.AddButton("Clear", (ContextMenuButton bt) =>
                {
                    SetValue(null);
                });

                cxm.AddSeparator();

                //todo
                //editor is needed
                //cxm.AddButton("Display Full Texture", (ContextMenuButton bt) =>
                //{
                //});

                //todo
                //
                //cxm.AddButton("Save To Asset", (ContextMenuButton bt) =>
                //{
                //});
            };
            Paint();
            group.Panel.Close();
        }
        /// <summary>
        /// </summary>
        public override void Refresh()
        {
            Paint();
            base.Refresh();
        }
        private void Paint()
        {
            string name = null;
            string tt = null;
            if (Values[0] is GPUTexture gputex)
            {
                name = gputex.Name;
                tt += "Type: " + gputex.ResourceType.ToString()   + "\n";
                tt += "Memory Usage: " + gputex.MemoryUsage + "B" + "\n";
                tt += "Format: " + gputex.Format.ToString() + "\n";
                //shorten the name it is a full path
                if (name.EndsWith(".flax"))
                {
                    if (name != ".flax")//sanity guard
                    {
                        var nameStartIndexWithEx = Globals.ProjectFolder.Length + 9 /* +9 to remove the "/Content/" */;
                        name = name.Substring
                        (
                            nameStartIndexWithEx,
                            nameStartIndexWithEx - 5  /* -5 to remove the .flax */ + 2
                        );

                        tt += "Path: " + gputex.Name.Remove(0, Globals.ProjectFolder.Length + 1);
                    }
                }

                if (imageElement.Image.Brush is GPUTextureBrush brush)
                {
                    brush.Texture = gputex;
                    imageElement.Control.Size = new Float2(group.Control.Width);
                }
                else
                {
                    imageElement.Image.Brush = new GPUTextureBrush();
                    Paint();
                }
            }
            name ??= "...";

            DropPanel p = group.Control as DropPanel;

            p.HeaderText = name;
            p.TooltipText = tt;
        }
    }
}
#endif
