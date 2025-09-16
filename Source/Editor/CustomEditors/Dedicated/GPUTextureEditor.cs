// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Basic editor/viewer for <see cref="GPUTexture"/>.
    /// </summary>
    [CustomEditor(typeof(GPUTexture)), DefaultEditor]
    public class GPUTextureEditor : CustomEditor
    {
        private Image _image;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _image = new Image
            {
                Brush = new GPUTextureBrush(),
                Size = new Float2(200, 100),
                Parent = layout.ContainerControl,
            };
            _image.Clicked += OnImageClicked;
        }

        private void OnImageClicked(Image image, MouseButton button)
        {
            var texture = Values[0] as GPUTexture;
            if (!texture)
                return;
            var menu = new ContextMenu();
            menu.AddButton("Save...", () => Screenshot.Capture(Values[0] as GPUTexture));
            menu.AddButton("Enlarge", () => _image.Size *= 2);
            menu.AddButton("Shrink", () => _image.Size /= 2).Enabled = _image.Height > 32;
            var location = image.PointFromScreen(Input.MouseScreenPosition);
            menu.Show(image, location);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            var texture = Values[0] as GPUTexture;
            ((GPUTextureBrush)_image.Brush).Texture = texture;
            if (texture)
            {
                var desc = texture.Description;
#if BUILD_RELEASE
                var name = string.Empty;
#else
                var name = texture.Name;
#endif
                _image.TooltipText = $"{name}\nType: {texture.ResourceType}\nSize: {desc.Width}x{desc.Height}\nFormat: {desc.Format}\nMemory: {Utilities.Utils.FormatBytesCount(texture.MemoryUsage)}";
            }
            else
            {
                _image.TooltipText = "None";
            }
        }
    }
}
