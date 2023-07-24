using FlaxEditor.Content;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.States;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using System.Collections.Generic;
using System.Linq;

namespace FlaxEditor.Content.GUI
{
    class ContentSettingsDropdown : ComboBox
    {
        internal readonly List<string> Settings = new()
        {
            "Show Engine Content",
            "Show Plugin Content",
            "Show Build Files",
            "Show Game Settings",
            "Show Shader Source"
        };
        public ContentSettingsDropdown()
        {
            SupportMultiSelect = true;
            MaximumItemsInViewCount = 20;
            var style = Style.Current;
            BackgroundColor = style.BackgroundNormal;
            BackgroundColorHighlighted = BackgroundColor;
            BackgroundColorSelected = BackgroundColor;
            
        }
        protected override ContextMenu OnCreatePopup()
        {
            UpdateDropDownItems();
            return base.OnCreatePopup();
        }
        internal void UpdateDropDownItems()
        {
            ClearItems();
            
            for (int i = 0; i < Settings.Count; i++)
            {
                AddItem(Settings[i]);
            }
        }
        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            var clientRect = new Rectangle(Float2.Zero, Size);
            // Draw background
            if (IsDragOver)
            {
                Render2D.FillRectangle(clientRect, Style.Current.BackgroundSelected * 0.6f);
            }
            else if (_mouseDown)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundSelected);
            }
            else if (IsMouseOver)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
            }
            float size = (Size.Y / 1.5f); //Icon size
            // Draw text
            Render2D.DrawText(Font.GetFont(), "Settings", new Rectangle(size, 0, Size.X - Size.Y, Size.Y), TextColor, TextAlignment.Center, TextAlignment.Center);
            Render2D.DrawSprite(Editor.Instance.Icons.Settings12, new Rectangle((Size.Y - size) / 2.0f, (Size.Y - size) / 2.0f, size, size));
        }
        protected override void OnLayoutMenuButton(ref ContextMenuButton button, int index, bool construct = false)
        {
            var style = Style.Current;
            if (_selectedIndices.Contains(index))
            {
                button.Icon = style.CheckBoxTick;
            }
            else
            {
                button.Icon = SpriteHandle.Default;
            }
            if (_tooltips != null && _tooltips.Length > index)
            {
                button.TooltipText = _tooltips[index];
            }
        }
        protected override void OnSelectedIndexChanged()
        {
            ContentFilter.UpdateFilterVisability(this);
            base.OnSelectedIndexChanged();
        }
    }
}
