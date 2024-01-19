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
            NativeUIHost host = AddChild<NativeUIHost>();
            host.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            host.Asset = _asset;
            host.Height -= _toolstrip.Size.Y;
            host.Location = new Float2(Location.X, _toolstrip.Size.Y);
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
        UIComponent Selection;
        private void SetSelection(UIComponent NewSelection) 
        {
            if (Selection != null)
            {
                Selection.DeselectByDesigner();
            }
            if (!NewSelection)
            {
                Selection.SelectByDesigner();
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                if (_asset.Component is UIPanelComponent panelComponent)
                {
                    if (_asset.Component == null)
                        return base.OnMouseDown(location, button);
                    if (_asset.Component.Transform.Rect.Contains(location) && Selection != panelComponent && !panelComponent.IsLockedInDesigner())
                    {
                        SetSelection(panelComponent);
                        return base.OnMouseDown(location, button);
                    }
                    var children = panelComponent.GetAllChildren();
                    for (int i = 0; i < children.Length; i++)
                    {
                        if (children[i].Transform.Rect.Contains(location) && Selection != children[i] && !children[i].IsLockedInDesigner())
                        {
                            SetSelection(children[i]);
                            return base.OnMouseDown(location, button);
                        }
                    }
                }
            }
            return base.OnMouseDown(location, button);
        }
    }
}
