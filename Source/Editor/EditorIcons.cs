// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Reflection;
using FlaxEngine;

#pragma warning disable 1591

namespace FlaxEditor
{
    /// <summary>
    /// The editor icons atlas. Cached internally to improve performance.
    /// </summary>
    /// <remarks>
    /// Postfix number informs about the sprite resolution (in pixels).
    /// </remarks>
    [HideInEditor]
    public sealed class EditorIcons
    {
        public SpriteHandle FolderClosed12;
        public SpriteHandle FolderOpened12;
        public SpriteHandle DragBar12;
        public SpriteHandle ArrowDown12;
        public SpriteHandle ArrowRight12;
        public SpriteHandle Search12;
        public SpriteHandle Settings12;
        public SpriteHandle Cross12;
        public SpriteHandle CheckBoxIntermediate12;
        public SpriteHandle CheckBoxTick12;
        public SpriteHandle StatusBarSizeGrip12;

        public SpriteHandle ArrowRightBorder16;
        public SpriteHandle World16;
        public SpriteHandle ScaleStep16;
        public SpriteHandle RotateStep16;
        public SpriteHandle Grid16;
        public SpriteHandle Translate16;
        public SpriteHandle Rotate16;
        public SpriteHandle Scale16;
        public SpriteHandle Link16;
        public SpriteHandle Docs16;

        public SpriteHandle Save32;
        public SpriteHandle Undo32;
        public SpriteHandle Redo32;
        public SpriteHandle Translate32;
        public SpriteHandle Rotate32;
        public SpriteHandle Scale32;
        public SpriteHandle Play32;
        public SpriteHandle Pause32;
        public SpriteHandle Step32;
        public SpriteHandle Stop32;
        public SpriteHandle PageScale32;
        public SpriteHandle Bone32;
        public SpriteHandle Docs32;
        public SpriteHandle Import32;
        public SpriteHandle AddDoc32;
        public SpriteHandle RemoveDoc32;
        public SpriteHandle BracketsSlash32;
        public SpriteHandle Find32;
        public SpriteHandle Reload32;
        public SpriteHandle ArrowLeft32;
        public SpriteHandle ArrowRight32;
        public SpriteHandle ArrowDown32;
        public SpriteHandle ArrowUp32;
        public SpriteHandle Error32;
        public SpriteHandle Warning32;
        public SpriteHandle Info32;
        public SpriteHandle UV32;
        public SpriteHandle Image32;
        public SpriteHandle Link32;
        public SpriteHandle Next32;
        public SpriteHandle Camera32;
        public SpriteHandle Build32;

        public SpriteHandle Add48;
        public SpriteHandle Paint48;
        public SpriteHandle Foliage48;
        public SpriteHandle Mountain48;

        public SpriteHandle Plugin64;
        public SpriteHandle Document64;
        public SpriteHandle CSharpScript64;
        public SpriteHandle CppScript64;
        public SpriteHandle Folder64;
        public SpriteHandle Scene64;
        public SpriteHandle CodeScript64;

        public SpriteHandle Logo128;

        public SpriteHandle VisjectBoxOpen;
        public SpriteHandle VisjectBoxClose;
        public SpriteHandle VisjectArrowOpen;
        public SpriteHandle VisjectArrowClose;

        public SpriteHandle AssetShadow;
        public SpriteHandle ColorWheel;
        public SpriteHandle Windows;
        public SpriteHandle XboxOne;
        public SpriteHandle WindowsStore;
        public SpriteHandle Linux;
        public SpriteHandle PS4;
        public SpriteHandle XboxSeriesX;
        public SpriteHandle Android;

        internal void GetIcons()
        {
            // Load asset
            var iconsAtlas = FlaxEngine.Content.LoadAsyncInternal<SpriteAtlas>(EditorAssets.IconsAtlas);
            if (iconsAtlas == null)
            {
                Editor.LogError("Cannot load editor icons atlas.");
                return;
            }
            if (iconsAtlas.WaitForLoaded())
            {
                Editor.LogError("Failed to load editor icons atlas.");
                return;
            }

            // Find icons
            var fields = GetType().GetFields(BindingFlags.Instance | BindingFlags.Public);
            for (int i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                var sprite = iconsAtlas.FindSprite(field.Name);
                if (!sprite.IsValid)
                {
                    Editor.LogWarning(string.Format("Failed to load sprite icon \'{0}\'.", field.Name));
                }
                field.SetValue(this, sprite);
            }
        }
    }
}
