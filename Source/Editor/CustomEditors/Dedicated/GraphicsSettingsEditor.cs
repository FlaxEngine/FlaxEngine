// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Editors;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Graphics settings inspector with presentation restart guidance.
    /// </summary>
    [CustomEditor(typeof(GraphicsSettings))]
    sealed class GraphicsSettingsEditor : GenericEditor
    {
        private bool[] _asyncPresentation;
        private GraphicsSettings.EditorGraphicsBackends[] _backends;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            RememberPresentationValues();
            layout.Label("Backend and Async Presentation changes require saving and restarting.");
            var device = GPUDevice.Instance;
            layout.Label("Active backend: " + (device ? device.RendererType.ToString() : "Unavailable"));
            base.Initialize(layout);
        }

        private void RememberPresentationValues()
        {
            _asyncPresentation = new bool[Values.Count];
            _backends = new GraphicsSettings.EditorGraphicsBackends[Values.Count];
            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is GraphicsSettings settings)
                {
                    _asyncPresentation[i] = settings.UseAsyncPresentation;
                    _backends[i] = settings.EditorGraphicsBackend;
                }
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();
            if (_asyncPresentation == null || _asyncPresentation.Length != Values.Count)
            {
                RememberPresentationValues();
                return;
            }

            bool changed = false;
            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is GraphicsSettings settings &&
                    (_asyncPresentation[i] != settings.UseAsyncPresentation || _backends[i] != settings.EditorGraphicsBackend))
                {
                    // Update before showing a modal so subsequent refreshes do not repeat it.
                    _asyncPresentation[i] = settings.UseAsyncPresentation;
                    _backends[i] = settings.EditorGraphicsBackend;
                    changed = true;
                }
            }
            if (changed)
            {
                MessageBox.Show("Graphics startup settings have changed. Save Graphics Settings, then restart the editor/engine to apply them. " +
                                "The current backend and presentation mode remain active until restart. Command-line backend switches take priority over the saved preference. Async Presentation supports Windows DirectX 11 and DirectX 12.",
                                "Restart Required", MessageBoxButtons.OK);
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _asyncPresentation = null;
            _backends = null;
            base.Deinitialize();
        }
    }
}
