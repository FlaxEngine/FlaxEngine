// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Editors;
using FlaxEngine.Networking;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="FlaxEngine.Object"/>.
    /// </summary>
    public class ScriptingObjectEditor : GenericEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // Network objects debugging
            var obj = Values[0] as FlaxEngine.Object;
            if (Editor.IsPlayMode && NetworkManager.IsConnected && NetworkReplicator.HasObject(obj))
            {
                var group = layout.Group("Network");
                group.Panel.Open();
                group.Label("Role", Utilities.Utils.GetPropertyNameUI(NetworkReplicator.GetObjectRole(obj).ToString()));
                group.Label("Owner Client Id", NetworkReplicator.GetObjectOwnerClientId(obj).ToString());
            }

            base.Initialize(layout);
        }
    }
}
