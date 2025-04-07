// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.CustomEditors.Editors
{
    internal sealed class DummyEditor : CustomEditor
    {
        public override void Initialize(LayoutElementsContainer layout)
        {
            string valueName;
            if (Values.Count != 0 && Values[0] != null)
                valueName = Values[0].ToString();
            else
                valueName = "null";
            layout.Label($"{valueName} ({Values.Type})");
        }
    }
}
