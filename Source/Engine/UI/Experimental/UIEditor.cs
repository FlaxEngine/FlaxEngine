using FlaxEditor.Windows.Assets;
using FlaxEditor.GUI;
using FlaxEngine.Experimental.UI;
using FlaxEngine;
using FlaxEngine.Json;

namespace FlaxEditor.Experimental.UI
{

    internal class UIEditor : AssetEditorWindow
    {
        private UIBlueprintAsset _asset;
        ToolStripButton _saveButton;

        public UIEditor(Editor editor, UIBlueprintAssetItem item) : base(editor, item)
        {
            _asset = FlaxEngine.Content.Load<UIBlueprintAsset>(item.Path);

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Add64, AddComponent).LinkTooltip("Add Component");
            _toolstrip.AddSeparator();

            if (_asset.Component == null)
            {
                _asset.Component = new UIPanelComponent();
                MarkAsEdited();
            }
        }
        public void AddComponent() 
        {
            (_asset.Component as UIPanelComponent)?.AddChild(new UIComponent());
        }
        public override void Save()
        {
            if (_asset.Save(_item.Path))
            {
                Editor.LogError("Cannot save asset.");
                return;
            }
            ClearEditedFlag();
        }
    }
}
