using FlaxEditor.CustomEditors.Editors;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated;

/// <summary>
/// The missing script editor.
/// </summary>
[CustomEditor(typeof(MissingScript)), DefaultEditor]
public class MissingScriptEditor : GenericEditor
{
    /// <inheritdoc />
    public override void Initialize(LayoutElementsContainer layout)
    {
        if (layout.ContainerControl is not DropPanel dropPanel)
        {
            base.Initialize(layout);
            return;
        }

        dropPanel.HeaderTextColor = Color.OrangeRed;

        base.Initialize(layout);
    }
}
