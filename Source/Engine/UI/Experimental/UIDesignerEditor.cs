using FlaxEditor.Windows.Assets;
using FlaxEditor.GUI;
using FlaxEngine.Experimental.UI;
using FlaxEngine;
using System.Linq;
using System;
using FlaxEngine.Json;
using System.Text;
using FlaxEngine.Experimental.UI.Editor;
using FlaxEngine.GUI;
using FlaxEditor.Windows;
using FlaxEditor.CustomEditors;

namespace FlaxEditor.Experimental.UI
{
    internal class UITransformationTool : UIBlueprint 
    {
        UIButton LeftHandle;
        UIButton RightHandle;
        UIButton TopHandle;
        UIButton TopRightHandle;
        UIButton TopLeftHandle;
        UIButton BottomHandle;
        UIButton BottomLeftHandle;
        UIButton BottomRightHandle;
        protected override void OnInitialized()
        {
            LeftHandle        = GetVariable<UIButton>("LeftHandle");
            RightHandle       = GetVariable<UIButton>("RightHandle");
            TopHandle         = GetVariable<UIButton>("TopHandle");
            TopRightHandle    = GetVariable<UIButton>("TopRightHandle");
            TopLeftHandle     = GetVariable<UIButton>("TopLeftHandle");
            BottomHandle      = GetVariable<UIButton>("BottomHandle");
            BottomLeftHandle  = GetVariable<UIButton>("BottomLeftHandle");
            BottomRightHandle = GetVariable<UIButton>("BottomRightHandle");
        }
    }


    internal class UIDesignerEditor : AssetEditorWindow
    {
        (Type,UIDesignerAttribute)[] Attributes;
        CustomEditorPresenter Presenter;

        private UIBlueprint EditedBluprint;
        private NativeUIHost UITransformationTool;
        ToolStripButton _saveButton;
        ToolStripButton _addComponentButton;

        UIBlueprint TransformationTool;
        public UIDesignerEditor(Editor editor, UIBlueprintAssetItem item) : base(editor, item)
        {
            TransformationTool = UISystem.LoadEditorBlueprintAsset("Editor\\UI\\UITransformationTool.json");
            EditedBluprint = UISystem.CreateFromBlueprintAsset(FlaxEngine.Content.Load<UIBlueprintAsset>(item.Path));
            Presenter = new CustomEditorPresenter(editor.Undo, null, null);
            Presenter.Panel.Parent = this;
            

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _addComponentButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Folder32, AddComponent).LinkTooltip("Add Component");
            _toolstrip.AddSeparator();
            AddChild<Label>();
            NativeUIHost host = AddChild<NativeUIHost>();
            host.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            host.Blueprint = EditedBluprint;
            host.Height -= _toolstrip.Size.Y;
            host.Location = new Float2(Location.X, _toolstrip.Size.Y);

            UITransformationTool = AddChild<NativeUIHost>();
            UITransformationTool.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            UITransformationTool.Blueprint = TransformationTool;
            UITransformationTool.Height -= _toolstrip.Size.Y;
            UITransformationTool.Location = new Float2(Location.X, _toolstrip.Size.Y);

            TransformationTool.SetDesinerFlags(UIComponentDesignFlags.None);



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

            EditedBluprint.SetDesinerFlags(UIComponentDesignFlags.Designing);
        }
        public void AddComponent()
        {
            if (EditedBluprint.Component == null)
            {
                var rootcomp = new UIPanelComponent();
                UISystem.SetDesinerFlags(rootcomp, UIComponentDesignFlags.Designing);
                EditedBluprint.Component = rootcomp;
                rootcomp.CreatedByUIBlueprint = true;
                return;
            }
            var comp = new UIComponent();
            UISystem.SetDesinerFlags(comp, UIComponentDesignFlags.Designing);
            (EditedBluprint.Component as UIPanelComponent)?.AddChild(comp);
            comp.CreatedByUIBlueprint = true;
        }
        public override void Save()
        {
            UISystem.SaveBlueprint(EditedBluprint);
            ClearEditedFlag();
        }
        Data dat = new Data();
        struct Data 
        {
            public Vector2 mouseloc;
            public Vector2 Offset;
            public bool Drag;
            public bool Resize;
            public UIComponent Selection;
        }
        private void SetSelection(UIComponent NewSelection)
        {
            if (dat.Selection != null)
                dat.Selection.Deselect();
            dat.Selection = NewSelection;
            if (dat.Selection != null)
            {
                dat.Selection.Select();
                dat.Offset = dat.mouseloc - dat.Selection.Translation;
                UITransformationTool.PanelComponent.Transform = dat.Selection.Transform;
                dat.Drag = true;
                Presenter.Select(dat.Selection);
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                dat.mouseloc = location - new Float2(0, _toolstrip.Size.Y);

                SelectElement(EditedBluprint.Component, dat.mouseloc);
                
            }
            if(dat.Selection != null && button == MouseButton.Right) 
            {
                dat.Resize = true;
            }
            return base.OnMouseDown(location, button);
        }

        private bool SelectElement(UIComponent comp, Float2 location)
        {
            bool GotSelection = false;
            if (comp == null)
                return false;

            if (comp.Contains(location) && dat.Selection != comp && !comp.IsLockedInDesigner())
            {
                SetSelection(comp);
                GotSelection = true;
            }
            if (comp is UIPanelComponent panelComponent)
            {
                var children = panelComponent.GetAllChildren();
                for (int i = 0; i < children.Length; i++)
                {
                    if (children[i].Contains(location) && dat.Selection != children[i] && !children[i].IsLockedInDesigner())
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
                dat.Drag = false;
                dat.Resize = false;
            }
            if(button == MouseButton.Right)
            {
                dat.Resize = false;
            }
            return base.OnMouseUp(location, button);
        }
        public override void OnMouseMove(Float2 location)
        {
            
            dat.mouseloc = (location - new Float2(0, _toolstrip.Size.Y)) - dat.Offset;
            if (dat.Selection != null && dat.Drag)
            {
                dat.Selection.Translation = dat.mouseloc;
                Debug.Log("Got selection");
            }
            base.OnMouseMove(location);
        }
    }
}
