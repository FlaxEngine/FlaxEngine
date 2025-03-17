using FlaxEditor;
using FlaxEditor.GUI.Dialogs;
using FlaxEngine;
using FlaxEngine.GUI;
using System;


namespace FlaxEditor.GUI
{
    /// <summary>
    /// A special kind of button used to display saved colors in the <see cref="ColorPickerDialog"/>.
    /// </summary>
    public class SavedColorButton : Button
    {
        private const float GridScale = 4;
        private const String ScaleParamName = "Scale";

        private MaterialBase checkerMaterial;

        /// <summary>
        /// Creates a new instance of the <see cref="SavedColorButton"/>.
        /// </summary>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="width">The width.</param>
        /// <param name="height">The height.</param>
        public SavedColorButton(float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            // Load alpha grid material
            checkerMaterial = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.ColorAlphaBackgroundGrid);
            checkerMaterial = checkerMaterial.CreateVirtualInstance();
            checkerMaterial.SetParameterValue(ScaleParamName, GridScale);
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            // Prevent drawing the alpha grid on disabled add new color button
            if (Tag != null)
                Render2D.DrawMaterial(checkerMaterial, new Rectangle(Float2.Zero, Size));
            
            base.DrawSelf();
        }
    }
}
