using FlaxEditor.Windows.Assets;
using FlaxEditor.GUI;
using FlaxEngine.Experimental.UI;
using FlaxEngine;
using System.Linq;
using System;
using FlaxEngine.Json;
using System.Text;
using FlaxEngine.Experimental.UI.Editor;
namespace FlaxEditor.Experimental.UI
{
    internal class UIDesignerEditor : AssetEditorWindow
    {

        (Type,UIDesignerAttribute)[] Attributes;


        private UIBlueprintAsset _asset;
        private NativeUIHost UITransformationTool;
        ToolStripButton _saveButton;
        ToolStripButton _addComponentButton;

        public UIDesignerEditor(Editor editor, UIBlueprintAssetItem item) : base(editor, item)
        {
            var _UITransformationToolAsset = FlaxEngine.Content.LoadAsyncInternal<UIBlueprintAsset>("Editor\\UI\\UITransformationTool.json");
            _UITransformationToolAsset.WaitForLoaded();
            _asset = FlaxEngine.Content.Load<UIBlueprintAsset>(item.Path);
            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _addComponentButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Folder32, AddComponent).LinkTooltip("Add Component");
            _toolstrip.AddSeparator();
            NativeUIHost host = AddChild<NativeUIHost>();
            host.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            host.Asset = _asset;
            host.Height -= _toolstrip.Size.Y;
            host.Location = new Float2(Location.X, _toolstrip.Size.Y);

            UITransformationTool = AddChild<NativeUIHost>();
            UITransformationTool.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            UITransformationTool.PanelComponent = _UITransformationToolAsset.CreateInstance() as UIPanelComponent;
            UITransformationTool.Height -= _toolstrip.Size.Y;
            UITransformationTool.Location = new Float2(Location.X, _toolstrip.Size.Y);

            
            //System.Linq madnes :D
            Attributes = typeof(UIComponent).Assembly.GetTypes().
                Where(t => t.IsSubclassOf(typeof(UIComponent)) && !t.IsAbstract).
                Select(t =>
                {
                    var o = t.GetCustomAttributes(typeof(UIDesignerAttribute), false);
                    if (o.Length != 0)
                        return (t,(UIDesignerAttribute)o[0]);
                    return (t, null);
                }).Where(t => t.Item2 != null).ToArray();
            
            UIBlueprintAsset.SetDesinerFlags(_asset.Component, UIComponentDesignFlags.Designing);
        }
        public void AddComponent()
        {
            if(_asset.Component == null)
            {
                var rootcomp = new UIPanelComponent();
                UIBlueprintAsset.SetDesinerFlags(rootcomp, UIComponentDesignFlags.Designing);
                _asset.Component = rootcomp;
                rootcomp.CreatedByUIBlueprint = true;
                return;
            }
            var comp = new UIComponent();
            UIBlueprintAsset.SetDesinerFlags(comp, UIComponentDesignFlags.Designing);
            (_asset.Component as UIPanelComponent)?.AddChild(comp);
            comp.CreatedByUIBlueprint = true;
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
        Vector2 mouseloc;
        Vector2 Offset;
        bool Drag;
        bool Resize;
        UIComponent Selection;
        private void SetSelection(UIComponent NewSelection)
        {
            if (Selection != null)
                Selection.Deselect();
            Selection = NewSelection;
            if (Selection != null)
            {
                Selection.Select();
                Offset = mouseloc - Selection.Translation;
                UITransformationTool.PanelComponent.Transform = Selection.Transform;
                Drag = true;
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                mouseloc = location - new Float2(0, _toolstrip.Size.Y);

                SelectElement(_asset.Component, mouseloc);
            }
            if(Selection != null && button == MouseButton.Right) 
            {
                Resize = true;
            }
            return base.OnMouseDown(location, button);
        }

        private bool SelectElement(UIComponent comp, Float2 location)
        {
            bool GotSelection = false;
            if (comp == null)
                return false;

            if (comp.Contains(location) && Selection != comp && !comp.IsLockedInDesigner())
            {
                SetSelection(comp);
                GotSelection = true;
            }
            if (comp is UIPanelComponent panelComponent)
            {
                var children = panelComponent.GetAllChildren();
                for (int i = 0; i < children.Length; i++)
                {
                    if (children[i].Contains(location) && Selection != children[i] && !children[i].IsLockedInDesigner())
                    {
                        if (SelectElement(children[i], location))
                        {
                            GotSelection = true;
                            break;
                        }
                    }
                }
            }
            if (!GotSelection)
            {
                SetSelection(null);
            }
            return GotSelection;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                Drag = false;
                Resize = false;
            }
            if(button == MouseButton.Right)
            {
                Resize = false;
            }
            return base.OnMouseUp(location, button);
        }
        public override void OnMouseMove(Float2 location)
        {
            if (Selection != null && Drag)
            {
                mouseloc = (location - new Float2(0, _toolstrip.Size.Y)) - Offset;
                Selection.Translation = mouseloc;
                Debug.Log("Got selection");
            }
            base.OnMouseMove(location);
        }
    }
}
