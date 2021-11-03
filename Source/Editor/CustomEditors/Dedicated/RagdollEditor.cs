// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="Ragdoll"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(Ragdoll)), DefaultEditor]
    public class RagdollEditor : ActorEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            // Add info box
            if (IsSingleObject && Values[0] is Ragdoll ragdoll)
            {
                var text = $"Total mass: {Utils.RoundTo1DecimalPlace(ragdoll.TotalMass)}kg";
                var label = layout.Label(text);
                label.Label.AutoHeight = true;
            }
        }
    }
}
