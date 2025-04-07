// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.Surface;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Animation Graph function window allows to view and edit <see cref="AnimationGraphFunction"/> asset.
    /// </summary>
    /// <seealso cref="AnimationGraphFunction" />
    /// <seealso cref="AnimationGraphFunctionSurface" />
    public sealed class AnimationGraphFunctionWindow : VisjectFunctionSurfaceWindow<AnimationGraphFunction, AnimationGraphFunctionSurface>
    {
        private readonly NavigationBar _navigationBar;

        /// <inheritdoc />
        public AnimationGraphFunctionWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Surface
            _surface = new AnimationGraphFunctionSurface(this, Save, _undo)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _panel,
                Enabled = false
            };
            _surface.ContextChanged += OnSurfaceContextChanged;

            // Toolstrip
            SurfaceUtils.PerformCommonSetup(this, _toolstrip, _surface, out _saveButton, out _undoButton, out _redoButton);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/anim-graph/index.html")).LinkTooltip("See documentation to learn more");

            // Navigation bar
            _navigationBar = new NavigationBar
            {
                Parent = this
            };
        }

        private void OnSurfaceContextChanged(VisjectSurfaceContext context)
        {
            _surface.UpdateNavigationBar(_navigationBar, _toolstrip);
        }

        /// <inheritdoc />
        public override string SurfaceName => "Animation Graph Function";

        /// <inheritdoc />
        public override byte[] SurfaceData
        {
            get => _asset.LoadSurface();
            set
            {
                if (_asset.SaveSurface(value))
                {
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save surface data");
                }
                _asset.Reload();
            }
        }

        /// <inheritdoc />
        protected override bool LoadSurface()
        {
            var result = base.LoadSurface();

            // Update navbar
            _surface.UpdateNavigationBar(_navigationBar, _toolstrip);

            return result;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            _navigationBar?.UpdateBounds(_toolstrip);
        }
    }
}
