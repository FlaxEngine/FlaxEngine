// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for picking actor static flags. Overrides the default enum editor logic to provide more useful functionalities.
    /// </summary>
    public sealed class ActorStaticFlagsEditor : EnumEditor
    {
        /// <inheritdoc />
        protected override void OnValueChanged()
        {
            var value = (StaticFlags)element.ComboBox.EnumTypeValue;

            // If selected is single actor that has children, ask if apply flags to the sub objects as well
            if (Values.IsSingleObject && (StaticFlags)Values[0] != value && ParentEditor.Values[0] is Actor actor && actor.HasChildren)
            {
                // Ask user
                var result = MessageBox.Show(
                    "Do you want to set static flags to all child actors as well?",
                    "Change actor static flags", MessageBoxButtons.YesNoCancel);
                if (result == DialogResult.Cancel)
                    return;

                if (result == DialogResult.Yes)
                {
                    // Note: this possibly breaks the design a little bit
                    // But it's the easiest way to set value for selected actor and its children with one undo action
                    List<Actor> actors = new List<Actor>(32);
                    Utilities.Utils.GetActorsTree(actors, actor);
                    if (Presenter.Undo != null && Presenter.Undo.Enabled)
                    {
                        using (new UndoMultiBlock(Presenter.Undo, actors.ToArray(), "Change static flags"))
                        {
                            for (int i = 0; i < actors.Count; i++)
                            {
                                actors[i].StaticFlags = value;
                            }
                        }
                    }
                    else
                    {
                        for (int i = 0; i < actors.Count; i++)
                        {
                            actors[i].StaticFlags = value;
                        }
                    }

                    OnUnDirty();
                    return;
                }
            }

            base.OnValueChanged();
        }
    }
}
