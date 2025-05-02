// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.Surface;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Particle function window allows to view and edit <see cref="ParticleEmitterFunction"/> asset.
    /// </summary>
    /// <seealso cref="ParticleEmitterFunction" />
    /// <seealso cref="ParticleEmitterFunctionSurface" />
    public sealed class ParticleEmitterFunctionWindow : VisjectFunctionSurfaceWindow<ParticleEmitterFunction, ParticleEmitterFunctionSurface>
    {
        /// <inheritdoc />
        public ParticleEmitterFunctionWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Surface
            _surface = new ParticleEmitterFunctionSurface(this, Save, _undo)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _panel,
                Enabled = false
            };

            // Toolstrip
            SurfaceUtils.PerformCommonSetup(this, _toolstrip, _surface, out _saveButton, out _undoButton, out _redoButton);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/particles/index.html")).LinkTooltip("See documentation to learn more");
        }

        /// <inheritdoc />
        public override string SurfaceName => "Particle Emitter Function";

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
