using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors;

internal static class CustomEditorUtils
{
    public static CustomElementsContainer<GridPanel> CreateGridContainer(CustomElementsContainer<UniformGridPanel> grid, string unit, out LabelElement label)
    {
        var container = grid.CustomContainer<GridPanel>();
        var containerControl = container.CustomControl;
        containerControl.ClipChildren = false;
        containerControl.Height = grid.CustomControl.Height;
        containerControl.RowFill = new[] { 1f, 1f };
        containerControl.ColumnFill = new[] { -20f, 1f };
        containerControl.SlotPadding = new Margin(0);
        
        label = container.Label(unit);
        var labelElement = label.Label;
        labelElement.VerticalAlignment = TextAlignment.Center;
        labelElement.HorizontalAlignment = TextAlignment.Center;
            
        return container;
    }
}
