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

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            RememberPresentationValues();
            layout.Label("Async Presentation changes require saving and an engine restart.");
            base.Initialize(layout);
        }

        private void RememberPresentationValues()
        {
            _asyncPresentation = new bool[Values.Count];
            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is GraphicsSettings settings)
                    _asyncPresentation[i] = settings.UseAsyncPresentation;
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
                if (Values[i] is GraphicsSettings settings && _asyncPresentation[i] != settings.UseAsyncPresentation)
                {
                    // Update before showing a modal so subsequent refreshes do not repeat it.
                    _asyncPresentation[i] = settings.UseAsyncPresentation;
                    changed = true;
                }
            }
            if (changed)
            {
                MessageBox.Show("Async Presentation has changed. Save Graphics Settings, then restart the editor/engine to apply it. " +
                                "The current presentation mode remains active until restart. This experimental option supports Windows DirectX 11 only.",
                                "Restart Required", MessageBoxButtons.OK);
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _asyncPresentation = null;
            base.Deinitialize();
        }
    }
}
