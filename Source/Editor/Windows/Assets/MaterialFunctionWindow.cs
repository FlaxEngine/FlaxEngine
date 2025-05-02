// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.Surface;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Material function window allows to view and edit <see cref="MaterialFunction"/> asset.
    /// </summary>
    /// <seealso cref="MaterialFunction" />
    /// <seealso cref="MaterialFunctionSurface" />
    public sealed class MaterialFunctionWindow : VisjectFunctionSurfaceWindow<MaterialFunction, MaterialFunctionSurface>
    {
        /// <inheritdoc />
        public MaterialFunctionWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Surface
            _surface = new MaterialFunctionSurface(this, Save, _undo)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _panel,
                Enabled = false
            };

            // Toolstrip
            SurfaceUtils.PerformCommonSetup(this, _toolstrip, _surface, out _saveButton, out _undoButton, out _redoButton);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/graphics/materials/index.html")).LinkTooltip("See documentation to learn more");
        }

        /// <inheritdoc />
        public override string SurfaceName => "Material Function";

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
    }
}
