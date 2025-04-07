// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Version value type properties.
    /// </summary>
    [CustomEditor(typeof(Margin)), DefaultEditor]
    public class MarginEditor : GenericEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                var limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    for (var i = 0; i < ChildrenEditors.Count; i++)
                    {
                        if (ChildrenEditors[i] is FloatEditor floatEditor)
                            floatEditor.Element.SetLimits(limit);
                    }
                }
            }
        }
    }
}
